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
 * File:   main.cpp
 * Author: Stephan de Georgio
 *
 * Created on 20 July 2016, 14:32
 */

#include "EnergyMonitorAppGlobal.h"
#include "EnergyMonitor.h"

extern "C" {
	#include <wiringPi.h>
};

#include <signal.h>
#include <unistd.h>

#include <QApplication>
#include <QCoreApplication>

void setShutDownSignal( int signalId );
void handleShutDownSignal( int signalId );


int main(int argc, char *argv[]) {
    /* Initialise the wiringPi library */
    wiringPiSetup();

    // initialise resources, if needed
    // Q_INIT_RESOURCE(resfile);

    QApplication app(argc, argv);
    QApplication::setApplicationName(SOFTWARE_NAME);
    QApplication::setApplicationVersion(SOFTWARE_VERSION);
    
    EnergyMonitor e;

    setShutDownSignal( SIGINT ); // shut down on ctrl-c
    setShutDownSignal( SIGTERM ); // shut down on killall
    setShutDownSignal( SIGHUP ); // shut down on reboot

    return app.exec();
}

void setShutDownSignal( int signalId )
{
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handleShutDownSignal;
    if (sigaction(signalId, &sa, NULL) == -1) {
    perror("setting up termination signal");
    exit(1);
    }
}

void handleShutDownSignal( int signalId )
{
    QApplication::exit(0);
}