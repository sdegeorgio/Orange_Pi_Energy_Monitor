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
 * File:   MCP39F511Calibration.cpp
 * Author: Stephan de Georgio
 * 
 * Created on 22 September 2016, 11:14
 */

#include "MCP39F511Calibration.h"
#include "MCP39F511Interface.h"

#include <unistd.h>

#include <QDebug>
#include <QIODevice>
#include <QtMath>
#include <QSocketNotifier>
#include <QTextStream>

MCP39F511Calibration::MCP39F511Calibration(QObject *parent, MCP39F511Interface *powerMeter) {
	setParent(parent);
	
	/* Set up the connections to the MCP39F511 power meter */
	calibrationState = CALIB_STATE_IDLE;
	mcp39F511Interface = powerMeter;
	connect(powerMeter, SIGNAL(dataReady(Mcp39F511Transaction)), this, SLOT(dataReady(Mcp39F511Transaction)));
	connect(powerMeter, SIGNAL(outputRegistersReady(McpOutputRegisters, int)), this, SLOT(mcpMeasurementsReady(McpOutputRegisters)));
	
	/* Create an instance of the Tektronix PA1000 power analyser */
	pa1000Analyer = new PA1000PowerAnalyser(this);
	connect(pa1000Analyer, SIGNAL(initialisationComplete()), this, SLOT(pa1000Connected()));
	connect(pa1000Analyer, SIGNAL(measurementsReady(PowerCalibrationData *)), this, SLOT(pa1000MeasurementsReady(PowerCalibrationData *)));
	
	/* Register a socket notifier on stdin so the user can issue interactive commands to the calibration routine. */
	QSocketNotifier *stdinSocketNotifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, this);
	connect(stdinSocketNotifier, SIGNAL(activated(int)), this, SLOT(slotOnStdinData()));
	stdinSocketNotifier->setEnabled(true);
}

MCP39F511Calibration::~MCP39F511Calibration() {
}

void MCP39F511Calibration::printMessage(QString message) {
	qDebug() << "Calibration module: " << message;
}

void MCP39F511Calibration::startCalibration(QString hostName, bool performReactiveCal) {
    performReactive = performReactiveCal;
	pa1000Hostname = hostName;
    
    /* Ensure factory reset doesn't happen when writing out calibration data! */
    mcp39F511Interface->mcpCalibReg.calibration_register_delimieter = 0x1234; /* 0x1234 is POR default */
    mcp39F511Interface->setRegister(MCP_CALIB_REG_DELIMETER, (u_int8_t *)&mcp39F511Interface->mcpCalibReg.calibration_register_delimieter, sizeof(mcp39F511Interface->mcpCalibReg.calibration_register_delimieter));
    
	/* Start by writing out accumulation, range and zero offset registers */
    
    /* Set the accumulation register to 2^7 = 128.  So all measurements averaged over 128 * 20 ms = 2.56 seconds */
    mcp39F511Interface->mcpConfigReg2.accumulation_interval_parameter = 7;
    /* Write out just the accumulation interval parameter register */
    mcp39F511Interface->setRegister(MCP_CONFIG_2_ACCUMULATION_INTERVAL, (u_int8_t*)&mcp39F511Interface->mcpConfigReg2.accumulation_interval_parameter, sizeof(mcp39F511Interface->mcpConfigReg2.accumulation_interval_parameter));

    mcp39F511Interface->mcpConfigReg1.range_voltage = 0x12;
    mcp39F511Interface->mcpConfigReg1.range_current = 0x9;
    mcp39F511Interface->mcpConfigReg1.range_power = 0x10;
    mcp39F511Interface->mcpConfigReg1.range_reserved = 0x0;
    
    mcp39F511Interface->setRegister(MCP_CONFIG_1_RANGE, (u_int8_t *)&mcp39F511Interface->mcpConfigReg1.range_voltage,                    
                                    sizeof(mcp39F511Interface->mcpConfigReg1.range_voltage) +
                                    sizeof(mcp39F511Interface->mcpConfigReg1.range_current) +
                                    sizeof(mcp39F511Interface->mcpConfigReg1.range_power) + 
                                    sizeof(mcp39F511Interface->mcpConfigReg1.range_reserved));

    /* Ensure all zero offsets are zero. */
    mcp39F511Interface->mcpCalibReg.offset_current_RMS = 0;
    mcp39F511Interface->mcpCalibReg.offset_active_power = 0;
    mcp39F511Interface->mcpCalibReg.offset_reactive_power = 0;
    mcp39F511Interface->setRegister(MCP_CALIB_OFFSET_CURRENT_RMS, (u_int8_t *)&mcp39F511Interface->mcpCalibReg.offset_current_RMS,
                                    sizeof(mcp39F511Interface->mcpCalibReg.offset_current_RMS) + 
                                    sizeof(mcp39F511Interface->mcpCalibReg.offset_active_power) + 
                                    sizeof(mcp39F511Interface->mcpCalibReg.offset_reactive_power));
    
    /* Set up some other system configuration options */
    /* Disable zero cross detect output */
    mcp39F511Interface->mcpConfigReg1.system_configuration |= MCP_SYSTEM_CONFIG_ZCD_OUTPUT_DIS;
    /* Enable temperature compensation */
    mcp39F511Interface->mcpConfigReg1.system_configuration |= MCP_SYSTEM_CONFIG_TEMPCOMP;
    /* Write out just the system config register. */
    transactionId = mcp39F511Interface->setRegister(MCP_CONFIG_1_SYSTEM_CONFIG, (u_int8_t*)&mcp39F511Interface->mcpConfigReg1.system_configuration, sizeof(mcp39F511Interface->mcpConfigReg1.system_configuration));
    calibrationState = CALIB_STATE_WAIT_FOR_LOAD_PF_ONE;
}

