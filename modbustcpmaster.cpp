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

#include "modbustcpmaster.h"
#include "extern-plugininfo.h"

ModbusTCPMaster::ModbusTCPMaster(QString IPv4Address, uint port, QObject *parent) :
    QObject(parent)
{
    m_modbusTcpClient = new QModbusTcpClient(this);
    m_modbusTcpClient->setConnectionParameter(QModbusDevice::NetworkPortParameter, port);
    m_modbusTcpClient->setConnectionParameter(QModbusDevice::NetworkAddressParameter, IPv4Address);
    //m_modbusTcpClient->setTimeout(100);
    //m_modbusTcpClient->setNumberOfRetries(1);

    connect(m_modbusTcpClient, &QModbusTcpClient::stateChanged, this, &ModbusTCPMaster::onModbusStateChanged);
    connect(m_modbusTcpClient, &QModbusRtuSerialMaster::errorOccurred, this, &ModbusTCPMaster::onModbusErrorOccurred);

    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, &ModbusTCPMaster::onReconnectTimer);
}

ModbusTCPMaster::~ModbusTCPMaster()
{
    if (!m_modbusTcpClient) {
        m_modbusTcpClient->disconnectDevice();
        m_modbusTcpClient->deleteLater();
    }
    if (!m_reconnectTimer) {
        m_reconnectTimer->stop();
        m_reconnectTimer->deleteLater();
    }
}

bool ModbusTCPMaster::connectDevice() {
    // TCP connction to target device
    qCDebug(dcModbusCommander()) << "Setting up TCP connecion";

    if (!m_modbusTcpClient)
        return false;

    return m_modbusTcpClient->connectDevice();
}

uint ModbusTCPMaster::port()
{
    return m_modbusTcpClient->connectionParameter(QModbusDevice::NetworkPortParameter).toUInt();
}

bool ModbusTCPMaster::setIPv4Address(QString ipv4Address)
{
    m_modbusTcpClient->setConnectionParameter(QModbusDevice::NetworkAddressParameter, ipv4Address);
    return connectDevice();
}

bool ModbusTCPMaster::setPort(uint port)
{
    m_modbusTcpClient->setConnectionParameter(QModbusDevice::NetworkPortParameter, port);
    return connectDevice();
}

void ModbusTCPMaster::onReconnectTimer()
{
    if(!m_modbusTcpClient->connectDevice()) {
        m_reconnectTimer->start(10000);
    }
}

QString ModbusTCPMaster::ipv4Address()
{
    return m_modbusTcpClient->connectionParameter(QModbusDevice::NetworkAddressParameter).toString();
}

QUuid ModbusTCPMaster::readCoil(uint slaveAddress, uint registerAddress)
{
    if (!m_modbusTcpClient) {
        return "";
    }
    QUuid requestId = QUuid::createUuid();

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils, registerAddress, 1);

    if (QModbusReply *reply = m_modbusTcpClient->sendReadRequest(request, slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, [reply, requestId, this] {
                reply->deleteLater();

                if (reply->error() == QModbusDevice::NoError) {
                    requestExecuted(requestId, true);
                    const QModbusDataUnit unit = reply->result();
                    uint modbusAddress = unit.startAddress();
                    emit receivedCoil(reply->serverAddress(), modbusAddress, unit.value(0));

                } else {
                    requestExecuted(requestId, false);
                    qCWarning(dcModbusCommander()) << "Read response error:" << reply->error();
                }
            });
            connect(reply, &QModbusReply::errorOccurred, [reply, requestId, this] (QModbusDevice::Error error){

                qCWarning(dcModbusCommander()) << "Modbus reply error:" << error;
                emit requestError(requestId, reply->errorString());
                reply->finished(); // To make sure it will be deleted
            });
            QTimer::singleShot(200, reply, &QModbusReply::deleteLater);
        } else {
            delete reply; // broadcast replies return immediately
            return "";
        }
    } else {
        qCWarning(dcModbusCommander()) << "Read error: " << m_modbusTcpClient->errorString();
        return "";
    }
    return requestId;
}

