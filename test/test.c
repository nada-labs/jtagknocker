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

#include "test.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <strings.h>
#include <stdarg.h>

//add test include files here
#include "tjtag.h"
#include "tjtagtap.h"
#include "tchain.h"
#include "tmessage.h"

#define MESSAGE_WRITE_BUFFER	128

typedef bool (*test_tFunc)(void);

static const test_tFunc test_Functions[] = 
{
	//Add test functions here

	//JTAG module tests
	jtag_TestInitSignalAlloc,
	jtag_TestInitRegisterSetup,
	jtag_TestSignalConfigSetInput,
	jtag_TestSignalConfigSetInvalid,
	jtag_TestSignalConfigUnSet,
	jtag_TestSignalConfigAlreadySetPin,
	jtag_TestSignalConfigAlreadySetSig,
	jtag_TestSetAndClear,
	jtag_TestSetUnallocatedSignal,
	jtag_TestSetInput,
	jtag_TestGet,
	jtag_TestGetUnallocated,
	jtag_TestIsAllocated,

	//JTAG TAP tests
	jtagTAP_TestInitilization,
	jtagTAP_TestTxFromUnknown,
	jtagTAP_TestReset,

	//Chain tests
	chain_TestFakeChain,
	chain_TestDeviceCount,
	chain_TestChainIRLength,
	chain_TestResetDRIDCode,
	chain_TestResetDRIDCodes,

	//Message tests
	message_TestInitialization,
	message_TestSetLevel,
	message_TestMessages,
};

#define TESTS (sizeof(test_Functions)/sizeof(test_tFunc))	///< Number of functions in the test

/**
 * @brief Entrypoint for the tests
 *
 */
void main()
{
	unsigned int tests_passed = 0, tests_failed = 0;
	unsigned int index;

	//Setup the system
	rcc_clock_setup_hsi(&hsi_8mhz[CLOCK_64MHZ]);	
	//UART on PA2 (TX) and PA3 (RX) @ 115200,8,N,1	
	rcc_periph_clock_enable(RCC_USART2);
	rcc_periph_clock_enable(RCC_GPIOA);

	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2 | GPIO3);
	gpio_set_af(GPIOA, GPIO_AF7, GPIO2| GPIO3);

	usart_set_baudrate(USART2, 115200);
	usart_set_databits(USART2, 8);
	usart_set_stopbits(USART2, USART_STOPBITS_1);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_mode(USART2, USART_MODE_TX);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
	usart_enable(USART2);

	// Run the tests
	test_Write("Running Tests...\r\n");

	for(index = 0; index < TESTS; ++index)
	{
		bool result = test_Functions[index]();
		if(result)
		{
			++tests_passed;
		}
		else
		{
			++tests_failed;
		}
	}

	test_Write("Done.\r\n%i tests run, %i passed, %i failed.\r\n", TESTS, tests_passed, tests_failed);

	while(true);		
}

/**
 * @brief Output messages to the serial port
 */
void test_Write(const char *fmt, ...)
{
	char buffer[MESSAGE_WRITE_BUFFER];
	const char *p = buffer;
	int n;
	int count;
	va_list args;

	va_start(args, fmt);
	n = vsnprintf(buffer, MESSAGE_WRITE_BUFFER, fmt, args);
	if((n > 0) && (n < MESSAGE_WRITE_BUFFER))
	{
		for(count = 0; count < n; ++count)
		{
			usart_send_blocking(USART2, *p++);
		}
	}
	va_end(args);
}

//Functions to keep newlib happy
void *_sbrk(int incr)
{
	return((void*)(-1));
}

void _exit(int v)
{
	test_Write("\r\n_exit(%i) called. Halting\r\n", v);
	while(true);
}
