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
 * File:   EnergyMonitor.cpp
 * Author: Stephan de Georgio
 * 
 * Created on 21 July 2016, 16:12
 */

#include "EnergyMonitorAppGlobal.h"
#include "EnergyMonitor.h"

#include <Qt>
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QPalette>
#include <QString>
#include <QDebug>
#include <QTimer>

EnergyMonitor::EnergyMonitor(QWidget *parent) {
    setParent(parent);
    /* Pass the command line parameters */
    QCommandLineParser commandLineParser;
    commandLineParser.setApplicationDescription(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, SOFTWARE_NAME));
    commandLineParser.addHelpOption();
    commandLineParser.addVersionOption();
    QCommandLineOption initiateCalibrationOption("c", QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Initiate calibration procedure using a calibrated reference Tektronix PA1000 at <IP address>."), QCoreApplication::translate("c", "IP address"));
    commandLineParser.addOption(initiateCalibrationOption);
    
/*    QCommandLineOption initiateReactiveCalibrationOption("z", QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Initiate calibration procedure including reactive calibration using a calibrated reference Tektronix PA1000 at <IP address>."), QCoreApplication::translate("c", "IP address"));
    commandLineParser.addOption(initiateReactiveCalibrationOption);*/
    
    QCommandLineOption factoryResetMcp39F511("r", QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Reset MCP39F511 to factory settings."));
    commandLineParser.addOption(factoryResetMcp39F511);
    
    QCommandLineOption softwareUpdateComplete("u", QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Used after self updater installed new software so correct message is displayed on screen."));
    commandLineParser.addOption(softwareUpdateComplete);

    commandLineParser.process(*QApplication::instance());
    
    
	/* Create and initialise the power meter */
	powerMeter = new MCP39F511Interface(this);
    connect(powerMeter, SIGNAL(initialisationComplete()), this, SLOT(initialisationComplete()));
    connect(powerMeter, SIGNAL(measurementsReady(DecodedMeasurements)), this, SLOT(processMeasurements(DecodedMeasurements)));
	powerMeter->initialise();
    
    /* Create and initialise the data logger */
    dataLogger = new DataLog(this);
    connect(powerMeter, SIGNAL(measurementsReady(DecodedMeasurements)), dataLogger, SLOT(slotMeasurementsReady(DecodedMeasurements)));
    connect(dataLogger, SIGNAL(sigLoggingStarted()), this, SLOT(loggingStarted()));
    connect(dataLogger, SIGNAL(sigLoggingStopped()), this, SLOT(loggingStopped()));
    
    dataLogServer = new DataLogServer(this);
    dataLogServer->startServer(); 
    
	/* Create and initialise the switch inputs */
	switchInputs = new InputControl(this);
	connect(switchInputs, SIGNAL(buttonPressed(ButtonState*)), this, SLOT(buttonPressed(ButtonState*)));

    /* Create an instance of the software updater */
    softwareUpdater = new SoftwareUpdater(this);
    updateInProgress = false;
    updaterSelectedItem = 0;
    /* Ensure the filesystem is set to readonly - this is for after software upgrade and app restart */
    if(commandLineParser.isSet(softwareUpdateComplete)) {
        softwareUpdater->slotSetFileSystemRO();
    }
    listModelFiles = new QStringListModel(this);
    connect(dataLogger, SIGNAL(sigUsbStorageConnected()), softwareUpdater, SLOT(slotUsbStorageConnected()));
    connect(dataLogger, SIGNAL(sigUsbStorageDisconnected()), softwareUpdater, SLOT(slotUsbStorageDisconnected()));
    connect(softwareUpdater, SIGNAL(sigSoftwareAvailableUSB(QFileInfoList)), this, SLOT(slotSoftwareAvailableUSB(QFileInfoList)));
    connect(softwareUpdater, SIGNAL(sigSoftwareAvailableNetwork(QList<QUrl>)), this, SLOT(slotSoftwareAvailableNetwork(QList<QUrl>)));
    connect(softwareUpdater, SIGNAL(sigInstallProgress(QString)), this, SLOT(slotUpdaterProgress(QString)));
    connect(softwareUpdater, SIGNAL(sigUpdateStarted()), this, SLOT(slotUpdaterStarted()));
    connect(softwareUpdater, SIGNAL(sigUpdateComplete(bool)), this, SLOT(slotUpdaterComplete(bool)));
    connect(dataLogger, SIGNAL(sigUsbStorageDisconnected()), this, SLOT(slotUsbStorageDisconnected()));
    
    /* Wait for initialisation to complete before calibration or factory reset */
    /* Start the calibration routine if the command line switch is set */
    optionCalibrate = false;
    optionReactiveCalibrate = false;
	if(commandLineParser.isSet(initiateCalibrationOption)) {
        pa1000IpAddress = commandLineParser.value(initiateCalibrationOption);
        optionCalibrate = true;
    } /* else
	if(commandLineParser.isSet(initiateReactiveCalibrationOption)) {
        pa1000IpAddress = commandLineParser.value(initiateReactiveCalibrationOption);
        optionReactiveCalibrate = true;
    } */
    
    /* Factory reset the MCP39F511 calibration if the command line switch is set */
    optionFactoryReset = false;
    if(commandLineParser.isSet(factoryResetMcp39F511)) {
        optionFactoryReset = true;
    }
    
	/* Initialise the LCD */
	initUI();
    
    /* Initialise the USB data logger once everything else is ready */
    dataLogger->initialiseDataLog();
    loggingActive = false;
    dataLogger->startLogging();
    
    /* So that we know when the app is about to quite */
    connect(QApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()));
    shuttingDown = false;
    
    /* Start checking for the IP address and continuously read to ensure it is up to date */
    QTimer::singleShot(0, this, SLOT(displayIPAddress()));
};

EnergyMonitor::~EnergyMonitor() {
};

/**
 * Called when MCP39F511 initialisation routine completes.
 */
void EnergyMonitor::initialisationComplete() {
    qDebug("MCP39F511 system version: 0x%x", powerMeter->mcpOutputReg.system_version);
    qDebug() << QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "MCP39F511 initialisation complete.");
    if(optionFactoryReset) {
        optionFactoryReset = false;
        qDebug() << QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Applying factory reset of MCP39F511...");
        connect(powerMeter, SIGNAL(factoryResetComplete(int)), this, SLOT(slotFactoryResetComplete(int)));
        powerMeter->factoryResetMcp39F511();
    } else {
        if(optionCalibrate) {
            optionCalibrate = false;
            qDebug() << QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Starting MCP39F511 calibration routine (no reactive power calibration)...");
            startCalibration(pa1000IpAddress, false);
        } else if(optionReactiveCalibrate) {
            optionReactiveCalibrate = false;
            qDebug() << QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Starting MCP39F511 calibration routine (with reactive power calibration)...");
            startCalibration(pa1000IpAddress, true);
        }
    }
}

