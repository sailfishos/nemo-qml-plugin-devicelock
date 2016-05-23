TEMPLATE = lib
TARGET  = nemodevicelockplugin
TARGET = $$qtLibraryTarget($$TARGET)
MODULENAME = org/nemomobile/devicelock
TARGETPATH = $$[QT_INSTALL_QML]/$$MODULENAME

QT -= gui
QT += dbus qml

CONFIG += \
        plugin \
        hide_symbols \
        c++11 \
        link_pkgconfig

PKGCONFIG += \
        keepalive \
        mlite5

INCLUDEPATH += $$PWD/../lib
LIBS += -L$$OUT_PWD/../lib -lnemodevicelock

DEFINES += \
        NEMO_DEVICE_LOCK_EMULATE_FINGER_SENSOR

SOURCES = \
        plugin.cpp

import.files = \
        qmldir
import.path = $$TARGETPATH

target.path = $$TARGETPATH

INSTALLS += \
        import \
        target
