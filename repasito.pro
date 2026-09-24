QT += core gui qml quick quickcontrols2 dbus sql

CONFIG += c++17 release
TARGET = repasito
TEMPLATE = app

HEADERS += \
    src/database.h \
    src/systemtheme.h \
    src/backend.h

SOURCES += \
    src/main.cpp \
    src/database.cpp \
    src/systemtheme.cpp \
    src/backend.cpp

RESOURCES += src/resources.qrc
