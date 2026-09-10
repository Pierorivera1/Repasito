#include <QCoreApplication>
#include <QDate>
#include <QGuiApplication>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtTest>

#include "database.h"
#include "backend.h"

class TestOmacalendar : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testAddTopicAndCadence();
    void testReviewCompletionAndTopicStatus();
    void testOverdueRollover();
    void testSnoozeReview();
    void testSearchFilter();
    void testDeleteCascade();
    void testDeleteCascadeFullLifecycle();
    void testDeleteByReviewId();
    void testDateFilterCadenceAndDeselect();
    void testDateFilterTodayWithOverdue();
    void testDecoupledSearchAndDateFilter();
    void testBackendStateDecoupling();
    void testOverdueReviewCompletedAndSnoozedWithDateFilter();
    void testInvalidAndBoundaryDateFilters();
    void testDayToDayNavigation();
    void testPastDateFilterSectionHeader();
    void testMidnightRolloverNotification();
    void testQmlUiComponentsAndShortcuts();
    void cleanupTestCase();

private:
    Database m_db;
};

void TestOmacalendar::initTestCase() {
    QVERIFY(m_db.init());
}

void TestOmacalendar::testAddTopicAndCadence() {
    const QDate baseDate(2026, 9, 1);
    const int topicId = m_db.addTopic(QStringLiteral("Docker Architecture"),
                                      QStringLiteral("- [ ] Review namespaces\n- [ ] Review cgroups"),
                                      baseDate);
    QVERIFY(topicId > 0);

    const QVariantList agenda = m_db.getAgenda(QStringLiteral("Docker Architecture"));
    // Exactly 3 reviews scheduled
    QCOMPARE(agenda.size(), 3);

    // Review 1: +1 day -> 2026-09-02
    const QVariantMap r1 = agenda.at(0).toMap();
    QCOMPARE(r1.value(QStringLiteral("stage")).toInt(), 1);
    QCOMPARE(r1.value(QStringLiteral("scheduledDate")).toString(), QStringLiteral("2026-09-02"));
    QCOMPARE(r1.value(QStringLiteral("isCompleted")).toBool(), false);

    // Review 2: +3 days -> 2026-09-04
    const QVariantMap r2 = agenda.at(1).toMap();
    QCOMPARE(r2.value(QStringLiteral("stage")).toInt(), 2);
    QCOMPARE(r2.value(QStringLiteral("scheduledDate")).toString(), QStringLiteral("2026-09-04"));

    // Review 3: +5 days -> 2026-09-06
    const QVariantMap r3 = agenda.at(2).toMap();
    QCOMPARE(r3.value(QStringLiteral("stage")).toInt(), 3);
    QCOMPARE(r3.value(QStringLiteral("scheduledDate")).toString(), QStringLiteral("2026-09-06"));

    // Clean up
    m_db.deleteTopic(topicId);
}

void TestOmacalendar::testReviewCompletionAndTopicStatus() {
    const QDate today = QDate::currentDate();
    const int topicId = m_db.addTopic(QStringLiteral("Kubernetes Services"),
                                      QStringLiteral("ClusterIP vs NodePort"),
                                      today);
    QVERIFY(topicId > 0);

    QVariantList agenda = m_db.getAgenda(QStringLiteral("Kubernetes Services"));
    QCOMPARE(agenda.size(), 3);

    const int r1Id = agenda.at(0).toMap().value(QStringLiteral("reviewId")).toInt();
    const int r2Id = agenda.at(1).toMap().value(QStringLiteral("reviewId")).toInt();
    const int r3Id = agenda.at(2).toMap().value(QStringLiteral("reviewId")).toInt();

    // Complete R1
    QVERIFY(m_db.completeReview(r1Id));
    agenda = m_db.getAgenda(QStringLiteral("Kubernetes Services"));
    // Topic should still be active
    QCOMPARE(agenda.at(0).toMap().value(QStringLiteral("status")).toString(), QStringLiteral("active"));

    // Complete R2
    QVERIFY(m_db.completeReview(r2Id));
    agenda = m_db.getAgenda(QStringLiteral("Kubernetes Services"));
    QCOMPARE(agenda.at(0).toMap().value(QStringLiteral("status")).toString(), QStringLiteral("active"));

    // Complete R3 (final review)
    QVERIFY(m_db.completeReview(r3Id));
    agenda = m_db.getAgenda(QStringLiteral("Kubernetes Services"));
    // Now topic should be marked completed
    QCOMPARE(agenda.at(0).toMap().value(QStringLiteral("status")).toString(), QStringLiteral("completed"));

    // Uncomplete R3
    QVERIFY(m_db.uncompleteReview(r3Id));
    agenda = m_db.getAgenda(QStringLiteral("Kubernetes Services"));
    QCOMPARE(agenda.at(0).toMap().value(QStringLiteral("status")).toString(), QStringLiteral("active"));

    m_db.deleteTopic(topicId);
}

void TestOmacalendar::testOverdueRollover() {
    // Topic studied 10 days ago: reviews were at -9d, -7d, -5d
    const QDate pastDate = QDate::currentDate().addDays(-10);
    const int topicId = m_db.addTopic(QStringLiteral("Past Topic"), QString(), pastDate);
    QVERIFY(topicId > 0);

    const QVariantList agenda = m_db.getAgenda(QStringLiteral("Past Topic"));
    QCOMPARE(agenda.size(), 3);

    for (const QVariant &itemVar : agenda) {
        const QVariantMap item = itemVar.toMap();
        // Since past reviews are uncompleted, they must be marked overdue
        QCOMPARE(item.value(QStringLiteral("isOverdue")).toBool(), true);
        // And roll over into the "Today" section!
        QCOMPARE(item.value(QStringLiteral("section")).toString(), QStringLiteral("Today"));
    }

    m_db.deleteTopic(topicId);
}

void TestOmacalendar::testSnoozeReview() {
    const QDate pastDate = QDate::currentDate().addDays(-2);
    const int topicId = m_db.addTopic(QStringLiteral("Snooze Topic"), QString(), pastDate);
    QVERIFY(topicId > 0);

    QVariantList agenda = m_db.getAgenda(QStringLiteral("Snooze Topic"));
    const int r1Id = agenda.at(0).toMap().value(QStringLiteral("reviewId")).toInt();

    // Snooze 1 day from today
    QVERIFY(m_db.snoozeReview(r1Id, 1));

    agenda = m_db.getAgenda(QStringLiteral("Snooze Topic"));
    for (const QVariant &itemVar : agenda) {
        const QVariantMap item = itemVar.toMap();
        if (item.value(QStringLiteral("reviewId")).toInt() == r1Id) {
            const QDate expectedTomorrow = QDate::currentDate().addDays(1);
            QCOMPARE(item.value(QStringLiteral("scheduledDate")).toString(),
                     expectedTomorrow.toString(QStringLiteral("yyyy-MM-dd")));
            QCOMPARE(item.value(QStringLiteral("section")).toString(), QStringLiteral("Tomorrow"));
            QCOMPARE(item.value(QStringLiteral("isOverdue")).toBool(), false);
        }
    }

    m_db.deleteTopic(topicId);
}

