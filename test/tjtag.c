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

#include "test.h"
#include "tjtag.h"
#include <stdint.h>

//defines to stop the inclusion of unwanted header files
#define LIBOPENCM3_GPIO_H
#define LIBOPENCM3_RCC_H

//define the registers that we are interested in
static uint32_t GPIOD_MODER;	///< GPIO D Mode Register. p143 STM32F302xx Reference Manual.
static uint32_t GPIOD_OTYPER;	///< GPIO D Output type Register. p143 STM32F302xx Reference Manual.
static uint32_t GPIOD_OSPEEDR;	///< GPIO D Output speed Register. p144 STM32F302xx Reference Manual.
static uint32_t GPIOD_PUPDR;	///< GPIO D Pull up/down Register. p144 STM32F302xx Reference Manual.
static uint32_t GPIOD_IDR;	///< GPIO D Input Data Register. p145 STM32F302xx Reference Manual.
static uint32_t GPIOD_ODR;	///< GPIO D Output Data Register. p145 STM32F302xx Reference Manual.
static uint32_t GPIOD_BSRR;	///< GPIO D Bit Set/Reset Register. p146 STM32F302xx Reference Manual.
static uint32_t RCC_AHBENR;	///< RCC AHB Enable Register. p116 STM32F302xx Reference Manual.

//include the file *source*
#include "../source/jtag.c"

/**
 * @brief Test that all signals are set to their default value.
 *
 * The initial state for all signals is JTAG_SIGNAL_NOT_ALLOCATED.
 * No pins should be marked as assigned.
 */
bool jtag_TestInitSignalAlloc()
{
	//set up the signal state
	unsigned int index;

	for(index = 0; index < JTAG_SIGNAL_MAX; ++index)
	{
		jtag_Signals[index] = 0xABCD1234;
	}
	jtag_PinUsage = 0xABCD1234;

	jtag_Init();

	//check that they are all set to the default state
	for(index = 0; index < JTAG_SIGNAL_MAX; ++index)
	{
		ASSERT((jtag_Signals[index] == JTAG_SIGNAL_NOT_ALLOCATED), "JTAG signal state not initialized correctly. Signal: %i", index);
	}
	
	ASSERT((jtag_PinUsage == 0), "Pin allocation wasn't initialized correctly.");

	return true;
}

/**
 * @brief Test that the STM32 GPIO registers are set up correctly.
 *
 * Upon initialization, all pins on GPIO D should be set as inputs.
 * Push-pull mode should be enabled and pullups should be disabled.
 * The output speed should be the lowest and the data outputs should
 * default to low.
 *
 */
bool jtag_TestInitRegisterSetup()
{
	//Set the registers to tag values
	GPIOD_MODER = 0xABCD1234;
	GPIOD_OTYPER = 0xABCD1234;
	GPIOD_OSPEEDR = 0xABCD1234;
	GPIOD_PUPDR = 0xABCD1234;
	GPIOD_IDR = 0xABCD1234;
	GPIOD_ODR = 0xABCD1234;
	GPIOD_BSRR = 0xABCD1234;
	RCC_AHBENR = 0x00000000;
	
	jtag_Init();	//call the function under test

	//MODER is 2 bits per pin, with the following values
	//	00 Input mode
	//	01 Gen purpose output mode
	//	10 Alternate function mode
	//	11 Analog mode.
	//MODER should be all zeros to set all GPIO D pins to inputs.
	ASSERT((GPIOD_MODER == 0x00000000), "GPIO Mode set incorrectly: %08X, should be %08X.", GPIOD_MODER, 0);

	//OTYPER is 1 bit per pin, with bits 0 - 15 defined as below:
	//	0 Push-Pull Output
	//	1 Open Drain Output
	//OTYPER should be 0x0000 for the lower bits to set as push-pull outputs.
	//additionally, the high bits need to be kept at their reset state of 0.
	ASSERT((GPIOD_OTYPER == 0x00000000), "GPIO Output type set incorrectly: %08X, should be %08X.", GPIOD_OTYPER, 0);

	//OSPEEDR is 2 bits per pin, with the following values
	//	00,10 2MHz Low speed
	//	01 10MHz Medium speed
	//	11 50MHz High Speed
	//OSPEEDR should be either 0x00000000 or 0xCCCCCCCC to set low speed mode.
	ASSERT(((GPIOD_OSPEEDR & 0x55555555) == 0x00000000), "GPIO Output speed set incorrectly: %08X, should be 00000000 or CCCCCCCC.", GPIOD_OTYPER);
	
	//PUPDR is 2 bits per pin, with the following values
	//	00 No pull up or down
	//	01 Pull Up
	//	10 Pull Down
	//	11 Reserved.
	//PUPDR should be 0x00000000 to disable pullups
	ASSERT((GPIOD_PUPDR == 0x00000000), "GPIO pull up/down set incorrectly: %08X, should be %08X.", GPIOD_PUPDR, 0);

	//BSRR bits 0 - 15 set the relevant pin if written as 1, 
	//bits 16 - 31 reset the relevant pin if written as 1.
	//ODR bits 0 - 15 define the state of the relevant pin and the others 
	//must be kept at reset value (0).
	//Either BSRR has to be set to 0xFFFF0000 or ODR set to 0x00000000 to
	//get the pins to default to low output
	ASSERT(((GPIOD_ODR == 0x00000000) || (GPIOD_BSRR == 0xFFFF0000)), "GPIO output state set incorrectly: ODR: %08X  BSRR: %08X.", GPIOD_ODR, GPIOD_BSRR);

	//RCC_AHBENR enables the clocks for various peripherals
	//we need to turn on bit 20 and not disturb the state of the other bits
	ASSERT((RCC_AHBENR == (1<<20)), "RCC clock wasn't enabled correctly. Was %08X, should be %08X.", RCC_AHBENR, (1<<20));
	RCC_AHBENR = 0xFFFFFFFF;
	jtag_Init();
	ASSERT((RCC_AHBENR == 0xFFFFFFFF), "RCC clock set disturbed other bits. Was %08X, should be %08X.", RCC_AHBENR, 0xFFFFFFFF);

	//we're done.
	return true;
}

