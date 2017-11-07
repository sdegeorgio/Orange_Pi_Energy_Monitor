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
 * File:   MCP39F511_interface.h
 * Author: Stephan de Georgio
 *
 * Created on 23 September 2016, 11:12
 */

#ifndef MCP39F511INTERFACE_H
#define MCP39F511INTERFACE_H

#include <QObject>
#include "MCP39F511Comms.h"

/* Output registers locations */
#define MCP_OUTPUT_REG_INSTRUCTION_POINTER 0x0000
#define MCP_OUTPUT_REG_SYSTEM_STATUS 0x0002
#define MCP_OUTPUT_REG_SYSTEM_VERSION 0x0004
#define MCP_OUTPUT_REG_VOLTAGE_RMS 0x0006
#define MCP_OUTPUT_REG_LINE_FREQUENCY 0x0008
#define MCP_OUTPUT_REG_ANALOG_INPUT_VOLTAGE 0x000A
#define MCP_OUTPUT_REG_POWER_FACTOR 0x000C
#define MCP_OUTPUT_REG_CURRENT_RMS 0x000E
#define MCP_OUTPUT_REG_ACTIVE_POWER 0x0012
#define MCP_OUTPUT_REG_REACTIVE_POWER 0x0016
#define MCP_OUTPUT_REG_APPARENT_POWER 0x001A

/* System status register bits */
#define MCP_SYSTEM_STATUS_EVENT2  (1<<11)
#define MCP_SYSTEM_STATUS_EVENT1 (1<<10)
#define MCP_SYSTEM_STATUS_OVERTEMP (1<<6)
#define MCP_SYSTEM_STATUS_SIGN_PR (1<<5)
#define MCP_SYSTEM_STATUS_SIGN_PA (1<<4)
#define MCP_SYSTEM_STATUS_OVERPOW (1<<3)
#define MCP_SYSTEM_STATUS_OVERCUR (1<<2)
#define MCP_SYSTEM_STATUS_VSURGE (1<<1)
#define MCP_SYSTEM_STATUS_VSAG (1<<0)

#define MCP_OUTPUT_REGISTERS_SIZE sizeof(McpOutputRegisters)
#define MCP_OUTPUT_REGISTERS_START 0x0000
typedef struct __attribute__((packed)) {
	u_int16_t instruction_pointer;
	u_int16_t system_status;
	u_int16_t system_version;
	u_int16_t voltage_RMS;
	u_int16_t line_frequency;
	u_int16_t analog_input_voltage;
	int16_t   power_factor;
	u_int32_t current_RMS;
	u_int32_t active_power;
	u_int32_t reactive_power;
	u_int32_t apparent_power;
} McpOutputRegisters;


/* Energy import / export counter register locations */
#define MCP_RECORD_IMP_ACTIVE_ENERGY_COUNTER 0x001E
#define MCP_RECORD_EXP_ACTIVE_ENERGY_COUNTER 0x0026
#define MCP_RECORD_IMP_REACTIVE_ENERGY_COUNTER 0x002E
#define MCP_RECORD_EXP_REACTIVE_ENERGY_COUNTER 0x0036

#define MCP_ENERGY_COUNTER_REGISTERS_SIZE sizeof(McpEnergyCounterRegisters)
#define MCP_ENERGY_COUNTER_REGISTERS_START 0x001E
typedef struct __attribute__((packed)) {
	u_int64_t import_active_energy_counter;
	u_int64_t export_active_energy_counter;
	u_int64_t import_reactive_energy_counter;
	u_int64_t export_reactive_energy_counter;
} McpEnergyCounterRegisters;


/* Record register locations */
#define MCP_RECORD_MIN_RECORD_1 0x003E
#define MCP_RECORD_MIN_RECORD_2 0x0042
#define MCP_RECORD_RESERVED_1 0x0046
#define MCP_RECORD_RESERVED_2 0x004A
#define MCP_RECORD_MAX_RECORD_1 0x004E
#define MCP_RECORD_MAX_RECORD_2 0x0052
#define MCP_RECORD_RESERVED_3 0x0056
#define MCP_RECORD_RESERVED_4 0x005A

