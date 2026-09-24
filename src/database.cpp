#include "database.h"

#include <QDir>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDateTime>
#include <QDebug>

Database::Database(QObject *parent) : QObject(parent) {
}

Database::~Database() {
    const QString connName = m_db.connectionName();
    if (m_db.isOpen()) {
        m_db.close();
    }
    m_db = QSqlDatabase();
    if (!connName.isEmpty()) {
        QSqlDatabase::removeDatabase(connName);
    }
}

bool Database::init() {
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir;
    if (!dir.exists(dataDir)) {
        dir.mkpath(dataDir);
    }
    m_dbPath = dataDir + QStringLiteral("/repasito.db");

    // Seamless migration from legacy omacalendar database if repasito.db doesn't exist yet
    if (!QFile::exists(m_dbPath)) {
        const QString legacyPath1 = dataDir + QStringLiteral("/omacalendar.db");
        const QString legacyPath2 = QDir::homePath() + QStringLiteral("/.local/share/Omacom/omacalendar/omacalendar.db");
        if (QFile::exists(legacyPath1)) {
            QFile::copy(legacyPath1, m_dbPath);
        } else if (QFile::exists(legacyPath2)) {
            QFile::copy(legacyPath2, m_dbPath);
        }
    }

    const QString connectionName = QStringLiteral("repasito_%1").arg(reinterpret_cast<quintptr>(this));
    if (QSqlDatabase::contains(connectionName)) {
        m_db = QSqlDatabase::database(connectionName);
    } else {
        m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
        m_db.setDatabaseName(m_dbPath);
    }

    if (!m_db.open()) {
        qWarning() << "Failed to open SQLite database:" << m_db.lastError().text();
        return false;
    }

    // Enable foreign keys and WAL mode for reliability
    QSqlQuery pragmaQuery(m_db);
    pragmaQuery.exec(QStringLiteral("PRAGMA foreign_keys = ON;"));
    pragmaQuery.exec(QStringLiteral("PRAGMA journal_mode = WAL;"));

    ensureSchema();
    return true;
}

void Database::ensureSchema() {
    QSqlQuery query(m_db);

    query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS topics ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  title TEXT NOT NULL,"
        "  notes TEXT DEFAULT '',"
        "  initial_date TEXT DEFAULT '',"
        "  created_at TEXT NOT NULL,"
        "  status TEXT DEFAULT 'active'"
        ");"
    ));

    // Ensure initial_date column exists for databases created with earlier schema
    QSqlQuery checkCol(m_db);
    checkCol.exec(QStringLiteral("PRAGMA table_info(topics);"));
    bool hasInitialDate = false;
    while (checkCol.next()) {
        if (checkCol.value(1).toString() == QStringLiteral("initial_date")) {
            hasInitialDate = true;
            break;
        }
    }
    if (!hasInitialDate) {
        QSqlQuery alterQuery(m_db);
        alterQuery.exec(QStringLiteral("ALTER TABLE topics ADD COLUMN initial_date TEXT DEFAULT '';"));
    }

    query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS reviews ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  topic_id INTEGER NOT NULL,"
        "  review_stage INTEGER NOT NULL,"
        "  scheduled_date TEXT NOT NULL,"
        "  completed_at TEXT DEFAULT NULL,"
        "  FOREIGN KEY(topic_id) REFERENCES topics(id) ON DELETE CASCADE"
        ");"
    ));

    query.exec(QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_reviews_date ON reviews(scheduled_date);"
    ));
    query.exec(QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_reviews_topic ON reviews(topic_id);"
    ));
}

