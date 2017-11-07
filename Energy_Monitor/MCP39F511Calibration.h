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
 * File:   MCP39F511Calibration.h
 * Author: Stephan de Georgio
 *
 * Created on 22 September 2016, 11:14
 */

#ifndef MCP39F511CALIBRATION_H
#define MCP39F511CALIBRATION_H

#include <QObject>
#include "MCP39F511Interface.h"
#include "PA1000PowerAnalyser.h"

/**
 * Uncomment to enable automatic calibration at half power factor
 */
/*#define CALIBRATE_POWER_FACTOR_HALF*/

typedef enum {
	CALIB_STATE_IDLE,
	CALIB_STATE_WAIT_FOR_LOAD_PF_ONE,
	CALIB_STATE_CONNECT_TO_PA1000,
	CALIB_STATE_GET_PA1000_PF_ONE,
	CALIB_STATE_GET_MCP_MEASUREMENTS_PF_ONE,
	CALIB_STATE_WRITE_FREQUENCY,
	CALIB_STATE_AUTO_CALIB_FREQUENCY,
	CALIB_STATE_WRITE_VOLTS_AMPS_WATTS_PF_ONE,
	CALIB_STATE_AUTO_CALIB_GAIN,
	CALIB_STATE_WAIT_FOR_LOAD_PF_HALF,
	CALIB_STATE_GET_PA1000_PF_HALF,
	CALIB_STATE_GET_MCP_MEASUREMENTS_PF_HALF,
	CALIB_STATE_WRITE_VOLTS_AMPS_WATTS_PF_HALF,
	CALIB_STATE_AUTO_CALIB_REACTIVE_GAIN,
	CALIB_STATE_SET_SYSTEM_CONFIG,
	CALIB_STATE_SAVE_TO_FLASH,
    CALIB_STATE_READ_ALL_REGISTERS
} CalibrationState;

typedef struct {
	u_int16_t volts;
	u_int16_t amps;
	u_int16_t activePower;
	u_int16_t frequency;
	u_int16_t reactivePower;
	int16_t powerFactor;
} PA1000AdjustedMeasurements;

class MCP39F511Calibration : public QObject {
	Q_OBJECT
	
public:
	MCP39F511Calibration(QObject *parent, MCP39F511Interface *power_meter);
	virtual ~MCP39F511Calibration();
	
signals:
	/**
	 * Signal emitted when calibration complete
	 * @param true if successful, false if calibration failed.
	 */
	void calibrationComplete(bool);

public slots:
	void startCalibration(QString hostName, bool performReactiveCal);
	void cancelCalibration();

private:
	void printMessage(QString);
	void adjustRangeAndCopyPA1000(McpOutputRegisters values);
	void writeMcpCalibrationData();
    void saveSettingsToFlash();
    void setMcpConfiguration();
	
	CalibrationState calibrationState;
	MCP39F511Interface *mcp39F511Interface;
	PA1000PowerAnalyser *pa1000Analyer;
	int transactionId;
	QString pa1000Hostname;
	McpOutputRegisters mcp39f511Values;
	PowerCalibrationData *pa1000Measurements;
	PA1000AdjustedMeasurements pa1000AdjustsedMeasurements;
    bool performReactive;
	
private slots:
	void dataReady(Mcp39F511Transaction);
	void pa1000Connected();
	void pa1000MeasurementsReady(PowerCalibrationData *);
	void mcpMeasurementsReady(McpOutputRegisters);
	void slotOnStdinData();
};

#endif /* MCP39F511CALIBRATION_H */

