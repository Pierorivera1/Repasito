QT += core gui qml quick quickcontrols2 dbus sql testlib

CONFIG += c++17 testcase
TARGET = tst_omacalendar
TEMPLATE = app

INCLUDEPATH += ../src

HEADERS += \
    ../src/database.h \
    ../src/backend.h \
    ../src/systemtheme.h

SOURCES += \
    tst_omacalendar.cpp \
    ../src/database.cpp \
    ../src/backend.cpp \
    ../src/systemtheme.cpp

RESOURCES += ../src/resources.qrc
