TEMPLATE = lib
TARGET  = nemodevicelock-host

CONFIG += \
        static \
        c++11 \
        link_pkgconfig

QT -= gui
QT += dbus

PKGCONFIG += \
        dbus-1 \
        keepalive \
        sailfish-minui \
        libsystemd

INCLUDEPATH += \
        $$PWD/.. \
        $$PWD/../.. \
        $$PWD/../private

LIBS += -L$$OUT_PWD/.. -lnemodevicelock

PUBLIC_HEADERS += \
        $$PWD/hostauthenticationinput.h \
        $$PWD/hostauthenticator.h \
        $$PWD/hostauthorization.h \
        $$PWD/hostdevicelock.h \
        $$PWD/hostdevicelocksettings.h \
        $$PWD/hostdevicereset.h \
        $$PWD/hostencryptionsettings.h \
        $$PWD/hostfingerprintsensor.h \
        $$PWD/hostfingerprintsettings.h \
        $$PWD/hostobject.h \
        $$PWD/hostservice.h \
        $$PWD/mcedevicelock.h

SOURCES += \
        $$PWD/hostauthenticationinput.cpp \
        $$PWD/hostauthenticator.cpp \
        $$PWD/hostauthorization.cpp \
        $$PWD/hostdevicelock.cpp \
        $$PWD/hostdevicelocksettings.cpp \
        $$PWD/hostdevicereset.cpp \
        $$PWD/hostencryptionsettings.cpp \
        $$PWD/hostfingerprintsensor.cpp \
        $$PWD/hostfingerprintsettings.cpp \
        $$PWD/hostobject.cpp \
        $$PWD/hostservice.cpp \
        $$PWD/mcedevicelock.cpp

include (cli/cli.pri)

HEADERS += \
        $$PUBLIC_HEADERS

headers.files = $$PUBLIC_HEADERS
headers.path = /usr/include/nemo-devicelock/host

# the auto generated pkgconfig file includes paths from the build tree, use a .prf to
# publish dependencies in a maintainable way instead.
prf.files = nemo-devicelock-host.prf
prf.path = $$[QT_INSTALL_DATA]/mkspecs/features

target.path = $$[QT_INSTALL_LIBS]

INSTALLS += \
        headers \
        prf \
        target