void TestOmacalendar::testSearchFilter() {
    const int t1 = m_db.addTopic(QStringLiteral("Rust Borrow Checker"), QStringLiteral("Lifetimes and ownership"), QDate::currentDate());
    const int t2 = m_db.addTopic(QStringLiteral("Go Goroutines"), QStringLiteral("Channels and select"), QDate::currentDate());

    QVERIFY(m_db.getAgenda(QStringLiteral("Rust")).size() >= 3);
    QVERIFY(m_db.getAgenda(QStringLiteral("Channels")).size() >= 3);
    QCOMPARE(m_db.getAgenda(QStringLiteral("NonexistentQueryXYZ")).size(), 0);

    m_db.deleteTopic(t1);
    m_db.deleteTopic(t2);
}

void TestOmacalendar::testDeleteCascade() {
    const int topicId = m_db.addTopic(QStringLiteral("Delete Me"), QStringLiteral("Notes"), QDate::currentDate());
    QVERIFY(topicId > 0);
    QCOMPARE(m_db.getAgenda(QStringLiteral("Delete Me")).size(), 3);

    QVERIFY(m_db.deleteTopic(topicId));
    QCOMPARE(m_db.getAgenda(QStringLiteral("Delete Me")).size(), 0);
}

void TestOmacalendar::testDeleteCascadeFullLifecycle() {
    Backend backend;
    const QDate today = QDate::currentDate();
    const QString date1 = today.addDays(1).toString(QStringLiteral("yyyy-MM-dd"));
    const QString date2 = today.addDays(3).toString(QStringLiteral("yyyy-MM-dd"));
    const QString date3 = today.addDays(5).toString(QStringLiteral("yyyy-MM-dd"));

    auto getDayCount = [](const QVariantList &strip, const QString &targetDate) -> int {
        for (const QVariant &itemVar : strip) {
            const QVariantMap item = itemVar.toMap();
            if (item.value(QStringLiteral("date")).toString() == targetDate) {
                return item.value(QStringLiteral("reviewCount")).toInt();
            }
        }
        return -1;
    };

    // 1. Initial DayStrip counts before adding the topic
    const QVariantList initialStrip = backend.dayStrip();
    const int countBefore1 = getDayCount(initialStrip, date1);
    const int countBefore2 = getDayCount(initialStrip, date2);
    const int countBefore3 = getDayCount(initialStrip, date3);
    QVERIFY(countBefore1 >= 0 && countBefore2 >= 0 && countBefore3 >= 0);

    // 2. Register a new topic with initial date today
    const QString topicTitle = QStringLiteral("Kubernetes Operator Pattern");
    const QString topicNotes = QStringLiteral("Custom Resource Definitions & Controllers");
    const int topicId = backend.database().addTopic(topicTitle, topicNotes, today);
    QVERIFY(topicId > 0);

    backend.refresh();

    // 3. Verify 3 reviews are generated on today+1 (R1), today+3 (R2), today+5 (R3)
    const QVariantList topicAgenda = backend.database().getAgenda(topicTitle, QString());
    QCOMPARE(topicAgenda.size(), 3);

    const QVariantMap r1Item = topicAgenda.at(0).toMap();
    const QVariantMap r2Item = topicAgenda.at(1).toMap();
    const QVariantMap r3Item = topicAgenda.at(2).toMap();

    QCOMPARE(r1Item.value(QStringLiteral("stage")).toInt(), 1);
    QCOMPARE(r1Item.value(QStringLiteral("scheduledDate")).toString(), date1);
    QCOMPARE(r1Item.value(QStringLiteral("topicId")).toInt(), topicId);

    QCOMPARE(r2Item.value(QStringLiteral("stage")).toInt(), 2);
    QCOMPARE(r2Item.value(QStringLiteral("scheduledDate")).toString(), date2);
    QCOMPARE(r2Item.value(QStringLiteral("topicId")).toInt(), topicId);

    QCOMPARE(r3Item.value(QStringLiteral("stage")).toInt(), 3);
    QCOMPARE(r3Item.value(QStringLiteral("scheduledDate")).toString(), date3);
    QCOMPARE(r3Item.value(QStringLiteral("topicId")).toInt(), topicId);

    // 4. Verify getAgenda("", date1) contains R1
    const QVariantList agendaDate1 = backend.database().getAgenda(QString(), date1);
    bool date1HasR1 = false;
    for (const QVariant &itemVar : agendaDate1) {
        const QVariantMap item = itemVar.toMap();
        if (item.value(QStringLiteral("topicId")).toInt() == topicId && item.value(QStringLiteral("stage")).toInt() == 1) {
            date1HasR1 = true;
            break;
        }
    }
    QVERIFY(date1HasR1);

    // 5. Verify getAgenda("", date2) contains R2
    const QVariantList agendaDate2 = backend.database().getAgenda(QString(), date2);
    bool date2HasR2 = false;
    for (const QVariant &itemVar : agendaDate2) {
        const QVariantMap item = itemVar.toMap();
        if (item.value(QStringLiteral("topicId")).toInt() == topicId && item.value(QStringLiteral("stage")).toInt() == 2) {
            date2HasR2 = true;
            break;
        }
    }
    QVERIFY(date2HasR2);

    // 6. Verify getAgenda("", date3) contains R3
    const QVariantList agendaDate3 = backend.database().getAgenda(QString(), date3);
    bool date3HasR3 = false;
    for (const QVariant &itemVar : agendaDate3) {
        const QVariantMap item = itemVar.toMap();
        if (item.value(QStringLiteral("topicId")).toInt() == topicId && item.value(QStringLiteral("stage")).toInt() == 3) {
            date3HasR3 = true;
            break;
        }
    }
    QVERIFY(date3HasR3);

    // 7. Check DayStrip reflects review counts on those days (each incremented by 1)
    const QVariantList stripAfterAdd = backend.dayStrip();
    QCOMPARE(getDayCount(stripAfterAdd, date1), countBefore1 + 1);
    QCOMPARE(getDayCount(stripAfterAdd, date2), countBefore2 + 1);
    QCOMPARE(getDayCount(stripAfterAdd, date3), countBefore3 + 1);

    // 8. Verify raw database initially has 1 topic and 3 reviews
    {
        QSqlQuery topicCountQuery(backend.database().database());
        topicCountQuery.prepare(QStringLiteral("SELECT COUNT(*) FROM topics WHERE id = :id"));
        topicCountQuery.bindValue(QStringLiteral(":id"), topicId);
        QVERIFY(topicCountQuery.exec() && topicCountQuery.next());
        QCOMPARE(topicCountQuery.value(0).toInt(), 1);

        QSqlQuery reviewsCountQuery(backend.database().database());
        reviewsCountQuery.prepare(QStringLiteral("SELECT COUNT(*) FROM reviews WHERE topic_id = :topic_id"));
        reviewsCountQuery.bindValue(QStringLiteral(":topic_id"), topicId);
        QVERIFY(reviewsCountQuery.exec() && reviewsCountQuery.next());
        QCOMPARE(reviewsCountQuery.value(0).toInt(), 3);
    }

    // 9. Simulate viewing date1 and deleting the registered topic via backend.deleteTopic(topicId)
    backend.setSelectedDate(date1);
    QCOMPARE(backend.selectedDate(), date1);
    QVERIFY(backend.deleteTopic(topicId));

    // 10. Verify R1 is deleted from date1
    const QVariantList agendaDate1AfterDel = backend.database().getAgenda(QString(), date1);
    for (const QVariant &itemVar : agendaDate1AfterDel) {
        const QVariantMap item = itemVar.toMap();
        QVERIFY(item.value(QStringLiteral("topicId")).toInt() != topicId);
        QVERIFY(item.value(QStringLiteral("title")).toString() != topicTitle);
    }

    // 11. Crucially verify that ALL subsequent reviews (R2 on date2, R3 on date3) created from this topic are also deleted!
    const QVariantList agendaDate2AfterDel = backend.database().getAgenda(QString(), date2);
    for (const QVariant &itemVar : agendaDate2AfterDel) {
        const QVariantMap item = itemVar.toMap();
        QVERIFY(item.value(QStringLiteral("topicId")).toInt() != topicId);
        QVERIFY(item.value(QStringLiteral("title")).toString() != topicTitle);
    }

    const QVariantList agendaDate3AfterDel = backend.database().getAgenda(QString(), date3);
    for (const QVariant &itemVar : agendaDate3AfterDel) {
        const QVariantMap item = itemVar.toMap();
        QVERIFY(item.value(QStringLiteral("topicId")).toInt() != topicId);
        QVERIFY(item.value(QStringLiteral("title")).toString() != topicTitle);
    }

    // 12. Verify getAgenda for topic across all dates returns 0 reviews
    QCOMPARE(backend.database().getAgenda(topicTitle, QString()).size(), 0);

    // 13. Query raw database: SELECT COUNT(*) FROM reviews WHERE topic_id = ... and verify it equals 0
    {
        QSqlQuery reviewsCountQuery(backend.database().database());
        reviewsCountQuery.prepare(QStringLiteral("SELECT COUNT(*) FROM reviews WHERE topic_id = :topic_id"));
        reviewsCountQuery.bindValue(QStringLiteral(":topic_id"), topicId);
        QVERIFY(reviewsCountQuery.exec() && reviewsCountQuery.next());
        QCOMPARE(reviewsCountQuery.value(0).toInt(), 0);
    }

    // 14. Query raw database: SELECT COUNT(*) FROM topics WHERE id = ... and verify it equals 0
    {
        QSqlQuery topicCountQuery(backend.database().database());
        topicCountQuery.prepare(QStringLiteral("SELECT COUNT(*) FROM topics WHERE id = :id"));
        topicCountQuery.bindValue(QStringLiteral(":id"), topicId);
        QVERIFY(topicCountQuery.exec() && topicCountQuery.next());
        QCOMPARE(topicCountQuery.value(0).toInt(), 0);
    }

    // 15. Verify DayStrip counts for all 3 dates update and decrement appropriately back to initial counts
    const QVariantList stripAfterDel = backend.dayStrip();
    QCOMPARE(getDayCount(stripAfterDel, date1), countBefore1);
    QCOMPARE(getDayCount(stripAfterDel, date2), countBefore2);
    QCOMPARE(getDayCount(stripAfterDel, date3), countBefore3);
}

