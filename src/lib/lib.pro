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

PUBLIC_HEADERS += \
        authenticator.h \
        authorization.h \
        devicelock.h \
        devicelocksettings.h \
        devicereset.h \
        encryptionsettings.h \
        fingerprintsettings.h \
        lockcodesettings.h

SOURCES += \
        authenticator.cpp \
        authorization.cpp \
        devicelock.cpp \
        devicelocksettings.cpp \
        devicereset.cpp \
        encryptionsettings.cpp \
        fingerprintsettings.cpp \
        lockcodesettings.cpp

include(private/private.pri)

HEADERS += \
        $$PUBLIC_HEADERS

headers.files = $$PUBLIC_HEADERS
headers.path = /usr/include/nemo-devicelock

target.path = /usr/lib

QMAKE_PKGCONFIG_NAME = nemodevicelock
QMAKE_PKGCONFIG_DESCRIPTION = Library for Nemo device lock.
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_INCDIR = $$headers.path
QMAKE_PKGCONFIG_DESTDIR = pkgconfig

INSTALLS += \
        headers \
        target
