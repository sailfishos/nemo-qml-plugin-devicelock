TEMPLATE = app
TARGET = nemo-devicelock

QT -= gui
QT += dbus

CONFIG += \
        link_pkgconfig

PKGCONFIG += \
        dbus-1 \
        keepalive \
        nemodbus \
        libsystemd

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
        -L$$OUT_PWD/../nemo-devicelock/host -lnemodevicelock-host\
        -L$$OUT_PWD/../nemo-devicelock -lnemodevicelock

SOURCES = \
        main.cpp

target.path = /usr/libexec

systemd.files = \
        systemd/nemo-devicelock.service
systemd.path= /usr/lib/systemd/system

!no_systemd{
	INSTALLS += systemd
}

INSTALLS += target
