#-------------------------------------------------
#
# Project created by QtCreator 2019-04-26T19:01:31
#
#-------------------------------------------------

QT       += core gui widgets network

TARGET = desktopclient
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

VERSION = 0.1.0
DEFINES += VERSION_STRING=\\\"$${VERSION}\\\"

CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp \
    accountsdialog.cpp \
    platform.cpp

HEADERS += \
        mainwindow.h \
    accountsdialog.h \
    platform.h

FORMS += \
        mainwindow.ui \
    accountsdialog.ui


win32 {
SOURCES += platform_win.cpp
HEADERS += platform_win.h
}
!win32 {
SOURCES += platform_linux.cpp
HEADERS += platform_linux.h
}

win32:QTPLUGIN += qsvg

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:RC_FILES=$$system(bash ./make-win-icon.sh)
CONFIG(release,debug|release) {
    win32:QMAKE_TARGET_COMPANY="ServerLoginDevs"
    win32:QMAKE_TARGET_COPYRIGHT="Copyright (c) 2019 The ServerLogin developers"
    win32:QMAKE_TARGET_PRODUCT="ServerLogin Desktop Client"
    win32:QMAKE_TARGET_DESCRIPTION="Desktop Client"
    VERSION = 0.1.0
}

OTHER_FILES += \
    images/sources/icon-logo.svg \
    images/sources/icon-tray-16.svg \
    make-win-icon.sh

RESOURCES += \
    resources.qrc
