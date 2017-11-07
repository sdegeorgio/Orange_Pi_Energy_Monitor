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
 * File:   SoftwareUpdater.cpp
 * Author: Stephan de Georgio
 * 
 * Created on 25 November 2016, 21:54
 */

/*
 * Automatic software updater from USB and network
 */

#include "SoftwareUpdater.h"
#include "DataLog.h"
#include "EnergyMonitorAppGlobal.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfoList>
#include <QStringList>
#include <QTimer>

#define COMMAND_DPKG "/usr/bin/dpkg"
#define COMMAND_DPKG_PARAM "-i"
#define COMMAND_MOUNT "mount"
#define COMMAND_MOUNT_OPTION "-o"
#define COMMAND_MOUNT_RW "remount,rw"
#define COMMAND_MOUNT_RO "remount,ro"
#define COMMAND_MOUNT_DIR "/"

#define COMMAND_RESTART_APP "/usr/bin/Energy_Monitor"
#define COMMAND_RESTART_APP_PARAM_1 "-platform"
#define COMMAND_RESTART_APP_PARAM_2 "linuxfb:fb=/dev/fb8"
#define COMMAND_RESTART_APP_PARAM_3 "-u"

SoftwareUpdater::SoftwareUpdater(QObject *parent) {
    setParent(parent);
    updaterStatus = UPDATER_IDLE;
}


SoftwareUpdater::~SoftwareUpdater() {
}

void SoftwareUpdater::slotUsbStorageConnected() {
    QDir dir(USB_STORAGE_DEVICE_MOUNT_POINT);
    QStringList extensionList("*.deb");
    QFileInfoList fileList = dir.entryInfoList(extensionList, QDir::Files, QDir::Name);
    if(!fileList.isEmpty()) {
        emit sigSoftwareAvailableUSB(fileList);
    }
}

void SoftwareUpdater::slotUsbStorageDisconnected() {
    
}

void SoftwareUpdater::slotInstallUpdate(QFileInfo file) {
    if(updaterStatus == UPDATER_IDLE){
        emit sigInstallProgress(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Setting filesystem rw..."));
        updaterStatus = UPDATER_RW;
        debToInstall = file;
        emit sigUpdateStarted();
        QString program = COMMAND_MOUNT;
        QStringList arguments;
        arguments << COMMAND_MOUNT_OPTION << COMMAND_MOUNT_RW << COMMAND_MOUNT_DIR;
        process = new QProcess(this);
        process->setProcessChannelMode(QProcess::MergedChannels);
        connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slotCommandComplete(int, QProcess::ExitStatus)));
        connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(slotStdOutDataReady()));
        process->start(program, arguments);
        process->waitForStarted();
    }
}

/**
 * Called whenever data is made available from the dpkg install process.
 */
void SoftwareUpdater::slotStdOutDataReady() {
    qDebug() << "Updater: " << process->readAll();
}

void SoftwareUpdater::setFileSystemRO() {
    QTimer::singleShot(1000, this, SLOT(slotSetFileSystemRO()));
}

void SoftwareUpdater::slotSetFileSystemRO() {
    QString program;
    QStringList arguments;
    emit sigInstallProgress(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Setting filesystem RO..."));
    updaterStatus = UPDATER_RO;
    program = COMMAND_MOUNT;
    arguments << COMMAND_MOUNT_OPTION << COMMAND_MOUNT_RO << COMMAND_MOUNT_DIR;
    process = new QProcess(this);
    process->setProcessChannelMode(QProcess::MergedChannels);
    connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slotCommandComplete(int, QProcess::ExitStatus)));
    connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(slotStdOutDataReady()));
    process->start(program, arguments);
    process->waitForStarted();
}

void SoftwareUpdater::restartApp() {
    QStringList arguments;
    /* Restart the application */
    QApplication::quit();
    arguments << COMMAND_RESTART_APP_PARAM_1 << COMMAND_RESTART_APP_PARAM_2 << COMMAND_RESTART_APP_PARAM_3;
    QProcess::startDetached(COMMAND_RESTART_APP, arguments);
}

void SoftwareUpdater::slotCommandComplete(int exitCode, QProcess::ExitStatus exitStatus) {
    QString program;
    QStringList arguments;
    /* Done with the current command */
    process->deleteLater();
        
    if(!exitStatus) {
        /* Command did not crash */
        if(exitCode) {
            /* Command returned with failure exit code */
            switch(updaterStatus) {
                case UPDATER_RW:
                    /* Setting filesystem to read only failed */
                    updaterStatus = UPDATER_IDLE;
                    emit sigUpdateComplete(false);
                    emit sigInstallProgress(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Set filesystem RW failed."));
                break;
                
                case UPDATER_RO:
                    updaterStatus = UPDATER_IDLE;
                    emit sigInstallProgress(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Set filesystem RO failed."));
                    emit sigUpdateComplete(true);
                    break;

                case UPDATER_DPKG:
                    /* Non zero exit code means dpkg failed to install the package */
                    emit sigInstallProgress(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Update failed."));
                    restartApp();
                    emit sigUpdateComplete(false);
                break;
                
                default:
                break;
            }
        } else {
            switch(updaterStatus) {
                case UPDATER_RW:
                    emit sigInstallProgress(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Installing update..."));
                    updaterStatus = UPDATER_DPKG;
                    program = COMMAND_DPKG;
                    arguments << COMMAND_DPKG_PARAM << debToInstall.absoluteFilePath();
                    process = new QProcess(this);
                    process->setProcessChannelMode(QProcess::MergedChannels);
                    connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(slotCommandComplete(int, QProcess::ExitStatus)));
                    connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(slotStdOutDataReady()));
                    process->start(program, arguments);
                    process->waitForStarted();
                break;
                
                case UPDATER_DPKG:
                    restartApp();
                    emit sigUpdateComplete(true);
                break;
                
                case UPDATER_RO:
                    /* Install completed successfully */
                    //emit sigInstallProgress(QCoreApplication::translate(TRANSLATE_CONTEXT_MAIN, "Update complete!"));
                    emit sigUpdateComplete(true);
                    updaterStatus = UPDATER_IDLE;
                break;
                
                default:
                break;
            }
        }
    }
}