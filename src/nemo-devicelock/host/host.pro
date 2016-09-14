TEMPLATE = lib
TARGET  = nemodevicelock-host

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
        dbus-1 \
        keepalive \
        libsystemd-daemon \
        mlite5

INCLUDEPATH += \
        $$PWD/.. \
        $$PWD/../.. \
        $$PWD/../private

LIBS += -L$$OUT_PWD/.. -lnemodevicelock

PUBLIC_HEADERS += \
        $$PWD/hostauthenticator.h \
        $$PWD/hostauthorization.h \
        $$PWD/hostdevicelock.h \
        $$PWD/hostdevicelocksettings.h \
        $$PWD/hostdevicereset.h \
        $$PWD/hostencryptionsettings.h \
        $$PWD/hostfingerprintsettings.h \
        $$PWD/hostlockcodesettings.h \
        $$PWD/hostobject.h \
        $$PWD/hostservice.h \
        $$PWD/mcedevicelock.h

SOURCES += \
        $$PWD/hostauthenticator.cpp \
        $$PWD/hostauthorization.cpp \
        $$PWD/hostdevicelock.cpp \
        $$PWD/hostdevicelocksettings.cpp \
        $$PWD/hostdevicereset.cpp \
        $$PWD/hostencryptionsettings.cpp \
        $$PWD/hostfingerprintsettings.cpp \
        $$PWD/hostlockcodesettings.cpp \
        $$PWD/hostobject.cpp \
        $$PWD/hostservice.cpp \
        $$PWD/mcedevicelock.cpp

include (cli/cli.pri)

HEADERS += \
        $$PUBLIC_HEADERS

headers.files = $$PUBLIC_HEADERS
headers.path = /usr/include/nemo-devicelock/host

target.path = /usr/lib

QMAKE_PKGCONFIG_NAME = nemodevicelock-host
QMAKE_PKGCONFIG_DESCRIPTION = Library for creating a device lock daemon
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_INCDIR = /usr/include/
QMAKE_PKGCONFIG_DESTDIR = pkgconfig

INSTALLS += \
        headers \
        target
