TEMPLATE = app
TARGET = nemodevicelockd

QT -= gui
QT += dbus

CONFIG += \
        c++11 \
        link_pkgconfig

PKGCONFIG += \
        keepalive \
        mlite5 \
        libsystemd-daemon

INCLUDEPATH += \
        $$PWD/../lib \
        $$PWD/../lib/cli \
        $$PWD/../lib/host \
        $$PWD/../lib/tools

DEPENDPATH += \
        $$PWD/../lib

PRE_TARGETDEPS += \
        $$PWD/../lib/libnemodevicelock.a

LIBS += -L$$OUT_PWD/../lib -lnemodevicelock

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