#define MCP_RECORD_REGISTERS_SIZE sizeof(McpRecordRegisters)
#define MCP_RECORD_REGISTERS_START 0x003E
typedef struct __attribute__((packed)) {
	u_int32_t minimum_record_1;
	u_int32_t minimum_record_2;
	u_int32_t reserved_1;
	u_int32_t reserved_2;
	u_int32_t maximum_record_1;
	u_int32_t maximum_record_2;
	u_int32_t reserved_3;
	u_int32_t reserved_4;
} McpRecordRegisters;


/* Calibration register locations */
#define MCP_CALIB_REG_DELIMETER 0x005E
#define MCP_CALIB_GAIN_CURRENT_RMS 0x0060
#define MCP_CALIB_GAIN_VOLTAGE_RMS 0x0062
#define MCP_CALIB_GAIN_ACTIVE_POWER 0x0064
#define MCP_CALIB_GAIN_REACTIVE_POWER 0x0066
#define MCP_CALIB_OFFSET_CURRENT_RMS 0x0068
#define MCP_CALIB_OFFSET_ACTIVE_POWER 0x006C
#define MCP_CALIB_OFFSET_REACTIVE_POWER 0x0070
#define MCP_CALIB_DC_OFFSET_CURRENT 0x0074
#define MCP_CALIB_PHASE_COMPENSATION 0x0076
#define MCP_CALIB_APPARENT_POWER_DIV 0x0078

#define MCP_CALIBRATION_REGISTERS_SIZE sizeof(McpCalibrationRegisters)
#define MCP_CALIBRATION_REGISTERS_START 0x005E
typedef struct __attribute__((packed)) {
	u_int16_t calibration_register_delimieter;
	u_int16_t gain_current_RMS;
	u_int16_t gain_voltage_RMS;
	u_int16_t gain_active_power;
	u_int16_t gain_reactive_power;
	int32_t   offset_current_RMS;
	int32_t   offset_active_power;
	int32_t   offset_reactive_power;
	int16_t   DC_offset_current;
	int16_t   phase_compensation;
	u_int16_t apparent_power_divisor;
} McpCalibrationRegisters;

/* System configuration register 1 locations */
#define MCP_CONFIG_1_SYSTEM_CONFIG 0x007A
#define MCP_CONFIG_1_EVENT_CONFIG 0x007E
#define MCP_CONFIG_1_RANGE 0x0082
#define MCP_CONFIG_1_CALIB_CURRENT 0x0086
#define MCP_CONFIG_1_CALIB_VOLTAGE 0x008A
#define MCP_CONFIG_1_CALIB_POWER_ACTIVE 0x008C
#define MCP_CONFIG_1_CALIB_POWER_REACTIVE 0x0090
#define MCP_CONFIG_1_LINE_FREQ_REF 0x0094

/* System config register bits */
#define MCP_SYSTEM_CONFIG_PGA_CH1 (1<<26)
#define MCP_SYSTEM_CONFIG_PGA_CH2 (1<<23)
#define MCP_SYSTEM_CONFIG_VREFCAL (1<<15)
#define MCP_SYSTEM_CONFIG_UART (1<<13)
#define MCP_SYSTEM_CONFIG_ZCD_INV (1<<12)
#define MCP_SYSTEM_CONFIG_ZCD_PULS (1<<11)
#define MCP_SYSTEM_CONFIG_ZCD_OUTPUT_DIS (1<<10)
#define MCP_SYSTEM_CONFIG_SINGLE_WIRE (1<<8)
#define MCP_SYSTEM_CONFIG_TEMPCOMP (1<<7)
#define MCP_SYSTEM_CONFIG_RESET (1<<5)
#define MCP_SYSTEM_CONFIG_SHUTDOWN (1<<3)
#define MCP_SYSTEM_CONFIG_VREFEXT (1<<2)

