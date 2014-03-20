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
#include "tchain.h"
#include <stdint.h>

//Mock out the functions we're interested in.
#define jtag_Set		chain_Mock_jtag_Set
#define jtag_Get		chain_Mock_jtag_Get
#define jtag_Clock		chain_Mock_jtag_Clock
#define jtagTAP_SetState	chain_Mock_jtagTAP_SetState
#define serial_Write		chain_Mock_serial_Write		//get rid of a unnneded function

#include "../source/jtag.h"
#include "../source/jtagtap.h"
#include "../source/chain.c"

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
		
		
		if(chain_Mock_jtag_Get(JTAG_SIGNAL_TDO))
		{
			res = 1;
		}

		dr_sample[index_byte] &= ~(1 << index_bit);	//mask the bit off
		dr_sample[index_byte] |= (res << index_bit);	//store the state

		chain_Mock_jtag_Clock();	//shift from TDI to TDO
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
 * All the important stuff happens on clock transitions, perform data shifting
 * as needed.
 *
 * The contents of the selected register (instruction or data) are shifted out
 * of TDO on the falling edge of TCK. Values presented at TDI are clocked into
 * the selected register (instruction or test data) on a rising edge of TCK.
 * 
 * When clock is called the line is already in a low state and the first bit
 * of the selected register has been clocked out.
 */
void chain_Mock_jtag_Clock()
{
	char *chain;
	unsigned index_byte, index_bit, chain_len;

	//select the right chain to use
	if(TAPState == JTAGTAP_STATE_IR_SHIFT)
	{
		if(chain_ir != NULL)
		{
			chain = chain_ir;
			chain_len = chain_ir_len;
		}
		else
		{
			usage_error = 5; //IR was selected when no IR was present (ie: it shouldn't have been used)
		}
	} 
	else if (TAPState == JTAGTAP_STATE_DR_SHIFT)
	{
		if(chain_dr != NULL)
		{
			chain = chain_dr;
			chain_len = chain_dr_len;
		}
		else
		{
			usage_error = 6; //DR was selected when it shouldn't have been
		}
	}
	else
	{
		usage_error = 4;	//clock called in a state is shouldn't have been
		return; //nothing to do
	}

	//perform the data shift and replacement

	//high CLOCK transition
	index_byte = chain_reg_head/8;
	index_bit = chain_reg_head % 8;
	chain[index_byte] &= ~(1<< index_bit);			//mask off the bit
	if(TDI)
	{
		chain[index_byte] |= (1<< index_bit);		//store TDI
	}
	
	//adjust the index
	chain_reg_head = (chain_reg_head+1) % chain_len;	//increment and wrap bit index

	//LOW clock transition
	index_byte = chain_reg_head/8;
	index_bit = chain_reg_head % 8;
	TDO = ((chain[index_byte] & (1<< index_bit)) != 0);	//set the state of tdo
}

/**
 * @brief Fake state transitions
 *
 * We don't need to walk the state table, just reset some counters based on 
 * the state chosen.
 * Because the clock ends low after the state transition, the first bit of the
 * selected register needs to be clocked out.
 */