/** 
 * @brief Test that a signal is configured correctly
 *
 * The pin is set correctly if it's:
 *	Configured as an output
 * 	The pin is marked as allocated
 *	The signal has a pin assigned to it
 */
bool jtag_TestSignalConfigSet()
{
	const unsigned int pin_num = 7;
	jtag_Init();	//Setup

	jtag_Cfg(JTAG_SIGNAL_TCK, pin_num);	//configure the pin

	//to set an output MODER should be set to 01 for the pin.
	ASSERT((GPIOD_MODER & (3 << (pin_num * 2)) ==  (1 << (pin_num * 2))), "Mode set incorrectly: %08X, should be %08X.", GPIOD_MODER & (3 << (pin_num * 2)), (1 << (pin_num * 2)));

	// has the pin been marked as assigned
	ASSERT(((jtag_PinUsage & (1 << pin_num)) == (1 << pin_num)), "Pin wasn't marked as allocated");

	// has the correct pin been assigned to the signal
	ASSERT((jtag_Signals[JTAG_SIGNAL_TCK] == pin_num), "Pin number not assigned to signal");

	return true;
}

/**
 * @brief Test that a input signal is configured correctly
 *
 * As jtag_TestSignalConfigSet, but it should remain as an input.
 */
bool jtag_TestSignalConfigSetInput()
{
	const unsigned int pin_num = 8;
	jtag_Init();	//Setup

	jtag_Cfg(JTAG_SIGNAL_TDO, pin_num);	//configure the pin

	//to set an input MODER should be set to 00 for the pin.
	ASSERT(((GPIOD_MODER & (3 << (pin_num * 2))) ==  0), "Mode set incorrectly: %08X, should be %08X.", GPIOD_MODER & (3 << (pin_num * 2)), 0);

	// has the pin been marked as assigned
	ASSERT(((jtag_PinUsage & (1 << pin_num)) == (1 << pin_num)), "Pin wasn't marked as allocated");

	// has the correct pin been assigned to the signal
	ASSERT((jtag_Signals[JTAG_SIGNAL_TDO] == pin_num), "Pin number not assigned to signal");

	return true;
}

/**
 * @brief Test that an invalid pin is handled correctly
 *
 * Nothing should change when the provided pin for a signal is outside of the 
 * allowed range. Which currently is pins 0 - 15.
 */
bool jtag_TestSignalConfigSetInvalid()
{
	const unsigned int pin_num = 22;
	unsigned int old_PinUsage;
	uint32_t old_MODER;

	jtag_Init();	//Setup

	jtag_Cfg(JTAG_SIGNAL_TMS, pin_num);	//configure the pin

	//to set an input MODER should be set to 00 for the pin.
	ASSERT((GPIOD_MODER ==  0), "Mode set incorrectly: %08X, should be %08X.", GPIOD_MODER, 0);

	// has the pin been marked as assigned
	ASSERT((jtag_PinUsage == 0), "Pin was marked as allocated");

	// has the correct pin been assigned to the signal
	ASSERT((jtag_Signals[JTAG_SIGNAL_TMS] == JTAG_SIGNAL_NOT_ALLOCATED), "Pin number was assigned to a signal");

	jtag_Cfg(JTAG_SIGNAL_TMS, 7);	//configure the pin properly

	old_PinUsage = jtag_PinUsage;
	old_MODER = GPIOD_MODER;

	jtag_Cfg(JTAG_SIGNAL_TMS, pin_num);	//configure the pin improperly

	ASSERT((old_PinUsage == jtag_PinUsage), "Pin Usage changed. Was %08X, is %08X", old_PinUsage, jtag_PinUsage);
	ASSERT((old_MODER == GPIOD_MODER), "MODER changed. Was %08X, is %08X", old_MODER, GPIOD_MODER);
	ASSERT((jtag_Signals[JTAG_SIGNAL_TMS] == 7), "Pin number assigned changes. Was 7, is %i", jtag_Signals[JTAG_SIGNAL_TMS]);

	return true;
}

