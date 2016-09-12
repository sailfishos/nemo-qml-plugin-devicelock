TEMPLATE = subdirs

SUBDIRS = \
        daemon \
        hostlib \
        lib \
        plugin

daemon.depends = \
        hostlib

hostlib.depends = \
        lib

plugin.depends = \
        lib
