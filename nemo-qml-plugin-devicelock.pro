TEMPLATE = subdirs

SUBDIRS = \
        src

OTHER_FILES += \
        rpm/nemo-qml-plugin-devicelock.spec

include (dbus/dbus.pri)
