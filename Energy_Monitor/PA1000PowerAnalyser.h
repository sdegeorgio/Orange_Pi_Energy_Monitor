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
 * File:   PA1000PowerAnalyser.h
 * Author: Stephan de Georgio
 *
 * Created on 26 September 2016, 09:24
 */

#ifndef PA1000POWERANALYSER_H
#define PA1000POWERANALYSER_H

#include <QObject>
#include <QString>
#include <QTcpSocket>

#define SCPI_PORT 5025

typedef struct {
	double voltsRMS;
	double ampsRMS;
	double activePower;
	double frequency;
	double powerFactor;
	double reactivePower;
    double apparentPower;
} PowerCalibrationData;

typedef enum {
	PA1000_VOLTS_RMS,
	PA1000_AMPS_RMS,
	PA1000_WATTS_RMS,
	PA1000_FREQUENCY,
	PA1000_POWER_FACTOR,
	PA1000_REACTIVE_POWER,
    PA1000_APPARENT_POWER
} PowerDataOrder;

typedef enum {
	STATE_PA1000_IDLE,
	STATE_PA1000_INITIALISING,
	STATE_PA1000_GET_MEASUREMENTS
} Pa1000CommsState;

class PA1000PowerAnalyser : public QObject {
	Q_OBJECT
public:
	PA1000PowerAnalyser(QObject *parent);
	virtual ~PA1000PowerAnalyser();
	
	void connectToAnalyser(QString ipAddress);
	void disconnectFromAnalyser();
	void getPowerMeasurements();
	
signals:
	void initialisationComplete();
	void measurementsReady(PowerCalibrationData *);
	
private:
	void printMessage(QString);
	void writeToPA1000(QString command);
	
	QTcpSocket *pa1000TcpCon;
	int responseIndex;
	QString pa1000Host;
	Pa1000CommsState commsState;
	PowerCalibrationData pa1000Data;
	
private slots:
	void slotHostFound();
	void slotConnected();
	void slotDisconnected();
	void slotError(QAbstractSocket::SocketError);
	void slotStateChanged(QAbstractSocket::SocketState);
	void slotDataReady();
};

#endif /* PA1000POWERANALYSER_H */

