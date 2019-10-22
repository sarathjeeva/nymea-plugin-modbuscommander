/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                         *
 *  Copyright (C) 2019 Bernhard Trinnes <bernhard.trinnes@nymea.io>        *
 *                                                                         *
 *  This file is part of nymea.                                            *
 *                                                                         *
 *  This library is free software; you can redistribute it and/or          *
 *  modify it under the terms of the GNU Lesser General Public             *
 *  License as published by the Free Software Foundation; either           *
 *  version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *  This library is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 *  Lesser General Public License for more details.                        *
 *                                                                         *
 *  You should have received a copy of the GNU Lesser General Public       *
 *  License along with this library; If not, see                           *
 *  <http://www.gnu.org/licenses/>.                                        *
 *                                                                         *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*!
    \page modbuscommander.html
    \title Modbus Commander
    \brief Plugin to write and read Modbus TCP coils and registers.

    \ingroup plugins
    \ingroup nymea-plugins


    This plugin allows to write and read modbus registers and coils.

    \underline{NOTE}: the library \c libmodbus has to be installed.

    \chapter Plugin properties
    Following JSON file contains the definition and the description of all available \l{DeviceClass}{DeviceClasses}
    and \l{Vendor}{Vendors} of this \l{DevicePlugin}.

    For more details how to read this JSON file please check out the documentation for \l{The plugin JSON File}.

    \quotefile plugins/deviceplugins/modbuscommander/devicepluginmodbuscommander.json
*/

#include "devicepluginmodbuscommander.h"
#include "plugininfo.h"

#include <QSerialPort>

DevicePluginModbusCommander::DevicePluginModbusCommander()
{
}

void DevicePluginModbusCommander::init()
{
    connect(this, &DevicePluginModbusCommander::configValueChanged, this, &DevicePluginModbusCommander::onPluginConfigurationChanged);
    QLoggingCategory::setFilterRules(QStringLiteral("qt.modbus* = false"));
}