void EnergyMonitor::displayIPAddress() {
    /* IP address screen */
    QString ipAddresses = "IPv4 Address<BR>";
    QList<QString> addressList = dataLogServer->getLocalIPAddress(QAbstractSocket::IPv4Protocol);
    for(int i=0; i < addressList.size(); i++) {
        ipAddresses += addressList.at(i);
    }
    /* IPv6 addresses, not displayed as too big! */
//    addressList = dataLogServer->getLocalIPAddress(QAbstractSocket::IPv6Protocol);
//    for(int i=0; i < addressList.size(); i++) {
//        QString address;
//        QString interface;
//        if(addressList.at(i).lastIndexOf("%") != -1) {
//            /* Get the interface name if it is in the string */
//            interface = address.right(address.size() - address.lastIndexOf("%") - 1);
//            /* Get the address without the interface name */
//            address = addressList.at(i).left(addressList.at(i).lastIndexOf("%"));
//            ipAddresses += "IPv6: " + address.insert(16, QString("<BR>")) + "<BR>";
//
//        } else {
//            address = addressList.at(i);
//            ipAddresses += "IPv6: " + address.insert(16, QString("<BR>")) + "<BR>";
//        }
//        
//    }
    QFont font;
    font.setPointSize(10);
    labelContents[IP_ADDRESSES].setFont(font);
    labelContents[IP_ADDRESSES].setWordWrap(true);
    ipAddresses = ipAddresses.trimmed();
    labelContents[IP_ADDRESSES].setText(ipAddresses);
    
    /* Check for a new IP address periodically */
    QTimer::singleShot(IP_ADDRESS_CHECK_INTERVAL, this, SLOT(displayIPAddress()));
}

