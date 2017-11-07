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
 * File:   MCP39F511Interface.cpp
 * Author: Stephan de Georgio
 * 
 * Created on 23 September 2016, 11:12
 */

#include "MCP39F511Interface.h"

#include <unistd.h>

#include <QDebug>
#include <QtMath>

/* The number of samples that have to be over the threshold before the measurements are displayed / logged */
#define NOISE_FILTER_SAMPLES 2
#define POWER_ACTIVE_THRESHOLD 0.05
#define POWER_REACTIVE_THRESHOLD 0.05

/* PWM module
 * PWM Frequency = 1 / (Ptimer * 2 * prescaler * PWM register)
 * PWM period register = 1 / (1/16000000) * 2 * PWM_REG_PRESCALE * PWM frequency)
 */
#define MCP39F511_CLOCK_FREQUENCY 16000000 /* 16 MHz */
#define MCP_PWM_REG_PRESCALER 0x02 /* 0x02 = prescale divide of 16 */
#define MCP_PWM_PRESCALER 16


/* Sounder parameters - frequency in Hz and duty cylce in % */
#define MCP_SOUNDER_FREQUENCY 4000
#define MCP_SOUNDER_PWM_DUTY_CYLCE 248


MCP39F511Interface::MCP39F511Interface(QObject *parent) {
	setParent(parent);
}

MCP39F511Interface::~MCP39F511Interface() {
}

/*
 * Initialise the MCP39F511 energy monitor
 */
bool MCP39F511Interface::initialise() {
	mcp_comms = new MCP39F511Comms(this);
	/* Ensure we are notified when a transaction has completed */
	connect(mcp_comms, SIGNAL(transactionComplete(Mcp39F511Transaction)), this, SLOT(transactionComplete(Mcp39F511Transaction)));
	if(!mcp_comms->initialise()) {
		return false;
	}
	
    outputTransactionId = 0;
	energyCounterTransactionId = 0;
	recordTransactionId = 0;
	calibrationTransactionId = 0;
	config1TransactionId = 0;
	config2TransactionId = 0;
	compPeriphTransactionId = 0;
    factoryResetId = 0;
    readAllRegistersId = 0;
    
    /* Setup PWM first so readAllRegisters() doesn't overwrite
       registers that have been configured whilst its in the queue
       to be sent to the chip. */
    setupPWM(MCP_SOUNDER_FREQUENCY, MCP_SOUNDER_PWM_DUTY_CYLCE);
	readAllRegistersId = readAllRegisters();
    initialisationInProgress = true;
	return true;
}

/**
 * Close the MCP39F511 serial port.
 * initialise() must be called again regardless of return state.
 * @return Returns TRUE if OK, otherwise FALSE.
 */
bool MCP39F511Interface::close() {
	bool close_state;
	close_state = mcp_comms->close();
	mcp_comms->deleteLater();
	return close_state;
}

void MCP39F511Interface::resetMCP39F511() {
    mcp_comms->resetMCP39F511();
}

/*
 * Beep on
 */
void MCP39F511Interface::beep_on(bool enabled) {
	if(enabled) {
		/* Ensure sounder is on */
		mcpCompPeriphReg.PWM_control = MCP_PWM_CONTROL_PWM_CNTRL;
		mcp_comms->enqueTransaction(MCP_COMP_PERIPH_REG_PWM_CONTROL, (u_int8_t*) &mcpCompPeriphReg.PWM_control, sizeof(mcpCompPeriphReg.PWM_control), MCP_CMD_REGISTER_WRITE);
	} else {
		/* Ensure sounder is off */
		mcpCompPeriphReg.PWM_control = 0;
		mcp_comms->enqueTransaction(MCP_COMP_PERIPH_REG_PWM_CONTROL, (u_int8_t*) &mcpCompPeriphReg.PWM_control, sizeof(mcpCompPeriphReg.PWM_control), MCP_CMD_REGISTER_WRITE);
	}
}

