
systemd.files = \
        $$PWD/nemo-devicelock.socket
systemd.path= /usr/lib/systemd/system

policy.files = \
        $$PWD/org.nemomobile.devicelock.conf
policy.path = /etc/dbus-1/system.d

INSTALLS += \
        policy \
        systemd
