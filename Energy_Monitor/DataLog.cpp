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
 * File:   DataLog.cpp
 * Author: Stephan de Georgio
 * 
 * Created on 04 October 2016, 16:20
 */

#include <cerrno>
#include <mntent.h>
#include <sys/mount.h>
#include <sys/statvfs.h>
#include <sys/time.h>

#include <Qt>
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>

#include "EnergyMonitorAppGlobal.h"
#include "DataLog.h"

/* Period in milliseconds to poll UDev for connected USB stick */
#define UDEV_POLL_PERIOD 200
#define UDEV_DEVICE_ADD "add"
#define UDEV_DEVICE_REMOVE "remove"
#define UDEV_DEVICE_DEV_TYPE "partition"

#define USB_STORAGE_DEVICE_FILESYSTEM_TYPE "vfat"
/* Minimum required storage space for logging to be enabled, in bytes */
#define USB_STORAGE_MINIMUM_SPACE 1000000

DataLog::DataLog(QObject *parent) {
	setParent(parent);

}

DataLog::~DataLog() {
}

void DataLog::initialiseDataLog() {
    struct udev *udev;
	/* Create the udev object */
	udev = udev_new();
	if (!udev) {
		printMessage(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Can't create udev\n"));
		QApplication::exit(0);
	}
	/* Set up a monitor to monitor block devices */
	udevMonitor = udev_monitor_new_from_netlink(udev, "udev");
	udev_monitor_filter_add_match_subsystem_devtype(udevMonitor, "block", NULL);
	udev_monitor_enable_receiving(udevMonitor);
	/* Get the file descriptor (fd) for the monitor.
	   This fd will get passed to select() */
	udevMonitorFileDescriptor = udev_monitor_get_fd(udevMonitor);
	
	loggingActive = false;
    newFile = true;
    
    if(!QDir(USB_STORAGE_DEVICE_MOUNT_POINT).exists()) {
        QDir().mkdir(USB_STORAGE_DEVICE_MOUNT_POINT);
    }
    
	/* Start the udev polling timer */
	this->startTimer(UDEV_POLL_PERIOD, Qt::CoarseTimer);
}

void DataLog::printMessage(QString message) {
	qDebug() << "Data logger: " << message;
}

void DataLog::startLogging() {
    /* Only start logging if USB stick is mounted */
    if(isMounted(USB_STORAGE_DEVICE_MOUNT_POINT)) {
        loggingActive = true;
        newFile = true;
        printMessage(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Logging started."));
        emit sigLoggingStarted();
    } else {
        /* Try and mount the USB storage device in case it is already connected */
        printMessage(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Attempting to mount the USB storage device..."));
        if(mountStorageDevice(DEFAULT_MOUNT_DEVICE, USB_STORAGE_DEVICE_MOUNT_POINT)) {
            loggingActive = true;
            newFile = true;
            printMessage(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Logging started."));
            emit sigLoggingStarted();
        } else {
            loggingActive = false;
            emit sigLoggingStopped();
        }
    }
}

void DataLog::stopLogging() {
    if(loggingActive) {
        printMessage(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Logging stopped."));
        umountStorageDevice(USB_STORAGE_DEVICE_MOUNT_POINT);
        loggingActive = false;
        newFile = true;
        emit sigLoggingStopped();
    }
}

QString DataLog::constructLogFilePath() {
    /* Get the current time and date */
    QDateTime currentTime = QDateTime::currentDateTime();
    QString filePath(USB_STORAGE_DEVICE_MOUNT_POINT);
    filePath += "/" + currentTime.toString("dd-MM-yyyy hh-mm-ss");
    filePath +=  " - Energy Monitor log.csv";
    return filePath;
}

void DataLog::slotMeasurementsReady(DecodedMeasurements values) {
    bool addHeader = false;
    long double storageAvailable = getStorageAvailable();
    /* Only append data to the log file if logging is enabled and there is available storage */
	if(loggingActive == true) {
        if(storageAvailable > USB_STORAGE_MINIMUM_SPACE) {
            QFile file(currentFilePath);
            if(newFile) {
                currentFilePath = constructLogFilePath();
                newFile = false;
                file.setFileName(currentFilePath);
                if(!file.exists()) {
                    printMessage(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "New log file created: ") + currentFilePath);
                    addHeader = true;
                } else {
                    printMessage(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Logging to existing file: ") + currentFilePath);
                }
            }
            if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
                printMessage(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Unable to open log file %1 for writing.").arg(currentFilePath));
            } else {
                QTextStream out(&file);
                /* Check if new file created and add headers. */
                if(addHeader) {
                    out << "Time,"
                        << "Active Power,"
                        << "RMS Voltage,"
                        << "RMS Current,"
                        << "Line Frequency,"
                        << "Power Factor,"
                        << "Apparent Power,"
                        << "Reactive Power"
                        << "\n";
                }
                QString format = QString("yyyy-MM-dd hh:mm:ss.zzz");
                QString currentTimeString = QDateTime::currentDateTime().toString(format);
                out << currentTimeString << ","
                    << values.powerActive << ","
                    << values.voltageRms << ","
                    << values.currentRms << ","
                    << values.frequency << ","
                    << values.powerFactor << ","
                    << values.powerApparent << ","
                    << values.powerReactive
                    << "\n";

                // optional, as QFile destructor will already do it:
                file.close();
            }
        } else {
            /* Print remaining storage in Megabytes */
            double storageAvailableMB = storageAvailable / 1000000;
            printMessage(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Storage space exhausted! Available space on %1: %2 MB remaining.").arg(USB_STORAGE_DEVICE_MOUNT_POINT).arg(storageAvailableMB));
            /* Out of storage so stop logging */
            stopLogging();
        }
	}
}

/**
 * Overridden QObject timerEvent method for servicing the USB storage device mount / unmount process.
 * @param QTimerEvent data.
 */
void DataLog::timerEvent(QTimerEvent *event) {
	fd_set fds;
	struct timeval tv;
	struct udev_device *udevDevice;
	int ret;
   

	/* Assign the file descriptor to udev */
	FD_ZERO(&fds);
	FD_SET(udevMonitorFileDescriptor, &fds);
	/* Set the timeout to 0 so accessing the descriptor never blocks */
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	/* Try to open the udev file descriptor */
	ret = select(udevMonitorFileDescriptor+1, &fds, NULL, NULL, &tv);
	
	/* Check if the file descriptor opened successfully */
	if (ret > 0 && FD_ISSET(udevMonitorFileDescriptor, &fds)) {
		/* Make the call to receive the device.
		   select() ensured that this will not block. */
		udevDevice = udev_monitor_receive_device(udevMonitor);
		if (udevDevice) {
			/* Check the device connected / disconnected is the default mount device
			   and that it is a partition (and not a device)
			 */
			if(!strcmp(udev_device_get_devnode(udevDevice), DEFAULT_MOUNT_DEVICE) &&
			   !strcmp(udev_device_get_devtype(udevDevice), UDEV_DEVICE_DEV_TYPE)) {
				
                if(!strcmp(udev_device_get_action(udevDevice), UDEV_DEVICE_ADD)) {
                    printMessage(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "USB storage device connected: ") + DEFAULT_MOUNT_DEVICE);
                    startLogging();
                }
                if(!strcmp(udev_device_get_action(udevDevice), UDEV_DEVICE_REMOVE)) {
                    printMessage(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "USB storage device removed: ") + DEFAULT_MOUNT_DEVICE);
                    /* Unmount the USB storage device */
                    stopLogging();
                }
				
			}
            udev_device_unref(udevDevice);
		}				
	}
}

/**
 * @param device USB storage device to be mounted.
 * @param mountPoint mount point that USB storage device is to be mounted on.
 * @return true if mount successful, false if mount failed.
 */
bool DataLog::mountStorageDevice(QString device, QString mountPoint) {
    if(mount(device.toLocal8Bit(), mountPoint.toLocal8Bit(), USB_STORAGE_DEVICE_FILESYSTEM_TYPE, MS_NOATIME, NULL)) {
        if(errno == EBUSY) {
            printMessage(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Mount point busy: ") + mountPoint);
        } else {
            printMessage(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Mount error: %1").arg(strerror(errno)));
        }
        return false;
    } else {
        printMessage(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Successfully mounted USB storage device %1 on %2").arg(device).arg(mountPoint));
        emit sigUsbStorageConnected();
        return true;
    }
}

/**
 * Unmount a USB storage device from the specified mount point.
 * @param mountPoint mount point that USB storage device is mounted on.
 * @return true if unmount successful, false if unmount failed.
 */
bool DataLog::umountStorageDevice(QString mountPoint) {
    if(umount2(mountPoint.toLocal8Bit(), MNT_FORCE)) {
        printMessage(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Unmount error: %1").arg(strerror(errno)));
        return false;
    } else {
        printMessage(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Successfully unmounted USB storage device from ") + mountPoint);
        emit sigUsbStorageDisconnected();
        return true;
    }
}

/**
 * Gets the storage space available on the USB storage device.
 * @return 
 */
long double DataLog::getStorageAvailable() {
    struct statvfs filesystemStat;
    long double storageAvailableBytes;
    
    if(isMounted(USB_STORAGE_DEVICE_MOUNT_POINT)) {
        if(!statvfs(USB_STORAGE_DEVICE_MOUNT_POINT, &filesystemStat)) {
            storageAvailableBytes = (long double)filesystemStat.f_bsize * filesystemStat.f_bavail;
            if(storageAvailableBytes) {
                return storageAvailableBytes;
            }
        } else {
            printMessage(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Unable to determine remaining space on USB storage device."));
            
        }
    }
    return 0;
}

/**
 * Check if anything is mounted at a particular directory.
 * @param mountPoint Mount point to check
 * @return true if a drive is mounted on the specified directory, false if not.
 */
bool DataLog::isMounted(QString mountPoint) {
    struct mntent *mountInfo;
    /* Open the mtab file for reading */
    FILE *mtabFile = setmntent("/etc/mtab", "r");
    /* Get an mtab entry */
    mountInfo = getmntent(mtabFile);
    while(mountInfo) {
        if(mountPoint == mountInfo->mnt_dir) {
            endmntent(mtabFile);
            return true;
        }
        /* Get the next mtab entry */
        mountInfo = getmntent(mtabFile);
    }
    /* Close the file descriptor */
    endmntent(mtabFile);
    return false;
}