void MCP39F511Calibration::cancelCalibration() {
	if(calibrationState != CALIB_STATE_IDLE) {
		pa1000Analyer->disconnectFromAnalyser();
		calibrationState = CALIB_STATE_IDLE;
		printMessage("Calibration cancelled.");
	}
}

void MCP39F511Calibration::pa1000Connected() {
	calibrationState = CALIB_STATE_GET_PA1000_PF_ONE;
	pa1000Analyer->getPowerMeasurements();
}

void MCP39F511Calibration::pa1000MeasurementsReady(PowerCalibrationData *measurements) {
	pa1000Measurements = measurements;
	/* Power factor 1 measurements received */
	if(calibrationState == CALIB_STATE_GET_PA1000_PF_ONE) {
		calibrationState = CALIB_STATE_GET_MCP_MEASUREMENTS_PF_ONE;
		mcp39F511Interface->getOutputRegisters();
	}
	
	/* Power factor 0.5 measurements received */
	if(calibrationState == CALIB_STATE_GET_PA1000_PF_HALF) {
		calibrationState = CALIB_STATE_GET_MCP_MEASUREMENTS_PF_HALF;
		mcp39F511Interface->getOutputRegisters();
	}
}

void MCP39F511Calibration::dataReady(Mcp39F511Transaction transaction) {
	if(transaction.unique_id == transactionId) {
		switch(calibrationState) {
			case CALIB_STATE_IDLE:
				/* Do nothing */
			break;
			case CALIB_STATE_WAIT_FOR_LOAD_PF_ONE:
                /* Wait for user input to continue */
                //		For bypassing [y] checks
                //		calibrationState = CALIB_STATE_CONNECT_TO_PA1000;
                //		pa1000Analyer->connectToAnalyser(pa1000Hostname);
                printMessage("Please apply approximately 60W load with a power factor of 1.  For example, connect a 60W light bulb.");
                printMessage("Press [y] and enter to continue.");
                calibrationState = CALIB_STATE_WAIT_FOR_LOAD_PF_ONE;
            break;
            
			case CALIB_STATE_WRITE_FREQUENCY:
				/* Frequency written */
				printMessage("Calibrated frequency written.");
				printMessage("Calling auto calibrate frequency command...");
				calibrationState = CALIB_STATE_AUTO_CALIB_FREQUENCY;
				transactionId = mcp39F511Interface->autoCalibrateFrequency();
			break;
			
			case CALIB_STATE_AUTO_CALIB_FREQUENCY:
				printMessage("Auto calibrate frequency command complete.");
				printMessage("Writing calibrated Amps, Volts and Watts...");
				calibrationState = CALIB_STATE_WRITE_VOLTS_AMPS_WATTS_PF_ONE;
				writeMcpCalibrationData();
			break;
			
			case CALIB_STATE_WRITE_VOLTS_AMPS_WATTS_PF_ONE:
				printMessage("Active Amps, Volts and Watts written.");
				printMessage("Calling auto calibrate gain command...");
				calibrationState = CALIB_STATE_AUTO_CALIB_GAIN;
				transactionId = mcp39F511Interface->autoCalibrateGain();
			break;
			
			case CALIB_STATE_AUTO_CALIB_GAIN:
				printMessage("Auto calibration at power factor 1 complete.");
                /* Bypass the reactive calibration if reactive cal procedure is not to be run. */
                if(performReactive) {
                    printMessage("Please apply approximately 10W load with a power factor of 0.5.");
                    printMessage("Press [y] and enter to continue.");
                    //	For bypassing [y] checks
                    //	calibrationState = CALIB_STATE_GET_PA1000_PF_HALF;
                    //	pa1000Analyer->getPowerMeasurements();
                    calibrationState = CALIB_STATE_WAIT_FOR_LOAD_PF_HALF;
                } else {
                    setMcpConfiguration();
                }
			break;
			
			case CALIB_STATE_WRITE_VOLTS_AMPS_WATTS_PF_HALF:
				printMessage("Reactive Amps, Volts and Watts written.");
				printMessage("Calling auto calibrate reactive gain command...");
				calibrationState = CALIB_STATE_AUTO_CALIB_REACTIVE_GAIN;
				transactionId = mcp39F511Interface->autoCalibrateReactiveGain();
			break;
			
			case CALIB_STATE_AUTO_CALIB_REACTIVE_GAIN:
				printMessage(QString("Auto calibration at power factor %1 complete.").arg(pa1000Measurements->powerFactor));
                setMcpConfiguration();
            break;
			
			case CALIB_STATE_SET_SYSTEM_CONFIG:
                calibrationState = CALIB_STATE_READ_ALL_REGISTERS;
                transactionId = mcp39F511Interface->readAllRegisters();
			break;
			
			case CALIB_STATE_SAVE_TO_FLASH:
                /* Delay before reset to allow for flash configuration to be written */
                usleep(1000000);
                mcp39F511Interface->close();
                mcp39F511Interface->initialise();
				printMessage("Settings stored in flash and MCP39F511.  Calibration complete!");
                calibrationState = CALIB_STATE_IDLE;
                emit calibrationComplete(true);
			break;
            
            case CALIB_STATE_READ_ALL_REGISTERS:
                qDebug() << "Voltage range register: " << mcp39F511Interface->mcpConfigReg1.range_voltage;
                qDebug() << "Vurrent range register: " << mcp39F511Interface->mcpConfigReg1.range_current;
                qDebug() << "Power range register: " << mcp39F511Interface->mcpConfigReg1.range_power;
                
                qDebug() << "Calibration voltage gain register: " << mcp39F511Interface->mcpCalibReg.gain_voltage_RMS;
                qDebug() << "Calibration current gain register: " << mcp39F511Interface->mcpCalibReg.gain_current_RMS;
                qDebug() << "Calibration power gain register: " << mcp39F511Interface->mcpCalibReg.gain_active_power;
                
                calibrationState = CALIB_STATE_SAVE_TO_FLASH;
                saveSettingsToFlash();
            break;
			
			default:
			
				break;
		}
	}
}

