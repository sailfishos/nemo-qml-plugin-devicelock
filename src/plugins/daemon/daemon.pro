TEMPLATE = lib
TARGET  = nemodevicelockplugin
TARGET = $$qtLibraryTarget($$TARGET)
MODULENAME = org/nemomobile/devicelock
TARGETPATH = $$[QT_INSTALL_QML]/$$MODULENAME

QT -= gui
QT += dbus qml

CONFIG += \
        plugin \
        hide_symbols \
        c++11 \
        link_pkgconfig

INCLUDEPATH += \
        $$PWD/../../lib \
        $$PWD/../../lib/tools

PRE_TARGETDEPS += \
        $$PWD/../../lib/libnemodevicelock.a

DEPENDPATH += \
        $$PWD/../../lib

LIBS += -L$$OUT_PWD/../../lib -lnemodevicelock

HEADERS = \
        clientauthenticator.h \
        clientauthorization.h \
        clientdevicelock.h \
        clientdevicelocksettings.h \
        clientdevicereset.h \
        clientencryptionsettings.h \
        clientfingerprintsettings.h \
        clientlockcodesettings.h \
        connection.h

SOURCES = \
        clientauthenticator.cpp \
        clientauthorization.cpp \
        clientdevicelock.cpp \
        clientdevicelocksettings.cpp \
        clientdevicereset.cpp \
        clientencryptionsettings.cpp \
        clientfingerprintsettings.cpp \
        clientlockcodesettings.cpp \
        connection.cpp \
        plugin.cpp

import.files = \
        qmldir
import.path = $$TARGETPATH

target.path = $$TARGETPATH

INSTALLS += \
        import \
        target
