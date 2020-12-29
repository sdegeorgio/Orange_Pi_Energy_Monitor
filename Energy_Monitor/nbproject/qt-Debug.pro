# This file is generated automatically. Do not edit.
# Use project properties -> Build -> Qt -> Expert -> Custom Definitions.
TEMPLATE = app
DESTDIR = dist/Debug/GNU-Linux
TARGET = Energy_Monitor
VERSION = 1.0.12
CONFIG -= debug_and_release app_bundle lib_bundle
CONFIG += debug 
PKGCONFIG +=
QT = core gui widgets network
SOURCES += DataLog.cpp DataLogServer.cpp DataLogServerThread.cpp EnergyMonitor.cpp InputControl.cpp MCP39F511Calibration.cpp MCP39F511Comms.cpp MCP39F511Interface.cpp PA1000PowerAnalyser.cpp SoftwareUpdater.cpp main.cpp
HEADERS += DataLog.h DataLogServer.h DataLogServerThread.h EnergyMonitor.h EnergyMonitorAppGlobal.h InputControl.h MCP39F511Calibration.h MCP39F511Comms.h MCP39F511Interface.h PA1000PowerAnalyser.h SoftwareUpdater.h telnet.h
FORMS +=
RESOURCES +=
TRANSLATIONS +=
OBJECTS_DIR = build/Debug/GNU-Linux
MOC_DIR = 
RCC_DIR = 
UI_DIR = 
QMAKE_CC = gcc
QMAKE_CXX = g++
DEFINES += 
INCLUDEPATH += 
LIBS += -lwiringPi -ludev  