int Database::addTopic(const QString &title, const QString &notes, const QDate &initialDate) {
    if (title.trimmed().isEmpty())
        return -1;

    m_db.transaction();

    QSqlQuery topicQuery(m_db);
    topicQuery.prepare(QStringLiteral(
        "INSERT INTO topics (title, notes, initial_date, created_at, status) "
        "VALUES (:title, :notes, :initial_date, :created_at, 'active')"
    ));
    topicQuery.bindValue(QStringLiteral(":title"), title.trimmed());
    topicQuery.bindValue(QStringLiteral(":notes"), notes);
    topicQuery.bindValue(QStringLiteral(":initial_date"), initialDate.toString(QStringLiteral("yyyy-MM-dd")));
    topicQuery.bindValue(QStringLiteral(":created_at"), QDateTime::currentDateTime().toString(Qt::ISODate));

    if (!topicQuery.exec()) {
        qWarning() << "Failed to insert topic:" << topicQuery.lastError().text();
        m_db.rollback();
        return -1;
    }

    const int topicId = topicQuery.lastInsertId().toInt();

    // Schedule 3 reviews: +1 day, +3 days, +5 days
    const int intervals[3] = {1, 3, 5};
    for (int i = 0; i < 3; ++i) {
        const QDate reviewDate = initialDate.addDays(intervals[i]);
        QSqlQuery reviewQuery(m_db);
        reviewQuery.prepare(QStringLiteral(
            "INSERT INTO reviews (topic_id, review_stage, scheduled_date) "
            "VALUES (:topic_id, :review_stage, :scheduled_date)"
        ));
        reviewQuery.bindValue(QStringLiteral(":topic_id"), topicId);
        reviewQuery.bindValue(QStringLiteral(":review_stage"), i + 1);
        reviewQuery.bindValue(QStringLiteral(":scheduled_date"), reviewDate.toString(QStringLiteral("yyyy-MM-dd")));

        if (!reviewQuery.exec()) {
            qWarning() << "Failed to insert review stage" << (i + 1) << ":" << reviewQuery.lastError().text();
            m_db.rollback();
            return -1;
        }
    }

    m_db.commit();
    return topicId;
}

bool Database::updateTopic(int topicId, const QString &title, const QString &notes, const QDate &initialDate) {
    if (topicId <= 0 || title.trimmed().isEmpty())
        return false;

    if (!m_db.transaction()) {
        qWarning() << "Failed to begin transaction for updateTopic:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery topicQuery(m_db);
    topicQuery.prepare(QStringLiteral(
        "UPDATE topics SET title = :title, notes = :notes, initial_date = :initial_date "
        "WHERE id = :id"
    ));
    topicQuery.bindValue(QStringLiteral(":title"), title.trimmed());
    topicQuery.bindValue(QStringLiteral(":notes"), notes);
    topicQuery.bindValue(QStringLiteral(":initial_date"), initialDate.toString(QStringLiteral("yyyy-MM-dd")));
    topicQuery.bindValue(QStringLiteral(":id"), topicId);

    if (!topicQuery.exec()) {
        qWarning() << "Failed to update topic:" << topicQuery.lastError().text();
        m_db.rollback();
        return false;
    }

    // Update scheduled dates for 3 reviews: +1 day, +3 days, +5 days
    const int intervals[3] = {1, 3, 5};
    for (int i = 0; i < 3; ++i) {
        const QDate reviewDate = initialDate.addDays(intervals[i]);
        QSqlQuery reviewQuery(m_db);
        reviewQuery.prepare(QStringLiteral(
            "UPDATE reviews SET scheduled_date = :scheduled_date "
            "WHERE topic_id = :topic_id AND review_stage = :stage"
        ));
        reviewQuery.bindValue(QStringLiteral(":scheduled_date"), reviewDate.toString(QStringLiteral("yyyy-MM-dd")));
        reviewQuery.bindValue(QStringLiteral(":topic_id"), topicId);
        reviewQuery.bindValue(QStringLiteral(":stage"), i + 1);

        if (!reviewQuery.exec()) {
            qWarning() << "Failed to update review stage" << (i + 1) << ":" << reviewQuery.lastError().text();
            m_db.rollback();
            return false;
        }
    }

    if (!m_db.commit()) {
        qWarning() << "Failed to commit transaction for updateTopic:" << m_db.lastError().text();
        m_db.rollback();
        return false;
    }

    return true;
}

bool Database::completeReview(int reviewId) {
    m_db.transaction();

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "UPDATE reviews SET completed_at = :completed_at WHERE id = :id"
    ));
    query.bindValue(QStringLiteral(":completed_at"), QDateTime::currentDateTime().toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":id"), reviewId);

    if (!query.exec()) {
        qWarning() << "Failed to complete review:" << query.lastError().text();
        m_db.rollback();
        return false;
    }

    // Check if stage 3 is completed for the topic
    QSqlQuery checkQuery(m_db);
    checkQuery.prepare(QStringLiteral(
        "SELECT topic_id, review_stage FROM reviews WHERE id = :id"
    ));
    checkQuery.bindValue(QStringLiteral(":id"), reviewId);
    if (checkQuery.exec() && checkQuery.next()) {
        const int topicId = checkQuery.value(0).toInt();
        const int stage = checkQuery.value(1).toInt();

        if (stage == 3) {
            QSqlQuery updateTopic(m_db);
            updateTopic.prepare(QStringLiteral(
                "UPDATE topics SET status = 'completed' WHERE id = :topic_id"
            ));
            updateTopic.bindValue(QStringLiteral(":topic_id"), topicId);
            updateTopic.exec();
        }
    }

    m_db.commit();
    return true;
}

