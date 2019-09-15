include($$[QT_INSTALL_PREFIX]/include/nymea/plugin.pri)

TARGET = $$qtLibraryTarget(nymea_devicepluginmodbuscommander)

QT += \
    serialport \
    network \
    serialbus \

SOURCES += \
    devicepluginmodbuscommander.cpp \  
    modbustcpmaster.cpp \
    modbusrtumaster.cpp \

HEADERS += \
    devicepluginmodbuscommander.h \
    modbustcpmaster.h \
    modbusrtumaster.h \
