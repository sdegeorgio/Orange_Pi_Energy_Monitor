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
 * File:   MCP39F511.cpp
 * Author: Stephan de Georgio
 * 
 * Created on 22 July 2016, 14:02
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <QtGlobal>
#include <QDebug>
#include <QTimer>
#include <QSharedDataPointer>

extern "C" {
	#include <wiringSerial.h>
	#include <wiringPi.h>
};

#include "MCP39F511Comms.h"

//#define COMMS_DEBUG

/* Service timer in milliseconds */
#define SERVICE_UPDATE_INTERVAL 50

/* Serial port configuration */
#define SERIAL_PORT (char*)"/dev/ttyS3"
#define SERIAL_BAUD_RATE 115200
#define SERIAL_FLAGS 0

/* MCP39F511 responses */
#define RESP_ACK 0x06
#define RESP_NAK 0x15
#define RESP_CSFAIL 0x51

/* MCP39F511 specifics */
#define FRAME_HEADER 0xA5

typedef struct {
	u_int8_t header;
	u_int8_t length;
	u_int8_t command;
	u_int8_t data;
	u_int8_t checksum;
} MCP39F511_frame;

MCP39F511Comms::MCP39F511Comms(QObject *parent) {
	setParent(parent);
	serial_handle = 0;
    transaction_id = 1;
}

MCP39F511Comms::~MCP39F511Comms() {
}

/*
 * Initialise the MCP39F511 energy monitor
 */
bool MCP39F511Comms::initialise() {
    resetMCP39F511();
    
	serial_handle = serialOpen(SERIAL_PORT, SERIAL_BAUD_RATE);
	if(serial_handle < 0) {
		printMessage("Failed to open serial port!");
		return false;	
	}
	
	mcp_command = MCP_CMD_IDLE;
	receiver_cur_state = RECV_HEADER;
	
	QTimer::singleShot(SERVICE_UPDATE_INTERVAL, this, SLOT(service()));

	return true;
}

void MCP39F511Comms::printMessage(QString message) {
	qDebug() << "MCP39F511 comms: " << message;
}

/**
 * Close the MCP39F511 serial port
 * @return Returns TRUE if OK, otherwise FALSE
 */
bool MCP39F511Comms::close() {
	serialClose(serial_handle);
	return true;
}

int MCP39F511Comms::sendCommand(mcp39F511_command command) {
	return enqueTransaction(0, 0, 0, command);
}

/**
 * Hardware resets the MCP39F511
 */
void MCP39F511Comms::resetMCP39F511() {
    /* Set reset pin as an output and set to low (reset state) */
    pinMode(GPIO_MCP39F511_RESET, OUTPUT);
	pullUpDnControl(GPIO_MCP39F511_RESET, PUD_OFF);
    /* Pull the reset low */
	digitalWrite (GPIO_MCP39F511_RESET, LOW);
    /* Delay 1000 milliseconds between reset */
	usleep(1000000);
    /* Release the reset */
    digitalWrite (GPIO_MCP39F511_RESET, HIGH);
}

int MCP39F511Comms::enqueTransaction(u_int16_t address, u_int8_t *data, u_int8_t length, mcp39F511_command command) {
	Mcp39F511Transaction transaction;
	transaction.command = command;
	transaction.regAddress = address;
	/* When performing a read no pointer is supplied so point to the pre-allocated buffer */
	if(data == NULL) {
		transaction.dataPtr = transaction.data;
	} else {
		transaction.dataPtr = data;
	}
	
	transaction.length = length;
	transaction_id++;
	transaction.unique_id = transaction_id;
#ifdef COMMS_DEBUG
	qDebug("Write transaction queued: %d, command: 0x%x", transaction_id, (u_int8_t)command);
#endif
	mcp39F511_queue.enqueue(transaction);
	return transaction_id;
}

/*
 * Service the MCP39F511 energy monitor
 */
