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
 * File:   DataLogServerThread.h
 * Author: Stephan de Georgio
 *
 * Created on 04 November 2020, 14:32
 */


#ifndef DATALOGSERVERTHREAD_H
#define DATALOGSERVERTHREAD_H

#include "MCP39F511Interface.h"

#include <QThread>
#include <QTcpSocket>
#include <QDebug>
#include <QByteArray>


class DataLogServerThread : public QThread {
    Q_OBJECT
public:
    explicit DataLogServerThread(qintptr ID, QObject *parent = 0);
    virtual ~DataLogServerThread();
    void run();
    
signals:
    void error(QTcpSocket::SocketError socketError);

public slots:
    void readyRead();
    void disconnected();
    void slotMeasurementsReady(DecodedMeasurements);

private:
    QTcpSocket *socket;
    int socketDescriptor;
    void processBytes(QByteArray bytes);
    QString responseData;
    bool sendImmediate;
    int updateIntervalMillis;
};

#endif /* DATALOGSERVERTHREAD_H */

