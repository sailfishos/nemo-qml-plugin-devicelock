TEMPLATE = app
TARGET = nemodevicelockd

QT -= gui
QT += dbus

CONFIG += \
        c++11 \
        link_pkgconfig

PKGCONFIG += \
        keepalive \
        mlite5

INCLUDEPATH += \
        $$PWD/../lib \
        $$PWD/../lib/private \
        $$PWD/../hostlib \
        $$PWD/../hostlib/cli \
        $$PWD/../hostlib/mce

DEPENDPATH += \
        $$PWD/../lib \
        $$PWD/../hostlib

PRE_TARGETDEPS += \ \
        $$PWD/../lib/libnemodevicelock.a
        $$PWD/../lib/libnemodevicelock-host.a

LIBS += \
        -L$$OUT_PWD/../lib -lnemodevicelock\
        -L$$OUT_PWD/../hostlib -lnemodevicelock-host

SOURCES = \
        main.cpp

target.path = /usr/bin

systemd.files = \
        systemd/nemodevicelock.service \
        systemd/nemodevicelock.socket
systemd.path= /lib/systemd/system

policy.files = \
        systemd/org.nemomobile.devicelock.conf
policy.path = /etc/dbus-1/system.d

INSTALLS += \
        policy \
        systemd \
        target
