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
 * File:   InputControl.cpp
 * Author: Stephan de Georgio
 * 
 * Created on 25 July 2016, 13:19
 */

#include "InputControl.h"

#include <QDebug>


extern "C" {
	#include <wiringPi.h>
};

InputControl::InputControl(QObject *parent) {
	setParent(parent);
	
	pinMode(GPIO_BUTTON_UP, INPUT);
	pinMode(GPIO_BUTTON_DOWN, INPUT);
	pinMode(GPIO_BUTTON_LEFT, INPUT);
	pinMode(GPIO_BUTTON_RIGHT, INPUT);
	pinMode(GPIO_BUTTON_SELECT, INPUT);
	
	pullUpDnControl(GPIO_BUTTON_UP, PUD_UP);
	pullUpDnControl(GPIO_BUTTON_DOWN, PUD_UP);
	pullUpDnControl(GPIO_BUTTON_LEFT, PUD_UP);
	pullUpDnControl(GPIO_BUTTON_RIGHT, PUD_UP);
	pullUpDnControl(GPIO_BUTTON_SELECT, PUD_UP);
	
	buttonState[0].gpio = GPIO_BUTTON_UP;
	buttonState[1].gpio = GPIO_BUTTON_DOWN;
	buttonState[2].gpio = GPIO_BUTTON_LEFT;
	buttonState[3].gpio = GPIO_BUTTON_RIGHT;
	buttonState[4].gpio = GPIO_BUTTON_SELECT;
	
	for(int i = 0; i<NUM_BUTTONS; i++) {
		buttonState[i].switchUID = (InputButtons)i;
		buttonState[i].debounceCounter = 0;
		buttonState[i].pressed = false;
		buttonState[i].changedState = false;
	}
	
	QTimer::singleShot(SWITCH_READ_INTERVAL, this, SLOT(timerHandler()));
}

InputControl::~InputControl() {
}

void InputControl::printMessage(QString message) {
	qDebug() << "Input module: " << message;
}

void InputControl::timerHandler() {
	int i;
	
	/* Debounce the switches */
	for(i=0; i<NUM_BUTTONS; i++) {
		if(!digitalRead (buttonState[i].gpio)) {
			if(buttonState[i].debounceCounter > SWITCH_DEBOUNCE_PERIOD) {
				buttonState[i].changedState = false;
				if(buttonState[i].pressed == false) {
					buttonState[i].pressed = true;
					buttonState[i].changedState = true;
					printMessage("Button pressed: " + QString::number(buttonState[i].switchUID));
					emit(buttonPressed(&buttonState[i]));
				}
			} else {
				buttonState[i].debounceCounter++;
			}
		} else {
			if(buttonState[i].debounceCounter <= 0) {
				buttonState[i].changedState = false;
				if(buttonState[i].pressed == true) {
					buttonState[i].pressed = false;
					buttonState[i].changedState = true;
					printMessage("Button released: " + QString::number(buttonState[i].switchUID));
					emit(buttonPressed(&buttonState[i]));
				}
			} else {
				buttonState[i].debounceCounter--;
			}
		}
	}
	
	QTimer::singleShot(SWITCH_READ_INTERVAL, this, SLOT(timerHandler()));
}