void DevicePluginModbusCommander::setupDevice(DeviceSetupInfo *info)
{
    Device *device = info->device();

    if (device->deviceClassId() == modbusTCPClientDeviceClassId) {
        QString ipAddress = device->paramValue(modbusTCPClientDeviceIpv4addressParamTypeId).toString();
        uint port = device->paramValue(modbusTCPClientDevicePortParamTypeId).toUInt();

        foreach (ModbusTCPMaster *modbusTCPMaster, m_modbusTCPMasters.values()) {
            if ((modbusTCPMaster->ipv4Address() == ipAddress) && (modbusTCPMaster->port() == port)){
                m_modbusTCPMasters.insert(device, modbusTCPMaster);
                return info->finish(Device::DeviceErrorNoError);
            }
        }

        ModbusTCPMaster *modbusTCPMaster = new ModbusTCPMaster(ipAddress, port, this);
        connect(modbusTCPMaster, &ModbusTCPMaster::connectionStateChanged, this, &DevicePluginModbusCommander::onConnectionStateChanged);
        connect(modbusTCPMaster, &ModbusTCPMaster::requestExecuted, this, &DevicePluginModbusCommander::onRequestExecuted);
        connect(modbusTCPMaster, &ModbusTCPMaster::receivedCoil, this, &DevicePluginModbusCommander::onReceivedCoil);
        connect(modbusTCPMaster, &ModbusTCPMaster::receivedDiscreteInput, this, &DevicePluginModbusCommander::onReceivedDiscreteInput);
        connect(modbusTCPMaster, &ModbusTCPMaster::receivedHoldingRegister, this, &DevicePluginModbusCommander::onReceivedHoldingRegister);
        connect(modbusTCPMaster, &ModbusTCPMaster::receivedInputRegister, this, &DevicePluginModbusCommander::onReceivedInputRegister);
        modbusTCPMaster->connectDevice();
        m_modbusTCPMasters.insert(device, modbusTCPMaster);
        m_asyncTCPSetup.insert(modbusTCPMaster, info);
        return;

    } else if (device->deviceClassId() == modbusRTUClientDeviceClassId) {

        QString serialPort = device->paramValue(modbusRTUClientDeviceSerialPortParamTypeId).toString();
        uint baudrate = device->paramValue(modbusRTUClientDeviceBaudRateParamTypeId).toUInt();
        uint stopBits = device->paramValue(modbusRTUClientDeviceStopBitsParamTypeId).toUInt();
        uint dataBits = device->paramValue(modbusRTUClientDeviceDataBitsParamTypeId).toUInt();
        QSerialPort::Parity parity = QSerialPort::Parity::NoParity;
        if (device->paramValue(modbusRTUClientDeviceParityParamTypeId).toString().contains("No")) {
            parity = QSerialPort::Parity::NoParity;
        } else if (device->paramValue(modbusRTUClientDeviceParityParamTypeId).toString().contains("Even")) {
            parity = QSerialPort::Parity::EvenParity;
        } else if (device->paramValue(modbusRTUClientDeviceParityParamTypeId).toString().contains("Odd")) {
            parity = QSerialPort::Parity::OddParity;
        }

        ModbusRTUMaster *modbusRTUMaster = new ModbusRTUMaster(serialPort, baudrate, parity, dataBits, stopBits, this);
        connect(modbusRTUMaster, &ModbusRTUMaster::connectionStateChanged, this, &DevicePluginModbusCommander::onConnectionStateChanged);
        connect(modbusRTUMaster, &ModbusRTUMaster::receivedCoil, this, &DevicePluginModbusCommander::onReceivedCoil);
        connect(modbusRTUMaster, &ModbusRTUMaster::receivedDiscreteInput, this, &DevicePluginModbusCommander::onReceivedDiscreteInput);
        connect(modbusRTUMaster, &ModbusRTUMaster::receivedHoldingRegister, this, &DevicePluginModbusCommander::onReceivedHoldingRegister);
        connect(modbusRTUMaster, &ModbusRTUMaster::receivedInputRegister, this, &DevicePluginModbusCommander::onReceivedInputRegister);
        modbusRTUMaster->connectDevice();
        m_modbusRTUMasters.insert(device, modbusRTUMaster);
        m_asyncRTUSetup.insert(modbusRTUMaster, info);
        return;

    } else if (device->deviceClassId() == coilDeviceClassId) {
        info->finish(Device::DeviceErrorNoError);
        return;
    } else if (device->deviceClassId() == discreteInputDeviceClassId) {
        info->finish(Device::DeviceErrorNoError);
        return;
    } else if (device->deviceClassId() == holdingRegisterDeviceClassId) {
        info->finish(Device::DeviceErrorNoError);
        return;
    } else if (device->deviceClassId() == inputRegisterDeviceClassId) {
        info->finish(Device::DeviceErrorNoError);
        return;
    }

    qCWarning(dcModbusCommander()) << "Unhandled device class in setupDevice!";
    info->finish(Device::DeviceErrorSetupFailed);
}