void TestOmacalendar::testDeleteByReviewId() {
    Backend backend;
    const QDate today = QDate::currentDate();
    const QString date1 = today.addDays(1).toString(QStringLiteral("yyyy-MM-dd"));
    const QString date2 = today.addDays(3).toString(QStringLiteral("yyyy-MM-dd"));
    const QString date3 = today.addDays(5).toString(QStringLiteral("yyyy-MM-dd"));

    const QString topicTitle = QStringLiteral("Raft Consensus Algorithm");
    const int topicId = backend.database().addTopic(topicTitle, QStringLiteral("Leader election notes"), today);
    QVERIFY(topicId > 0);

    backend.refresh();
    const QVariantList agenda = backend.database().getAgenda(topicTitle, QString());
    QCOMPARE(agenda.size(), 3);

    // Pick the middle review (R2 on date2)
    const int r2Id = agenda.at(1).toMap().value(QStringLiteral("reviewId")).toInt();
    QVERIFY(r2Id > 0);

    // Delete using the review ID of stage 2
    QVERIFY(backend.deleteReviewTopic(r2Id));

    // Verify raw database has 0 topics and 0 reviews
    {
        QSqlQuery reviewsCountQuery(backend.database().database());
        reviewsCountQuery.prepare(QStringLiteral("SELECT COUNT(*) FROM reviews WHERE topic_id = :topic_id"));
        reviewsCountQuery.bindValue(QStringLiteral(":topic_id"), topicId);
        QVERIFY(reviewsCountQuery.exec() && reviewsCountQuery.next());
        QCOMPARE(reviewsCountQuery.value(0).toInt(), 0);

        QSqlQuery topicCountQuery(backend.database().database());
        topicCountQuery.prepare(QStringLiteral("SELECT COUNT(*) FROM topics WHERE id = :id"));
        topicCountQuery.bindValue(QStringLiteral(":id"), topicId);
        QVERIFY(topicCountQuery.exec() && topicCountQuery.next());
        QCOMPARE(topicCountQuery.value(0).toInt(), 0);
    }

    // Agenda for all days must return 0
    QCOMPARE(backend.database().getAgenda(topicTitle, QString()).size(), 0);
    QCOMPARE(backend.database().getAgenda(topicTitle, date1).size(), 0);
    QCOMPARE(backend.database().getAgenda(topicTitle, date2).size(), 0);
    QCOMPARE(backend.database().getAgenda(topicTitle, date3).size(), 0);
}