bool Database::uncompleteReview(int reviewId) {
    m_db.transaction();

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "UPDATE reviews SET completed_at = NULL WHERE id = :id"
    ));
    query.bindValue(QStringLiteral(":id"), reviewId);

    if (!query.exec()) {
        m_db.rollback();
        return false;
    }

    // Ensure topic status is active
    QSqlQuery checkQuery(m_db);
    checkQuery.prepare(QStringLiteral("SELECT topic_id FROM reviews WHERE id = :id"));
    checkQuery.bindValue(QStringLiteral(":id"), reviewId);
    if (checkQuery.exec() && checkQuery.next()) {
        const int topicId = checkQuery.value(0).toInt();
        QSqlQuery updateTopic(m_db);
        updateTopic.prepare(QStringLiteral("UPDATE topics SET status = 'active' WHERE id = :topic_id"));
        updateTopic.bindValue(QStringLiteral(":topic_id"), topicId);
        updateTopic.exec();
    }

    m_db.commit();
    return true;
}

bool Database::snoozeReview(int reviewId, int days) {
    const QDate today = QDate::currentDate();
    const QDate newDate = today.addDays(days);

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "UPDATE reviews SET scheduled_date = :new_date WHERE id = :id"
    ));
    query.bindValue(QStringLiteral(":new_date"), newDate.toString(QStringLiteral("yyyy-MM-dd")));
    query.bindValue(QStringLiteral(":id"), reviewId);

    return query.exec();
}

bool Database::updateNotes(int topicId, const QString &notes) {
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "UPDATE topics SET notes = :notes WHERE id = :topic_id"
    ));
    query.bindValue(QStringLiteral(":notes"), notes);
    query.bindValue(QStringLiteral(":topic_id"), topicId);

    return query.exec();
}

bool Database::deleteTopic(int topicId) {
    if (!m_db.isOpen()) {
        return false;
    }

    if (!m_db.transaction()) {
        qWarning() << "Failed to begin transaction for deleting topic:" << m_db.lastError().text();
        return false;
    }

    QSqlQuery delReviews(m_db);
    delReviews.prepare(QStringLiteral("DELETE FROM reviews WHERE topic_id = :topic_id"));
    delReviews.bindValue(QStringLiteral(":topic_id"), topicId);
    if (!delReviews.exec()) {
        qWarning() << "Failed to delete reviews for topic" << topicId << ":" << delReviews.lastError().text();
        m_db.rollback();
        return false;
    }

    QSqlQuery delTopic(m_db);
    delTopic.prepare(QStringLiteral("DELETE FROM topics WHERE id = :topic_id"));
    delTopic.bindValue(QStringLiteral(":topic_id"), topicId);
    if (!delTopic.exec()) {
        qWarning() << "Failed to delete topic" << topicId << ":" << delTopic.lastError().text();
        m_db.rollback();
        return false;
    }

    if (!m_db.commit()) {
        qWarning() << "Failed to commit transaction for deleting topic" << topicId << ":" << m_db.lastError().text();
        m_db.rollback();
        return false;
    }

    return true;
}