void DevicePluginModbusCommander::discoverDevices(DeviceDiscoveryInfo *info)
{
    DeviceClassId deviceClassId = info->deviceClassId();

    if (deviceClassId == modbusRTUClientDeviceClassId) {
        Q_FOREACH(QSerialPortInfo port, QSerialPortInfo::availablePorts()) {
            //Serial port is not yet used, create now a new one
            qCDebug(dcModbusCommander()) << "Found serial port:" << port.systemLocation();
            QString description = port.manufacturer() + " " + port.description();
            DeviceDescriptor deviceDescriptor(deviceClassId, port.portName(), description);
            ParamList parameters;
            QString serialPort = port.systemLocation();
            foreach (Device *existingDevice, myDevices()) {
                if (existingDevice->paramValue(modbusRTUClientDeviceSerialPortParamTypeId).toString() == serialPort) {
                    deviceDescriptor.setDeviceId(existingDevice->id());
                    break;
                }
            }
            parameters.append(Param(modbusRTUClientDeviceSerialPortParamTypeId, serialPort));
            deviceDescriptor.setParams(parameters);
            info->addDeviceDescriptor(deviceDescriptor);
        }
        info->finish(Device::DeviceErrorNoError);
        return;
    }

    if (deviceClassId == discreteInputDeviceClassId) {
        Q_FOREACH(Device *clientDevice, myDevices()){
            if (clientDevice->deviceClassId() == modbusTCPClientDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, "Discrete input", clientDevice->name() + " " + clientDevice->paramValue(modbusTCPClientDeviceIpv4addressParamTypeId).toString() + " Port: " + clientDevice->paramValue(modbusTCPClientDevicePortParamTypeId).toString());
                descriptor.setParentDeviceId(clientDevice->id());
                info->addDeviceDescriptor(descriptor);
            }
            if (clientDevice->deviceClassId() == modbusRTUClientDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, "Discrete input", clientDevice->name() + " " + clientDevice->paramValue(modbusRTUClientDeviceSerialPortParamTypeId).toString());
                descriptor.setParentDeviceId(clientDevice->id());
                info->addDeviceDescriptor(descriptor);
            }
        }
        info->finish(Device::DeviceErrorNoError);
        return;
    }

    if (deviceClassId == coilDeviceClassId) {
        Q_FOREACH(Device *clientDevice, myDevices()){
            if (clientDevice->deviceClassId() == modbusTCPClientDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, "Coil", clientDevice->name() + " " + clientDevice->paramValue(modbusTCPClientDeviceIpv4addressParamTypeId).toString() + "Port: " + clientDevice->paramValue(modbusTCPClientDevicePortParamTypeId).toString());
                descriptor.setParentDeviceId(clientDevice->id());
                info->addDeviceDescriptor(descriptor);
            }
            if (clientDevice->deviceClassId() == modbusRTUClientDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, "Coil", clientDevice->name() + " " + clientDevice->paramValue(modbusRTUClientDeviceSerialPortParamTypeId).toString());
                descriptor.setParentDeviceId(clientDevice->id());
                info->addDeviceDescriptor(descriptor);
            }
        }
        info->finish(Device::DeviceErrorNoError);
        return;
    }

    if (deviceClassId == holdingRegisterDeviceClassId) {
        Q_FOREACH(Device *clientDevice, myDevices()){
            if (clientDevice->deviceClassId() == modbusTCPClientDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, "Holding register", clientDevice->name() + " " + clientDevice->paramValue(modbusTCPClientDeviceIpv4addressParamTypeId).toString() + " Port: " + clientDevice->paramValue(modbusTCPClientDevicePortParamTypeId).toString());
                descriptor.setParentDeviceId(clientDevice->id());
                info->addDeviceDescriptor(descriptor);
            }
            if (clientDevice->deviceClassId() == modbusRTUClientDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, "Holding register", clientDevice->name() + " " + clientDevice->paramValue(modbusRTUClientDeviceSerialPortParamTypeId).toString());
                descriptor.setParentDeviceId(clientDevice->id());
                info->addDeviceDescriptor(descriptor);
            }
        }
        info->finish(Device::DeviceErrorNoError);
        return;
    }

    if (deviceClassId == inputRegisterDeviceClassId) {
        Q_FOREACH(Device *ClientDevice, myDevices()){
            if (ClientDevice->deviceClassId() == modbusTCPClientDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, "Input register", ClientDevice->name() + " " + ClientDevice->paramValue(modbusTCPClientDeviceIpv4addressParamTypeId).toString() + " Port: " + ClientDevice->paramValue(modbusTCPClientDevicePortParamTypeId).toString());
                info->addDeviceDescriptor(descriptor);
            }
            if (ClientDevice->deviceClassId() == modbusRTUClientDeviceClassId) {
                DeviceDescriptor descriptor(deviceClassId, "Input register", ClientDevice->name() + " " + ClientDevice->paramValue(modbusRTUClientDeviceSerialPortParamTypeId).toString());
                info->addDeviceDescriptor(descriptor);
            }
        }
        info->finish(Device::DeviceErrorNoError);
        return;
    }
    info->finish(Device::DeviceErrorDeviceClassNotFound);
    qCWarning(dcModbusCommander()) << "Unhandled device class in discovery!";
}