void MCP39F511Interface::setupPWM(int frequency, int duty_cycle) {
	int calculated_duty_cycle;
	double calculated_period;
	
	calculated_period = 2 * MCP_PWM_PRESCALER * frequency;
	calculated_period = (1 /(double)MCP39F511_CLOCK_FREQUENCY) * calculated_period;
	calculated_period = 1 / calculated_period;
	/* Apply the period part */
	mcpCompPeriphReg.PWM_period = ((int)calculated_period & MCP_PWM_PERIOD_PWM_P_MASK) << MCP_PWM_PERIOD_PWM_P_POS;
	/* Apply the prescaler part */
	mcpCompPeriphReg.PWM_period |= (MCP_PWM_REG_PRESCALER & MCP_PWM_PERIOD_PRE_MASK) << MCP_PWM_PERIOD_PRE_POS;
	
	/* Apply the duty cycle */
	calculated_duty_cycle = duty_cycle;
	mcpCompPeriphReg.PWM_duty_cycle = (calculated_duty_cycle & MCP_PWM_DUTY_UPPER_MASK) << MCP_PWM_DUTY_UPPER_POS; //duty_cycle * 4 * frequency;
	mcpCompPeriphReg.PWM_duty_cycle |= (calculated_duty_cycle & MCP_PWM_DUTY_LOWER_MASK) << MCP_PWM_DUTY_LOWER_POS;

	mcp_comms->enqueTransaction(MCP_COMP_PERIPH_REG_PWM_PERIOD, (u_int8_t*) &mcpCompPeriphReg.PWM_period, sizeof(mcpCompPeriphReg.PWM_period) + sizeof(mcpCompPeriphReg.PWM_duty_cycle), MCP_CMD_REGISTER_WRITE);
    
	/* Ensure sounder is off */
	beep_on(false);
}

int MCP39F511Interface::readAllRegisters() {
    if(readAllRegistersId) {
        return readAllRegistersId;
    } else {
        getOutputRegisters();
        getEnergyCounterRegisters();
        getRecordRegisters();
        getCalibrationRegisters();
        getDesignConfig1Registers();
        getDesignConfig2Registers();
        readAllRegistersId = getCompPeriphRegisters();
        return readAllRegistersId;
    }
}

int MCP39F511Interface::getOutputRegisters() {
	if(outputTransactionId) {
		return outputTransactionId;
	} else {
		outputTransactionId = getRegister(MCP_OUTPUT_REGISTERS_START, (u_int8_t *)&mcpOutputReg, MCP_OUTPUT_REGISTERS_SIZE);
		return outputTransactionId;
	}
}

/**
 * Get energy counter registers
 * @return Unique transaction ID.
 */
int MCP39F511Interface::getEnergyCounterRegisters() {
	if(energyCounterTransactionId) {
		return energyCounterTransactionId;
	} else {
		energyCounterTransactionId = getRegister(MCP_ENERGY_COUNTER_REGISTERS_START, (u_int8_t *)&mcpEnergyCounterReg, MCP_ENERGY_COUNTER_REGISTERS_SIZE);
		return energyCounterTransactionId;
	}
}
	
/**
 * Get min / max record registers
 * @return Unique transaction ID.
 */
int MCP39F511Interface::getRecordRegisters() {
	if(recordTransactionId) {
		return recordTransactionId;
	} else {
		recordTransactionId = getRegister(MCP_RECORD_REGISTERS_START, (u_int8_t *)&mcpRecordReg, MCP_RECORD_REGISTERS_SIZE);
		return recordTransactionId;
	}
}

/**
 * Get calibration registers
 * @return Unique transaction ID.
 */
int MCP39F511Interface::getCalibrationRegisters() {
	if(calibrationTransactionId) {
		return calibrationTransactionId;
	} else {
		calibrationTransactionId = getRegister(MCP_CALIBRATION_REGISTERS_START, (u_int8_t *)&mcpCalibReg, MCP_CALIBRATION_REGISTERS_SIZE);
		return calibrationTransactionId;
	}
}

/**
 * Get first bank of design configuration registers
 * @return Unique transaction ID.
 */
int MCP39F511Interface::getDesignConfig1Registers() {
	if(config1TransactionId) {
		return config1TransactionId;
	} else {
		config1TransactionId = getRegister(MCP_CONFIG_REGISTERS_1_START, (u_int8_t *)&mcpConfigReg1, MCP_CONFIG_REGISTERS_1_SIZE);
		return config1TransactionId;
	}
}