bool Database::deleteReviewTopic(int reviewId) {
    if (!m_db.isOpen()) {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT topic_id FROM reviews WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), reviewId);
    if (!query.exec() || !query.next()) {
        qWarning() << "Review id" << reviewId << "not found for deletion";
        return false;
    }

    const int topicId = query.value(0).toInt();
    return deleteTopic(topicId);
}

QVariantList Database::getAgenda(const QString &searchQuery, const QString &dateFilter) const {
    QVariantList items;
    const QDate today = QDate::currentDate();
    const QString todayStr = today.toString(QStringLiteral("yyyy-MM-dd"));
    const QDate tomorrow = today.addDays(1);
    const QString tomorrowStr = tomorrow.toString(QStringLiteral("yyyy-MM-dd"));

    QString sql = QStringLiteral(
        "SELECT r.id, r.topic_id, r.review_stage, r.scheduled_date, r.completed_at, "
        "       t.title, t.notes, t.status, t.initial_date "
        "FROM reviews r "
        "JOIN topics t ON r.topic_id = t.id "
    );

    QStringList conditions;
    const bool hasSearch = !searchQuery.trimmed().isEmpty();
    if (hasSearch) {
        conditions.append(QStringLiteral("(t.title LIKE :query OR t.notes LIKE :query)"));
    }

    const QString cleanDate = dateFilter.trimmed();
    const bool hasDateFilter = !cleanDate.isEmpty();
    QDate filterDate;
    if (hasDateFilter) {
        filterDate = QDate::fromString(cleanDate, QStringLiteral("yyyy-MM-dd"));
        if (filterDate.isValid()) {
            if (filterDate == today) {
                // Today selected: reviews scheduled for today, uncompleted overdue reviews,
                // or reviews completed today (including overdue reviews finished today)
                conditions.append(QStringLiteral(
                    "(r.scheduled_date = :dateFilter OR (r.completed_at IS NULL AND r.scheduled_date < :dateFilter) OR (r.completed_at IS NOT NULL AND substr(r.completed_at, 1, 10) = :dateFilter))"
                ));
            } else {
                conditions.append(QStringLiteral("r.scheduled_date = :dateFilter"));
            }
        }
    }

    if (!conditions.isEmpty()) {
        sql += QStringLiteral("WHERE ") + conditions.join(QStringLiteral(" AND ")) + QStringLiteral(" ");
    }

    sql += QStringLiteral("ORDER BY r.completed_at IS NOT NULL ASC, r.scheduled_date ASC, r.id ASC");

    QSqlQuery query(m_db);
    query.prepare(sql);
    if (hasSearch) {
        const QString pattern = QStringLiteral("%") + searchQuery.trimmed() + QStringLiteral("%");
        query.bindValue(QStringLiteral(":query"), pattern);
    }
    if (hasDateFilter && filterDate.isValid()) {
        query.bindValue(QStringLiteral(":dateFilter"), cleanDate);
    }

    if (!query.exec()) {
        qWarning() << "Failed to fetch agenda:" << query.lastError().text();
        return items;
    }

    while (query.next()) {
        const int reviewId = query.value(0).toInt();
        const int topicId = query.value(1).toInt();
        const int stage = query.value(2).toInt();
        const QString scheduledDateStr = query.value(3).toString();
        const QString completedAtStr = query.value(4).toString();
        const QString title = query.value(5).toString();
        const QString notes = query.value(6).toString();
        const QString status = query.value(7).toString();
        QString initialDateStr = query.value(8).toString();

        const bool isCompleted = !completedAtStr.isEmpty();
        const QDate scheduledDate = QDate::fromString(scheduledDateStr, QStringLiteral("yyyy-MM-dd"));
        const bool isOverdue = (!isCompleted && scheduledDate < today);

        if (initialDateStr.isEmpty() && scheduledDate.isValid()) {
            const int stageOffset = (stage == 1 ? 1 : (stage == 2 ? 3 : 5));
            initialDateStr = scheduledDate.addDays(-stageOffset).toString(QStringLiteral("yyyy-MM-dd"));
        }

        QString section;
        int sectionOrder = 2; // Default Upcoming

        if (isCompleted) {
            section = QStringLiteral("Completed");
            sectionOrder = 99;
        } else if (scheduledDate == today || ((!hasDateFilter || filterDate == today) && isOverdue)) {
            section = QStringLiteral("Today");
            sectionOrder = 0;
        } else if (scheduledDate == tomorrow) {
            section = QStringLiteral("Tomorrow");
            sectionOrder = 1;
        } else {
            // E.g. "Monday, Sep 14"
            section = scheduledDate.toString(QStringLiteral("dddd, MMM d"));
            sectionOrder = 2;
        }

        QVariantMap item;
        item[QStringLiteral("reviewId")] = reviewId;
        item[QStringLiteral("topicId")] = topicId;
        item[QStringLiteral("stage")] = stage;
        item[QStringLiteral("scheduledDate")] = scheduledDateStr;
        item[QStringLiteral("initialDate")] = initialDateStr;
        item[QStringLiteral("formattedDate")] = scheduledDate.toString(QStringLiteral("MMM d"));
        item[QStringLiteral("completedAt")] = completedAtStr;
        item[QStringLiteral("title")] = title;
        item[QStringLiteral("notes")] = notes;
        item[QStringLiteral("status")] = status;
        item[QStringLiteral("isCompleted")] = isCompleted;
        item[QStringLiteral("isOverdue")] = isOverdue;
        item[QStringLiteral("section")] = section;
        item[QStringLiteral("sectionOrder")] = sectionOrder;

        items.append(item);
    }

    return items;
}

