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
 * File:   SoftwareUpdater.h
 * Author: Stephan de Georgio
 *
 * Created on 25 November 2016, 21:54
 */

/*
 * Automatic software updater from USB and network.
 */

#ifndef SOFTWAREUPDATER_H
#define SOFTWAREUPDATER_H

#include <QFileInfoList>
#include <QList>
#include <QObject>
#include <QProcess>
#include <QUrl>

typedef enum {
    UPDATER_IDLE,
    UPDATER_RW,     /* Set file system to read/write */
    UPDATER_DPKG,   /* Install the package */
    UPDATER_RO,     /* Set the file system back to read only */
} UpdaterStatus;

class SoftwareUpdater : public QObject {
    Q_OBJECT
    
public:
    SoftwareUpdater(QObject *parent);
    virtual ~SoftwareUpdater();

signals:
    void sigSoftwareAvailableUSB(QFileInfoList fileList);
    void sigSoftwareAvailableNetwork(QList<QUrl> fileList);
    void sigInstallProgress(QString);
    void sigUpdateStarted();
    void sigUpdateComplete(bool success);
    
public slots:
    void slotUsbStorageConnected();
    void slotUsbStorageDisconnected();
    void slotInstallUpdate(QFileInfo fileInfo);
    void slotSetFileSystemRO();
    
private:
    void setFileSystemRO();
    void restartApp();
    QProcess *process;
    UpdaterStatus updaterStatus;
    QFileInfo debToInstall;
    
private slots:
    void slotCommandComplete(int, QProcess::ExitStatus);
    void slotStdOutDataReady();

};

#endif /* SOFTWAREUPDATER_H */

