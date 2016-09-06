TEMPLATE = subdirs

contains(DEVICELOCK_DAEMON, yes): SUBDIRS += daemon
else: SUBDIRS += default