/**
 * Gets called when ctl-c is pressed on console
 */
void EnergyMonitor::aboutToQuit() {
    /* Shutdown the power meter */
    shuttingDown = true;
	powerMeter->close();
	powerMeter->deleteLater();
    
    dataLogger->stopLogging();
    dataLogger->deleteLater();
    
    dataLogServer->close();
    dataLogServer->deleteLater();
    
    widgetMain->setCurrentIndex(0);
    if(updateInProgress == true) {
        labelContents[POWER_ACTIVE].setText("Restarting");
    } else {
        labelContents[POWER_ACTIVE].setText("Stopped");
    }
    /* Two calls needed to display stopped message! */
    QApplication::processEvents();
    QApplication::processEvents();

}

void EnergyMonitor::initUI() {
	/* Turn off anti alias fonts for crisp font on b/w only display */
	QFont noAA = QApplication::font();
	noAA.setStyleStrategy(QFont::NoAntialias);
	QApplication::setFont(noAA);
	
	/* Make the background full screen and black with white text */
	QPalette palette = this->palette();
	palette.setColor(QPalette::Window, Qt::black);
	QLabel labelDummy;
	palette.setColor(labelDummy.backgroundRole(), Qt::black);
	palette.setColor(labelDummy.foregroundRole(), Qt::white);
	this->setPalette(palette);
	this->setAutoFillBackground(true);
	this->showFullScreen();
	
	/* Set the label font */
	QFont font;
	font.setPointSize(16);
	
	/* Put a grid layout manager into the background widget */
	layoutBase = new QGridLayout(this);
	layoutBase->setSpacing(0);
	layoutBase->setContentsMargins(0, 0, 0, 0);
	this->setLayout(layoutBase);
	
	widgetMain = new QStackedWidget(this);
	layoutBase->addWidget(widgetMain, 0, 0, 1, 3, Qt::AlignCenter);
    widgetMain->setMaximumSize(128, 52);
	
	for(int i = 0; i < MAX_SCREENS; i++) {
		labelContents[i].setParent(this);
		labelContents[i].setPalette(palette);
		labelContents[i].setFont(font);
		labelContents[i].setAlignment(Qt::AlignCenter);
		labelContents[i].setStyleSheet("border: 0px solid white");
		widgetMain->addWidget(&labelContents[i]);
	}
    
	font.setPointSize(6);
    labelStatus.setFont(font);
    labelStatus.setParent(this);
	labelStatus.setPalette(palette);
    labelStatus.setAlignment(Qt::AlignCenter);
    labelStatus.setStyleSheet("border: 0px solid white");
    layoutBase->addWidget(&labelStatus, 1, 1, 1, 1, Qt::AlignCenter);
    
    font.setPointSize(6);
    labelContents[SW_VERSION].setFont(font);
    QString version = SOFTWARE_VERSION;
    labelContents[SW_VERSION].setText(
                                      QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Software version\r\n") +
                                      version + "\r\n" +
                                      QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Designed by\r\n") +
                                      "Stephan de Georgio\r\n" +
                                      "License: GPLv3");
    
    /* USB upgrade screen */
    /* Widget to hold updater widgets, add to the QStackedWidget */
    updaterContainer = new QWidget(this);
    widgetMain->addWidget(updaterContainer);
    /* Layout manager to hold the widgets */
    updaterLayout = new QGridLayout();
    updaterLayout->setSpacing(0);
	updaterLayout->setContentsMargins(0, 0, 0, 0);
    updaterContainer->setLayout(updaterLayout);
    
    /* USB upgrade title */
    updaterLabelTitle.setFont(font);
    updaterLabelTitle.setText(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "USB upgrade"));
    updaterLayout->addWidget(&updaterLabelTitle, 0, 0, 1, 1, Qt::AlignCenter);
    listViewFiles = new QListView(this);
    listViewFiles->setModel(listModelFiles);
    listViewFiles->setFont(font);
    listViewFiles->setStyleSheet("background-color: black; color: white; selection-color: black; selection-background-color: white;");
    listViewFiles->setFrameStyle(0);
    listViewFiles->setSpacing(0);
    listViewFiles->setSelectionMode(QAbstractItemView::SingleSelection);
    listViewFiles->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    updaterLayout->addWidget(listViewFiles, 1, 0, 1, 1, Qt::AlignCenter);
    
};