void DevicePluginModbusCommander::postSetupDevice(Device *device)
{
    if (!m_refreshTimer) {
        // Refresh timer for TCP read
        uint refreshTime = configValue(modbusCommanderPluginUpdateIntervalParamTypeId).toUInt();
        m_refreshTimer = hardwareManager()->pluginTimerManager()->registerTimer(refreshTime);
        connect(m_refreshTimer, &PluginTimer::timeout, this, &DevicePluginModbusCommander::onRefreshTimer);
    }

    if ((device->deviceClassId() == coilDeviceClassId) ||
            (device->deviceClassId() == discreteInputDeviceClassId) ||
            (device->deviceClassId() == holdingRegisterDeviceClassId) ||
            (device->deviceClassId() == inputRegisterDeviceClassId)) {
        readRegister(device);
    }
}


void DevicePluginModbusCommander::executeAction(DeviceActionInfo *info)
{
    Device *device = info->device();

    if (device->deviceClassId() == coilDeviceClassId) {

        if (info->action().actionTypeId() == coilValueActionTypeId) {
            writeRegister(device, info);
            return;
        }
    } else if (device->deviceClassId() == holdingRegisterDeviceClassId) {

        if (info->action().actionTypeId() == holdingRegisterValueActionTypeId) {
            writeRegister(device, info);
            return;
        }
    }
    qCWarning(dcModbusCommander()) << "Unhandled deviceclass/actiontype in executeAction!";
    info->finish(Device::DeviceErrorDeviceClassNotFound);
}


void DevicePluginModbusCommander::deviceRemoved(Device *device)
{
    if (device->deviceClassId() == modbusTCPClientDeviceClassId) {
        ModbusTCPMaster *modbus = m_modbusTCPMasters.take(device);
        modbus->deleteLater();
    }

    if (device->deviceClassId() == modbusRTUClientDeviceClassId) {
        ModbusRTUMaster *modbus = m_modbusRTUMasters.take(device);
        modbus->deleteLater();
    }

    if (myDevices().empty()) {
        hardwareManager()->pluginTimerManager()->unregisterTimer(m_refreshTimer);
        m_refreshTimer = nullptr;
    }
}

void DevicePluginModbusCommander::onRefreshTimer()
{
    foreach (Device *device, myDevices()) {
        if ((device->deviceClassId() == coilDeviceClassId) ||
                (device->deviceClassId() == discreteInputDeviceClassId) ||
                (device->deviceClassId() == holdingRegisterDeviceClassId) ||
                (device->deviceClassId() == inputRegisterDeviceClassId)) {
            readRegister(device);
        }
    }
}

void DevicePluginModbusCommander::onPluginConfigurationChanged(const ParamTypeId &paramTypeId, const QVariant &value)
{
    // Check refresh schedule
    if (paramTypeId == modbusCommanderPluginUpdateIntervalParamTypeId) {;
        if (m_refreshTimer) {
            uint refreshTime = value.toUInt();
            m_refreshTimer->stop();
            m_refreshTimer->startTimer(refreshTime);
        }
    }
}

void DevicePluginModbusCommander::onConnectionStateChanged(bool status)
{
    auto modbus = sender();

    if (m_asyncRTUSetup.contains(static_cast<ModbusRTUMaster *>(modbus))) {
        DeviceSetupInfo *info = m_asyncRTUSetup.take(static_cast<ModbusRTUMaster *>(modbus));
        info->finish(Device::DeviceErrorNoError);

    } else if (m_asyncTCPSetup.contains(static_cast<ModbusTCPMaster *>(modbus))) {
        DeviceSetupInfo *info = m_asyncTCPSetup.take(static_cast<ModbusTCPMaster *>(modbus));
        info->finish(Device::DeviceErrorNoError);
    }

    if (m_modbusRTUMasters.values().contains(static_cast<ModbusRTUMaster *>(modbus))) {
        Device *device = m_modbusRTUMasters.key(static_cast<ModbusRTUMaster *>(modbus));
        device->setStateValue(modbusRTUClientConnectedStateTypeId, status);
    } else if (m_modbusTCPMasters.values().contains(static_cast<ModbusTCPMaster *>(modbus))) {
        Device *device = m_modbusTCPMasters.key(static_cast<ModbusTCPMaster *>(modbus));
        device->setStateValue(modbusTCPClientConnectedStateTypeId, status);
    }
}