/* Event config register */
#define MCP_EVENT_CONFIG_OVERTEMP_PIN2 (1<<27)
#define MCP_EVENT_CONFIG_OVERTEMP_PIN1 (1<<26)
#define MCP_EVENT_CONFIG_OVERTEMP_CL (1<<25)
#define MCP_EVENT_CONFIG_OVERTEMP_LA (1<<24)
#define MCP_EVENT_CONFIG_OVERTEMP_ST (1<<23)

#define MCP_EVENT_CONFIG_OVERPOW_PIN2 (1<<22)
#define MCP_EVENT_CONFIG_OVERCUR_PIN2 (1<<21)
#define MCP_EVENT_CONFIG_VSURGE_PIN2 (1<<20)
#define MCP_EVENT_CONFIG_VSAG_PIN2 (1<<19)
#define MCP_EVENT_CONFIG_OVERPOW_PIN1 (1<<18)
#define MCP_EVENT_CONFIG_OVERCUR_PIN1 (1<<17)
#define MCP_EVENT_CONFIG_VSURGE_PIN1 (1<<16)
#define MCP_EVENT_CONFIG_VSAG_PIN1 (1<<15)

#define MCP_EVENT_CONFIG_OVERCUR_CL (1<<10)
#define MCP_EVENT_CONFIG_OVERPOW_CL (1<<9)
#define MCP_EVENT_CONFIG_VSAG_CL (1<<8)
#define MCP_EVENT_CONFIG_VSUR_LA (1<<7)

#define MCP_EVENT_CONFIG_VSAG_LA (1<<6)
#define MCP_EVENT_CONFIG_OVERPOW_LA (1<<5)
#define MCP_EVENT_CONFIG_OVERCUR_LA (1<<4)
#define MCP_EVENT_CONFIG_VSUR_TST (1<<3)
#define MCP_EVENT_CONFIG_VSAG_TST (1<<2)
#define MCP_EVENT_CONFIG_OVERPOW_TST (1<<1)
#define MCP_EVENT_CONFIG_OVERCUR_TST (1<<0)

#define MCP_CONFIG_REGISTERS_1_SIZE sizeof(McpConfigurationRegisters1)
#define MCP_CONFIG_REGISTERS_1_START 0x007A
typedef struct __attribute__((packed)) {
	u_int32_t system_configuration;
	u_int16_t event_configuration;
	
    u_int16_t reserved;
    
    u_int8_t  range_voltage;
    u_int8_t  range_current;
    u_int8_t  range_power;
    u_int8_t  range_reserved;

	u_int32_t calibration_current;
	u_int16_t calibration_voltage;
	u_int32_t calibration_power_active;
	u_int32_t calibration_power_reactive;
	u_int16_t line_frequency_reference;
} McpConfigurationRegisters1;

/* System configuration register 2 locations */
#define MCP_CONFIG_2_RESERVED_1 0x0096
#define MCP_CONFIG_2_RESERVED_2 0x009A
#define MCP_CONFIG_2_ACCUMULATION_INTERVAL 0x009E
#define MCP_CONFIG_2_VOLTAGE_SAG_LIMIT 0x00A0
#define MCP_CONFIG_2_VOLTAGE_SURGE_LIMIT 0x00A2
#define MCP_CONFIG_2_OVERCURRENT_LIMIT 0x00A4
#define MCP_v_2_OVERPOWER_LIMIT 0x00A8

#define MCP_CONFIG_REGISTERS_2_SIZE sizeof(McpConfigurationRegisters2)
#define MCP_CONFIG_REGISTERS_2_START 0x0096
typedef struct __attribute__((packed)) {
	u_int32_t reserved_1;
	u_int32_t reserved_2;
	u_int16_t accumulation_interval_parameter;
	u_int16_t voltage_sag_limit;
	u_int16_t voltage_surge_limit;
	u_int32_t overcurrent_limit;
	u_int32_t overpower_limit;
} McpConfigurationRegisters2;

