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

static int jtag_Signals[JTAG_SIGNAL_MAX];
static unsigned int jtag_PinUsage;		///< Bit mask of the pins used for signals.

/**
 * @brief Initialises the JTAG local variables.
 */
void jtag_Init()
{
	unsigned int i;

	for(i = 0; i < JTAG_SIGNAL_MAX; ++i)
	{
		jtag_Signals[i] = JTAG_SIGNAL_NOT_ALLOCATED;
	}

	jtag_PinUsage = 0;	//No pins currently allocated.

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
 * JTAG_SIGNAL_NOT_ALLOCATED to de-configure the pin.
 */
void jtag_Cfg(jtag_Signal sig, int num)
{
	if((sig >= JTAG_SIGNAL_TCK) && (sig < JTAG_SIGNAL_MAX))
	{
		unsigned int mask_and, mask_or;
		if(num < JTAG_PIN_MAX)
		{
			if(num != JTAG_SIGNAL_NOT_ALLOCATED)
			{
				//is the pin being requested currently free?
				if((jtag_PinUsage & (1 << num)) == 0)
				{
					//Deconfigure the old pin if allocated
					if(jtag_Signals[sig] != JTAG_SIGNAL_NOT_ALLOCATED)
					{
						mask_and = 3 << (jtag_Signals[sig] * 2);
						GPIOD_MODER = (GPIOD_MODER & ~mask_and);
						jtag_PinUsage &= ~(1<<jtag_Signals[sig]);	//mark as un-allocated
					}

					//Configure the IO port mode
					mask_and = 3 << (num * 2);
					mask_or = ((sig == JTAG_SIGNAL_TDO) || (sig == JTAG_SIGNAL_RTCK)) ? 0 : 1;	//Input or output?
					mask_or = mask_or << (num * 2);
					GPIOD_MODER = (GPIOD_MODER & ~mask_and) | mask_or;
					jtag_PinUsage |= (1<<num);
					jtag_Signals[sig] = num;	//set the allocation
				}
			}
			else
			{
				//Configure the pin as an input
				unsigned int old_sig = jtag_Signals[sig];
				if(old_sig != JTAG_SIGNAL_NOT_ALLOCATED)
				{
					mask_and = 3 << (old_sig * 2);
					GPIOD_MODER = (GPIOD_MODER & ~mask_and);
					jtag_PinUsage &= ~(1<<jtag_Signals[sig]);	//mark as un-allocated
				}
				jtag_Signals[sig] = num;	//set the allocation
			}
			
		}
	}
}

/**
 * @brief Set one of the JTAG signals to the provided value
 *
 * @param[in] pin The pin to set
 * @param[in] val, true for High, false for Low.
 */
void jtag_Set(jtag_Signal sig, bool val)
{
	unsigned int pinNum;

	if((sig >= JTAG_SIGNAL_TCK) && (sig < JTAG_SIGNAL_MAX) && !((sig == JTAG_SIGNAL_TDO) || (sig == JTAG_SIGNAL_RTCK)))
	{
		pinNum = jtag_Signals[sig];

		if(pinNum != JTAG_SIGNAL_NOT_ALLOCATED)
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
bool jtag_Get(jtag_Signal sig)
{
	unsigned int pinNum = jtag_Signals[sig];
	bool pinState = false;

	if(pinNum != JTAG_SIGNAL_NOT_ALLOCATED)
	{
		//read the pin state from the input register
		pinState = ((GPIOD_IDR & (1 << pinNum)) != 0);
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
	jtag_Set(JTAG_SIGNAL_TCK, true);
	
	for(cnt = 8000; cnt > 0; --cnt)
	{
		__asm("nop");
	}

	jtag_Set(JTAG_SIGNAL_TCK, false);

	for(cnt = 8000; cnt > 0; --cnt)
	{
		__asm("nop");
	}
}

/**
 * @brief Get the allocated state of a signal
 *
 * @retval true Signal allocated to a pin
 * @retval false Signal no allocated to a pin
 */
bool jtag_IsAllocated(jtag_Signal sig)
{
	bool retval = false;

	if((sig >= JTAG_SIGNAL_TCK) && (sig < JTAG_SIGNAL_MAX))
	{
		retval = (jtag_Signals[sig] != JTAG_SIGNAL_NOT_ALLOCATED);
	}
	return retval;	
}