void MCP39F511Comms::service() {
	static int watchdogTimer = 0;
	comms_status comms_state = COMMS_COMPLETE;
	
	watchdogTimer++;
	
	/* Service the receiver */
	if(mcp_command != MCP_CMD_IDLE) {
		comms_state = get_mcp39f511_data();
	
		/* Retry if comms failed or software watchdog times out */
		if(comms_state == COMMS_FAIL || watchdogTimer > 10) {
			/* Move to IDLE state if comms failed
			   Item not dequeued so will get re-tried */
			watchdogTimer = 0;
			receiver_cur_state = RECV_HEADER;
			mcp_command = MCP_CMD_IDLE;
			printMessage("Packet receive error (NAK 0x15), retrying...");
			comms_state = COMMS_COMPLETE;
			
		} else if(comms_state == COMMS_CHECKSUM_FAIL) {
			/* Move to IDLE state if checksum failed 
			   Item not dequeued so will get re-tried */
			printMessage("Packet checksum error (CSFAIL 0x51), retrying...");
			comms_state = COMMS_COMPLETE;
			mcp_command = MCP_CMD_IDLE;
			
		} else if(comms_state == COMMS_COMPLETE) {
			/* Transaction successful so dequeue the item and emit signal with return data */
			if(!mcp39F511_queue.isEmpty()) {
				emit(transactionComplete(mcp39F511_queue.dequeue()));
			}
			
			mcp_command = MCP_CMD_IDLE;
		}
	}
	
	/* Perform the next transfer in the queue if the transfer has completed */
	if(comms_state == COMMS_COMPLETE && !mcp39F511_queue.isEmpty()) {
		
		/* Flush out any junk left in the buffer before starting a new transaction */
		serialFlush(serial_handle);
		
		/* Clear the software based watchdog timer */
		watchdogTimer = 0;

		/* Get the next item from the head of the queue */
		Mcp39F511Transaction mcp_transaction = mcp39F511_queue.head();
		mcp_command = mcp_transaction.command;
		
		switch(mcp_command) {
			case MCP_CMD_IDLE:
				/* Must never reach this case! */
				break;
				
			case MCP_CMD_REGISTER_READ:
				register_read(&mcp_transaction);
				break;
			
			case MCP_CMD_REGISTER_WRITE:
				register_write(&mcp_transaction);
				break;

			case MCP_CMD_PAGE_READ_EEPROM:
				page_read_eeprom(*(u_int8_t*)mcp_transaction.dataPtr);
				break;
				
			case MCP_CMD_PAGE_WRITE_EEPROM:
				send_frame(MCP_CMD_PAGE_WRITE_EEPROM, (u_int8_t*) mcp_transaction.dataPtr, mcp_transaction.length, false);
				break;
				
			/* Commands that do not need further processing */
			case MCP_CMD_SET_ADDRESS_POINTER:
			case MCP_CMD_AUTO_CALIBRATE_FREQUENCY:
			case MCP_CMD_AUTO_CALIBRATE_REACTIVE_GAIN:
			case MCP_CMD_AUTO_CALIBRATE_GAIN:
			case MCP_CMD_BULK_ERASE_EEPROM:
			case MCP_CMD_SAVE_REGISTERS_TO_FLASH:
				send_frame(mcp_command, (u_int8_t*) mcp_transaction.dataPtr, mcp_transaction.length, false);
				break;
		}
	}
	
	QTimer::singleShot(SERVICE_UPDATE_INTERVAL, this, SLOT(service()));
}

bool MCP39F511Comms::register_read(Mcp39F511Transaction *transaction) {
	receiver_data_ptr = (u_int8_t *)transaction->dataPtr;
	u_int8_t buffer[4];
	buffer[0] = (transaction->regAddress >> 8) & 0xFF;
	buffer[1] = transaction->regAddress & 0xFF;
	buffer[2] = MCP_CMD_REGISTER_READ;
	buffer[3] = transaction->length;
	return send_frame(MCP_CMD_SET_ADDRESS_POINTER, buffer, 4, true);
}

bool MCP39F511Comms::register_write(Mcp39F511Transaction *transaction) {
	int i = 0;
	int j = 0;
	int frame_length = 4 + transaction->length;
	u_int8_t *byte_ptr;
	u_int8_t buffer[frame_length];
	buffer[i++] = (transaction->regAddress >> 8) & 0xFF;
	buffer[i++] = transaction->regAddress & 0xFF;
	buffer[i++] = MCP_CMD_REGISTER_WRITE;
	buffer[i++] = transaction->length;
	
	for(j=0; j<transaction->length; j++) {
		byte_ptr = (u_int8_t *)transaction->dataPtr;
		buffer[i++] = byte_ptr[j];
	}
	return send_frame(MCP_CMD_SET_ADDRESS_POINTER, buffer, frame_length, false);
}