void MCP39F511Calibration::setMcpConfiguration() {
    pa1000Analyer->disconnectFromAnalyser();
    printMessage("Setting system configuration registers...");
    calibrationState = CALIB_STATE_SET_SYSTEM_CONFIG;
    
    /* Set the accumulation register to 2^7 = 128.  So all measurements averaged over 128 * 20 ms = 2.56 seconds */
    mcp39F511Interface->mcpConfigReg2.accumulation_interval_parameter = 7;
    /* Write out just the accumulation interval parameter register */
    transactionId = mcp39F511Interface->setRegister(MCP_CONFIG_2_ACCUMULATION_INTERVAL, (u_int8_t*)&mcp39F511Interface->mcpConfigReg2.accumulation_interval_parameter, sizeof(mcp39F511Interface->mcpConfigReg2.accumulation_interval_parameter));
}

void MCP39F511Calibration::saveSettingsToFlash() {
    printMessage("Storing settings in flash...");
    /* Save registers to flash */
    transactionId = mcp39F511Interface->saveRegistersToFlash();
}

void MCP39F511Calibration::adjustRangeAndCopyPA1000(McpOutputRegisters values) {
	/* Convert PA1000 measurements to the range used by MCP39F511 */
	pa1000AdjustsedMeasurements.volts = pa1000Measurements->voltsRMS * 10;
	pa1000AdjustsedMeasurements.amps = pa1000Measurements->ampsRMS * 10000;
	pa1000AdjustsedMeasurements.activePower = pa1000Measurements->activePower * 100;
	pa1000AdjustsedMeasurements.frequency = pa1000Measurements->frequency * 1000;
	pa1000AdjustsedMeasurements.powerFactor = pa1000Measurements->powerFactor / qPow(2, -15);
	pa1000AdjustsedMeasurements.reactivePower = pa1000Measurements->reactivePower * 100;

	printMessage(QString("PA1000 Volts: %1").arg(pa1000AdjustsedMeasurements.volts));
	printMessage(QString("MCP Volts: %1").arg(values.voltage_RMS));
    printMessage(QString("MCP Volts gain register: %1").arg(mcp39F511Interface->mcpCalibReg.gain_voltage_RMS));
	printMessage(QString("PA1000 Amps: %1").arg(pa1000AdjustsedMeasurements.amps));
	printMessage(QString("MCP Amps: %1").arg(values.current_RMS));
    printMessage(QString("MCP Amps gain register: %1").arg(mcp39F511Interface->mcpCalibReg.gain_current_RMS));
	printMessage(QString("PA1000 active Watts: %1").arg(pa1000AdjustsedMeasurements.activePower));
	printMessage(QString("MCP active Watts: %1").arg(values.active_power));
    printMessage(QString("MCP active Watts gain register: %1").arg(mcp39F511Interface->mcpCalibReg.gain_active_power));
	printMessage(QString("PA1000 frequency: %1").arg(pa1000AdjustsedMeasurements.frequency));
	printMessage(QString("MCP frequency: %1").arg(values.line_frequency));
	printMessage(QString("PA1000 power factor: %1").arg(pa1000AdjustsedMeasurements.powerFactor));
	printMessage(QString("MCP power factor: %1").arg(values.power_factor));
	printMessage(QString("PA1000 reactive Watts: %1").arg(pa1000AdjustsedMeasurements.reactivePower));
	printMessage(QString("MCP reactive Watts: %1").arg(values.reactive_power));
    printMessage(QString("MCP reactive Watts gain register: %1").arg(mcp39F511Interface->mcpCalibReg.gain_reactive_power));

	/* Copy the calibrated values from the PA1000 into the structure that gets written to the MCP39F511 */
	mcp39F511Interface->mcpConfigReg1.calibration_voltage = pa1000AdjustsedMeasurements.volts;
	mcp39F511Interface->mcpConfigReg1.calibration_current = pa1000AdjustsedMeasurements.amps;
	mcp39F511Interface->mcpConfigReg1.calibration_power_active = pa1000AdjustsedMeasurements.activePower;
	mcp39F511Interface->mcpConfigReg1.line_frequency_reference = pa1000AdjustsedMeasurements.frequency;
	mcp39F511Interface->mcpConfigReg1.calibration_power_reactive = pa1000AdjustsedMeasurements.reactivePower;
}