void TestOmacalendar::testDateFilterCadenceAndDeselect() {
    const QDate today = QDate::currentDate();
    const QString tomorrowStr = today.addDays(1).toString(QStringLiteral("yyyy-MM-dd"));
    const QString day3Str = today.addDays(3).toString(QStringLiteral("yyyy-MM-dd"));
    const QString day5Str = today.addDays(5).toString(QStringLiteral("yyyy-MM-dd"));

    const int topicId = m_db.addTopic(QStringLiteral("Date Filter Topic"),
                                      QStringLiteral("Testing date filtering"),
                                      today);
    QVERIFY(topicId > 0);

    // Filter by tomorrow: should show ONLY R1
    const QVariantList tomorrowAgenda = m_db.getAgenda(QStringLiteral("Date Filter Topic"), tomorrowStr);
    QCOMPARE(tomorrowAgenda.size(), 1);
    const QVariantMap r1 = tomorrowAgenda.at(0).toMap();
    QCOMPARE(r1.value(QStringLiteral("stage")).toInt(), 1);
    QCOMPARE(r1.value(QStringLiteral("scheduledDate")).toString(), tomorrowStr);

    // Filter by day 3: should show ONLY R2
    const QVariantList day3Agenda = m_db.getAgenda(QStringLiteral("Date Filter Topic"), day3Str);
    QCOMPARE(day3Agenda.size(), 1);
    const QVariantMap r2 = day3Agenda.at(0).toMap();
    QCOMPARE(r2.value(QStringLiteral("stage")).toInt(), 2);
    QCOMPARE(r2.value(QStringLiteral("scheduledDate")).toString(), day3Str);

    // Filter by day 5: should show ONLY R3
    const QVariantList day5Agenda = m_db.getAgenda(QStringLiteral("Date Filter Topic"), day5Str);
    QCOMPARE(day5Agenda.size(), 1);
    const QVariantMap r3 = day5Agenda.at(0).toMap();
    QCOMPARE(r3.value(QStringLiteral("stage")).toInt(), 3);
    QCOMPARE(r3.value(QStringLiteral("scheduledDate")).toString(), day5Str);

    // Query on an off-day (e.g. today + 2 days): should show 0 reviews
    const QString day2Str = today.addDays(2).toString(QStringLiteral("yyyy-MM-dd"));
    const QVariantList day2Agenda = m_db.getAgenda(QStringLiteral("Date Filter Topic"), day2Str);
    QCOMPARE(day2Agenda.size(), 0);

    // Deselect date (empty date filter): restores full multi-day agenda with all 3 reviews
    const QVariantList fullAgenda = m_db.getAgenda(QStringLiteral("Date Filter Topic"), QString());
    QCOMPARE(fullAgenda.size(), 3);

    m_db.deleteTopic(topicId);
}

void TestOmacalendar::testDateFilterTodayWithOverdue() {
    const QDate today = QDate::currentDate();
    const QString todayStr = today.toString(QStringLiteral("yyyy-MM-dd"));
    const QString tomorrowStr = today.addDays(1).toString(QStringLiteral("yyyy-MM-dd"));

    // Topic A: studied 6 days ago (overdue uncompleted reviews)
    const int topicOverdue = m_db.addTopic(QStringLiteral("Overdue Study"), QStringLiteral("Past"), today.addDays(-6));
    QVERIFY(topicOverdue > 0);

    // Topic B: studied yesterday (R1 scheduled for today)
    const int topicToday = m_db.addTopic(QStringLiteral("Yesterday Study"), QStringLiteral("Current"), today.addDays(-1));
    QVERIFY(topicToday > 0);

    // Topic C: studied today (R1 scheduled for tomorrow)
    const int topicTomorrow = m_db.addTopic(QStringLiteral("Today Study"), QStringLiteral("Future"), today);
    QVERIFY(topicTomorrow > 0);

    // Query with Today selected: must include reviews scheduled for today OR overdue reviews
    const QVariantList todayAgenda = m_db.getAgenda(QString(), todayStr);
    bool foundOverdue = false;
    bool foundToday = false;
    bool foundTomorrow = false;

    for (const QVariant &itemVar : todayAgenda) {
        const QVariantMap item = itemVar.toMap();
        const QString title = item.value(QStringLiteral("title")).toString();
        if (title == QStringLiteral("Overdue Study")) {
            foundOverdue = true;
            QVERIFY(item.value(QStringLiteral("isOverdue")).toBool());
            QCOMPARE(item.value(QStringLiteral("section")).toString(), QStringLiteral("Today"));
        } else if (title == QStringLiteral("Yesterday Study") && item.value(QStringLiteral("stage")).toInt() == 1) {
            foundToday = true;
            QCOMPARE(item.value(QStringLiteral("scheduledDate")).toString(), todayStr);
            QCOMPARE(item.value(QStringLiteral("section")).toString(), QStringLiteral("Today"));
        } else if (title == QStringLiteral("Today Study") && item.value(QStringLiteral("stage")).toInt() == 1) {
            foundTomorrow = true;
        }
    }

    QVERIFY(foundOverdue);
    QVERIFY(foundToday);
    QVERIFY(!foundTomorrow); // Tomorrow's review must NOT appear in Today's filter!

    // Query with Tomorrow selected: must show Topic C R1, and NOT overdue or today's reviews
    const QVariantList tomorrowAgenda = m_db.getAgenda(QString(), tomorrowStr);
    bool tomorrowHasTopicC = false;
    bool tomorrowHasOverdue = false;
    bool tomorrowHasTodayStudy = false;

    for (const QVariant &itemVar : tomorrowAgenda) {
        const QVariantMap item = itemVar.toMap();
        const QString title = item.value(QStringLiteral("title")).toString();
        if (title == QStringLiteral("Today Study") && item.value(QStringLiteral("stage")).toInt() == 1) {
            tomorrowHasTopicC = true;
        } else if (title == QStringLiteral("Overdue Study")) {
            tomorrowHasOverdue = true;
        } else if (title == QStringLiteral("Yesterday Study") && item.value(QStringLiteral("stage")).toInt() == 1) {
            tomorrowHasTodayStudy = true;
        }
    }

    QVERIFY(tomorrowHasTopicC);
    QVERIFY(!tomorrowHasOverdue);
    QVERIFY(!tomorrowHasTodayStudy);

    m_db.deleteTopic(topicOverdue);
    m_db.deleteTopic(topicToday);
    m_db.deleteTopic(topicTomorrow);
}