/* Temperature compensation and peripheral control registers */
#define MCP_COMP_PERIPH_TEMP_COMP_FREQUENCY 0x00C6
#define MCP_COMP_PERIPH_TEMP_COMP_CURRENT 0x00C8
#define MCP_COMP_PERIPH_TEMP_COMP_POWER 0x00CA
#define MCP_COMP_PERIPH_REG_AMB_TEMP_REF_VOLTS 0x00CC
#define MCP_COMP_PERIPH_REG_PWM_PERIOD 0x00CE
#define MCP_COMP_PERIPH_REG_PWM_DUTY 0x00D0
#define MCP_COMP_PERIPH_RESERVED_1 0x00D2
#define MCP_COMP_PERIPH_MIN_MAX_PTR_1 0x00D4
#define MCP_COMP_PERIPH_MIN_MAX_PTR_2 0x00D6
#define MCP_COMP_PERIPH_OVERTEMP_LIMIT 0x00D8
#define MCP_COMP_PERIPH_RESERVED_2 0x00DA
#define MCP_COMP_PERIPH_ENERGY_CONTROL 0x00DC
#define MCP_COMP_PERIPH_REG_PWM_CONTROL 0x00DE
#define MCP_COMP_PERIPH_NO_LOAD_THRESHOLD 0x00E0

/* PWM period register */
#define MCP_PWM_PERIOD_PWM_P_POS 8 /* 8-bit value shifted to top of 16-bit register */
#define MCP_PWM_PERIOD_PWM_P_MASK 0xFF
#define MCP_PWM_PERIOD_PRE_POS 0
#define MCP_PWM_PERIOD_PRE_MASK 0x03
#define MCP_PWM_DUTY_UPPER_MASK 0x3FC /* upper 10-bit mask prior to shift */
#define MCP_PWM_DUTY_UPPER_POS 6 /* 10-bit value shifted to top of 16-bit register */
#define MCP_PWM_DUTY_LOWER_MASK 0x03 /* lower 2-bit mask */
#define MCP_PWM_DUTY_LOWER_POS 0

/* PWM control register */
#define MCP_PWM_CONTROL_PWM_CNTRL (1 << 0)

#define MCP_COMP_PERIPH_REGISTERS_SIZE sizeof(McpCompPeriphRegisters)
#define MCP_COMP_PERIPH_REGISTERS_START 0x00C6
typedef struct __attribute__((packed)) {
	u_int16_t temperature_compensation_for_frequency;
	u_int16_t temperature_compensation_for_current;
	u_int16_t temperature_compensation_for_power;
	u_int16_t ambient_temperature_reference_voltage;
	u_int16_t PWM_period;
	u_int16_t PWM_duty_cycle;
	u_int16_t reserved1;
	u_int16_t min_max_pointer1;
	u_int16_t min_max_pointer2;
	u_int16_t overtemperature_limit;
	u_int16_t reserved2;
	u_int16_t energy_control;
	u_int16_t PWM_control;
	u_int16_t no_load_threshold;
} McpCompPeriphRegisters;

/* MCP39F511 integrated EEPROM details */
#define MCP_EEPROM_PAGE_SIZE 16
#define MCP_EEPROM_PAGE_COUNT 32

typedef enum {
	BEEPER_ON,
	BEEPER_OFF,
	READ_POWER_REGISTERS
} transaction_name;

typedef struct {
    double voltageRms;
	double frequency;
	double powerFactor;
	double currentRms;
	double powerActive;
	double powerReactive;
	double powerApparent;
} DecodedMeasurements;


class MCP39F511Interface : public QObject {
	Q_OBJECT
	
public:
	MCP39F511Interface(QObject *parent);
	virtual ~MCP39F511Interface();
	
	/* 
	 * Initialise the energy monitor
	 * @return Returns TRUE if OK, otherwise FALSE
	 */
	bool initialise();
	
    /**
     * Hardware resets the MCP39F511.
     * It appears comms still operates when the chip is held in reset.
     */
    void resetMCP39F511();
    
	/**
	 * Close the MCP39F511 serial port
	 * @return Returns TRUE if OK, otherwise FALSE
	 */
	bool close();
	
	/**
	 * Turn the sounder on or off
	 * @param enabled true for on, false for off
	 */
	void beep_on(bool enabled);
	
	/**
	 * Reads all registers.  A signal will be generate for each as they are read in.
     * @return Unique transaction ID.
	 */
	int readAllRegisters();
	
