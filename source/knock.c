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
#include "jtag.h"
#include "jtagtap.h"
#include "knock.h"
#include "serial.h"
#include "chain.h"
#include <stdint.h>

#include <libopencm3/stm32/gpio.h>	//for IO port access

#define KNOCK_RESULTS		(1024)		///< Number of results to store per run (max)
#define KNOCK_UNCHANGED		(48)		///< Number of results to store for unchanging inputs, has to be longer than an ID CODE

static uint16_t knock_ScanIDCode();
static void knock_ScanIDCodeTDI(unsigned int tck, unsigned int tms, uint16_t pins);
static void knock_ScanIR(unsigned int tck, unsigned int tms);

static uint16_t knock_Result[KNOCK_RESULTS];
static unsigned int knock_Results;

static const unsigned int knock_PinCount = 4;
static const unsigned int knock_IRShiftCount = 100;
/**
 * @brief Attempts to determine if a device is attached via JTAG
 *
 * The JTAG pins need to have been previously defined through jtag_Cfg()
 *
 * For the provided pin combination, this function will reset the tap and scan
 * out the data register, either getting an ID CODE, a BYPASS or garbage. It
 * will attempt to analyse the results to see if it's a valid result.
 *
 * @retval true A valid combination was detected for the specified TCK and TMS
 */
static uint16_t knock_ScanIDCode()
{
	unsigned int count;
	int unchanged_count = -1;	//the first time through always results in a unchanged count
	uint16_t data, data_changed = 0x0000;
	uint16_t prev_data;
	uint16_t data_interesting = 0x0000;

	jtagTAP_SetState(JTAGTAP_STATE_UNKNOWN);
	jtagTAP_SetState(JTAGTAP_STATE_DR_SHIFT);

	prev_data = GPIOD_IDR;
	for(count = 0; count < KNOCK_RESULTS; ++count)
	{
		data = GPIOD_IDR;
		data_changed = data ^ prev_data;
		data_interesting |= data_changed;

		prev_data = data;

		knock_Result[count] = data;

		jtag_Clock();

		if(data_changed == 0)
		{
			if(++unchanged_count == KNOCK_UNCHANGED)
			{
				break;
			}
		}
		else
		{
			unchanged_count = 0;	//something changed, reset the count.
		}
	}
	knock_Results = count;

	if(data_interesting != 0)
	{
		//a line changed state at least once, lets inspect
		unsigned int bit = 0;
		for(bit = 0; bit < knock_PinCount; ++bit)
		{
			if(((data_interesting >> bit) & 0x01) == 1)
			{
				//this line changed
				unsigned int index;

				data_interesting &= ~(1 << bit);	//mask off this bit, until it proves itself

				for(index = 0; index < count; ++index)
				{
					if((((knock_Result[index] >> bit) & 0x01) == 1) && (count - index >= 32))
					{
						//this could be a potential ID CODE, get it
						uint32_t idcode = 0x80000000;
						unsigned int code_end = index + 32;
						while(++index < code_end)
						{
							idcode >>= 1;
							idcode |= ((knock_Result[index] >> bit) & 0x01) << 31;
						}
						if((idcode != 0xFFFFFFFF))
						{
							data_interesting |= (1 << bit);		//this is interesting, keep it
						}
						--index; //fixup
					}
				}

			}
		}
	}
	return data_interesting;
}

/**
 * @brief Try to find TDI
 *
 * Based on the scan results and a list of interesting pins, which could be
 * TDO, try and find TDI.
 *
 * The TAP is currently in JTAGTAP_STATE_DR_SHIFT and the shift registers are
 * full of whatever TDI is set to.
 *
 * @param[in] tck The pin that TCK is on.
 * @param[in] tms The pin that TMS is on.
 * @param[in] pins A bitmask of potential TDOs.
 *
 */
static void knock_ScanIDCodeTDI(unsigned int tck, unsigned int tms, uint16_t pins)
{
	uint16_t tdi_state = knock_Result[knock_Results - 1];
	unsigned int tdo;

	for(tdo = 0; tdo < knock_PinCount; ++tdo)
	{
		if(((pins >> tdo) & 0x01) == 1)
		{
			unsigned int tdi;

			//look for pins that match the value above
			for(tdi = 0; tdi < knock_PinCount; ++tdi)
			{
				if((tdi != tck) && (tdi != tms) && (tdi != tdo))
				{
					//we aren't already using this pin
					unsigned int clocks, changes = 0;
					unsigned int prev_tdo_val = (GPIOD_IDR & (1 << tdo));

					jtag_Cfg(JTAG_PIN_TDI, tdi);
					jtag_Set(JTAG_PIN_TDI, ((tdi_state >> tdi) & 1) == 0);	//toggle the TDI pin
					
					for(clocks = 0; clocks < knock_Results; ++clocks)
					{
						unsigned int tdo_val = (GPIOD_IDR & (1 << tdo));
						jtag_Clock();
							
						if((tdo_val ^ prev_tdo_val) != 0)
						{
							++changes;
						}
						prev_tdo_val = tdo_val;
					}

					if(changes == 1)
					{
						serial_Write("[!] Potential Chain: TCK: %i TMS: %i TDO: %i TDI: %i\r\n", tck, tms, tdo, tdi);
						jtag_Cfg(JTAG_PIN_TDO, tdo);
						chain_Detect();
					}

					//reset the pin state and clock again, undoing what we just did
					jtag_Set(JTAG_PIN_TDI, ((tdi_state >> tdi) & 1) == 1);	//toggle the TDI pin
					for(clocks = 0; clocks < knock_Results; ++clocks)
					{
						jtag_Clock();	
					}
					jtag_Cfg(JTAG_PIN_TDI, JTAG_PIN_NOT_ALLOCATED);
					jtag_Cfg(JTAG_PIN_TDO, JTAG_PIN_NOT_ALLOCATED);
				}
			}

		}
	}
}

