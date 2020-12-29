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
 * File:   EnergyMonitor.h
 * Author: Stephan de Georgio
 *
 * Created on 21 July 2016, 16:12
 */

#ifndef ENERGYMONITOR_H
#define ENERGYMONITOR_H

/* Display and log update interval in milliseconds.  The minimum this can be is 200 ms */
#define DISPLAY_UPDATE_INTERVAL 1000
#define DISPLAY_INITIALISING 2000

/* Check interval to update the IP address screen */
#define IP_ADDRESS_CHECK_INTERVAL 1000

#include <QFont>
#include <QGridLayout>
#include <QLabel>
#include <QListView>
#include <QStringListModel>
#include <QStackedWidget>
#include <QTimer>
#include <QWidget>

#include "MCP39F511Interface.h"
#include "InputControl.h"
#include "MCP39F511Calibration.h"
#include "DataLog.h"
#include "DataLogServer.h"
#include "SoftwareUpdater.h"

typedef enum {
	POWER_ACTIVE,
	VOLTAGE_RMS,
	CURRENT_RMS,
	FREQUENCY,
	POWER_FACTOR,
//	POWER_REACTIVE, /* Removed as we have no way to calibrate reactive power */
	POWER_APPARENT,
    IP_ADDRESSES,
    SW_VERSION,
	MAX_SCREENS	
} Measurements;

#define USB_UPGRADE MAX_SCREENS

class EnergyMonitor : public QWidget {
	Q_OBJECT
	public:
		EnergyMonitor(QWidget *parent);
		~EnergyMonitor();
                MCP39F511Interface *powerMeter;
	
	private:
		void initUI();
		void startCalibration(QString pa1000IpAddress, bool reactiveCal);
		void cancelCalibration();
        void selectStart();
        void selectStop();
		
		QGridLayout *layoutBase;
		QStackedWidget *widgetMain;
		QFont *fontMain;
        QFrame *statusBar;
		MCP39F511Calibration *powerCalibration;
        DataLog *dataLogger;
        DataLogServer *dataLogServer;
		InputControl *switchInputs;
		
		QLabel labelContents[MAX_SCREENS];
        
        QGridLayout *layoutStatus;
        QLabel labelStatus;
        
        bool loggingActive;
        bool optionCalibrate;
        bool optionReactiveCalibrate;
        QString pa1000IpAddress;
        bool optionFactoryReset;
        bool shuttingDown;
		
        /* Software updater */
        SoftwareUpdater *softwareUpdater;
        QWidget *updaterContainer;
        QGridLayout *updaterLayout;
        QStringListModel *listModelFiles;
        QListView *listViewFiles;
        QFileInfoList updaterFileList;
        int updaterSelectedItem;
        QLabel updaterLabelTitle;
        bool updateInProgress;
        
	private slots:
		void processMeasurements(DecodedMeasurements);
		void buttonPressed(ButtonState *button);
		void slotCalibrationComplete(bool);
        void slotFactoryResetComplete(int transactionId);
        void aboutToQuit();
        void displayIPAddress();
        void loggingStarted();
        void loggingStopped();
        void startMeasurements();
        void initialisationComplete();
        void slotSoftwareAvailableUSB(QFileInfoList fileList);
        void slotSoftwareAvailableNetwork(QList<QUrl> urlList);
        void slotUpdaterProgress(QString status);
        void slotUpdaterStarted();
        void slotUpdaterComplete(bool success);
        void slotUsbStorageDisconnected();
};


#endif /* ENERGYMONITOR_H */

