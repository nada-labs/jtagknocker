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
#include "tchain.h"
#include <stdint.h>

//Mock out the functions we're interested in.
#define jtag_Set		chain_Mock_jtag_Set
#define jtag_Get		chain_Mock_jtag_Get
#define jtag_Clock		chain_Mock_jtag_Clock
#define jtagTAP_SetState	chain_Mock_jtagTAP_SetState

#include "../source/jtag.h"
#include "../source/jtagtap.h"

static unsigned int chain_ir_len;	///< Length of the fake chain instruction register, in bits
static unsigned int chain_dr_len;	///< Length of the fake chain data register, in bits
static char *chain_ir;			///< Fake chain IR
static char *chain_dr; 			///< Fake chain DR
static unsigned int chain_reg_head;	///< Fake chain head index

static jtagTAP_TAPState TAPState;	///< Fake chain TAP state
static bool TDO;			///< Fake chain TDO state
static bool TDI;			///< Fake chain TDI state
static bool chain_reset;		///< Was the chain reset
static int usage_error;			///< did a usage error occur?

/**
 * @brief Test the fake chain implementation
 *
 * This needs to be tested first as the other test rely on it operating
 * correctly. This is essentially a fake TAP controller and several
 * instruction and data registers joined together.
 */
bool chain_TestFakeChain()
{
	//data is stored in the fake registers in little endian format.
	//i.e. the LSB of the first byte will be shifted out. It's also a
	//circular buffer of length chain_xx_len.
	char ir[] = { 0x12, 0xFF };
	char dr[] = { 0x00, 0xFF, 0x00, 0xAB, 0x93, 0xFF };

	char expect_ir[] = { 0x01, 0xFC };	//expected ir value after the shift
	char dr_sample[5];				//storage for DR output
	unsigned int count;
	
	//set up the fake chain
	chain_ir_len = 10;
	chain_ir = ir;	
	chain_dr_len = 32;
	chain_dr = dr;
	chain_reset = false;
	usage_error = 0;
	
	chain_Mock_jtagTAP_SetState(JTAGTAP_STATE_RESET);
	ASSERT(chain_reset, "Chain didn't reset");

	//Lets shift some data through the IR register
	//shifting chain_ir_len bits through
	chain_Mock_jtagTAP_SetState(JTAGTAP_STATE_IR_SHIFT);
	chain_Mock_jtag_Set(JTAG_SIGNAL_TDI, true);
	chain_Mock_jtag_Clock();
	chain_Mock_jtag_Set(JTAG_SIGNAL_TDI, false);
	
	for(count = 1; count < chain_ir_len; ++count)
	{
		chain_Mock_jtag_Clock();
	}

	ASSERT((memcmp(ir, expect_ir, 2) == 0), "IR End data didn't match: %04X %04X", ir, expect_ir);

	//Lets move to DR and sample it's output to test it's correct.
	chain_Mock_jtagTAP_SetState(JTAGTAP_STATE_DR_SHIFT);
	for(count = 0; count < chain_dr_len; ++count)
	{
		unsigned int index_byte = count/8;
		unsigned int index_bit = count % 8;

		//get the TDO signal state and store it
		unsigned int res = 0;
		
		chain_Mock_jtag_Clock();	//shift from TDI to TDO
	
		if(chain_Mock_jtag_Get(JTAG_SIGNAL_TDO))
		{
			res = 1;
		}

		dr_sample[index_byte] &= ~(1 << index_bit);	//mask the bit off
		dr_sample[index_byte] |= (res << index_bit);	//store the state
	}

	ASSERT(*((uint32_t* )dr_sample) == 0xAB00FF00, "DR output didn't match: %08X %08X", dr_sample, 0xAB00FF00);

	//reset the index and test bit wrapping is correct
	chain_reg_head = 0;
	for(count = 0; count < 16; ++count)
	{
		chain_Mock_jtag_Set(JTAG_SIGNAL_TDI, true);		
		chain_Mock_jtag_Clock();	//shift from TDI to TDO
		chain_Mock_jtag_Set(JTAG_SIGNAL_TDI, false);		
		chain_Mock_jtag_Clock();	//shift from TDI to TDO
	}
	ASSERT(*((uint32_t* )dr) == 0x55555555, "DR didn't match: %08X %08X", dr, 0x55555555);
	
	chain_dr_len = 33;	//increase the bit count, now the bit positions should shift by one
				//when it wraps.
	chain_reg_head = 0;
	for(count = 0; count < 33; ++count)	//run through twice to check the bit wrap
	{
		chain_Mock_jtag_Set(JTAG_SIGNAL_TDI, true);		
		chain_Mock_jtag_Clock();	//shift from TDI to TDO
		chain_Mock_jtag_Set(JTAG_SIGNAL_TDI, false);		
		chain_Mock_jtag_Clock();	//shift from TDI to TDO
	}
	ASSERT(*((uint32_t* )dr) == 0xAAAAAAAA, "DR didn't match: %08X %08X", dr, 0xAAAAAAAA);
	ASSERT(usage_error == 0, "Usage error: %i", usage_error);	//show any errors
	return true;
}

/**
 * @brief Fake the clock signal
 *
 * All the important stuff happens on a low to high clock transition, perform
 * data shifting as needed.
 *
 * Data is shifted from TDI towards TDO at every rising edge of the clock in
 * shift_ir or shift_dr modes. This is the only part of the TAP we are 
 * simulating here.
 */
void chain_Mock_jtag_Clock()
{
	char *chain;
	unsigned index_byte, index_bit, chain_len;

	//select the right chain to use
	if(TAPState == JTAGTAP_STATE_IR_SHIFT)
	{
		chain = chain_ir;
		chain_len = chain_ir_len;
	} 
	else if (TAPState == JTAGTAP_STATE_DR_SHIFT)
	{
		chain = chain_dr;
		chain_len = chain_dr_len;
	}
	else
	{
		usage_error = 4;	//clock called in a state is shouldn't have been
		return; //nothing to do
	}

	//perform the data shift and replacement
	index_byte = chain_reg_head/8;
	index_bit = chain_reg_head % 8;

	TDO = ((chain[index_byte] & (1<< index_bit)) != 0);	//set the state of tdo
	chain[index_byte] &= ~(1<< index_bit);			//mask off the bit
	if(TDI)
	{
		chain[index_byte] |= (1<< index_bit);		//store TDI
	}
	chain_reg_head = (chain_reg_head+1) % chain_len;	//increment and wrap bit index
}

/**
 * @brief Fake state transitions
 *
 * We don't need to walk the state table, just reset some counters based on 
 * the state chosen.
 */
void chain_Mock_jtagTAP_SetState(jtagTAP_TAPState target)
{
	if((target == JTAGTAP_STATE_IR_SHIFT) || (target == JTAGTAP_STATE_DR_SHIFT))
	{
		//reset the chain index
		chain_reg_head = 0;
	}
	else 
	{
		if(target == JTAGTAP_STATE_RESET)
		{
			//flag that reset was called
			chain_reset = true;
		}
		else
		{
			usage_error = 1;
		}
	}
	TAPState = target;
}

/**
 * @brief Get the faked TDO
 */
bool chain_Mock_jtag_Get(jtag_Signal sig)
{
	if(sig == JTAG_SIGNAL_TDO)
	{
		return TDO;
	}
	usage_error = 2;
	return false;
}

/**
 * @brief Set the fake TDI
 */
void chain_Mock_jtag_Set(jtag_Signal sig, bool state)
{
	if(sig == JTAG_SIGNAL_TDI)
	{
		TDI = state;
	}
	else
	{
		usage_error = 3;
	}
}