void DevicePluginModbusCommander::onRequestExecuted(QUuid requestId, bool success)
{
    if (m_asyncActions.contains(requestId)){
        DeviceActionInfo *info = m_asyncActions.take(requestId);
        if (success)
            info->finish(Device::DeviceErrorNoError);
        else
            info->finish(Device::DeviceErrorHardwareNotAvailable);
    }
}

void DevicePluginModbusCommander::onReceivedCoil(quint32 slaveAddress, quint32 modbusRegister, bool value)
{
    auto modbus = sender();

    if (m_modbusRTUMasters.values().contains(static_cast<ModbusRTUMaster *>(modbus))) {
        Device *parentDevice = m_modbusRTUMasters.key(static_cast<ModbusRTUMaster *>(modbus));
        foreach(Device *device, myDevices().filterByParentDeviceId(parentDevice->id())) {
            if ((device->paramValue(coilDeviceSlaveAddressParamTypeId) == slaveAddress) && (device->paramValue(coilDeviceRegisterAddressParamTypeId) == modbusRegister))
                device->setStateValue(coilValueStateTypeId, value);
        }
    }

    if (m_modbusTCPMasters.values().contains(static_cast<ModbusTCPMaster *>(modbus))) {
        Device *parentDevice = m_modbusTCPMasters.key(static_cast<ModbusTCPMaster *>(modbus));
        foreach(Device *device, myDevices().filterByParentDeviceId(parentDevice->id())) {
            if ((device->paramValue(coilDeviceSlaveAddressParamTypeId) == slaveAddress) && (device->paramValue(coilDeviceRegisterAddressParamTypeId) == modbusRegister))
                device->setStateValue(coilValueStateTypeId, value);
        }
    }
}

void DevicePluginModbusCommander::onReceivedDiscreteInput(quint32 slaveAddress, quint32 modbusRegister, bool value)
{
    auto modbus = sender();

    if (m_modbusRTUMasters.values().contains(static_cast<ModbusRTUMaster *>(modbus))) {
        Device *parentDevice = m_modbusRTUMasters.key(static_cast<ModbusRTUMaster *>(modbus));
        foreach(Device *device, myDevices().filterByParentDeviceId(parentDevice->id())) {
            if ((device->paramValue(discreteInputDeviceSlaveAddressParamTypeId) == slaveAddress) && (device->paramValue(discreteInputDeviceRegisterAddressParamTypeId) == modbusRegister))
                device->setStateValue(discreteInputValueStateTypeId, value);
        }
    }

    if (m_modbusTCPMasters.values().contains(static_cast<ModbusTCPMaster *>(modbus))) {
        Device *parentDevice = m_modbusTCPMasters.key(static_cast<ModbusTCPMaster *>(modbus));
        foreach(Device *device, myDevices().filterByParentDeviceId(parentDevice->id())) {
            if ((device->paramValue(discreteInputDeviceSlaveAddressParamTypeId) == slaveAddress) && (device->paramValue(discreteInputDeviceRegisterAddressParamTypeId) == modbusRegister))
                device->setStateValue(discreteInputValueStateTypeId, value);
        }
    }
}

void DevicePluginModbusCommander::onReceivedHoldingRegister(quint32 slaveAddress, quint32 modbusRegister, int value)
{
    auto modbus = sender();

    if (m_modbusRTUMasters.values().contains(static_cast<ModbusRTUMaster *>(modbus))) {
        Device *parentDevice = m_modbusRTUMasters.key(static_cast<ModbusRTUMaster *>(modbus));
        foreach(Device *device, myDevices().filterByParentDeviceId(parentDevice->id())) {
            if ((device->paramValue(holdingRegisterDeviceSlaveAddressParamTypeId) == slaveAddress) && (device->paramValue(holdingRegisterDeviceRegisterAddressParamTypeId) == modbusRegister))
                device->setStateValue(holdingRegisterValueStateTypeId, value);
        }
    }

    if (m_modbusTCPMasters.values().contains(static_cast<ModbusTCPMaster *>(modbus))) {
        Device *parentDevice = m_modbusTCPMasters.key(static_cast<ModbusTCPMaster *>(modbus));
        foreach(Device *device, myDevices().filterByParentDeviceId(parentDevice->id())) {
            if ((device->paramValue(holdingRegisterDeviceSlaveAddressParamTypeId) == slaveAddress) && (device->paramValue(holdingRegisterDeviceRegisterAddressParamTypeId) == modbusRegister))
                device->setStateValue(holdingRegisterValueStateTypeId, value);
        }
    }
}

