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
 * File:   DataLog.h
 * Author: Stephan de Georgio
 *
 * Created on 04 October 2016, 16:20
 */

#ifndef DATALOG_H
#define DATALOG_H

/* This is the device that will be mounted by default to store the logs */
#define DEFAULT_MOUNT_DEVICE "/dev/sda1"
#define USB_STORAGE_DEVICE_MOUNT_POINT "/tmp/usblog"


#include "MCP39F511Interface.h"

#include <libudev.h>

#include <QObject>

class DataLog : public QObject {
    Q_OBJECT
public:
    DataLog(QObject *parent);
    virtual ~DataLog();
    
    void initialiseDataLog();
    long double getStorageAvailable();
    
signals:
    void sigUsbStorageConnected();
    void sigUsbStorageDisconnected();
    void sigUsbStorageFull();
    void sigLoggingStarted();
    void sigLoggingStopped();

public slots:
    void slotMeasurementsReady(DecodedMeasurements);
    void startLogging();
    void stopLogging();
        
private:
	void printMessage(QString);
	bool mountStorageDevice(QString device, QString mountPoint);
    bool umountStorageDevice(QString device);
    bool isMounted(QString mountPoint);
	QString constructLogFilePath();
    
	bool loggingActive;
    bool newFile;
    QString currentFilePath;
	udev_monitor *udevMonitor;
	int udevMonitorFileDescriptor;

private slots:
	void timerEvent(QTimerEvent *event);
};

#endif /* DATALOG_H */

