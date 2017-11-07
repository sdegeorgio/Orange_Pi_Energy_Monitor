# This file is generated automatically. Do not edit.
# Use project properties -> Build -> Qt -> Expert -> Custom Definitions.
TEMPLATE = app
DESTDIR = dist/Release/GNU-Linux
TARGET = Energy_Monitor
VERSION = 1.0.0
CONFIG -= debug_and_release app_bundle lib_bundle
CONFIG += release 
PKGCONFIG +=
QT = core gui widgets
SOURCES += DataLog.cpp EnergyMonitor.cpp InputControl.cpp MCP39F511Calibration.cpp MCP39F511Comms.cpp MCP39F511Interface.cpp PA1000PowerAnalyser.cpp main.cpp
HEADERS += DataLog.h EnergyMonitorAppGlobal.h EnergyMonitor.h InputControl.h MCP39F511Calibration.h MCP39F511Comms.h MCP39F511Interface.h PA1000PowerAnalyser.h
FORMS +=
RESOURCES +=
TRANSLATIONS +=
OBJECTS_DIR = build/Release/GNU-Linux
MOC_DIR = 
RCC_DIR = 
UI_DIR = 
QMAKE_CC = gcc
QMAKE_CXX = g++
DEFINES += 
INCLUDEPATH += 
LIBS += -lwiringPi  
