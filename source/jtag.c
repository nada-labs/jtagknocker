/**
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

#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include "jtag.h"

static unsigned int jtag_Pins[JTAG_PIN_MAX];

/**
 * @brief Initialises the JTAG local variables.
 */
void jtag_Init()
{
	unsigned int i;

	for(i = 0; i < JTAG_PIN_MAX; ++i)
	{
		jtag_Pins[i] = JTAG_PIN_NOT_ALLOCATED;
	}

	//set up the io ports to be all inputs, push-pull, no pullups and slow when set as outputs.
	//outputs default to 0
	GPIOD_MODER = 0x00000000;
	GPIOD_OTYPER = 0x00000000;
	GPIOD_OSPEEDR = 0x00000000;
	GPIOD_PUPDR = 0x00000000;
	GPIOD_BSRR = 0xFFFF0000;
	RCC_AHBENR |= 0x00100000;	//Enable GPIOD clock

}

/**
 * @brief Sets a JTAG signal to a STM32 pin number
 *
 * The supported pin numbers are from 0 - 15 and correspond to PD0 - PD15
 * Respectively.
 *
 * @param[in] pin The JTAG signal to configure.
 * @param[in] num The pin number the signal is connected to, or
 * JTAG_PIN_NOT_ALLOCATED to de-configure the pin.
 */
void jtag_Cfg(jtag_Pin pin, unsigned int num)
{
	if((pin >= JTAG_PIN_TCK) && (pin < JTAG_PIN_MAX))
	{
		unsigned int mask_and, mask_or;
		if(num > JTAG_PIN_NOT_ALLOCATED)
		{
			num = JTAG_PIN_NOT_ALLOCATED;
		}

		if(num != JTAG_PIN_NOT_ALLOCATED)
		{
			//Deconfigure the old pin if allocated
			if(jtag_Pins[pin] != JTAG_PIN_NOT_ALLOCATED)
			{
				mask_and = 3 << (jtag_Pins[pin] * 2);
				GPIOD_MODER = (GPIOD_MODER & ~mask_and);
			}

			//Configure the IO port mode
			mask_and = 3 << (num * 2);
			mask_or = (pin == JTAG_PIN_TDO) ? 0 : 1;
			mask_or = mask_or << (num * 2);
			GPIOD_MODER = (GPIOD_MODER & ~mask_and) | mask_or;
		}
		else
		{
			//Configure the pin as an input
			unsigned int old_pin = jtag_Pins[pin];
			if(old_pin != JTAG_PIN_NOT_ALLOCATED)
			{
				mask_and = 3 << (old_pin * 2);
				GPIOD_MODER = (GPIOD_MODER & ~mask_and);
			}

		}
		jtag_Pins[pin] = num;	//set the allocation
	}
}

/**
 * @brief Set one of the JTAG signals to the provided value
 *
 * @param[in] pin The pin to set
 * @param[in] val, true for High, false for Low.
 */
void jtag_Set(jtag_Pin pin, bool val)
{
	unsigned int pinNum;

	if((pin >= JTAG_PIN_TCK) && (pin < JTAG_PIN_MAX) && (pin != JTAG_PIN_TDO))
	{
		pinNum = jtag_Pins[pin];

		if(pinNum != JTAG_PIN_NOT_ALLOCATED)
		{
			//set/reset the appropriate pin
			pinNum = 1 << pinNum;
			if(!val)
			{
				//resetting, shift it further
				pinNum = pinNum << 16;
			}
			GPIOD_BSRR = pinNum;
		}
	}
}

/**
 *@brief Retruns the state of one of the JTAG signals
 *
 *@retval true pin is high
 *@retval false pin is low
 */
bool jtag_Get(jtag_Pin pin)
{
	unsigned int pinNum = jtag_Pins[pin];
	bool pinState = false;

	if(pinNum != JTAG_PIN_NOT_ALLOCATED)
	{
		//read the pin state from the input register
		pinState = ((GPIOD_IDR & (1 << pin)) != 0);
	}

	return pinState;
}

/**
 * @brief Toggles the JTAG clock
 *
 */
void jtag_Clock()
{
	unsigned int cnt;
	jtag_Set(JTAG_PIN_TCK, true);
	
	for(cnt = 8000; cnt > 0; --cnt)
	{
		__asm("nop");
	}

	jtag_Set(JTAG_PIN_TCK, false);

	for(cnt = 8000; cnt > 0; --cnt)
	{
		__asm("nop");
	}
}
