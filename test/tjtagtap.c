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
#include "tjtagtap.h"
#include <stdint.h>

//rename the functions that we want to mock
//jtag.h (included by jtagtap.c) will prototype the functions for us.
#define jtag_Set	jtagTAP_Mock_jtag_Set
#define jtag_Clock	jtagTAP_Mock_jtag_Clock
#define jtag_IsAssigned	jtagTAP_Mock_jtag_IsAssigned

//include the file *source*
#include "../source/jtagtap.c"

static bool HasTRST = false;		///< is the TRST signal assigned. Used by jtagTAP_Mock_jtag_IsAssigned
static uint32_t TMSStateTx = 0;		///< A log of the TMS state every time jtag_Clock is called
static uint32_t TRSTChanged = 0;	///< Number of times TRST was set
static bool InvalidSignal = false;	///< Was an invalid signal set
static bool TMSStateCurrent = false;	///< The current state of TMS

/**
 * @brief Test that the TAP state is initialized correctly.
 *
 * The correct state to start off in is UNKNOWN
 */
bool jtagTAP_TestInitilization()
{
	TAPState = 123;	//set to some randomish value
	jtagTAP_Init();

	ASSERT(TAPState == JTAGTAP_STATE_UNKNOWN, "State not initialized correctly. %i should be %i", TAPState, JTAGTAP_STATE_UNKNOWN);

	return true;
}

/**
 * @brief Test the first state transition
 *
 * When transitioning from UNKNOWN to a state, the TAP should be reset.
 * If TRST is assigned, that signal should be used. Otherwise the TAP 
 * should be clocked 5 times with TMS high to enter into the RESET state.
 */
bool jtagTAP_TestTxFromUnknown()
{
	//Setup
	HasTRST = false;
	TMSStateTx = 0;
	TRSTChanged = 0;
	InvalidSignal = false;

	jtagTAP_Init();
	
	//Test
	jtagTAP_SetState(JTAGTAP_STATE_IDLE);

	//Check
	//Without TRST, TMS should be high for 5 clock cycles and then low for one.
	//The earliest TMS value appears in the higher bits of TMSState so the ending
	//value of TMS state should be 0x0000003E
	ASSERT(TMSStateTx == 0x3E, "Incorrect transitions. TMS values are %08X, should be %08X", TMSStateTx, 0x3E);
	ASSERT(TRSTChanged == 0, "TRST transitioned %i times. It shouldn't have", TRSTChanged);

	//re-setup
	HasTRST = true;
	TMSStateTx = 0xFFFFFFFF;
	TRSTChanged = 0;

	jtagTAP_Init();
	
	//Test
	jtagTAP_SetState(JTAGTAP_STATE_IDLE);

	//Check
	//With TRST, TMS should be low for one clock cycle. TRST should toggle low then high.
	//The ending value of TMS state should be 0xFFFFFFFE.
	ASSERT(TMSStateTx == 0xFFFFFFFE, "Incorrect transitions. TMS values are %08X, should be %08X", TMSStateTx, 0xFFFFFFFE);
	ASSERT(TRSTChanged == 2, "TRST transitioned %i times. It should have done it %i times", TRSTChanged, 2);

	//Check that an invalid signal wasn't set at some point
	ASSERT(!InvalidSignal, "An invalid signal was specified at some point");
	return true;
}

/**
 * @brief Test the RESET transitions
 *
 * When transitioning from a state to RESET, the TAP should walk the states 
 * and finish at reset. If TRST is assigned, that signal should be used and 
 * the state set as RESET with TMS high.
 */
bool jtagTAP_TestReset()
{
	//Setup
	HasTRST = false;
	TMSStateTx = 0;
	TRSTChanged = 0;
	InvalidSignal = false;

	TAPState = JTAGTAP_STATE_DR_SHIFT;
	jtagTAP_SetState(JTAGTAP_STATE_RESET);

	//Check
	//Without TRST, TMS should be high for 5 clock cycles.
	//The earliest TMS value appears in the higher bits of TMSState so the ending
	//value of TMS state should be 0x0000001F
	ASSERT(TMSStateTx == 0x1F, "Incorrect transitions. TMS values are %08X, should be %08X", TMSStateTx, 0x1F);
	ASSERT(TRSTChanged == 0, "TRST transitioned %i times. It shouldn't have", TRSTChanged);

	//re-setup
	HasTRST = true;
	TMSStateTx = 0x0FFFFFF0;
	TRSTChanged = 0;

	jtagTAP_Init();
	
	//Test
	TAPState = JTAGTAP_STATE_DR_SHIFT;
	jtagTAP_SetState(JTAGTAP_STATE_RESET);

	//Check
	//With TRST, TMS shouldn't change. TRST should toggle low then high.
	//TMS should be set high and the state should be RESET
	ASSERT(TMSStateTx == 0x0FFFFFF0, "Incorrect transitions. TMS values are %08X, should be %08X", TMSStateTx, 0x0FFFFFF0);
	ASSERT(TRSTChanged == 2, "TRST transitioned %i times. It should have done it %i times", TRSTChanged, 2);
	ASSERT(TAPState == JTAGTAP_STATE_RESET, "State isn't RESET: %i", TAPState);
	ASSERT(TMSStateCurrent, "Current state of TMS isn't high");

	//Check that an invalid signal wasn't set at some point
	ASSERT(!InvalidSignal, "An invalid signal was specified at some point");
	return true;
}

/**
 * @brief Mock function for setting the state of a signal
 *
 * We only care about TRST and TMS here, anything else is invalid.
 */
void jtagTAP_Mock_jtag_Set(jtag_Signal sig, bool val)
{
	if(sig == JTAG_SIGNAL_TMS)
	{
		TMSStateCurrent = val;
	}
	else if (sig == JTAG_SIGNAL_TRST)
	{
		++TRSTChanged;
		if(!HasTRST)
		{
			InvalidSignal = true;
		}
	}
	else
	{
		InvalidSignal = true;
	}
}

/**
 * @brief Mock the allocation signal for TRST
 */
bool jtagTAP_Mock_jtag_IsAllocated(jtag_Signal sig)
{
	bool retval = false;
	if(sig == JTAG_SIGNAL_TRST)
	{
		retval = HasTRST;
	}
	else
	{
		InvalidSignal = true;
	}
	return retval;
}

/**
 * @brief Mock jtag_Clock for recording the state of TMS
 *
 * The first state of TMS is in the high bits and the last state is in the
 * LSB.
 */
void jtagTAP_Mock_jtag_Clock()
{
	TMSStateTx <<= 1;
	TMSStateTx |= (TMSStateCurrent ? 1 : 0);
}
