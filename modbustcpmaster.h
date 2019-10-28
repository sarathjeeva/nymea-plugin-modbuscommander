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

#ifndef MODBUSTCPMASTER_H
#define MODBUSTCPMASTER_H

#include <QObject>
#include <QHostAddress>
#include <QtSerialBus>
#include <QTimer>

class ModbusTCPMaster : public QObject
{
    Q_OBJECT
public:
    explicit ModbusTCPMaster(QString ipAddress, uint port, QObject *parent = nullptr);
    ~ModbusTCPMaster();

    bool connectDevice();

    bool readCoil(uint slaveAddress, uint registerAddress);
    bool readDiscreteInput(uint slaveAddress, uint registerAddress);
    bool readInputRegister(uint slaveAddress, uint registerAddress);
    bool readHoldingRegister(uint slaveAddress, uint registerAddress);

    bool writeCoil(uint slaveAddress, uint registerAddress, bool status);
    bool writeHoldingRegister(uint slaveAddress, uint registerAddress, uint data);

    QString ipv4Address();
    uint port();
    bool setIPv4Address(QString ipAddress);
    bool setPort(uint port);


private:
    QTimer *m_reconnectTimer = nullptr;
    QModbusTcpClient *m_modbusTcpClient;

private slots:
    void onReplyFinished();
    void onReplyErrorOccured(QModbusDevice::Error error);
    void onReconnectTimer();

    void onModbusErrorOccurred(QModbusDevice::Error error);
    void onModbusStateChanged(QModbusDevice::State state);

signals:
    void connectionStateChanged(bool status);
    void receivedCoil(uint slaveAddress, uint modbusRegister, bool value);
    void receivedDiscreteInput(uint slaveAddress, uint modbusRegister, bool value);
    void receivedHoldingRegister(uint slaveAddress, uint modbusRegister, uint value);
    void receivedInputRegister(uint slaveAddress, uint modbusRegister, uint value);
};

#endif // MODBUSTCPMASTER_H