void TestOmacalendar::testDecoupledSearchAndDateFilter() {
    const QDate today = QDate::currentDate();
    const QString tomorrowStr = today.addDays(1).toString(QStringLiteral("yyyy-MM-dd"));

    const int t1 = m_db.addTopic(QStringLiteral("Systems Paxos"), QStringLiteral("Consensus notes"), today);
    const int t2 = m_db.addTopic(QStringLiteral("Systems Raft"), QStringLiteral("Leader notes"), today.addDays(-2));
    const int t3 = m_db.addTopic(QStringLiteral("Compiler LLVM"), QStringLiteral("IR optimization"), today);

    QVERIFY(t1 > 0 && t2 > 0 && t3 > 0);

    // 1. Date filter only (tomorrow): includes t1 (R1), t2 (R2), t3 (R1)
    const QVariantList dateOnly = m_db.getAgenda(QString(), tomorrowStr);
    bool hasT1 = false, hasT2 = false, hasT3 = false;
    for (const QVariant &v : dateOnly) {
        const QString title = v.toMap().value(QStringLiteral("title")).toString();
        if (title == QStringLiteral("Systems Paxos")) hasT1 = true;
        if (title == QStringLiteral("Systems Raft")) hasT2 = true;
        if (title == QStringLiteral("Compiler LLVM")) hasT3 = true;
    }
    QVERIFY(hasT1);
    QVERIFY(hasT2);
    QVERIFY(hasT3);

    // 2. Search query only: search "Systems" across all dates
    const QVariantList searchOnly = m_db.getAgenda(QStringLiteral("Systems"), QString());
    for (const QVariant &v : searchOnly) {
        const QString title = v.toMap().value(QStringLiteral("title")).toString();
        QVERIFY(title.startsWith(QStringLiteral("Systems")));
        QVERIFY(title != QStringLiteral("Compiler LLVM"));
    }

    // 3. Both date AND search query active: "Systems" on tomorrow
    const QVariantList combined = m_db.getAgenda(QStringLiteral("Systems"), tomorrowStr);
    hasT1 = false;
    hasT2 = false;
    hasT3 = false;
    for (const QVariant &v : combined) {
        const QString title = v.toMap().value(QStringLiteral("title")).toString();
        const QString sDate = v.toMap().value(QStringLiteral("scheduledDate")).toString();
        QCOMPARE(sDate, tomorrowStr);
        if (title == QStringLiteral("Systems Paxos")) hasT1 = true;
        if (title == QStringLiteral("Systems Raft")) hasT2 = true;
        if (title == QStringLiteral("Compiler LLVM")) hasT3 = true;
    }
    QVERIFY(hasT1);
    QVERIFY(hasT2);
    QVERIFY(!hasT3); // Compiler LLVM filtered out by search

    // 4. Combined search for "Raft" on tomorrow: only t2
    const QVariantList raftTomorrow = m_db.getAgenda(QStringLiteral("Raft"), tomorrowStr);
    QCOMPARE(raftTomorrow.size(), 1);
    QCOMPARE(raftTomorrow.at(0).toMap().value(QStringLiteral("title")).toString(), QStringLiteral("Systems Raft"));

    // 5. Search for non-existent keyword on tomorrow: 0 results
    const QVariantList noMatch = m_db.getAgenda(QStringLiteral("UnknownKeyword999"), tomorrowStr);
    QCOMPARE(noMatch.size(), 0);

    // 6. Clear search while keeping date filter: preserves date filter and returns all 3 topics
    const QVariantList clearedSearch = m_db.getAgenda(QString(), tomorrowStr);
    hasT1 = false; hasT2 = false; hasT3 = false;
    for (const QVariant &v : clearedSearch) {
        const QString title = v.toMap().value(QStringLiteral("title")).toString();
        if (title == QStringLiteral("Systems Paxos")) hasT1 = true;
        if (title == QStringLiteral("Systems Raft")) hasT2 = true;
        if (title == QStringLiteral("Compiler LLVM")) hasT3 = true;
    }
    QVERIFY(hasT1 && hasT2 && hasT3);

    m_db.deleteTopic(t1);
    m_db.deleteTopic(t2);
    m_db.deleteTopic(t3);
}

void TestOmacalendar::testBackendStateDecoupling() {
    Backend backend;
    const QDate today = QDate::currentDate();
    const QString tomorrowStr = today.addDays(1).toString(QStringLiteral("yyyy-MM-dd"));

    QSignalSpy dateSpy(&backend, &Backend::selectedDateChanged);
    QSignalSpy querySpy(&backend, &Backend::searchQueryChanged);
    QSignalSpy agendaSpy(&backend, &Backend::agendaChanged);

    // Initial state
    QCOMPARE(backend.selectedDate(), QString());
    QCOMPARE(backend.searchQuery(), QString());
    QCOMPARE(backend.selectedDateDisplay(), QString());

    // 1. Selecting date must NOT touch search query
    backend.setSelectedDate(tomorrowStr);
    QCOMPARE(backend.selectedDate(), tomorrowStr);
    QCOMPARE(backend.searchQuery(), QString()); // Search bar remains empty!
    QCOMPARE(dateSpy.count(), 1);
    QCOMPARE(querySpy.count(), 0); // Decoupled!
    QCOMPARE(agendaSpy.count(), 1);
    QVERIFY(backend.selectedDateDisplay().contains(QStringLiteral("Tomorrow")));

    // 2. Entering text search must NOT alter selected date
    backend.setSearchQuery(QStringLiteral("Linux Kernel"));
    QCOMPARE(backend.searchQuery(), QStringLiteral("Linux Kernel"));
    QCOMPARE(backend.selectedDate(), tomorrowStr); // Selected date preserved!
    QCOMPARE(querySpy.count(), 1);
    QCOMPARE(dateSpy.count(), 1); // No change to date!

    // 3. Clearing search must preserve selected date
    backend.setSearchQuery(QString());
    QCOMPARE(backend.searchQuery(), QString());
    QCOMPARE(backend.selectedDate(), tomorrowStr); // Still tomorrow!
    QCOMPARE(querySpy.count(), 2);
    QCOMPARE(dateSpy.count(), 1);

    // 4. Toggle behavior (clicking selected date a second time deselects it)
    QString nextDate = (backend.selectedDate() == tomorrowStr) ? QString() : tomorrowStr;
    backend.setSelectedDate(nextDate);
    QCOMPARE(backend.selectedDate(), QString());
    QCOMPARE(backend.selectedDateDisplay(), QString());
    QCOMPARE(dateSpy.count(), 2);

    // 5. Clear via clearSelectedDate()
    backend.setSelectedDate(tomorrowStr);
    QCOMPARE(backend.selectedDate(), tomorrowStr);
    backend.clearSelectedDate();
    QCOMPARE(backend.selectedDate(), QString());
}