/**
 * @brief Test that a signal is deconfigured correctly
 *
 * The pin should return to being an input, the signal set back to 
 * JTAG_PIN_NOT_ALLOCATED and the pin marked as being free.
 * The output state of the pin should also reset to low.
 */
bool jtag_TestSignalConfigUnSet()
{
	const unsigned int pin_num = 5;
	//setup
	jtag_Init();	
	jtag_Cfg(JTAG_SIGNAL_TCK, pin_num);
	jtag_Set(JTAG_SIGNAL_TCK, true);

	//test
	jtag_Cfg(JTAG_SIGNAL_TCK, JTAG_SIGNAL_NOT_ALLOCATED);
	
	//check
	//to set an input MODER should be set to 00 for the pin.
	ASSERT(((GPIOD_MODER & (3 << (pin_num * 2))) ==  0), "Mode set incorrectly: %08X, should be %08X.", GPIOD_MODER & (3 << (pin_num * 2)), 0);

	// has the pin been marked as assigned
	ASSERT(((jtag_PinUsage & (1 << pin_num)) == 0), "Pin wasn't de-allocated");

	// has the correct pin been assigned to the signal
	ASSERT((jtag_Signals[JTAG_SIGNAL_TCK] == JTAG_SIGNAL_NOT_ALLOCATED), "Pin number wasn't removed from the signal");

	//is the output low
	ASSERT((GPIOD_BSRR == (1 << pin_num)), "Pin wasn't reset to low");
	return true;
}

/**
 * @brief Test that no changes happen if a pin is already in use
 *
 * Nothing should happen if a signal is set to a pin that is alreay
 * being used. 
 */
bool jtag_TestSignalConfigAlreadySetPin()
{
	const unsigned int pin_num = 5;
	unsigned int old_PinUsage;
	uint32_t old_MODER;
	uint32_t old_BSRR;

	//setup
	jtag_Init();	
	jtag_Cfg(JTAG_SIGNAL_TCK, pin_num);
	jtag_Set(JTAG_SIGNAL_TCK, true);

	old_MODER = GPIOD_MODER;
	old_BSRR = GPIOD_BSRR;
	old_PinUsage = jtag_PinUsage;

	//test
	jtag_Cfg(JTAG_SIGNAL_TMS, pin_num);

	//check
	ASSERT((old_PinUsage == jtag_PinUsage), "Pin Usage changed. Was %08X, is %08X", old_PinUsage, jtag_PinUsage);
	ASSERT((old_MODER == GPIOD_MODER), "MODER changed. Was %08X, is %08X", old_MODER, GPIOD_MODER);
	ASSERT((old_BSRR == GPIOD_BSRR), "BSRR changed. Was %08X, is %08X", old_BSRR, GPIOD_BSRR);
	ASSERT((jtag_Signals[JTAG_SIGNAL_TMS] == JTAG_SIGNAL_NOT_ALLOCATED), "Pin number was assigned: %i", jtag_Signals[JTAG_SIGNAL_TMS]);
	ASSERT((jtag_Signals[JTAG_SIGNAL_TCK] == pin_num), "Assigned pin number changed. %i, should be %i", jtag_Signals[JTAG_SIGNAL_TCK], pin_num);
	return true;
}

/**
 * @brief Test that a signal is reconfigured correctly.
 *
 * The old pin the signal was on should be deallocated correctly and 
 * the new one configured properly.
 * See the jtag_TestSignalConfigSet and jtag_TestSignalConfigUnSet tests.
 */
bool jtag_TestSignalConfigAlreadySetSig()
{
	const unsigned int pin_num = 5;
	const unsigned int old_pin_num = 7;

	//setup
	jtag_Init();	
	jtag_Cfg(JTAG_SIGNAL_TCK, old_pin_num);
	jtag_Set(JTAG_SIGNAL_TCK, true);

	//test
	jtag_Cfg(JTAG_SIGNAL_TCK, pin_num);

	//check old deconfiguration
	ASSERT(((jtag_PinUsage & (1 << old_pin_num)) == 0), "Old pin wasn't un-assigned");
	ASSERT(((GPIOD_MODER & (3 << (old_pin_num * 2))) ==  0), "Mode set incorrectly: %08X, should be %08X.", GPIOD_MODER & (3 << (old_pin_num * 2)), 0);
	ASSERT((GPIOD_BSRR == (1 << old_pin_num)), "Pin wasn't reset to low");
	
	//check new configuration
	ASSERT(((jtag_PinUsage & (1 << pin_num)) != 0), "New pin wasn't assigned");
	ASSERT(((GPIOD_MODER & (3 << (pin_num * 2))) ==  (1 << (pin_num * 2))), "Mode set incorrectly: %08X, should be %08X.", GPIOD_MODER & (3 << (old_pin_num * 2)), (1 << (pin_num * 2)));
	ASSERT((jtag_Signals[JTAG_SIGNAL_TCK] == pin_num), "New pin number was assigned to the signal");

	return true;
}