bool MCP39F511Comms::set_address_pointer(u_int16_t address) {
	u_int8_t buffer[2];
	buffer[0] = (address >> 8) & 0xFF;
	buffer[1] = address & 0xFF;
	return send_frame(MCP_CMD_SET_ADDRESS_POINTER, buffer, 2, false);
}


bool MCP39F511Comms::page_read_eeprom(u_int8_t page) {
	u_int8_t buffer[1];
	buffer[0] = page;
	return send_frame(MCP_CMD_PAGE_READ_EEPROM, buffer, 1, true);
}

/**
 * Sends a frame to the MCP39F511
 * @param command A valid command from the MCP39F511 instruction set
 * @param data Pointer to u_int8_t array of bytes to send
 * @param length Number of bytes to send
 * @param read true if the command returns more than just an ack
 * @return TRUE if frame sent successfully, FALSE if not
 */
bool MCP39F511Comms::send_frame(u_int8_t command, u_int8_t *data, int length, bool read) {
	int i = 0;
	int j = 0;
	int frame_length = length + 4;
	u_int8_t buffer[frame_length];
	
	serial_read = read;
	
	buffer[i++] = FRAME_HEADER;
	buffer[i++] = frame_length;
	buffer[i++] = command;
	
	for(j=0; j<length + 1; j++) {
		buffer[i++] = data[j];
	}
	
	buffer[frame_length - 1] = calculate_checksum(buffer, frame_length - 1);
	
	for(i=0; i<frame_length; i++) {
		/* Delay 2 ms between bytes as the MCP39F511 can't keep up otherwise */
		usleep(2000);
#ifdef COMMS_DEBUG
        qDebug("byte tx: 0x%x", buffer[i]);
#endif
		serialPutchar(serial_handle, buffer[i]);
	}
	return true;
}

u_int8_t MCP39F511Comms::calculate_checksum(u_int8_t *pkt, int length) {
	int i = 0;
	u_int8_t checksum = 0;
	
	for(i=0; i<length; i++) {
		checksum += pkt[i];
	}
	
	// take the modulus after dividing by 256
	checksum &= 0xFF;
	return checksum;
}

comms_status MCP39F511Comms::get_mcp39f511_data() {
	int read_byte = 0;
	int8_t cur_byte = 0;
	
	int bytes_available = serialDataAvail(serial_handle);
	
	if(bytes_available < 0) {
		return COMMS_FAIL;
	} else {
		while(bytes_available > 0) {
			bytes_available--;
			read_byte = serialGetchar(serial_handle);
			
			/* Check if serial port threw an error when reading it */
			if(read_byte < 0) {
				return COMMS_FAIL;
				break;
			}
			
			/* Ensure we only process the lowest byte read in from serial port */
			cur_byte = (int8_t) (read_byte & 0xFF);
#ifdef COMMS_DEBUG
			qDebug("byte rx: 0x%x", (u_int8_t)cur_byte);
#endif
			switch(receiver_cur_state) {
				case RECV_HEADER:
					receiver_checksum = cur_byte;
					if(cur_byte == RESP_NAK) {
						return COMMS_FAIL;
					}
					if(cur_byte == RESP_CSFAIL) {
						return COMMS_CHECKSUM_FAIL;
					}
					if(cur_byte == RESP_ACK) {
#ifdef COMMS_DEBUG
                        qDebug("Transaction complete: %d", mcp39F511_queue.head().unique_id);
#endif
						if(serial_read == false) {
							return COMMS_COMPLETE;
						} else {
							receiver_cur_state = RECV_NUM_BYTES;
						}
					}
					break;
				
				case RECV_NUM_BYTES:
					receiver_packet_length = cur_byte;
					receiver_checksum += cur_byte;
					receiver_cur_state = RECV_DATA;
					break;
					
				case RECV_DATA:
					receiver_checksum += cur_byte;
					receiver_packet_length--;
					*receiver_data_ptr = cur_byte;
					receiver_data_ptr++;
					if(receiver_packet_length - 3 == 0) {
						receiver_cur_state = RECV_CHECKSUM;
					}
					break;
					
				case RECV_CHECKSUM:
					receiver_cur_state = RECV_HEADER;
					// take the modulus after dividing by 256
					receiver_checksum &= 0xFF;
					if(cur_byte == (int8_t) receiver_checksum) {
						return COMMS_COMPLETE;
					} else {
						printMessage("Receive checksum failed.");
						return COMMS_FAIL;
					}
					break;
			}
		}	
		return COMMS_BUSY;
	}
}
