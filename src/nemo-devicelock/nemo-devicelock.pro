TEMPLATE = subdirs

SUBDIRS = \
        lib.pro \
        host

host.depends = lib.pro
