TEMPLATE = subdirs

SUBDIRS = \
        daemon \
        nemo-devicelock \
        plugin

daemon.depends = \
        nemo-devicelock

plugin.depends = \
        nemo-devicelock
