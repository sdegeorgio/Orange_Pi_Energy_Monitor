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
 * File:   InputControl.h
 * Author: Stephan de Georgio
 *
 * Created on 25 July 2016, 13:19
 */

#ifndef INPUTCONTROL_H
#define INPUTCONTROL_H

/* Switch read interval in milliseconds */
#define SWITCH_READ_INTERVAL 10
/* Period a switch has to be pressed for it to be considered pressed
 * In multiples of SWITCH_READ_INTERVAL milliseconds */
#define SWITCH_DEBOUNCE_PERIOD 3

#include <QObject>
#include <QTimer>

#define GPIO_BUTTON_UP 11
#define GPIO_BUTTON_DOWN 29
#define GPIO_BUTTON_LEFT 26
#define GPIO_BUTTON_RIGHT 28
#define GPIO_BUTTON_SELECT 27

/* Button masks */
typedef enum {
	BUTTON_UP,
	BUTTON_DOWN,
	BUTTON_LEFT,
	BUTTON_RIGHT,
	BUTTON_SELECT,
	NUM_BUTTONS
} InputButtons;

typedef struct {
	InputButtons switchUID;
	int gpio;
	int debounceCounter;
	bool pressed;
	bool changedState;
} ButtonState;

class InputControl : public QObject {
	Q_OBJECT
	
	public:
		InputControl(QObject *parent);
		virtual ~InputControl();

	signals:
		void buttonPressed(ButtonState*);

private:
	void printMessage(QString message);
	ButtonState buttonState[NUM_BUTTONS];
	
private slots:
	void timerHandler();
};

#endif /* INPUTCONTROL_H */

