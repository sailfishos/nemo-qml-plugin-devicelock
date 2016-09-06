TEMPLATE = subdirs

SUBDIRS = \
        src

OTHER_FILES += \
        rpm/nemo-qml-plugin-devicelock.spec \
        rpm/nemo-qml-plugin-devicelock-daemon.spec

include (dbus/dbus.pri)
