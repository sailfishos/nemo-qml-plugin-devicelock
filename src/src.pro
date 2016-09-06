TEMPLATE = subdirs

SUBDIRS = \
        lib \
        plugins

contains(DEVICELOCK_DAEMON, yes): SUBDIRS += daemon

plugins.depends = \
        lib
