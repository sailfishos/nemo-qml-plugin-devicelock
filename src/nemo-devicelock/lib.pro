TEMPLATE = lib
TARGET  = nemodevicelock

CONFIG += \
        hide_symbols \
        link_pkgconfig \
        create_pc \
        create_prl \
        no_install_prl

QT -= gui
QT += dbus

PKGCONFIG += \
        glib-2.0 \
        nemodbus

INCLUDEPATH += ..

DEFINES += \
        NEMODEVICELOCK_BUILD_LIBRARY

PUBLIC_HEADERS += \
        authenticationinput.h \
        authenticator.h \
        authorization.h \
        devicelock.h \
        devicelocksettings.h \
        devicereset.h \
        encryptionsettings.h \
        fingerprintsensor.h \
        global.h \
        securitycodesettings.h

SOURCES += \
        authenticationinput.cpp \
        authenticator.cpp \
        authorization.cpp \
        devicelock.cpp \
        devicelocksettings.cpp \
        devicereset.cpp \
        encryptionsettings.cpp \
        fingerprintsensor.cpp \
        securitycodesettings.cpp

include(private/private.pri)

HEADERS += \
        $$PRIVATE_HEADERS \
        $$PUBLIC_HEADERS

public_headers.files = $$PUBLIC_HEADERS
public_headers.path = /usr/include/nemo-devicelock

private_headers.files = $$PRIVATE_HEADERS
private_headers.path = /usr/include/nemo-devicelock/private

target.path = $$[QT_INSTALL_LIBS]

QMAKE_PKGCONFIG_NAME = nemodevicelock
QMAKE_PKGCONFIG_DESCRIPTION = Library for Nemo device lock.
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_VERSION = $$VERSION
QMAKE_PKGCONFIG_INCDIR = /usr/include
QMAKE_PKGCONFIG_DESTDIR = pkgconfig
QMAKE_PKGCONFIG_VERSION = $$VERSION

INSTALLS += \
        private_headers \
        public_headers \
        target