	/**
	 * Gets a set of power measurements.  measurementsReady signal emitted when data ready.
	 * @return Unique transaction ID.
	 */
	int getOutputRegisters();
	
	/**
	 * Get energy counter registers
	 * @return Unique transaction ID.
	 */
	int getEnergyCounterRegisters();
	
	/**
	 * Get min / max record registers
	 * @return Unique transaction ID.
	 */
	int getRecordRegisters();
	
	/**
	 * Get calibration registers
	 * @return Unique transaction ID.
	 */
	int getCalibrationRegisters();
	
	/**
	 * Get first bank of design configuration registers
	 * @return Unique transaction ID.
	 */
	int getDesignConfig1Registers();
	
	/**
	 * Get second bank of design configuration registers
	 * @return Unique transaction ID.
	 */
	int getDesignConfig2Registers();
	
	/**
	 * Get temperature compensation and peripheral control registers
	 * @return Unique transaction ID.
	 */
	int getCompPeriphRegisters();
	
	/**
	 * Set a register and return a unique ID for the transaction
	 * @return Unique transaction ID.
	 */
	int setRegister(u_int16_t address, u_int8_t *data, u_int8_t length);

	/**
	 * Get a register and return a unique ID for the transaction
	 * @return Unique transaction ID.
	 */
	int getRegister(u_int16_t address, u_int8_t *data, u_int8_t length);

	/* Save flash registers */
	int saveRegistersToFlash();

	/* Read a page of EEPROM */
	int eepromReadPage(u_int8_t page);
	
	/* Write a page of EEPROM */
	int eepromWritePage(u_int8_t page, u_int8_t *data);
	
	/* Erase the entire contents of EEPROM */
	int eepromBulkErase();
	
	/* Auto calibrate gain */
	int autoCalibrateGain();
	
	/* Auto calibrate gain */
	int autoCalibrateReactiveGain();
	
	/* Auto calibrate gain */
	int autoCalibrateFrequency();

    /**
     * Factory reset MCP39F511 to defaults.
     */
    int factoryResetMcp39F511();
	
    /**
     * Configure the PWM for a given frequency and duty cycle.
     * @param frequency PWM frequency
     * @param duty_cycle PWM duty cycle
     */
    void setupPWM(int frequency, int duty_cycle);
    
	McpOutputRegisters mcpOutputReg;
	McpEnergyCounterRegisters mcpEnergyCounterReg;
	McpRecordRegisters mcpRecordReg;
	McpCalibrationRegisters mcpCalibReg;
	McpConfigurationRegisters1 mcpConfigReg1;
	McpConfigurationRegisters2 mcpConfigReg2;
	McpCompPeriphRegisters mcpCompPeriphReg;
	
signals:
    void measurementsReady(DecodedMeasurements);
	void outputRegistersReady(McpOutputRegisters, int transactionId);
	void energyCounterRegistersReady(McpEnergyCounterRegisters, int transactionId);
	void recordRegistersReady(McpRecordRegisters, int transactionId);
	void calibrationRegistersReady(McpCalibrationRegisters, int transactionId);
	void config1RegistersReady(McpConfigurationRegisters1, int transactionId);
	void config2RegistersReady(McpConfigurationRegisters2, int transactionId);
	void compPeriphRegistersReady(McpCompPeriphRegisters, int transactionId);
    void factoryResetComplete(int transactionId);
    void readAllRegistersComplete(int transactionId);
	void dataReady(Mcp39F511Transaction);
    void initialisationComplete();
	
private slots:
	void transactionComplete(Mcp39F511Transaction);

private:
	MCP39F511Comms *mcp_comms;
    bool initialisationInProgress;
	
    int readAllRegistersId;
	int outputTransactionId;
	int energyCounterTransactionId;
	int recordTransactionId;
	int calibrationTransactionId;
	int config1TransactionId;
	int config2TransactionId;
	int compPeriphTransactionId;
    int factoryResetId;
	
	u_int8_t eepromBuffer[MCP_EEPROM_PAGE_SIZE + 1];
};

#endif /* MCP39F511INTERFACE_H */