void DevicePluginModbusCommander::onReceivedInputRegister(uint slaveAddress, uint modbusRegister, int value)
{
    auto modbus = sender();

    if (m_modbusRTUMasters.values().contains(static_cast<ModbusRTUMaster *>(modbus))) {
        Device *parentDevice = m_modbusRTUMasters.key(static_cast<ModbusRTUMaster *>(modbus));
        foreach(Device *device, myDevices().filterByParentDeviceId(parentDevice->id())) {
            if ((device->paramValue(inputRegisterDeviceSlaveAddressParamTypeId) == slaveAddress) && (device->paramValue(inputRegisterDeviceRegisterAddressParamTypeId) == modbusRegister))
                device->setStateValue(inputRegisterValueStateTypeId, value);
        }
    }

    if (m_modbusTCPMasters.values().contains(static_cast<ModbusTCPMaster *>(modbus))) {
        Device *parentDevice = m_modbusTCPMasters.key(static_cast<ModbusTCPMaster *>(modbus));
        foreach(Device *device, myDevices().filterByParentDeviceId(parentDevice->id())) {
            if ((device->paramValue(inputRegisterDeviceSlaveAddressParamTypeId) == slaveAddress) && (device->paramValue(inputRegisterDeviceRegisterAddressParamTypeId) == modbusRegister))
                device->setStateValue(inputRegisterValueStateTypeId, value);
        }
    }
}

void DevicePluginModbusCommander::readRegister(Device *device)
{
    Device *parent = myDevices().findById(device->parentId());

    if (!parent)
        return;

    uint registerAddress;
    uint slaveAddress;

    if (parent->deviceClassId() == modbusTCPClientDeviceClassId) {
        ModbusTCPMaster *modbus = m_modbusTCPMasters.value(parent);
        if (!modbus)
            return;

        if (device->deviceClassId() == coilDeviceClassId) {
            registerAddress = device->paramValue(coilDeviceRegisterAddressParamTypeId).toUInt();
            slaveAddress = device->paramValue(coilDeviceSlaveAddressParamTypeId).toUInt();
            modbus->readCoil(slaveAddress, registerAddress);
        }
        if (device->deviceClassId() == discreteInputDeviceClassId) {
            registerAddress = device->paramValue(discreteInputDeviceRegisterAddressParamTypeId).toUInt();
            slaveAddress = device->paramValue(discreteInputDeviceSlaveAddressParamTypeId).toUInt();
            modbus->readDiscreteInput(slaveAddress, registerAddress);
        }
        if (device->deviceClassId() == holdingRegisterDeviceClassId) {
            registerAddress = device->paramValue(holdingRegisterDeviceRegisterAddressParamTypeId).toUInt();
            slaveAddress = device->paramValue(holdingRegisterDeviceSlaveAddressParamTypeId).toUInt();
            modbus->readHoldingRegister(slaveAddress, registerAddress);
        }
        if (device->deviceClassId() == inputRegisterDeviceClassId) {
            registerAddress = device->paramValue(inputRegisterDeviceRegisterAddressParamTypeId).toUInt();
            slaveAddress = device->paramValue(inputRegisterDeviceSlaveAddressParamTypeId).toUInt();
            modbus->readInputRegister(slaveAddress, registerAddress);
        }
    }

    if (parent->deviceClassId() == modbusRTUClientDeviceClassId) {

        ModbusRTUMaster *modbus = m_modbusRTUMasters.value(parent);
        if (!modbus)
            return;

        if (device->deviceClassId() == coilDeviceClassId) {
            registerAddress = device->paramValue(coilDeviceRegisterAddressParamTypeId).toUInt();
            slaveAddress = device->paramValue(coilDeviceSlaveAddressParamTypeId).toUInt();
            modbus->readCoil(slaveAddress, registerAddress);
        }
        if (device->deviceClassId() == discreteInputDeviceClassId) {
            registerAddress = device->paramValue(discreteInputDeviceRegisterAddressParamTypeId).toUInt();
            slaveAddress = device->paramValue(discreteInputDeviceSlaveAddressParamTypeId).toUInt();
            modbus->readDiscreteInput(slaveAddress, registerAddress);
        }
        if (device->deviceClassId() == holdingRegisterDeviceClassId) {
            registerAddress = device->paramValue(holdingRegisterDeviceRegisterAddressParamTypeId).toUInt();
            slaveAddress = device->paramValue(holdingRegisterDeviceSlaveAddressParamTypeId).toUInt();
            modbus->readHoldingRegister(slaveAddress, registerAddress);
        }
        if (device->deviceClassId() == inputRegisterDeviceClassId) {
            registerAddress = device->paramValue(inputRegisterDeviceRegisterAddressParamTypeId).toUInt();
            slaveAddress = device->paramValue(inputRegisterDeviceSlaveAddressParamTypeId).toUInt();
            modbus->readInputRegister(slaveAddress, registerAddress);
        }
    }
}