void TestOmacalendar::testOverdueReviewCompletedAndSnoozedWithDateFilter() {
    const QDate today = QDate::currentDate();
    const QString todayStr = today.toString(QStringLiteral("yyyy-MM-dd"));
    const QString tomorrowStr = today.addDays(1).toString(QStringLiteral("yyyy-MM-dd"));

    // Create a topic studied 4 days ago -> R1 was 3 days ago (overdue)
    const int topicId = m_db.addTopic(QStringLiteral("Overdue Lifecycle"),
                                      QStringLiteral("Testing overdue completion"),
                                      today.addDays(-4));
    QVERIFY(topicId > 0);

    const QVariantList initialAgenda = m_db.getAgenda(QStringLiteral("Overdue Lifecycle"), todayStr);
    QVERIFY(!initialAgenda.isEmpty());

    int r1Id = -1;
    for (const QVariant &itemVar : initialAgenda) {
        const QVariantMap item = itemVar.toMap();
        if (item.value(QStringLiteral("stage")).toInt() == 1) {
            r1Id = item.value(QStringLiteral("reviewId")).toInt();
            QCOMPARE(item.value(QStringLiteral("isCompleted")).toBool(), false);
            QCOMPARE(item.value(QStringLiteral("isOverdue")).toBool(), true);
            QCOMPARE(item.value(QStringLiteral("section")).toString(), QStringLiteral("Today"));
        }
    }
    QVERIFY(r1Id > 0);

    // 1. Complete overdue review: must NOT vanish! Must remain in Today's agenda under section "Completed"
    QVERIFY(m_db.completeReview(r1Id));

    const QVariantList completedAgenda = m_db.getAgenda(QStringLiteral("Overdue Lifecycle"), todayStr);
    bool foundCompletedInToday = false;
    for (const QVariant &itemVar : completedAgenda) {
        const QVariantMap item = itemVar.toMap();
        if (item.value(QStringLiteral("reviewId")).toInt() == r1Id) {
            foundCompletedInToday = true;
            QCOMPARE(item.value(QStringLiteral("isCompleted")).toBool(), true);
            QCOMPARE(item.value(QStringLiteral("section")).toString(), QStringLiteral("Completed"));
        }
    }
    QVERIFY(foundCompletedInToday);

    // 2. Uncomplete review: returns to section "Today" with isOverdue=true
    QVERIFY(m_db.uncompleteReview(r1Id));
    const QVariantList uncompletedAgenda = m_db.getAgenda(QStringLiteral("Overdue Lifecycle"), todayStr);
    bool foundUncompletedInToday = false;
    for (const QVariant &itemVar : uncompletedAgenda) {
        const QVariantMap item = itemVar.toMap();
        if (item.value(QStringLiteral("reviewId")).toInt() == r1Id) {
            foundUncompletedInToday = true;
            QCOMPARE(item.value(QStringLiteral("isCompleted")).toBool(), false);
            QCOMPARE(item.value(QStringLiteral("isOverdue")).toBool(), true);
            QCOMPARE(item.value(QStringLiteral("section")).toString(), QStringLiteral("Today"));
        }
    }
    QVERIFY(foundUncompletedInToday);

    // 3. Snooze review by 1 day: moves to tomorrow
    QVERIFY(m_db.snoozeReview(r1Id, 1));

    // Must no longer appear in Today's filter
    const QVariantList afterSnoozeToday = m_db.getAgenda(QStringLiteral("Overdue Lifecycle"), todayStr);
    bool inTodayAfterSnooze = false;
    for (const QVariant &itemVar : afterSnoozeToday) {
        if (itemVar.toMap().value(QStringLiteral("reviewId")).toInt() == r1Id) {
            inTodayAfterSnooze = true;
        }
    }
    QVERIFY(!inTodayAfterSnooze);

    // Must appear in Tomorrow's filter
    const QVariantList afterSnoozeTomorrow = m_db.getAgenda(QStringLiteral("Overdue Lifecycle"), tomorrowStr);
    bool inTomorrowAfterSnooze = false;
    for (const QVariant &itemVar : afterSnoozeTomorrow) {
        const QVariantMap item = itemVar.toMap();
        if (item.value(QStringLiteral("reviewId")).toInt() == r1Id) {
            inTomorrowAfterSnooze = true;
            QCOMPARE(item.value(QStringLiteral("scheduledDate")).toString(), tomorrowStr);
            QCOMPARE(item.value(QStringLiteral("section")).toString(), QStringLiteral("Tomorrow"));
            QCOMPARE(item.value(QStringLiteral("isOverdue")).toBool(), false);
        }
    }
    QVERIFY(inTomorrowAfterSnooze);

    m_db.deleteTopic(topicId);
}

void TestOmacalendar::testInvalidAndBoundaryDateFilters() {
    const int topicId = m_db.addTopic(QStringLiteral("Boundary Date Topic"), QStringLiteral("Boundary notes"), QDate::currentDate());
    QVERIFY(topicId > 0);

    // Empty date string falls back to full agenda
    const QVariantList emptyFilter = m_db.getAgenda(QStringLiteral("Boundary Date Topic"), QString());
    QCOMPARE(emptyFilter.size(), 3);

    // Whitespace date string falls back to full agenda
    const QVariantList wsFilter = m_db.getAgenda(QStringLiteral("Boundary Date Topic"), QStringLiteral("   "));
    QCOMPARE(wsFilter.size(), 3);

    // Malformed date string falls back gracefully without errors
    const QVariantList invalidFilter = m_db.getAgenda(QStringLiteral("Boundary Date Topic"), QStringLiteral("not-a-valid-date"));
    QCOMPARE(invalidFilter.size(), 3);

    // Backend formatting for invalid date returns raw string gracefully
    Backend backend;
    backend.setSelectedDate(QStringLiteral("invalid-format"));
    QCOMPARE(backend.selectedDateDisplay(), QStringLiteral("invalid-format"));
    backend.clearSelectedDate();
    QCOMPARE(backend.selectedDateDisplay(), QString());

    m_db.deleteTopic(topicId);
}