void EnergyMonitor::loggingStarted() {
    labelStatus.setText("Logging active");
    loggingActive = true;
}

void EnergyMonitor::loggingStopped() {
    labelStatus.setText("Logging inactive");
    loggingActive = false;
}

void EnergyMonitor::processMeasurements(DecodedMeasurements energyValues){
    if(!shuttingDown) {
        QFont font;
        font.setPointSize(12);
        labelContents[POWER_ACTIVE].setText("Power\r\n" + QString::number(energyValues.powerActive, 'f', 2) + " W");
        labelContents[VOLTAGE_RMS].setText("Voltage\r\n" + QString::number(energyValues.voltageRms, 'f', 1) + " V");
        labelContents[CURRENT_RMS].setText("Current\r\n" + QString::number(energyValues.currentRms, 'f', 4) + " A");
        labelContents[FREQUENCY].setText("Frequency\r\n" + QString::number(energyValues.frequency, 'f', 2) + " Hz");
        labelContents[POWER_FACTOR].setText("Pwr factor\r\n" + QString::number(energyValues.powerFactor, 'f', 2) + "");
        /* Removed as we have no way to calibrate reactive power */
        /*labelContents[POWER_REACTIVE].setText("Reactive Pwr\r\n" + QString::number(energyValues.powerReactive, 'f', 2) + " W");
        labelContents[POWER_REACTIVE].setFont(font);*/
        labelContents[POWER_APPARENT].setText("Apparent Pwr\r\n" + QString::number(energyValues.powerApparent, 'f', 2) + " W");
        labelContents[POWER_APPARENT].setFont(font);

        /*if(!optionCalibrate) {*/
            /* Instigate the next set of after a delay */
            QTimer::singleShot(DISPLAY_UPDATE_INTERVAL, this, SLOT(startMeasurements()));
        /*}*/
    }
};

void EnergyMonitor::startMeasurements() {
    powerMeter->getOutputRegisters();
}

