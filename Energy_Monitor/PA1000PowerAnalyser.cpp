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
 * File:   PA1000PowerAnalyser.cpp
 * Author: Stephan de Georgio
 * 
 * Created on 26 September 2016, 09:24
 */

#include "PA1000PowerAnalyser.h"

#include <QtGlobal>
#include <QtDebug>
#include <QAbstractSocket>
#include <QStringList>
#include <QThread>

#define INIT_COMMAND_COUNT 2
#define MEASUREMENT_COUNT 7
#define INIT_LIST_SIZE (INIT_COMMAND_COUNT + MEASUREMENT_COUNT)

const char *initCommandList[INIT_LIST_SIZE] = {
								":SHU:INT1A;\r\n",  // Init command 1: Set 1A internal shunt
	                            ":SEL:CLR;\r\n",   // Init command 2: Clear all measurements
								
                                ":SEL:VLT;\r\n",   // Measurement 1: Volts RMS
                                ":SEL:AMP;\r\n",   // Measurement 2: Amps RMS
                                ":SEL:WAT;\r\n",   // Measurement 3: Active power
                                ":SEL:FRQ;\r\n",   // Measurement 4: Frequency
                                ":SEL:PWF;\r\n",   // Measurement 5: Power factor
								":SEL:VAR;\r\n",    // Measurement 6: Reactive power
								":SEL:VAS;\r\n"    // Measurement 7: Apparent power
                            };

const char *measurementCommand = ":FRD?;\r\n";

PA1000PowerAnalyser::PA1000PowerAnalyser(QObject *parent) {
	setParent(parent);
	pa1000TcpCon = new QTcpSocket(this);
	commsState = STATE_PA1000_IDLE;
}

PA1000PowerAnalyser::~PA1000PowerAnalyser() {
}

void PA1000PowerAnalyser::printMessage(QString message) {
	qDebug() << "PA1000 module: " << message;
}

void PA1000PowerAnalyser::connectToAnalyser(QString ipAddress) {
	pa1000Host = ipAddress;
	connect(pa1000TcpCon, SIGNAL(hostFound()), this, SLOT(slotHostFound()));
	connect(pa1000TcpCon, SIGNAL(connected()), this, SLOT(slotConnected()));
	connect(pa1000TcpCon, SIGNAL(disconnected()), this, SLOT(slotDisconnected()));
	connect(pa1000TcpCon, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(slotError(QAbstractSocket::SocketError)));
	connect(pa1000TcpCon, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(slotStateChanged(QAbstractSocket::SocketState)));
	connect(pa1000TcpCon, SIGNAL(readyRead()), this, SLOT(slotDataReady()));
	printMessage("Connecting to PA1000 at: " + ipAddress);
	pa1000TcpCon->connectToHost(pa1000Host, SCPI_PORT, QIODevice::ReadWrite, QAbstractSocket::AnyIPProtocol);
	responseIndex = 0;
	commsState = STATE_PA1000_INITIALISING;
}

void PA1000PowerAnalyser::disconnectFromAnalyser() {
	if(pa1000TcpCon->isOpen()) {
		printMessage("Disconnecting from PA1000...");
		/* Close the socket when complete */
		pa1000TcpCon->close();
	}
}
	
void PA1000PowerAnalyser::getPowerMeasurements() {
	commsState = STATE_PA1000_GET_MEASUREMENTS;
	responseIndex = 0;
	pa1000TcpCon->write(measurementCommand);
}

void PA1000PowerAnalyser::slotHostFound() {
	printMessage("Host " + pa1000Host + " found...");
}

void PA1000PowerAnalyser::slotConnected() {
	printMessage("Connected to PA1000.");
	/* Start writing out the initialisation command sequence */
	writeToPA1000(initCommandList[responseIndex++]);
}

void PA1000PowerAnalyser::slotDisconnected() {
	printMessage("Disconnected from PA1000.");
}

