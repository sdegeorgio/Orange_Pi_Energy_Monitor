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
 * File:   MCP39F511Comms.h
 * Author: Stephan de Georgio
 *
 * Created on 22 July 2016, 14:02
 */

#ifndef MCP39F511COMMS_H
#define MCP39F511COMMS_H

#include <QObject>
#include <QQueue>
#include <QSharedData>

/* Maximum number of bytes that can be read from the MCP39F511 in one go (32 bytes)
   Any more than this will need to be split into separate transactions
 */
#define SERIAL_MAX_LENGTH_RX 0x20

/**
 * Defines the GPIO pin used to reset the MCP39F511
 */
#define GPIO_MCP39F511_RESET 1

typedef enum {
	COMMS_BUSY,
	COMMS_COMPLETE,
	COMMS_NO_DATA,
	COMMS_FAIL,
	COMMS_CHECKSUM_FAIL
} comms_status;

/* MCP39F511 command bytes */
typedef enum {
	MCP_CMD_IDLE,
	MCP_CMD_REGISTER_READ = 0x4E,
	MCP_CMD_REGISTER_WRITE = 0x4D,
	MCP_CMD_SET_ADDRESS_POINTER = 0x41,
	MCP_CMD_SAVE_REGISTERS_TO_FLASH = 0x53,
	MCP_CMD_PAGE_READ_EEPROM = 0x42,
	MCP_CMD_PAGE_WRITE_EEPROM = 0x50,
	MCP_CMD_BULK_ERASE_EEPROM = 0x4F,
	MCP_CMD_AUTO_CALIBRATE_GAIN = 0x5A,
	MCP_CMD_AUTO_CALIBRATE_REACTIVE_GAIN = 0x7A,
	MCP_CMD_AUTO_CALIBRATE_FREQUENCY = 0x76
} mcp39F511_command;

typedef enum {
	RECV_HEADER,
	RECV_NUM_BYTES,
	RECV_DATA,
	RECV_CHECKSUM,

} receive_state;

typedef struct {
	u_int16_t regAddress;
	u_int8_t data[SERIAL_MAX_LENGTH_RX];
	u_int8_t *dataPtr;
	u_int8_t length;
	mcp39F511_command command;
	int unique_id;
} Mcp39F511Transaction;

class MCP39F511Comms : public QObject {
	Q_OBJECT
	
public:
	MCP39F511Comms(QObject *parent);
	virtual ~MCP39F511Comms();

	/* Initialise the energy monitor */
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
	 * Enqueues a read or write to the MCP39F511
	 * @param address Address in memory to access.  Set to 0 if the command does not require it.
	 * @param data Data pointer to read from when writing to MCP39F511.
	 * When reading from MCP39F511 an internal buffer will be used so a copy is made when the signal is emitted.
	 * @param length Length of the data to read or write.
	 * @param command MCP39F511 command to use.
	 * @return Unique ID for the transaction
	 */
	int enqueTransaction(u_int16_t address, u_int8_t *data, u_int8_t length, mcp39F511_command command);

	/**
	 * Send a command byte to the MCP39F511
	 * @param command to send
	 * @return Unique ID for the transaction
	 */
	int sendCommand(mcp39F511_command command);
	
signals:
	void transactionComplete(Mcp39F511Transaction);

private:
	bool send_frame(u_int8_t command, u_int8_t *data, int length, bool read);
	u_int8_t calculate_checksum(u_int8_t *pkt, int length);
	bool register_read(Mcp39F511Transaction *transaction);
	bool set_address_pointer(u_int16_t address);
	bool register_write(Mcp39F511Transaction *transaction);
	bool page_read_eeprom(u_int8_t page);
	comms_status get_mcp39f511_data();
	void printMessage(QString message);

	int8_t serial_handle;
	mcp39F511_command mcp_command;
	int8_t serial_read;
	QQueue<Mcp39F511Transaction> mcp39F511_queue;

	/* Receiver members*/
	u_int8_t *receiver_data_ptr;
	int receiver_packet_length;
	receive_state receiver_cur_state;
	u_int receiver_checksum;
	int transaction_id;
	
private slots:
	void service();
};

#endif /* MCP39F511COMMS_H */

