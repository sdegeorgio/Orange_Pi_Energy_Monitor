/*
 * EM100 - Energy Monitor for Volts / Amps / Power displayed on LCD, with USB and Ethernet interfaces.
 * Copyright (C) 2016-2017 Stephan de Georgio
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

/* 
 * File:   DataLogServer.h
 * Author: Stephan de Georgio
 *
 * Created on 04 November 2016, 14:32
 */

#ifndef DATALOGSERVER_H
#define DATALOGSERVER_H

#define DATA_LOG_SERVER_LISTEN_PORT 23


#include "MCP39F511Interface.h"

#include <QBuffer>
#include <QList>
#include <QTcpSocket>
#include <QTcpServer>
#include <QObject>

class DataLogServer : public QObject {
    Q_OBJECT
    
public:
    DataLogServer(QObject *parent);
    virtual ~DataLogServer();
    
    bool startServer();
    void stopServer();
    bool isServerRunning();
    QList<QString> getLocalIPAddress(QAbstractSocket::NetworkLayerProtocol protocolType);

public slots:
    void slotMeasurementsReady(DecodedMeasurements);
    
private:
    QTcpServer *logServer;
    QList<QTcpSocket*> socketList;
    bool serverRunning;
    
private slots:
    void acceptConnection();
    void clientDisconnected();
};

#endif /* DATALOGSERVER_H */

