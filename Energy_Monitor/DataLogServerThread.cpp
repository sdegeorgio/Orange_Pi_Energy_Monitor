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
 * File:   DataLogServerThread.cpp
 * Author: Stephan de Georgio
 *
 * Created on 04 November 2020, 14:32
 */

#include <QDateTime>
#include <QtDebug>
#include "EnergyMonitorAppGlobal.h"
#include "DataLogServerThread.h"
#include "telnet.h"

/* Commands are always 7 characters long */
#define COMMAND_LENGTH 7
#define COMMAND_GET_ALL "GET ALL"
/* Send data to client as soon as it is available */
#define COMMAND_SET_SEND_IMMEDIATE "SET NOW"
/* Stop sending data to client as soon as it is available - Request only from now on*/
#define COMMAND_CLR_SEND_IMMEDIATE "CLR NOW"
/* Sets the power measurement update interval, 1 second by default.
 * This option takes a parameter of up to xxxxx in milliseconds
 */
#define COMMAND_SET_UPDATE_INTERVAL "SET UPD"

#define COMMAND_PROMPT "\r\n# "

DataLogServerThread::DataLogServerThread(qintptr ID, QObject *parent)
    : QThread(parent) {
    this->socketDescriptor = ID;
    sendImmediate = false;
    updateIntervalMillis = 1000;
}

DataLogServerThread::~DataLogServerThread() {
}

void DataLogServerThread::run() {
    socket = new QTcpSocket;
    if (!socket->setSocketDescriptor(this->socketDescriptor)) {
        emit error(socket->error());
        return;
    }
    
    // connect socket and signal
    // note - Qt::DirectConnection is used because it's multi-threaded
    //        This ensures the slot is invoked immediately when the signal is emitted.

    connect(socket, SIGNAL(readyRead()), this, SLOT(readyRead()), Qt::DirectConnection);
    connect(socket, SIGNAL(disconnected()), this, SLOT(disconnected()));
    
    // We'll have multiple clients, we want to know which is which
    qDebug() << "Client " << socketDescriptor << " connected";
    
    // make this thread a loop,
    // thread will stay alive so that signal/slot functions properly
    // and does not drop out when the thread completes
    QString ident;
    ident = QString(SOFTWARE_NAME) + QString(" ") + QString(SOFTWARE_VERSION) + QString("\r\n") + "# ";
    socket->write(ident.toLocal8Bit());
    exec();
}

/* Processes all Telnet data sent from the client.
 * For Telnet control data received, currently this method
 * just prints out what is received.
 */