/**
 * @brief Try and find a JTAG chain
 *
 */
void knock_Knock()
{
	unsigned int tck, tms;

	serial_Write("JTAG Knocker\r\n");

	for(tck = 0; tck < knock_PinCount; ++tck)
	{
		for(tms = 0; tms < knock_PinCount; ++tms)
		{
			if(tck != tms)
			{
				uint16_t pins;
				//reset the TAP for this test
				jtag_Init();
				jtagTAP_SetState(JTAGTAP_STATE_UNKNOWN);

				jtag_Cfg(JTAG_PIN_TCK, tck);
				jtag_Cfg(JTAG_PIN_TMS, tms);
				pins = knock_ScanIDCode();
				if(pins != 0)
				{
					//we found what looks like an id code on at least one of the lines
					//see if we can recover TDI
					knock_ScanIDCodeTDI(tck, tms, pins);
				} 

				knock_ScanIR(tck, tms);
			}
		}
	}	
	serial_Write("Done.\r\n");
}

/**
 * @brief Scan for JTAG ports by looking for a IR register
 *
 * @param[in] tck The pin TCK is on
 * @param[in] tms The pin TMS is on
 */
void knock_ScanIR(unsigned int tck, unsigned int tms)
{
	unsigned int tdi, tdo;
	unsigned int count;

	for(tdi = 0; tdi < knock_PinCount; ++tdi)
	{
		if((tdi != tck) && (tdi !=tms))
		{
			uint16_t tdo_candidates;
			int tdo_change_clocks[16];
			jtag_Init();
			jtagTAP_SetState(JTAGTAP_STATE_UNKNOWN);

			jtag_Cfg(JTAG_PIN_TCK, tck);
			jtag_Cfg(JTAG_PIN_TMS, tms);

			//we can use this pin
			jtag_Cfg(JTAG_PIN_TDI, tdi);
			jtag_Set(JTAG_PIN_TDI, true);	//set the pin to a known state

			//put the JTAG TAP into a known state
			jtagTAP_SetState(JTAGTAP_STATE_UNKNOWN);
			jtagTAP_SetState(JTAGTAP_STATE_IR_SHIFT);

			for(count = 0; count < knock_IRShiftCount; ++count)
			{
				jtag_Clock();
			}
			tdo_candidates = GPIOD_IDR;	//any pin which is set here and changes to 
							//0 once and stays there is probably TDO
	
			jtag_Set(JTAG_PIN_TDI, false);
			for(count = 0; count < 16; ++count)
			{
				tdo_change_clocks[count] = 0;
			}

			for(count = 1; count < knock_IRShiftCount; ++count)
			{
				uint16_t tdo_sample ;
				jtag_Clock();

				tdo_sample = GPIOD_IDR;

				for(tdo = 0; tdo < knock_PinCount; ++tdo)
				{
					//check if this is a candidate pin and not in use
					if((tdo != tck) && (tdo != tms) && (tdo != tdi) && ((tdo_candidates & (1 << tdo)) != 0))
					{
						if((tdo_sample & (1 << tdo)) == 0)
						{
							//the pin went low, this is good
							if(tdo_change_clocks[tdo] == 0)
							{
								tdo_change_clocks[tdo] = count;
							}
						}
						else if(tdo_change_clocks[tdo] > 0)
						{
							//it had gone low, but went high. damn.
							tdo_change_clocks[tdo] = -1;
						}
					} 
				}
			} 

			for(tdo = 0; tdo < knock_PinCount; ++tdo)
			{
				if(tdo_change_clocks[tdo] >= 2)
				{
					serial_Write("[!] Potential Chain: TCK: %i TMS: %i TDO: %i TDI: %i\r\n", tck, tms, tdo, tdi);
					jtag_Cfg(JTAG_PIN_TDO, tdo);
					chain_Detect();
					jtag_Cfg(JTAG_PIN_TDO, JTAG_PIN_NOT_ALLOCATED);
				}
			}

			//we may have found a chain, put it back into bypass
			//LX4F120HQ5R locks up with an IR full of 0
			jtag_Set(JTAG_PIN_TDI, true);	//set the pin to a known state
			for(count = 0; count < knock_IRShiftCount; ++count)
			{
				jtag_Clock();
			}

		}
	}
}
