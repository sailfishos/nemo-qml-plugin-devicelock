TEMPLATE = lib
TARGET  = nemodevicelock

CONFIG += \
        static \
        c++11 \
        link_pkgconfig \
        create_pc \
        create_prl \
        no_install_prl

QT -= gui
QT += dbus

PKGCONFIG += \
        keepalive \
        mlite5
PUBLIC_HEADERS = \
        authenticator.h \
        authorization.h \
        devicelock.h \
        devicelocksettings.h \
        devicereset.h \
        encryptionsettings.h \
        fingerprintsettings.h \
        lockcodesettings.h \
        mcedevicelock.h \
        nemoauthenticator.h \
        nemoauthorization.h \
        nemodevicelock.h \
        nemodevicelocksettings.h \
        nemodevicereset.h \
        nemoencryptionsettings.h \
        nemofingerprintsettings.h \
        nemolockcodesettings.h

HEADERS = \
        $$PUBLIC_HEADERS \
        lockcodewatcher.h \
        settingswatcher.h

SOURCES = \
        authenticator.cpp \
        authorization.cpp \
        devicelock.cpp \
        devicelocksettings.cpp \
        devicereset.cpp \
        encryptionsettings.cpp \
        fingerprintsettings.cpp \
        lockcodesettings.cpp \
        lockcodewatcher.cpp \
        mcedevicelock.cpp \
        nemoauthenticator.cpp \
        nemoauthorization.cpp \
        nemodevicelock.cpp \
        nemodevicelocksettings.cpp \
        nemodevicereset.cpp \
        nemoencryptionsettings.cpp \
        nemofingerprintsettings.cpp \
        nemolockcodesettings.cpp \
        settingswatcher.cpp

headers.files = $$PUBLIC_HEADERS
headers.path = /usr/include/nemo-devicelock

target.path = /usr/lib

QMAKE_PKGCONFIG_NAME = nemodevicelock
QMAKE_PKGCONFIG_DESCRIPTION = Library for creating a device lock QML plugin
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_INCDIR = $$headers.path
QMAKE_PKGCONFIG_DESTDIR = pkgconfig


INSTALLS += \
        headers \
        target
