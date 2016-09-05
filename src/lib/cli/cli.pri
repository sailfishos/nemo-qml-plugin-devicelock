
INCLUDEPATH += $$PWD

PUBLIC_HEADERS += \
        $$PWD/cliauthenticator.h \
        $$PWD/cliauthorization.h \
        $$PWD/clidevicelock.h \
        $$PWD/clidevicelocksettings.h \
        $$PWD/clidevicereset.h \
        $$PWD/cliencryptionsettings.h \
        $$PWD/clifingerprintsettings.h \
        $$PWD/clilockcodesettings.h

HEADERS +=  \
        $$PWD/lockcodewatcher.h

SOURCES += \
        $$PWD/cliauthenticator.cpp \
        $$PWD/cliauthorization.cpp \
        $$PWD/clidevicelock.cpp \
        $$PWD/clidevicelocksettings.cpp \
        $$PWD/clidevicereset.cpp \
        $$PWD/cliencryptionsettings.cpp \
        $$PWD/clifingerprintsettings.cpp \
        $$PWD/clilockcodesettings.cpp \
        $$PWD/lockcodewatcher.cpp