QUuid ModbusTCPMaster::writeHoldingRegister(uint slaveAddress, uint registerAddress, uint value)
{
    if (!m_modbusTcpClient) {
        return "";
    }
    QUuid requestId = QUuid::createUuid();
    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::HoldingRegisters, registerAddress, 1);
    request.setValue(0, static_cast<uint16_t>(value));

    if (QModbusReply *reply = m_modbusTcpClient->sendWriteRequest(request, slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, [reply, requestId, this] {

                if (reply->error() == QModbusDevice::NoError) {
                    requestExecuted(requestId, true);
                    const QModbusDataUnit unit = reply->result();
                    uint modbusAddress = unit.startAddress();
                    emit receivedHoldingRegister(reply->serverAddress(), modbusAddress, unit.value(0));

                } else {
                    requestExecuted(requestId, false);
                    qCWarning(dcModbusCommander()) << "Read response error:" << reply->error();
                }
                reply->deleteLater();
            });
            connect(reply, &QModbusReply::errorOccurred, [reply, requestId, this] (QModbusDevice::Error error){

                qCWarning(dcModbusCommander()) << "Modbus replay error:" << error;
                emit requestError(requestId, reply->errorString());
                reply->finished(); // To make sure it will be deleted
            });
            QTimer::singleShot(200, reply, &QModbusReply::deleteLater);
        } else {
            delete reply; // broadcast replies return immediately
            return "";
        }
    } else {
        qCWarning(dcModbusCommander()) << "Read error: " << m_modbusTcpClient->errorString();
        return "";
    }
    return requestId;
}

QUuid ModbusTCPMaster::readDiscreteInput(uint slaveAddress, uint registerAddress)
{
    if (!m_modbusTcpClient) {
        return "";
    }
    QUuid requestId = QUuid::createUuid();

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::DiscreteInputs, registerAddress, 1);

    if (QModbusReply *reply = m_modbusTcpClient->sendReadRequest(request, slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, [reply, requestId, this] {
                reply->deleteLater();
                if (reply->error() == QModbusDevice::NoError) {
                    requestExecuted(requestId, true);
                    const QModbusDataUnit unit = reply->result();
                    uint modbusAddress = unit.startAddress();
                    emit receivedDiscreteInput(reply->serverAddress(), modbusAddress, unit.value(0));

                } else {
                    requestExecuted(requestId, false);
                    qCWarning(dcModbusCommander()) << "Read response error:" << reply->error();
                }
            });
            connect(reply, &QModbusReply::errorOccurred, [requestId, this] (QModbusDevice::Error error){

                qCWarning(dcModbusCommander()) << "Modbus replay error:" << error;
                QModbusReply *reply = qobject_cast<QModbusReply *>(sender());
                emit requestError(requestId, reply->errorString());
                reply->finished(); // To make sure it will be deleted
            });
            QTimer::singleShot(200, reply, &QModbusReply::deleteLater);
        } else {
            delete reply; // broadcast replies return immediately
            return "";
        }
    } else {
        qCWarning(dcModbusCommander()) << "Read error: " << m_modbusTcpClient->errorString();
        return "";
    }
    return requestId;
}

QUuid ModbusTCPMaster::readInputRegister(uint slaveAddress, uint registerAddress)
{
    if (!m_modbusTcpClient) {
        return "";
    }
    QUuid requestId = QUuid::createUuid();

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::InputRegisters, registerAddress, 1);

    if (QModbusReply *reply = m_modbusTcpClient->sendReadRequest(request, slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, [reply, requestId, this] {
                reply->deleteLater();
                if (reply->error() == QModbusDevice::NoError) {
                    requestExecuted(requestId, true);
                    const QModbusDataUnit unit = reply->result();
                    uint modbusAddress = unit.startAddress();
                    emit receivedInputRegister(reply->serverAddress(), modbusAddress, unit.value(0));

                } else {
                    requestExecuted(requestId, false);
                    qCWarning(dcModbusCommander()) << "Read response error:" << reply->error();
                }
            });
            connect(reply, &QModbusReply::errorOccurred, [reply, requestId, this] (QModbusDevice::Error error){

                qCWarning(dcModbusCommander()) << "Modbus reply error:" << error;
                emit requestError(requestId, reply->errorString());
                reply->finished(); // To make sure it will be deleted
            });
            QTimer::singleShot(200, reply, &QModbusReply::deleteLater);
        } else {
            delete reply; // broadcast replies return immediately
            return "";
        }
    } else {
        qCWarning(dcModbusCommander()) << "Read error: " << m_modbusTcpClient->errorString();
        return "";
    }
    return requestId;
}