QVariantList Database::getDayStrip(const QDate &startDate, int numDays) const {
    QVariantList strip;
    const QDate today = QDate::currentDate();

    // Query pending counts per date
    QSqlQuery query(m_db);
    query.exec(QStringLiteral(
        "SELECT scheduled_date, COUNT(*) FROM reviews "
        "WHERE completed_at IS NULL "
        "GROUP BY scheduled_date"
    ));

    QMap<QString, int> counts;
    while (query.next()) {
        counts[query.value(0).toString()] = query.value(1).toInt();
    }

    // Overdue count (scheduled < today) gets added to today's count
    int overdueCount = 0;
    QSqlQuery overdueQuery(m_db);
    overdueQuery.prepare(QStringLiteral(
        "SELECT COUNT(*) FROM reviews WHERE completed_at IS NULL AND scheduled_date < :today"
    ));
    overdueQuery.bindValue(QStringLiteral(":today"), today.toString(QStringLiteral("yyyy-MM-dd")));
    if (overdueQuery.exec() && overdueQuery.next()) {
        overdueCount = overdueQuery.value(0).toInt();
    }

    for (int i = 0; i < numDays; ++i) {
        const QDate d = startDate.addDays(i);
        const QString dStr = d.toString(QStringLiteral("yyyy-MM-dd"));
        int count = counts.value(dStr, 0);
        if (d == today) {
            count += overdueCount;
        }

        QVariantMap dayItem;
        dayItem[QStringLiteral("date")] = dStr;
        dayItem[QStringLiteral("dayOfWeek")] = d.toString(QStringLiteral("ddd")).toUpper();
        dayItem[QStringLiteral("dayNumber")] = d.toString(QStringLiteral("d"));
        dayItem[QStringLiteral("isToday")] = (d == today);
        dayItem[QStringLiteral("reviewCount")] = count;

        strip.append(dayItem);
    }

    return strip;
}