void PA1000PowerAnalyser::slotError(QAbstractSocket::SocketError socketError) {
	switch(socketError) {
		case QAbstractSocket::ConnectionRefusedError:
			printMessage("Error: Connection to PA1000 refused.");
			break;
		case QAbstractSocket::RemoteHostClosedError:
			printMessage("Error: PA1000 closed connection.");
			break;
		case QAbstractSocket::HostNotFoundError:
			printMessage("Error: PA1000 not found, check hostname / IP address.");
			break;
		case QAbstractSocket::SocketTimeoutError:
		case QAbstractSocket::NetworkError:
			printMessage("Error: timeout trying to connect to PA1000.  Check IP address or try power cycling PA1000.");
			break;
		default:
			QString output;
			QDebug(&output) << socketError;
			printMessage("Error: Unable to connect to PA1000 with error: " + output);
			break;
	}
}

void PA1000PowerAnalyser::slotStateChanged(QAbstractSocket::SocketState socketState) {
	switch(socketState) {
		case QAbstractSocket::UnconnectedState:
			printMessage("Not connected to PA1000.");
			break;
		case QAbstractSocket::HostLookupState:
			printMessage("Looking up PA1000 host: " + pa1000Host);
			break;
		case QAbstractSocket::ConnectingState:
			printMessage("Connecting to PA1000.");
			break;
		case QAbstractSocket::ConnectedState:
			printMessage("Connected to PA1000.");
			break;
		case QAbstractSocket::BoundState:
			printMessage("Connection bound to PA1000.");
			break;
		case QAbstractSocket::ClosingState:
			printMessage("Closing connection to PA1000.");
			break;
		default:
			/* Get the name of the socket state from QDebug */
			QString output;
			QDebug(&output) << socketState;
			printMessage("State: " + output);
			break;
	}
}

void PA1000PowerAnalyser::writeToPA1000(QString command) {
	printMessage("To PA1000: " + command);
	pa1000TcpCon->write(command.toUtf8());
}

void PA1000PowerAnalyser::slotDataReady() {
	QStringList list;
	QString data = pa1000TcpCon->readLine();
	data = data.trimmed();
	printMessage("From PA1000: " + data);
	switch(commsState) {
		case STATE_PA1000_INITIALISING:
			if(responseIndex < INIT_LIST_SIZE) {
				/* Write out the next initialisation command */
				writeToPA1000(initCommandList[responseIndex++]);
			} else {
                /* Wait for power analyser to perform a measurement before completing. */
                QThread::sleep(1);
				emit initialisationComplete();
			}
			pa1000TcpCon->flush();
		break;
		
		case STATE_PA1000_GET_MEASUREMENTS:
			bool conversionSuccess;
			double value;
			list = (data.split(","));
			conversionSuccess = false;
			if(list.size() != MEASUREMENT_COUNT) {
				pa1000TcpCon->close();
			}
			for(int i = 0; i < MEASUREMENT_COUNT; i++) {
				value = list.at(i).toDouble(&conversionSuccess);
				if(!conversionSuccess) {
					pa1000TcpCon->close();
				} else {
					switch(i) {
						case PA1000_VOLTS_RMS:
							pa1000Data.voltsRMS = value;
							break;
						case PA1000_AMPS_RMS:
							pa1000Data.ampsRMS = value;
							break;
						case PA1000_WATTS_RMS:
							pa1000Data.activePower = value;
							break;
						case PA1000_FREQUENCY:
							pa1000Data.frequency = value;
							break;
						case PA1000_POWER_FACTOR:
							pa1000Data.powerFactor = value;
							break;
						case PA1000_REACTIVE_POWER:
							pa1000Data.reactivePower = value;
							break;
                        case PA1000_APPARENT_POWER:
                            pa1000Data.apparentPower = value;
                            break;
					}
				}
			}
			
			/* Let listeners know that the data has been received from the PA1000 */
			emit measurementsReady(&pa1000Data);
		break;
		
		default:
		break;
	}
}