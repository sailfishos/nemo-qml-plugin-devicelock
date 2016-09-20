TEMPLATE = app
TARGET = nemo-devicelock

QT -= gui
QT += dbus

CONFIG += \
        c++11 \
        link_pkgconfig

PKGCONFIG += \
        dbus-1 \
        keepalive \
        libsystemd-daemon

INCLUDEPATH += \
        $$PWD/../ \
        $$PWD/../nemo-devicelock/host \
        $$PWD/../nemo-devicelock/host/cli

DEPENDPATH += \
        $$PWD/../nemo-devicelock \
        $$PWD/../nemo-devicelock/host

PRE_TARGETDEPS += \ \
        $$PWD/../nemo-devicelock/host/libnemodevicelock-host.a

LIBS += \
        -L$$OUT_PWD/../nemo-devicelock -lnemodevicelock\
        -L$$OUT_PWD/../nemo-devicelock/host -lnemodevicelock-host

SOURCES = \
        main.cpp

target.path = /usr/libexec

systemd.files = \
        systemd/nemo-devicelock.service \
        systemd/nemo-devicelock.socket
systemd.path= /lib/systemd/system

policy.files = \
        systemd/org.nemomobile.devicelock.conf
policy.path = /etc/dbus-1/system.d

INSTALLS += \
        policy \
        systemd \
        target
