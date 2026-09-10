#include <QCoreApplication>
#include <QDate>
#include <QGuiApplication>
#include <QSignalSpy>
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

void TestOmacalendar::cleanupTestCase() {
}

QTEST_MAIN(TestOmacalendar)
#include "tst_omacalendar.moc"