void TestOmacalendar::testDayToDayNavigation() {
    Backend backend;
    const QDate today = QDate::currentDate();
    const QString todayStr = today.toString(QStringLiteral("yyyy-MM-dd"));
    const QString tomorrowStr = today.addDays(1).toString(QStringLiteral("yyyy-MM-dd"));
    const QString day2Str = today.addDays(2).toString(QStringLiteral("yyyy-MM-dd"));
    const QString yesterdayStr = today.addDays(-1).toString(QStringLiteral("yyyy-MM-dd"));

    QSignalSpy dateSpy(&backend, &Backend::selectedDateChanged);
    QSignalSpy stripSpy(&backend, &Backend::dayStripChanged);

    // Initial state: selectedDate is empty
    QCOMPARE(backend.selectedDate(), QString());

    // 1. Calling nextDay() when selectedDate is empty selects tomorrow
    backend.nextDay();
    QCOMPARE(backend.selectedDate(), tomorrowStr);
    QCOMPARE(dateSpy.count(), 1);
    QCOMPARE(stripSpy.count(), 1);

    // 2. Calling nextDay() again advances to +2 days
    backend.nextDay();
    QCOMPARE(backend.selectedDate(), day2Str);
    QCOMPARE(dateSpy.count(), 2);
    QCOMPARE(stripSpy.count(), 2);

    // 3. Calling previousDay() goes back to +1 day, then today, then yesterday (-1 day)
    backend.previousDay();
    QCOMPARE(backend.selectedDate(), tomorrowStr);
    QCOMPARE(dateSpy.count(), 3);
    QCOMPARE(stripSpy.count(), 3);

    backend.previousDay();
    QCOMPARE(backend.selectedDate(), todayStr);
    QCOMPARE(dateSpy.count(), 4);
    QCOMPARE(stripSpy.count(), 4);

    backend.previousDay();
    QCOMPARE(backend.selectedDate(), yesterdayStr);
    QCOMPARE(dateSpy.count(), 5);
    QCOMPARE(stripSpy.count(), 5);

    // 4. Test that DayStrip reflects the visible range when selected date is in the past (yesterday)
    // When sel < today, stripStart = sel = yesterday
    QVariantList strip = backend.dayStrip();
    QCOMPARE(strip.size(), 7);
    QCOMPARE(strip.first().toMap().value(QStringLiteral("date")).toString(), yesterdayStr);
    QCOMPARE(strip.last().toMap().value(QStringLiteral("date")).toString(), today.addDays(5).toString(QStringLiteral("yyyy-MM-dd")));

    // 5. Advance DayStrip into the future beyond default week (+8 days)
    for (int i = 0; i < 9; ++i) { // from -1 to +8 is 9 nextDay calls
        backend.nextDay();
    }
    const QString day8Str = today.addDays(8).toString(QStringLiteral("yyyy-MM-dd"));
    QCOMPARE(backend.selectedDate(), day8Str);

    // When sel >= today.addDays(7), stripStart = sel.addDays(-6) = today.addDays(2)
    strip = backend.dayStrip();
    QCOMPARE(strip.size(), 7);
    QCOMPARE(strip.first().toMap().value(QStringLiteral("date")).toString(), today.addDays(2).toString(QStringLiteral("yyyy-MM-dd")));
    QCOMPARE(strip.last().toMap().value(QStringLiteral("date")).toString(), day8Str);

    // 6. Clearing selectedDate resets DayStrip to start from today
    backend.clearSelectedDate();
    QCOMPARE(backend.selectedDate(), QString());
    strip = backend.dayStrip();
    QCOMPARE(strip.size(), 7);
    QCOMPARE(strip.first().toMap().value(QStringLiteral("date")).toString(), todayStr);
    QCOMPARE(strip.last().toMap().value(QStringLiteral("date")).toString(), today.addDays(6).toString(QStringLiteral("yyyy-MM-dd")));

    // 7. Calling previousDay() when selectedDate is empty selects yesterday
    backend.previousDay();
    QCOMPARE(backend.selectedDate(), yesterdayStr);
    strip = backend.dayStrip();
    QCOMPARE(strip.size(), 7);
    QCOMPARE(strip.first().toMap().value(QStringLiteral("date")).toString(), yesterdayStr);
}

void TestOmacalendar::testPastDateFilterSectionHeader() {
    const QDate today = QDate::currentDate();
    const QDate pastDate = today.addDays(-3);
    const QString pastDateStr = pastDate.toString(QStringLiteral("yyyy-MM-dd"));
    const QString expectedSection = pastDate.toString(QStringLiteral("dddd, MMM d"));

    // Study date was 4 days ago -> R1 was 3 days ago (pastDate)
    const int topicId = m_db.addTopic(QStringLiteral("Past Filter Topic"),
                                      QStringLiteral("Testing past date section"),
                                      today.addDays(-4));
    QVERIFY(topicId > 0);

    // 1. When filtering specifically by pastDate:
    // Review must appear, isOverdue must be true, but section must be the past date, NOT "Today"!
    const QVariantList pastAgenda = m_db.getAgenda(QStringLiteral("Past Filter Topic"), pastDateStr);
    QCOMPARE(pastAgenda.size(), 1);
    const QVariantMap item = pastAgenda.at(0).toMap();
    QCOMPARE(item.value(QStringLiteral("isOverdue")).toBool(), true);
    QCOMPARE(item.value(QStringLiteral("scheduledDate")).toString(), pastDateStr);
    QCOMPARE(item.value(QStringLiteral("section")).toString(), expectedSection);

    // 2. When filtering by Today:
    // Overdue review rolls over into "Today" section as expected
    const QString todayStr = today.toString(QStringLiteral("yyyy-MM-dd"));
    const QVariantList todayAgenda = m_db.getAgenda(QStringLiteral("Past Filter Topic"), todayStr);
    bool foundInToday = false;
    for (const QVariant &v : todayAgenda) {
        if (v.toMap().value(QStringLiteral("scheduledDate")).toString() == pastDateStr) {
            foundInToday = true;
            QCOMPARE(v.toMap().value(QStringLiteral("section")).toString(), QStringLiteral("Today"));
        }
    }
    QVERIFY(foundInToday);

    // 3. When viewing full agenda (empty date filter):
    // Overdue review rolls into "Today" section
    const QVariantList fullAgenda = m_db.getAgenda(QStringLiteral("Past Filter Topic"), QString());
    bool foundInFull = false;
    for (const QVariant &v : fullAgenda) {
        if (v.toMap().value(QStringLiteral("scheduledDate")).toString() == pastDateStr) {
            foundInFull = true;
            QCOMPARE(v.toMap().value(QStringLiteral("section")).toString(), QStringLiteral("Today"));
        }
    }
    QVERIFY(foundInFull);

    m_db.deleteTopic(topicId);
}

