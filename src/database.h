#pragma once

#include <QDate>
#include <QObject>
#include <QSqlDatabase>
#include <QVariantList>
#include <QVariantMap>

class Database : public QObject {
    Q_OBJECT

public:
    explicit Database(QObject *parent = nullptr);
    ~Database();

    bool init();

    int addTopic(const QString &title, const QString &notes, const QDate &initialDate);
    bool updateTopic(int topicId, const QString &title, const QString &notes, const QDate &initialDate);
    bool completeReview(int reviewId);
    bool uncompleteReview(int reviewId);
    bool snoozeReview(int reviewId, int days = 1);
    bool updateNotes(int topicId, const QString &notes);
    bool deleteTopic(int topicId);
    bool deleteReviewTopic(int reviewId);

    QSqlDatabase database() const { return m_db; }

    QVariantList getAgenda(const QString &searchQuery = QString(), const QString &dateFilter = QString()) const;
    QVariantList getDayStrip(const QDate &startDate, int numDays = 7) const;

private:
    void ensureSchema();
    QSqlDatabase m_db;
    QString m_dbPath;
};
