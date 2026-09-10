#include "backend.h"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRect>
#include <QSettings>
#include <QTextStream>
#include <QDebug>

namespace {
const QString windowGeometrySetting = QStringLiteral("window/geometry");
}

Backend::Backend(QObject *parent)
    : QObject(parent) {
    m_db.init();

    loadOmarchyTheme();
    watchOmarchyTheme();
    updateAgenda();
}

void Backend::setDarkMode(bool darkMode) {
    if (m_darkMode == darkMode)
        return;

    m_darkMode = darkMode;
    loadOmarchyTheme();
    emit darkModeChanged();
}

void Backend::setTextScale(qreal textScale) {
    if (qFuzzyCompare(m_textScale, textScale))
        return;

    m_textScale = textScale;
    emit textScaleChanged();
}

void Backend::setSearchQuery(const QString &query) {
    if (m_searchQuery == query)
        return;

    m_searchQuery = query;
    emit searchQueryChanged();
    updateAgenda();
}

QString Backend::todayDateString() const {
    return QDate::currentDate().toString(QStringLiteral("dddd, MMMM d"));
}

QString Backend::todayDateIso() const {
    return QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
}

bool Backend::addTopic(const QString &title, const QString &notes, const QString &initialDateIso) {
    QDate initialDate = QDate::fromString(initialDateIso, QStringLiteral("yyyy-MM-dd"));
    if (!initialDate.isValid()) {
        initialDate = QDate::currentDate();
    }

    const int topicId = m_db.addTopic(title, notes, initialDate);
    if (topicId > 0) {
        updateAgenda();
        return true;
    }
    return false;
}

bool Backend::toggleReview(int reviewId, bool completed) {
    bool ok = false;
    if (completed) {
        ok = m_db.completeReview(reviewId);
    } else {
        ok = m_db.uncompleteReview(reviewId);
    }

    if (ok) {
        updateAgenda();
    }
    return ok;
}

bool Backend::snoozeReview(int reviewId, int days) {
    const bool ok = m_db.snoozeReview(reviewId, days);
    if (ok) {
        updateAgenda();
    }
    return ok;
}

bool Backend::updateNotes(int topicId, const QString &notes) {
    const bool ok = m_db.updateNotes(topicId, notes);
    if (ok) {
        updateAgenda();
    }
    return ok;
}

bool Backend::deleteTopic(int topicId) {
    const bool ok = m_db.deleteTopic(topicId);
    if (ok) {
        updateAgenda();
    }
    return ok;
}

void Backend::refresh() {
    updateAgenda();
}

void Backend::updateAgenda() {
    m_agenda = m_db.getAgenda(m_searchQuery);
    m_dayStrip = m_db.getDayStrip(QDate::currentDate(), 7);

    emit agendaChanged();
    emit dayStripChanged();
}

QVariantMap Backend::windowGeometry() const {
    const QSettings settings;
    const QRect geometry = settings.value(windowGeometrySetting).toRect();
    QVariantMap map;
    map.insert(QStringLiteral("valid"), geometry.isValid());
    map.insert(QStringLiteral("x"), geometry.x());
    map.insert(QStringLiteral("y"), geometry.y());
    map.insert(QStringLiteral("width"), geometry.width());
    map.insert(QStringLiteral("height"), geometry.height());
    map.insert(QStringLiteral("maximized"),
               settings.value(QStringLiteral("window/maximized"), false).toBool());
    return map;
}

void Backend::saveWindowGeometry(int x, int y, int width, int height, bool maximized) {
    QSettings settings;
    settings.setValue(windowGeometrySetting, QRect(x, y, width, height));
    settings.setValue(QStringLiteral("window/maximized"), maximized);
}

void Backend::loadOmarchyTheme() {
    m_themeBackground = m_darkMode ? QStringLiteral("#101315") : QStringLiteral("#f8f9fa");
    m_themeForeground = m_darkMode ? QStringLiteral("#cacccc") : QStringLiteral("#212529");
    m_themeAccent = m_darkMode ? QStringLiteral("#798186") : QStringLiteral("#3b82f6");
    m_themeSelection = m_darkMode ? QStringLiteral("#343d41") : QStringLiteral("#dbeafe");
    m_themeLighterBg = m_darkMode ? QStringLiteral("#181c1f") : QStringLiteral("#ffffff");
    m_themeMuted = m_darkMode ? QStringLiteral("#767e85") : QStringLiteral("#6c757d");

    const QString colorsPath = QDir::homePath()
        + QStringLiteral("/.local/state/omarchy/current/theme/colors.toml");
    QFile file(colorsPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            const QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
                continue;

            const int equals = line.indexOf(QLatin1Char('='));
            if (equals < 0)
                continue;

            const QString key = line.left(equals).trimmed();
            QString value = line.mid(equals + 1).trimmed();
            if (value.size() >= 2
                    && ((value.front() == QLatin1Char('"') && value.back() == QLatin1Char('"'))
                        || (value.front() == QLatin1Char('\'') && value.back() == QLatin1Char('\''))))
                value = value.mid(1, value.size() - 2);

            if (key == QStringLiteral("background"))
                m_themeBackground = value;
            else if (key == QStringLiteral("foreground"))
                m_themeForeground = value;
            else if (key == QStringLiteral("accent"))
                m_themeAccent = value;
            else if (key == QStringLiteral("selection"))
                m_themeSelection = value;
            else if (key == QStringLiteral("lighter_background"))
                m_themeLighterBg = value;
            else if (key == QStringLiteral("muted"))
                m_themeMuted = value;
        }
    }

    emit themeColorsChanged();
}

void Backend::watchOmarchyTheme() {
    const QString colorsPath = QDir::homePath()
        + QStringLiteral("/.local/state/omarchy/current/theme/colors.toml");
    const QString themeDir = QDir::homePath()
        + QStringLiteral("/.local/state/omarchy/current/theme");

    if (QFile::exists(colorsPath)) {
        m_themeWatcher.addPath(colorsPath);
    }
    if (QDir(themeDir).exists()) {
        m_themeWatcher.addPath(themeDir);
    }

    connect(&m_themeWatcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &) {
        loadOmarchyTheme();
    });
    connect(&m_themeWatcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString &) {
        loadOmarchyTheme();
    });
}
