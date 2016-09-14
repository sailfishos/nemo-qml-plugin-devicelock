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
        $$PWD/../ \
        $$PWD/../nemo-devicelock/host \
        $$PWD/../nemo-devicelock/host/cl

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