void DataLogServerThread::processBytes(QByteArray bytes) {
    int position = 0;
    char dataByte[3];
    QDebug debug = qDebug();
    debug.nospace();
    
    while(position < bytes.length()) {
        // Process Telnet protocol command bytes
        if(bytes.at(position) == TELNET_IAC) {
            position++;
            switch(bytes.at(position)) {
                case TELNET_EOF:
                    /* control-d received so close the connection */
                    debug << "EOF";
                    socket->close();
                    break;
                case TELNET_IP:
                    /* control-c received so close the connection */
                    debug << "Interrupt process";
                    socket->close();
                    break;
                case TELNET_WILL:
                    debug << "WILL";
                    position++;
                    // There is another byte to this command we are ignoring but printing below
                    break;
                case TELNET_WONT:
                    debug << "WONT";
                    position++;
                    // There is another byte to this command we are ignoring but printing below
                    break;
                case TELNET_DO:
                    debug << "DO";
                    position++;
                    // Check if we are being requested to echo data
                    if(bytes.at(position) == TELNET_ECHO_DATA) {
                        // Tell them we will not echo data!
                        /*
                        dataByte[0] = TELNET_IAC;
                        dataByte[1] = TELNET_WONT;
                        dataByte[2] = TELNET_ECHO_DATA;
                        socket->write(dataByte);
                        */
                    }
                    // There is another byte to this command we are ignoring but printing below
                    break;
                case TELNET_DONT:
                    debug << "DONT";
                    position++;
                    // There is another byte to this command we are ignoring but printing below
                    break;
                default:
                    debug << "Unhandled command";
                    break;
            }
            sprintf(dataByte, "%02X", (unsigned char)bytes.at(position));
            debug <<  " received: " << dataByte << "\r\n";
            position++;
        } else {
            // Check if a command is received
            // Commands always end in \r + \0
            if(position <= bytes.length() + 1 && (unsigned int)(bytes.length() - position) >= COMMAND_LENGTH) {
                QString command = QString(bytes.mid(position, COMMAND_LENGTH));
                //Check if GET all data command is received
                if(command == COMMAND_GET_ALL) {
                     debug << COMMAND_GET_ALL << " received!\r\n";
                     position += COMMAND_LENGTH;
                     // Send the data to the client
                     socket->write("\r\n" + responseData.toLocal8Bit());
                } else
                // Check if set update interval command is received
                if(command == COMMAND_SET_UPDATE_INTERVAL) {
                     debug << COMMAND_SET_UPDATE_INTERVAL << " received!\r\n";
                     position += COMMAND_LENGTH;
                     
                     if(bytes.length() - position >= 2) {
                        // Increment for the space
                        position++;
                        // Get all characters after the command to the end of the string as the interval
                        QString updateInterval = QString(bytes).mid(position, bytes.length() - position - 2);
                        position += updateInterval.length();
                        int millis = updateInterval.toInt();
                        if(millis >= 100 && millis <= 10000) {
                            updateIntervalMillis = millis;
                            socket->write("Update interval changed to ");
                            socket->write(QString::number(updateIntervalMillis).toLocal8Bit());
                        } else {
                            socket->write("You have entered an incorrect update interval! ");
                            socket->write(QString::number(millis).toLocal8Bit());
                        }
                     }
                } else
                // Check if send immediate command is received
                if(command == COMMAND_SET_SEND_IMMEDIATE) {
                     debug << COMMAND_SET_SEND_IMMEDIATE << " received!\r\n";
                     position += COMMAND_LENGTH;
                     sendImmediate = true;
                } else
                // Check if clear send immediate command is received
                if(command == COMMAND_CLR_SEND_IMMEDIATE) {
                     debug << COMMAND_CLR_SEND_IMMEDIATE << " received!\r\n";
                     position += COMMAND_LENGTH;
                     sendImmediate = false;
                } else {
                    position++;
                }
                // Print the prompt once the command has been fully processed including the end of line and string termination
                if(bytes.at(position) == '\r') {
                    position+=2;
                    socket->write(COMMAND_PROMPT);
                }
            } else {
                // Catches anything that is not a command
                if(bytes.at(position) == '\r') {
                    socket->write("\r\nInvalid command!");
                    socket->write(QString(COMMAND_PROMPT).toLocal8Bit());
                }
                position++;
            }
        }
    }
}

/* Data arrives here from the Telnet connection */
void DataLogServerThread::readyRead() {
    //QByteArray response(3, 0);
    // get the information
    QByteArray data = socket->readAll();
    //QString stringData = QString(data.toHex());
    // Print the entire string of data received from the client
    //qDebug() << "Client " << socketDescriptor << " Data in: " << data << " in hex: " << stringData;

    processBytes(data);
}

void DataLogServerThread::disconnected() {
    qDebug() << "Client " << socketDescriptor << " Disconnected";
    socket->deleteLater();
    exit(0);
}

/* Slot is called every time some new measurements are ready
 * This method formats the data ready to be sent over Telnet
 */
void DataLogServerThread::slotMeasurementsReady(DecodedMeasurements values) {
    QString format = QString("yyyy-MM-dd hh:mm:ss.zzz");
    //qint64 currentTimeInt = QDateTime::currentMSecsSinceEpoch();
    //QString currentTimeString = QString::number(currentTimeInt);
    QString currentTimeString = QDateTime::currentDateTime().toString(format);
    responseData = 
            currentTimeString + ","
            + QString::number(values.powerActive) + ","
            + QString::number(values.voltageRms) + ","
            + QString::number(values.currentRms) + ","
            + QString::number(values.frequency) + ","
            + QString::number(values.powerFactor) + ","
            + QString::number(values.powerApparent) + ","
            + QString::number(values.powerReactive)
            + "\r\n";
    
    /* If sendImmediate has been activated, send the data straight out to the client */
    if(sendImmediate) {
        socket->write(responseData.toLocal8Bit());
    }
}