void TestOmacalendar::testMidnightRolloverNotification() {
    Backend backend;
    QSignalSpy dateChangedSpy(&backend, &Backend::todayDateChanged);

    const QString iso = backend.todayDateIso();
    const QString str = backend.todayDateString();
    QCOMPARE(iso, QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd")));
    QCOMPARE(str, QDate::currentDate().toString(QStringLiteral("dddd, MMMM d")));

    // Triggering the signal works cleanly and QSignalSpy detects it
    emit backend.todayDateChanged();
    QCOMPARE(dateChangedSpy.count(), 1);

    // Calling checkDateRollover when date hasn't changed does not re-emit
    backend.checkDateRollover();
    QCOMPARE(dateChangedSpy.count(), 1);
}

void TestOmacalendar::testQmlUiComponentsAndShortcuts() {
    Backend backend;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    QCOMPARE(engine.rootObjects().size(), 1);

    QObject *rootWin = engine.rootObjects().first();
    QVERIFY(rootWin);

    // Verify key UI components exist via objectName
    QObject *searchField = rootWin->findChild<QObject *>(QStringLiteral("searchField"));
    QObject *dayStrip = rootWin->findChild<QObject *>(QStringLiteral("dayStrip"));
    QObject *agendaView = rootWin->findChild<QObject *>(QStringLiteral("agendaView"));
    QObject *addTopicModal = rootWin->findChild<QObject *>(QStringLiteral("addTopicModal"));
    QObject *editNotesModal = rootWin->findChild<QObject *>(QStringLiteral("editNotesModal"));
    QObject *deleteConfirmDialog = rootWin->findChild<QObject *>(QStringLiteral("deleteConfirmDialog"));

    QVERIFY(searchField);
    QVERIFY(dayStrip);
    QVERIFY(agendaView);
    QVERIFY(addTopicModal);
    QVERIFY(editNotesModal);
    QVERIFY(deleteConfirmDialog);

    // Verify modal state property initialized to false
    QCOMPARE(rootWin->property("isAnyModalOpen").toBool(), false);

    // Verify initial state
    QCOMPARE(backend.selectedDate(), QString());
    QCOMPARE(backend.searchQuery(), QString());

    // 1. Text search entered directly in searchField QML object
    searchField->setProperty("text", QStringLiteral("Raft"));
    QCOMPARE(backend.searchQuery(), QStringLiteral("Raft"));
    QCOMPARE(backend.selectedDate(), QString()); // Date untouched!

    // 2. Setting date filter does not alter searchField text
    const QString tomorrowStr = QDate::currentDate().addDays(1).toString(QStringLiteral("yyyy-MM-dd"));
    backend.setSelectedDate(tomorrowStr);
    QCOMPARE(backend.selectedDate(), tomorrowStr);
    QCOMPARE(searchField->property("text").toString(), QStringLiteral("Raft")); // Search preserved!

    // 3. Clearing search preserves date filter
    searchField->setProperty("text", QString());
    QCOMPARE(backend.searchQuery(), QString());
    QCOMPARE(backend.selectedDate(), tomorrowStr); // Date preserved!

    // 4. Modal open toggles isAnyModalOpen and blocks shortcuts
    QVERIFY(QMetaObject::invokeMethod(addTopicModal, "openModal"));
    QTRY_COMPARE(rootWin->property("isAnyModalOpen").toBool(), true);

    // 4b. Test notesField keybindings: Shift+Return (newline) vs Return (schedule review)
    QObject *titleField = addTopicModal->findChild<QObject *>(QStringLiteral("titleField"));
    QObject *dateField = addTopicModal->findChild<QObject *>(QStringLiteral("dateField"));
    QObject *notesField = addTopicModal->findChild<QObject *>(QStringLiteral("notesField"));
    QVERIFY(titleField);
    QVERIFY(dateField);
    QVERIFY(notesField);

    // Verify Tab skips dateField/Today/Yesterday and goes directly from titleField to notesField
    QVERIFY(titleField->property("activeFocus").toBool());
    QKeyEvent tabEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier);
    QCoreApplication::sendEvent(titleField, &tabEvent);
    QCOMPARE(notesField->property("activeFocus").toBool(), true);
    QCOMPARE(dateField->property("activeFocus").toBool(), false);

    // Verify Shift+Tab returns directly from notesField to titleField
    QKeyEvent backtabEvent(QEvent::KeyPress, Qt::Key_Backtab, Qt::ShiftModifier);
    QCoreApplication::sendEvent(notesField, &backtabEvent);
    QCOMPARE(titleField->property("activeFocus").toBool(), true);
    QCOMPARE(dateField->property("activeFocus").toBool(), false);

    titleField->setProperty("text", QStringLiteral("Keybinding Topic"));
    notesField->setProperty("text", QStringLiteral("Line 1"));
    QVERIFY(QMetaObject::invokeMethod(notesField, "forceActiveFocus"));
    notesField->setProperty("cursorPosition", 6);

    // Send Shift+Return: Must NOT close modal (keeps editing note with newline)
    QKeyEvent shiftReturnEvent(QEvent::KeyPress, Qt::Key_Return, Qt::ShiftModifier, QStringLiteral("\n"));
    QCoreApplication::sendEvent(notesField, &shiftReturnEvent);
    QCOMPARE(rootWin->property("isAnyModalOpen").toBool(), true);
    QCOMPARE(notesField->property("text").toString(), QStringLiteral("Line 1\n"));

    // Send plain Return: Triggers submit() and schedules reviews, closing the modal
    QKeyEvent returnEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
    QCoreApplication::sendEvent(notesField, &returnEvent);
    QTRY_COMPARE(rootWin->property("isAnyModalOpen").toBool(), false);

    // Verify topic was created and clean it up
    const QVariantList addedAgenda = backend.database().getAgenda(QStringLiteral("Keybinding Topic"), QString());
    QCOMPARE(addedAgenda.size(), 3);
    const int createdTopicId = addedAgenda.first().toMap().value(QStringLiteral("topicId")).toInt();
    backend.database().deleteTopic(createdTopicId);

    // 5. Delete dialog open / reject clears state
    deleteConfirmDialog->setProperty("topicIdToDelete", 42);
    QVERIFY(QMetaObject::invokeMethod(deleteConfirmDialog, "open"));
    QTRY_COMPARE(rootWin->property("isAnyModalOpen").toBool(), true);
    QVERIFY(QMetaObject::invokeMethod(deleteConfirmDialog, "reject"));
    QTRY_COMPARE(rootWin->property("isAnyModalOpen").toBool(), false);
    QCOMPARE(deleteConfirmDialog->property("topicIdToDelete").toInt(), -1);

    // 6. Clear date filter
    backend.clearSelectedDate();
    QCOMPARE(backend.selectedDate(), QString());
}


void TestOmacalendar::cleanupTestCase() {
}

QTEST_MAIN(TestOmacalendar)
#include "tst_omacalendar.moc"
