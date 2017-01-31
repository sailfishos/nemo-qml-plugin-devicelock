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
        nemodbus \
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
        systemd/nemo-devicelock.service
systemd.path= /lib/systemd/system

INSTALLS += \
        systemd \
        target