QUuid ModbusTCPMaster::readHoldingRegister(uint slaveAddress, uint registerAddress)
{
    if (!m_modbusTcpClient) {
        return "";
    }
    QUuid requestId = QUuid::createUuid();

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::HoldingRegisters, registerAddress, 1);

    if (QModbusReply *reply = m_modbusTcpClient->sendReadRequest(request, slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, [reply, requestId, this] {

                if (reply->error() == QModbusDevice::NoError) {
                    requestExecuted(requestId, true);
                    const QModbusDataUnit unit = reply->result();
                    uint modbusAddress = unit.startAddress();
                    emit receivedHoldingRegister(reply->serverAddress(), modbusAddress, unit.value(0));

                } else {
                    requestExecuted(requestId, false);
                    qCWarning(dcModbusCommander()) << "Read response error:" << reply->error();
                }
                reply->deleteLater();
            });
            connect(reply, &QModbusReply::errorOccurred, [reply, requestId, this] (QModbusDevice::Error error){

                qCWarning(dcModbusCommander()) << "Modbus replay error:" << error;
                emit requestError(requestId, reply->errorString());
                reply->finished(); // To make sure it will be deleted
            });
            QTimer::singleShot(200, reply, &QModbusReply::deleteLater);
        } else {
            delete reply; // broadcast replies return immediately
            return "";
        }
    } else {
        qCWarning(dcModbusCommander()) << "Read error: " << m_modbusTcpClient->errorString();
        return "";
    }
    return requestId;
}

QUuid ModbusTCPMaster::writeCoil(uint slaveAddress, uint registerAddress, bool value)
{
    if (!m_modbusTcpClient) {
        return "";
    }
    QUuid requestId = QUuid::createUuid();

    QModbusDataUnit request = QModbusDataUnit(QModbusDataUnit::RegisterType::Coils, registerAddress, 1);
    request.setValue(0, static_cast<uint16_t>(value));

    if (QModbusReply *reply = m_modbusTcpClient->sendWriteRequest(request, slaveAddress)) {
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, [reply, requestId, this] {

                if (reply->error() == QModbusDevice::NoError) {
                    requestExecuted(requestId, true);
                    const QModbusDataUnit unit = reply->result();
                    uint modbusAddress = unit.startAddress();
                    emit receivedCoil(reply->serverAddress(), modbusAddress, unit.value(0));

                } else {
                    requestExecuted(requestId, false);
                    qCWarning(dcModbusCommander()) << "Read response error:" << reply->error();
                }
                reply->deleteLater();
            });
            connect(reply, &QModbusReply::errorOccurred, [reply, requestId, this] (QModbusDevice::Error error){

                qCWarning(dcModbusCommander()) << "Modbus reply error:" << error;
                emit requestError(requestId, reply->errorString());
                reply->finished(); // To make sure it will be deleted
            });
            QTimer::singleShot(200, reply, &QModbusReply::deleteLater);
        } else {
            delete reply; // broadcast replies return immediately
            return "";
        }
    } else {
        qCWarning(dcModbusCommander()) << "Read error: " << m_modbusTcpClient->errorString();
        return "";
    }
    return requestId;
}


void ModbusTCPMaster::onModbusErrorOccurred(QModbusDevice::Error error)
{
    qCWarning(dcModbusCommander()) << "An error occured" << error;
}


void ModbusTCPMaster::onModbusStateChanged(QModbusDevice::State state)
{
    bool connected = (state != QModbusDevice::UnconnectedState);
    if (!connected) {
        //try to reconnect in 10 seconds
        m_reconnectTimer->start(10000);
    }
    emit connectionStateChanged(connected);
}
