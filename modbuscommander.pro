include($$[QT_INSTALL_PREFIX]/include/nymea/plugin.pri)

QT += \
    serialport \
    network \
    serialbus \

SOURCES += \
    integrationpluginmodbuscommander.cpp \  
    modbustcpmaster.cpp \
    modbusrtumaster.cpp \

HEADERS += \
    integrationpluginmodbuscommander.h \
    modbustcpmaster.h \
    modbusrtumaster.h \