/**
 * Get second bank of design configuration registers
 * @return Unique transaction ID.
 */
int MCP39F511Interface::getDesignConfig2Registers() {
	if(config2TransactionId) {
		return config2TransactionId;
	} else {
		config2TransactionId = getRegister(MCP_CONFIG_REGISTERS_2_START, (u_int8_t *)&mcpConfigReg2, MCP_CONFIG_REGISTERS_2_SIZE);
		return config2TransactionId;
	}
}

/**
 * Get temperature compensation and peripheral control registers
 * @return Unique transaction ID.
 */
int MCP39F511Interface::getCompPeriphRegisters() {
	if(compPeriphTransactionId) {
		return compPeriphTransactionId;
	} else {
		compPeriphTransactionId = getRegister(MCP_COMP_PERIPH_REGISTERS_START, (u_int8_t *)&mcpCompPeriphReg, MCP_COMP_PERIPH_REGISTERS_SIZE);
		return compPeriphTransactionId;
	}
}


/* Set a register and return a unique ID for the transaction */
int MCP39F511Interface::setRegister(u_int16_t address, u_int8_t *data, u_int8_t length) {
	return mcp_comms->enqueTransaction(address, data, length, MCP_CMD_REGISTER_WRITE);
}

/* Get a register and return a unique ID for the transaction */
int MCP39F511Interface::getRegister(u_int16_t address, u_int8_t *data, u_int8_t length) {
	return mcp_comms->enqueTransaction(address, data, length, MCP_CMD_REGISTER_READ);
}

/* Save flash registers */
int MCP39F511Interface::saveRegistersToFlash() {
	return mcp_comms->sendCommand(MCP_CMD_SAVE_REGISTERS_TO_FLASH);
}

/* Read a page of EEPROM */
int MCP39F511Interface::eepromReadPage(u_int8_t page) {
	eepromBuffer[0] = page;
	return mcp_comms->enqueTransaction(0, eepromBuffer, 1, MCP_CMD_PAGE_READ_EEPROM);
}

/* Write a page of EEPROM */
int MCP39F511Interface::eepromWritePage(u_int8_t page, u_int8_t *data) {
	eepromBuffer[0] = page;
	for(int i = 1; i < MCP_EEPROM_PAGE_SIZE + 1; i++) {
		eepromBuffer[i] = data[i - 1];
	}
	return mcp_comms->enqueTransaction(0, eepromBuffer, MCP_EEPROM_PAGE_SIZE + 1, MCP_CMD_PAGE_WRITE_EEPROM);
}

/* Erase the entire contents of EEPROM */
int MCP39F511Interface::eepromBulkErase() {
	return mcp_comms->sendCommand(MCP_CMD_BULK_ERASE_EEPROM);
}

/* Auto calibrate gain */
int MCP39F511Interface::autoCalibrateGain() {
	return mcp_comms->sendCommand(MCP_CMD_AUTO_CALIBRATE_GAIN);
}

/* Auto calibrate gain */
int MCP39F511Interface::autoCalibrateReactiveGain() {
	return mcp_comms->sendCommand(MCP_CMD_AUTO_CALIBRATE_REACTIVE_GAIN);
}

/* Auto calibrate gain */
int MCP39F511Interface::autoCalibrateFrequency() {
	return mcp_comms->sendCommand(MCP_CMD_AUTO_CALIBRATE_FREQUENCY);
}

int MCP39F511Interface::factoryResetMcp39F511() {
    if(factoryResetId) {
        return factoryResetId;
    } else {
        mcpCalibReg.calibration_register_delimieter = 0xA5A5;
        setRegister(MCP_CALIB_REG_DELIMETER, (u_int8_t *)&mcpCalibReg.calibration_register_delimieter, sizeof(mcpCalibReg.calibration_register_delimieter));
        factoryResetId = saveRegistersToFlash();
        return factoryResetId;
    }
}

/**
 * Called when a comms transaction is complete
 * @param 
 */