void chain_Mock_jtagTAP_SetState(jtagTAP_TAPState target)
{
	if(target == JTAGTAP_STATE_IR_SHIFT)
	{
		//reset the chain index
		chain_reg_head = 0;
		//clock out first bit of ir, if present
		if(chain_ir != NULL)
		{
			TDO = ((chain_ir[0] & 1) == 1);
		}
		else
		{
			usage_error = 5;	//IR used when not meant to
		}
	}
	else if(target == JTAGTAP_STATE_DR_SHIFT)
	{
		//reset the chain index
		chain_reg_head = 0;
		//clock out first bit of dr, if present
		if(chain_dr != NULL)
		{
			TDO = ((chain_dr[0] & 1) == 1);
		}
		else
		{
			usage_error = 6;	//IR used when not meant to
		}
	}
	else if(target == JTAGTAP_STATE_RESET)
	{
		//flag that reset was called
		chain_reset = true;
	}
	else
	{
		usage_error = 1;
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

/**
 * @brief Test the device counting algorithm
 *
 * To determine the number of devices on the chain, the following algorithm
 * is used:
 *	Enter into bypass mode (IR = 0xFF....)
 *	Enter into shift_dr and fill with ones
 * 	Clock a 0 through and count the clocks until it appears
 *	DR can be left in whatever state
 * When the device is in BYPASS mode, the data register has a length of one,
 * so the devices on a chain is just the length of the data register. The DR
 * value in BYPASS is 0.
 */
bool chain_TestDeviceCount()
{
	char devicecount_ir[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};		//Up to 48 bits of IR
	char devicecount_dr[] = {0x00};	//Up to 8 bits of BYPASS data register	

	//fake chain setup
	chain_ir = devicecount_ir;
	chain_ir_len = 47;	//seems like a good length...
	chain_dr = devicecount_dr;
	chain_dr_len = 3;	//there are going to be 3 devices on this chain.
	usage_error = 0;

	//module setup and test
	chain_Devices = 99;
	chain_IRLength = 47;	//chain_findDevices needs to know the total IR length
	chain_findDevices();

	//check results
	ASSERT(memcmp(devicecount_ir, "\xFF\xFF\xFF\xFF\xFF\x7F", 6) == 0, "Not in BYPASS");
	ASSERT(chain_Devices == chain_dr_len, "Wrong number of devices found: %i, should be %i", chain_Devices, chain_dr_len);
	ASSERT(usage_error == 0, "Usage Error: %i", usage_error);
	return true;
}

/**
 * @brief Test the algorithm to determine the chain IR length
 *
 * Determining the chain IR length is similar to finding the number of devices
 * on the chain, but only IR is used. If more than one device is on the chain
 * this approach finds the sum of the device IR lengths. The IR should also be
 * left in the BYPASS instruction (filled with ones)
 */
bool chain_TestChainIRLength()
{
	char chainirlen_ir[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};		//Up to 48 bits of IR

	//fake chain setup
	chain_ir = chainirlen_ir;
	chain_ir_len = 47;	//seems like a good length...
	chain_dr = NULL;	//DR isn't used
	chain_dr_len = 0;	
	usage_error = 0;

	//module setup and test
	chain_IRLength = 12345;
	chain_findIRLength();

	//check results
	ASSERT(memcmp(chainirlen_ir, "\xFF\xFF\xFF\xFF\xFF\x7F", 6) == 0, "Not in BYPASS");
	ASSERT(chain_IRLength == chain_ir_len, "Wrong IR length found: %i, should be %i", chain_IRLength, chain_ir_len);
	ASSERT(usage_error == 0, "Usage Error: %i", usage_error);
	return true;
}

/**
 * @brief Test the function for finding an IDCODE from a reset device
 *
 * When a device enters RESET the instruction register is loaded with IDCODE
 * (if supported) or BYPASS. By shifting from RESET to DR_SHIFT we can read
 * out the ID code. ID Code is always 32 bits and BYPASS is always 1.
 *
 * The function under test assumes that the TAP has been reset and is in
 * DR_SHIFT state.
 */
bool chain_TestResetDRIDCode()
{
	//ID codes we want to test
	char IDTests_dr[][4] = {
		{ 0x77, 0x04, 0xA0, 0x4B },	//TI LX4F120H5QRF
		{ 0x09, 0x60, 0x94, 0x15 },	//ARM 946e Core in a Canon Digic 4 processor
		{ 0x00, 0x00, 0x00, 0x00 },	//BYPASS
		{ 0xDD, 0x20, 0x0B, 0x02 },	//Altera EP2C8
		{ 0xDD, 0x40, 0x81, 0x02 },	//Altera EP4CGX110
		{ 0x00, 0x00, 0x00, 0x00 },	//BYPASS
		{ 0x00, 0x00, 0x00, 0x00 },	//BYPASS
		{ 0x93, 0x20, 0xC2, 0x21 },	//Xilinx XCS500E
	};
	uint32_t IDCodes[] = { 0x4BA00477, 0x15946009, 0, 0x020B20DD, 0x028140DD, 0, 0, 0x21C22093 }; 
	const unsigned int tests = 8;
	unsigned int test = 0;

	chain_ir_len = 0;
	chain_ir = NULL;	//IR isn't used.
	usage_error = 0;

	for(test = 0; test < tests; ++test)
	{
		uint32_t IdCode;
		//set up the fake DR
		chain_dr = IDTests_dr[test];
		chain_dr_len = (IDTests_dr[test][0] == 0x00 ? 1 : 32);
	
		//Set the chain state so TDO is the right value
		chain_Mock_jtagTAP_SetState(JTAGTAP_STATE_DR_SHIFT);

		//test the function
		IdCode = chain_findIDCode();
		ASSERT(IdCode == IDCodes[test], "Test %i ID Code is incorrect: %08X, should be %08X", test, IdCode, IDCodes[test]);
	}
	ASSERT(usage_error == 0, "Usage Error: %i", usage_error);
	return true;
}	

/**
 * @brief Test the function for finding an IDCODEs from a reset device chain
 *
 * Same as the test above, but not resetting the chain in between. The results
 * should be the same but the input is exactly what would appear on the wire.
 */
bool chain_TestResetDRIDCodes()
{
	//ID codes we want to test
	char IDTests_dr[] = { 0x77, 0x04, 0xA0, 0x4B, 0x09, 0x60, 0x94, 0x15, 0xBA, 0x41, 0x16, 0x04, 0xBA, 0x81, 0x02, 0x05, 0x98, 0x04, 0x11, 0x0E, 0x01}; 
	uint32_t IDCodes[] = { 0x4BA00477, 0x15946009, 0, 0x020B20DD, 0x028140DD, 0, 0, 0x21C22093 }; 
	const unsigned int tests = 8;
	unsigned int test = 0;

	chain_ir_len = 0;
	chain_ir = NULL;	//IR isn't used.
	usage_error = 0;
	chain_dr = IDTests_dr;
	chain_dr_len = (32*5)+3;
	chain_Mock_jtagTAP_SetState(JTAGTAP_STATE_DR_SHIFT);

	for(test = 0; test < tests; ++test)
	{
		uint32_t IdCode;
		//test the function
		IdCode = chain_findIDCode();
		ASSERT(IdCode == IDCodes[test], "Test %i ID Code is incorrect: %08X, should be %08X", test, IdCode, IDCodes[test]);
	}
	ASSERT(usage_error == 0, "Usage Error: %i", usage_error);
	return true;
}

/**
 * NULL function to get rid of serial_Write linking
 */
int chain_Mock_serial_Write(const char *fmt, ...)
{

}
