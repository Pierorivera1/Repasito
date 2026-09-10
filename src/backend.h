#pragma once

#include <QDate>
#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "database.h"

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool darkMode READ darkMode WRITE setDarkMode NOTIFY darkModeChanged)
    Q_PROPERTY(qreal textScale READ textScale WRITE setTextScale NOTIFY textScaleChanged)
    Q_PROPERTY(QString themeBackground READ themeBackground NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeForeground READ themeForeground NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeAccent READ themeAccent NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeAccentForeground READ themeAccentForeground NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeSelection READ themeSelection NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeLighterBg READ themeLighterBg NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeMuted READ themeMuted NOTIFY themeColorsChanged)

    Q_PROPERTY(QVariantList agenda READ agenda NOTIFY agendaChanged)
    Q_PROPERTY(QVariantList dayStrip READ dayStrip NOTIFY dayStripChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery WRITE setSearchQuery NOTIFY searchQueryChanged)
    Q_PROPERTY(QString todayDateString READ todayDateString CONSTANT)
    Q_PROPERTY(QString todayDateIso READ todayDateIso CONSTANT)

public:
    explicit Backend(QObject *parent = nullptr);

    bool darkMode() const { return m_darkMode; }
    void setDarkMode(bool darkMode);
    qreal textScale() const { return m_textScale; }
    void setTextScale(qreal textScale);

    QString themeBackground() const { return m_themeBackground; }
    QString themeForeground() const { return m_themeForeground; }
    QString themeAccent() const { return m_themeAccent; }
    QString themeAccentForeground() const;
    QString themeSelection() const { return m_themeSelection; }
    QString themeLighterBg() const { return m_themeLighterBg; }
    QString themeMuted() const { return m_themeMuted; }

    QVariantList agenda() const { return m_agenda; }
    QVariantList dayStrip() const { return m_dayStrip; }
    QString searchQuery() const { return m_searchQuery; }
    void setSearchQuery(const QString &query);

    QString todayDateString() const;
    QString todayDateIso() const;

    Q_INVOKABLE bool addTopic(const QString &title, const QString &notes, const QString &initialDateIso);
    Q_INVOKABLE bool toggleReview(int reviewId, bool completed);
    Q_INVOKABLE bool snoozeReview(int reviewId, int days = 1);
    Q_INVOKABLE bool updateNotes(int topicId, const QString &notes);
    Q_INVOKABLE bool deleteTopic(int topicId);
    Q_INVOKABLE void refresh();

    Q_INVOKABLE QVariantMap windowGeometry() const;
    Q_INVOKABLE void saveWindowGeometry(int x, int y, int width, int height, bool maximized);

signals:
    void darkModeChanged();
    void textScaleChanged();
    void themeColorsChanged();
    void agendaChanged();
    void dayStripChanged();
    void searchQueryChanged();

private:
    void loadOmarchyTheme();
    void watchOmarchyTheme();
    void updateAgenda();

    Database m_db;
    QVariantList m_agenda;
    QVariantList m_dayStrip;
    QString m_searchQuery;

    bool m_darkMode = true;
    qreal m_textScale = 1.0;
    QString m_themeBackground;
    QString m_themeForeground;
    QString m_themeAccent;
    QString m_themeSelection;
    QString m_themeLighterBg;
    QString m_themeMuted;
    QFileSystemWatcher m_themeWatcher;
};