void EnergyMonitor::buttonPressed(ButtonState *button) {
	if(button->changedState) {
		if(button->pressed) {
			if(button->switchUID == BUTTON_RIGHT) {
				int index = widgetMain->currentIndex();
				if(index < widgetMain->count()) {
					widgetMain->setCurrentIndex(++index);
				}
			}
			if(button->switchUID == BUTTON_LEFT) {
				int index = widgetMain->currentIndex();
				if(index > 0) {
					widgetMain->setCurrentIndex(--index);
				}
			}
            if(button->switchUID == BUTTON_UP) {
                /* Move up the USB upgrade list selection */
				if(widgetMain->currentIndex() == USB_UPGRADE) {
                    if(updaterSelectedItem > 0) {
                        updaterSelectedItem--;
                    }
                    QModelIndex index = listModelFiles->index(updaterSelectedItem, 0);
                    listViewFiles->setCurrentIndex(index);
                }
			}
            if(button->switchUID == BUTTON_DOWN) {
                /* Move down the USB upgrade list selection */
				if(widgetMain->currentIndex() == USB_UPGRADE) {
                    if(updaterSelectedItem < listModelFiles->rowCount() - 1) {
                        updaterSelectedItem++;
                    }
                    QModelIndex index = listModelFiles->index(updaterSelectedItem, 0);
                    listViewFiles->setCurrentIndex(index);
                }
			}
            if(button->switchUID == BUTTON_SELECT) {
                /* Select button starts USB upgrade if upgrade screen in view */
                if(widgetMain->currentIndex() == USB_UPGRADE) {
                    /* Check there is not already an update in progress and an item is selected in the list */
                    if(!updateInProgress && updaterFileList.size() > 0) {
                        updateInProgress = true;
                        /* Start the USB upgrade based on the selected item */
                        softwareUpdater->slotInstallUpdate(updaterFileList.at(updaterSelectedItem));
                    }
                } else 
                /* All other screens the enter button starts / stops logging */    
                if(loggingActive) {
                    dataLogger->stopLogging(); 
                } else {
                    dataLogger->startLogging(); 
                }
			}
			/* powerMeter->beep_on(true); */
		} else {
			/* powerMeter->beep_on(false); */
		}
	}
}

void EnergyMonitor::startCalibration(QString pa1000IpAddress, bool reactiveCal) {
	powerCalibration = new MCP39F511Calibration(this, powerMeter);
	connect(powerCalibration, SIGNAL(calibrationComplete(bool)), this, SLOT(slotCalibrationComplete(bool)));
	powerCalibration->startCalibration(pa1000IpAddress, reactiveCal);
}

void EnergyMonitor::cancelCalibration() {
	powerCalibration->cancelCalibration();
	disconnect(powerCalibration, SIGNAL(calibrationComplete(bool)), this, SLOT(slotCalibrationComplete(bool)));
	powerCalibration->deleteLater();
}

void EnergyMonitor::slotCalibrationComplete(bool success) {
    optionCalibrate = false;
    /* Instigate the next set of after a delay */
    QTimer::singleShot(DISPLAY_UPDATE_INTERVAL, this, SLOT(startMeasurements()));
}

/**
 * Gets called when factory reset of MCP39F511 is complete.
 */
void EnergyMonitor::slotFactoryResetComplete(int transactionId) {
    qDebug() << QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Factory reset of MCP39F511 complete. You must now power cycle the unit.");
    disconnect(powerMeter, SIGNAL(factoryResetComplete(int)), this, SLOT(slotFactoryResetComplete(int)));
}

void EnergyMonitor::slotSoftwareAvailableUSB(QFileInfoList fileList) {
    updaterFileList = fileList;
    QStringList list;
    listModelFiles->removeRows(0, listModelFiles->rowCount());
    for(int i = 0; i < updaterFileList.length(); i++) {
        list.append(updaterFileList.at(i).fileName());
    }
    if(list.size()) {
        /* Load the list into the list view and select the first item */
        listModelFiles->setStringList(list);
        QModelIndex index = listModelFiles->index(0, 0);
        listViewFiles->setCurrentIndex(index);
        updaterSelectedItem = 0;
    }
}

void EnergyMonitor::slotSoftwareAvailableNetwork(QList<QUrl> urlList) {
    qDebug() << "FILES FOUND: " << urlList;
}

void EnergyMonitor::slotUpdaterProgress(QString status) {
    qDebug() << status;
}

void EnergyMonitor::slotUpdaterStarted() {
    labelStatus.setText(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Installing update..."));
}

void EnergyMonitor::slotUpdaterComplete(bool success) {
    if(success) {
        labelStatus.setText(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Update successful!"));
    } else {
        updateInProgress = false;
        labelStatus.setText(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Update failed."));
    }
}

void EnergyMonitor::slotUsbStorageDisconnected() {
    /* Clear the software update list */
    QStringList list;
    listModelFiles->setStringList(list);
    updaterFileList.clear();
}