void DevicePluginModbusCommander::writeRegister(Device *device, DeviceActionInfo *info)
{
    Device *parent = myDevices().findById(device->parentId());
    uint registerAddress;
    uint slaveAddress;
    QUuid requestId;
    Action action = info->action();

    if (parent->deviceClassId() == modbusTCPClientDeviceClassId) {
        ModbusTCPMaster *modbus = m_modbusTCPMasters.value(parent);
        if (!modbus)
            return;

        if (device->deviceClassId() == coilDeviceClassId) {
            registerAddress = device->paramValue(coilDeviceRegisterAddressParamTypeId).toUInt();
            slaveAddress = device->paramValue(coilDeviceSlaveAddressParamTypeId).toUInt();
            requestId = modbus->writeCoil(slaveAddress, registerAddress, action.param(coilValueActionValueParamTypeId).value().toBool());
        }
        if (device->deviceClassId() == holdingRegisterDeviceClassId) {
            registerAddress = device->paramValue(holdingRegisterDeviceRegisterAddressParamTypeId).toUInt();
            slaveAddress = device->paramValue(holdingRegisterDeviceSlaveAddressParamTypeId).toUInt();
            requestId = modbus->writeHoldingRegister(slaveAddress, registerAddress, action.param(holdingRegisterValueActionValueParamTypeId).value().toUInt());
        }
    }

    if (parent->deviceClassId() == modbusRTUClientDeviceClassId) {
        ModbusRTUMaster *modbus = m_modbusRTUMasters.value(parent);
        if (!modbus)
            return;

        if (device->deviceClassId() == coilDeviceClassId) {
            registerAddress = device->paramValue(coilDeviceRegisterAddressParamTypeId).toUInt();
            slaveAddress = device->paramValue(coilDeviceSlaveAddressParamTypeId).toUInt();
            requestId = modbus->writeCoil(slaveAddress, registerAddress, action.param(coilValueActionValueParamTypeId).value().toBool());
        }
        if (device->deviceClassId() == holdingRegisterDeviceClassId) {
            registerAddress = device->paramValue(holdingRegisterDeviceRegisterAddressParamTypeId).toUInt();
            slaveAddress = device->paramValue(holdingRegisterDeviceSlaveAddressParamTypeId).toUInt();
            requestId = modbus->writeHoldingRegister(slaveAddress, registerAddress, action.param(holdingRegisterValueActionValueParamTypeId).value().toUInt());
        }
    }
    if (requestId.isNull()){
        info->finish(Device::DeviceErrorHardwareNotAvailable);
    } else {
        m_asyncActions.insert(requestId, info);
        connect(info, &DeviceActionInfo::aborted, [requestId, this] {m_asyncActions.remove(requestId);});
    }
}
