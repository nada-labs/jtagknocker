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

#include "chain.h"
#include "jtag.h"
#include "jtagtap.h"
#include "serial.h"

#include <stdint.h>
#include <libopencm3/stm32/gpio.h>

// Module local variables
static unsigned int chain_IRLength;
static unsigned int chain_Devices;

// Module local functions
static bool chain_findDevices();
static bool chain_findIRLength();

/**
 * @brief Initializes the chain module
 *
 */
void chain_Init()
{
	chain_IRLength = 0;
	chain_Devices = 0;
}

/**
 * @brief Find the total IR length of the chain
 *
 */
static bool chain_findIRLength()
{
	bool success = false;
	unsigned int count;

	//set TDI high and clock it through the chain in IR_SHIFT
	jtag_Set(JTAG_SIGNAL_TDI, true);
	jtagTAP_SetState(JTAGTAP_STATE_IR_SHIFT);
	
	for(count = 0; count < CHAIN_MAX_IRLEN; ++count)
	{
		jtag_Clock();
	}

	//now set TDI low and count the number of clocks until TDO goes low
	jtag_Set(JTAG_SIGNAL_TDI, false);

	for(count = 1; count < CHAIN_MAX_IRLEN; ++count)
	{
		jtag_Clock();

		if(!jtag_Get(JTAG_SIGNAL_TDO))
		{
			//went to 0
			chain_IRLength = count;
			success = true;
			break;
		}
	}
	return success;
}

/**
 * @brief Determines the number of devices in the chain
 *
 * @retval true Devices were found
 */
static bool chain_findDevices()
{
	bool success = false;
	unsigned int count;

	//get the TAP into the right state and set TDI high
	jtagTAP_SetState(JTAGTAP_STATE_IR_SHIFT);
	jtag_Set(JTAG_SIGNAL_TDI, true);
 
	//load BYPASS into every device in the chain (TDI = all ones)
	for(count = 0; count < chain_IRLength; ++count)
	{
		jtag_Clock();
	}

	jtagTAP_SetState(JTAGTAP_STATE_DR_SHIFT);

	for(count = 0; count < CHAIN_MAX_DEVICES; ++count)
	{
		if(jtag_Get(JTAG_SIGNAL_TDO))
		{
			//went to 1, all bypass registers have been emptied
			chain_Devices = count;
			success = true;
			break;
		}
		jtag_Clock();
	}
	return success;
}

/**
 *@brief Detects the devices on the chain
 *
 */
bool chain_Detect()
{
	bool success = false;

	//get some of the chain information
	if(chain_findIRLength() && chain_findDevices())
	{
		unsigned int count;
		unsigned int device = 0;

		//reset the TAP and hope the devices support ID Code
		jtagTAP_SetState(JTAGTAP_STATE_RESET);
		jtagTAP_SetState(JTAGTAP_STATE_DR_SHIFT);
		jtag_Set(JTAG_SIGNAL_TDI, true);

		serial_Write("[+] %i Device(s) found, with total IR Length of %i\r\n", chain_Devices, chain_IRLength);

		while(device != chain_Devices)
		{
			jtag_Clock();

			if(jtag_Get(JTAG_SIGNAL_TDO))
			{
				//start of an ID Code
				uint32_t idcode = 0x80000000;
				for(count = 0; count < 31; ++count)
				{	
					idcode >>= 1;
					idcode |= (jtag_Get(JTAG_SIGNAL_TDO) ? 0x80000000 : 0);
					jtag_Clock();
				}
				if(idcode == 0xFFFFFFFF)
				{
					break;	//invalid code
				}
				else
				{
					serial_Write("[+]  Device %i - ID Code %08X\r\n", device +1, idcode);
					++device;
				}
			}
			else
			{
				//this device is in BYPASS
				serial_Write("[+]  Device %i - BYPASS\r\n", device +1);
				++device;
			}

		} 		

	}

	return success;
}