void MCP39F511Interface::transactionComplete(Mcp39F511Transaction transaction) {
    static DecodedMeasurements energyValuesBuffer[NOISE_FILTER_SAMPLES] = {{0, 0, 0, 0, 0, 0, 0}};
    static int measurementCount = 0;

	//qDebug("Transaction complete: %d", transaction.unique_id);
	emit dataReady(transaction);

	/* Power measurement data recieved so emit it as a signal */
	if(transaction.unique_id == outputTransactionId) {
		outputTransactionId = 0;
		emit outputRegistersReady(mcpOutputReg, transaction.unique_id);
        
        bool usePowerActive = true;
        bool usePowerReactive = true;
        
        energyValuesBuffer[measurementCount].powerActive = mcpOutputReg.active_power / (double)100;
        energyValuesBuffer[measurementCount].powerReactive = mcpOutputReg.reactive_power / (double)100;
        
        /* Zero these measurement if less than threshold for NOISE_FILTER_SAMPLES measurements */
        for(int i = 0; i < NOISE_FILTER_SAMPLES; i++) {
            if(energyValuesBuffer[i].powerActive <= POWER_ACTIVE_THRESHOLD) {
                usePowerActive = false;
            }
            if(energyValuesBuffer[i].powerReactive <= POWER_REACTIVE_THRESHOLD) {
                usePowerReactive = false;
            }
        }
        
        DecodedMeasurements energyValues = {0, 0, 0, 0, 0, 0, 0};
        /* When the Energy values are valid make a copy and scale them to human readable values */
        energyValues.voltageRms = mcpOutputReg.voltage_RMS / (double)10;
        energyValues.frequency = mcpOutputReg.line_frequency / (double)1000;
        /* Each LSB is then equivalent to a weight of 2^(-15) - See MCP39F511 datasheet */
        energyValues.powerFactor = mcpOutputReg.power_factor * qPow(2, -15);
        energyValues.currentRms = mcpOutputReg.current_RMS / (double)10000;
        /* Check active and reactive power have been non zero for several measurements */
        if(usePowerActive) {
            energyValues.powerActive = energyValuesBuffer[measurementCount].powerActive;
        } else {
            energyValues.powerActive = 0;
        }
        if(usePowerReactive) {
            energyValues.powerReactive = energyValuesBuffer[measurementCount].powerReactive;
        } else {
            energyValues.powerReactive = 0;
        }
        energyValues.powerApparent = mcpOutputReg.apparent_power / (double)100;

        /* Loop through the filter */
        if(measurementCount < NOISE_FILTER_SAMPLES) {
            measurementCount++;
        } else {
            measurementCount = 0;
        }

        emit measurementsReady(energyValues);
	}
	
	if(transaction.unique_id == energyCounterTransactionId) {
		energyCounterTransactionId = 0;
		emit energyCounterRegistersReady(mcpEnergyCounterReg, transaction.unique_id);
	}

	if(transaction.unique_id == recordTransactionId) {
		recordTransactionId = 0;
		emit recordRegistersReady(mcpRecordReg, recordTransactionId);
	}
	
	if(transaction.unique_id == calibrationTransactionId) {
		calibrationTransactionId = 0;
		emit calibrationRegistersReady(mcpCalibReg, transaction.unique_id);
	}
	
	if(transaction.unique_id == config1TransactionId) {
		config1TransactionId = 0;
		emit config1RegistersReady(mcpConfigReg1, transaction.unique_id);
	}
	
	if(transaction.unique_id == config2TransactionId) {
		config2TransactionId = 0;
		emit config2RegistersReady(mcpConfigReg2, transaction.unique_id);
	}
	
	if(transaction.unique_id == compPeriphTransactionId) {
		compPeriphTransactionId = 0;
		emit compPeriphRegistersReady(mcpCompPeriphReg, transaction.unique_id);
	}
    	
    if(transaction.unique_id == readAllRegistersId) {
        readAllRegistersId = 0;
        emit readAllRegistersComplete(transaction.unique_id);
        if(initialisationInProgress == true) {
            initialisationInProgress = false;
            emit initialisationComplete();
        }
    }
	
    if(transaction.unique_id == factoryResetId) {
        factoryResetId = 0;
        /* Delay 1000 ms to ensure the MCP39F511 has reset its flash */
        usleep(1000000);
        /* Reset the MCP39F511 after reset flash is complete. */
        resetMCP39F511();
        emit factoryResetComplete(transaction.unique_id);
    }
}