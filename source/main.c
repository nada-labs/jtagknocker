/*
 *  JtagKnocker - JTAG finder and enumerator for STM32 dev boards
 *  Copyright (C) 2014 Nathan Dyer
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/rcc.h>
#include "jtag.h"
#include "jtagtap.h"
#include "serial.h"
#include "knock.h"
#include "message.h"
#include "comprocessor.h"

/**
 * Development board entry point
 */
void main()
{

	//setup
	rcc_clock_setup_hsi(&hsi_8mhz[CLOCK_64MHZ]);
	serial_Init();
	message_Init();
	comproc_Init();

	//processing
	while(true)
	{

	}

	//whoops, we dropped out of the main loop
	while(true);
}

//functions to keep newlib happy
void *_sbrk(int incr)
{
	return((void*)(-1));
}

void _exit(int v)
{
	message_Write(MESSAGE_LEVEL_REQUIRED, "\r\n_exit(%i) called. Halting\r\n", v);
	while(true);
}
