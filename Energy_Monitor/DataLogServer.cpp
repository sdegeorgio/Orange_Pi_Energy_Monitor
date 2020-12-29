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
 * This file sets up a TCP/IP server bound to port 23.
 * Incoming connections will be sent the CSV data.
 */

/* 
 * File:   DataLogServer.cpp
 * Author: Stephan de Georgio
 * 
 * Created on 04 November 2016, 14:33
 */


#include <QHostAddress>
#include <QList>
#include <QNetworkInterface>
#include <QString>

#include "EnergyMonitor.h"
#include "DataLogServer.h"
#include "DataLogServerThread.h"

DataLogServer::DataLogServer(QObject *parent) {
    setParent(parent);
}

DataLogServer::~DataLogServer() {
}

void DataLogServer::startServer() {
    if(!this->listen(QHostAddress::Any, DATA_LOG_SERVER_LISTEN_PORT)) {
        qDebug() << "Could not start the Telnet server";
    } else {
        qDebug() << "Server started and listening on address: " << getLocalIPAddress(QAbstractSocket::IPv4Protocol) << getLocalIPAddress(QAbstractSocket::IPv6Protocol);
    }
}


QList<QString> DataLogServer::getLocalIPAddress(QAbstractSocket::NetworkLayerProtocol protocolType) {
    QList<QHostAddress> list = QNetworkInterface::allAddresses();
    QList<QString> addressList;
    for(int nIter=0; nIter<list.count(); nIter++) {
        if(!list[nIter].isLoopback()) {
            if (list[nIter].protocol() == protocolType) {
                addressList.append(list[nIter].toString());
            }
        }
    }
    return addressList;
}

void DataLogServer::incomingConnection(qintptr socketDescriptor) {
    EnergyMonitor *em = (EnergyMonitor*)parent();
    DataLogServerThread *thread = new DataLogServerThread(socketDescriptor, this);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    connect(em->powerMeter, SIGNAL(measurementsReady(DecodedMeasurements)), thread, SLOT(slotMeasurementsReady(DecodedMeasurements)));
    thread->start();
}