void MCP39F511Calibration::mcpMeasurementsReady(McpOutputRegisters values) {
	if(calibrationState == CALIB_STATE_GET_MCP_MEASUREMENTS_PF_ONE) {
		adjustRangeAndCopyPA1000(values);
		/* Write out the calibrated frequency value to the MCP39F511 */
		calibrationState = CALIB_STATE_WRITE_FREQUENCY;
		printMessage("Writing calibrated frequency...");
        usleep(1000000);
		transactionId = mcp39F511Interface->setRegister(MCP_CONFIG_1_LINE_FREQ_REF, (u_int8_t *)&mcp39F511Interface->mcpConfigReg1.line_frequency_reference, sizeof(mcp39F511Interface->mcpConfigReg1.line_frequency_reference));
	}
	if(calibrationState == CALIB_STATE_GET_MCP_MEASUREMENTS_PF_HALF) {
		adjustRangeAndCopyPA1000(values);
		printMessage("Got power factor 0.5 measurements.");
		calibrationState = CALIB_STATE_WRITE_VOLTS_AMPS_WATTS_PF_HALF;
		writeMcpCalibrationData();
	}
}

void MCP39F511Calibration::writeMcpCalibrationData() {
	transactionId = mcp39F511Interface->setRegister(MCP_CONFIG_1_CALIB_CURRENT, (u_int8_t *)&mcp39F511Interface->mcpConfigReg1.calibration_current, 
						        sizeof(mcp39F511Interface->mcpConfigReg1.calibration_current) + 
						        sizeof(mcp39F511Interface->mcpConfigReg1.calibration_voltage) + 
						        sizeof(mcp39F511Interface->mcpConfigReg1.calibration_power_active) + 
			                    sizeof(mcp39F511Interface->mcpConfigReg1.calibration_power_reactive));
}

/**
 * Gets called when data is entered on stdin and enter is pressed.
 */
void MCP39F511Calibration::slotOnStdinData() {
	QTextStream stream(stdin, QIODevice::ReadOnly);
	QString inputString;
	inputString = stream.readLine();
	
	switch(calibrationState) {
		/* Waiting for input for 60 W load and power factor 1 load to be connected... */
		case CALIB_STATE_WAIT_FOR_LOAD_PF_ONE:
			if(inputString == "y") {
				/* Get the calibrated data from the PA1000 */
				calibrationState = CALIB_STATE_CONNECT_TO_PA1000;
				pa1000Analyer->connectToAnalyser(pa1000Hostname);
			}
		break;
		
		case CALIB_STATE_WAIT_FOR_LOAD_PF_HALF:
			if(inputString == "y") {
				/* Get the calibrated data from the PA1000 */
				calibrationState = CALIB_STATE_GET_PA1000_PF_HALF;
				pa1000Analyer->getPowerMeasurements();
			}
		break;
		
		default:
			/* Do nothing. */
		break;
	}
}