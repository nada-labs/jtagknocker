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
#include "comexecute.h"
#include "message.h"
#include "chain.h"
#include "knock.h"
#include "jtag.h"
#include "jtagtap.h"
#include <string.h>
#include <errno.h>

static void comexec_SendReply(bool Success);

//Command handlers
static void comexec_MessageLevel(message_Levels Level);
static void comexec_Chain();
static void comexec_ScanForJTAG(unsigned int Pins, knock_Mode Mode);
static void comexec_SignalConfig(jtag_Signal Signal, int Pin);
static void comexec_Config();
static void comexec_TAP(jtagTAP_TAPState State);
static void comexec_Clock(unsigned int Counts);
static void comexec_SetSignal(jtag_Signal Signal, bool State);
static void comexec_GetSignal(jtag_Signal Signal);
static void comexec_Help();

/**
 * @brief Sets the current message level
 *
 * The higher the level, the more information is displayed. If the provided
 * level is @ref MESSAGE_LEVEL_MAX, the current level is displayed.
 *
 * @param[in] Level Message level to set.
 */
void comexec_MessageLevel(message_Levels Level)
{
	bool success = false;
	if(Level == MESSAGE_LEVEL_MAX)
	{
		message_Write(MESSAGE_LEVEL_GENERAL, "Current Level: %i\r\n", message_GetLevel());
		success = true;
	}
	else
	{
		if((Level >= MESSAGE_LEVEL_REQUIRED) && (Level < MESSAGE_LEVEL_MAX))
		{
			message_SetLevel(Level);
			success = true;
		}
		else
		{
			message_Write(MESSAGE_LEVEL_GENERAL, "Level must be between %i and %i inclusive.\r\n", MESSAGE_LEVEL_REQUIRED, MESSAGE_LEVEL_MAX - 1);
		}
	}
	comexec_SendReply(success);
}

/**
 * @brief Enumerates devices on the JTAG chain
 *
 * Once a valid interface has been configured, scans the chain and determines
 * the properities of the devices. It attempts to find the number of devices
 * on the chain and their IDCODE(s).
 */
void comexec_Chain()
{
	bool success = chain_Detect();
	if(!success)
	{
		message_Write(MESSAGE_LEVEL_VERBOSE, "No devices found on chain. Are the signal assignments correct?\r\n");
	}
	comexec_SendReply(success);
}

/**
 * @brief Scans for a JTAG port
 *
 * Scans for a JTAG interface on pins 1 - npins
 *
 * reset mode uses a TAP Reset to look for idcodes, this mode will fail if no
 * devices on the chain support IDCODE. Takes (npins*(npins-1)+(npins-2)
 * operations.
 *
 * bypass mode scans BYPASS commands into the TAPs and looks for TDO. Takes
 * (npins*(npins-1)*(npins-2) operations.
 *
 * All pins are left deconfigured when the scan finishes.
 *
 * @param[in] Pins The number of pins to use in the scan, must be 4 or more
 * @param[in] Mode The scanning mode to use
 */
void comexec_ScanForJTAG(unsigned int Pins, knock_Mode Mode)
{
	bool success = false;
	if((Pins >=4 ) && (Pins <= JTAG_PIN_MAX))
	{
		knock_Scan(Mode, Pins);
		success = true;	//the command itself doesn't fail, even if the scan doesn't find anything.
	}
	else
	{
		message_Write(MESSAGE_LEVEL_GENERAL, "At least 4 pins are required for a scan. Max 16.\r\n");
	}
	comexec_SendReply(success);
}

/**
 * @brief Configures a signal
 *
 * Specifing a pin of 0 deconfigures the signal, otherwise the signal is
 * assigned to the pin.
 *
 * @param[in] Signal The signal to configure.
 * @param[in] The pin number to assign to the signal.
 */
void comexec_SignalConfig(jtag_Signal Signal, int Pin)
{
	bool success = false;
	//test the signal is in range
	if((Signal >= JTAG_SIGNAL_TCK) && (Signal < JTAG_SIGNAL_MAX))
	{
		if((Pin >= 0) && (Pin <= JTAG_PIN_MAX))
		{
			//set Pin to the correct value
			if(Pin == 0)
			{
				Pin = JTAG_SIGNAL_NOT_ALLOCATED;
			}
			else
			{
				Pin -= 1;	//turn into 0 based pins
			}
			success = jtag_Cfg(Signal, Pin);

			if(!success)
			{
				message_Write(MESSAGE_LEVEL_GENERAL, "Configuration failed: Pin in use.\r\n");
			}
		}
		else
		{
			message_Write(MESSAGE_LEVEL_GENERAL, "Pin must be between %i and %i inclusive.\r\n", 0, JTAG_PIN_MAX);
		}

	}
	else
	{
		message_Write(MESSAGE_LEVEL_GENERAL, "Invalid signal\r\n");
	}
	comexec_SendReply(success);
}

/**
 * @brief Display the current configuration
 */
void comexec_Config()
{
	jtag_Signal sig;
	int pin;

	message_Write(MESSAGE_LEVEL_GENERAL, "Signal Configuration:\r\n  Signal  Pin\r\n");
	for(sig = JTAG_SIGNAL_TCK; sig < JTAG_SIGNAL_MAX; ++sig)
	{
		pin = jtag_GetCfg(sig);
		if(pin != JTAG_SIGNAL_NOT_ALLOCATED)
		{
			//display the signal name and pin
			message_Write(MESSAGE_LEVEL_GENERAL, "    %4s   %2i\r\n", jtag_SignalNames[sig], pin + 1);
		}
	}
	comexec_SendReply(true);
}

/**
 * @brief Set or display the current TAP state
 *
 * If State is JTAGTAP_STATE_MAX the current state is displayed, otherwise
 * the TAP is shifted to the provided state.
 *
 * @param[in] State the state to transfer to.
 */
void comexec_TAP(jtagTAP_TAPState State)
{
	bool success = false;

	if(State == JTAGTAP_STATE_MAX)
	{
		//displaying the current state
		message_Write(MESSAGE_LEVEL_GENERAL, "TAP State: %s\r\n", jtagTAP_StateNames[jtagTAP_GetState()]);
		success = true;
	}
	else
	{
		//setting the state
		jtagTAP_TAPState oldState = jtagTAP_GetState();
		jtagTAP_SetState(State);
		message_Write(MESSAGE_LEVEL_VERBOSE, "TAP State: %s -> %s\r\n", jtagTAP_StateNames[oldState], jtagTAP_StateNames[jtagTAP_GetState()]);
		success = true;
	}

	comexec_SendReply(success);
}

/**
 * @brief Toggle the clock signal.
 *
 * Assumes that the clock signal has been assigned to a pin.
 * Does nothing otherwise.
 *
 * @param[in] Counts The number of clock pulses to provide
 */
void comexec_Clock(unsigned int Counts)
{
	while((Counts--) > 0)
	{
		jtag_Clock();
	}
	comexec_SendReply(true);
}

/**
 * @brief Set a signal
 *
 * Trying to set an input has no effect.
 *
 * @param[in] Signal The signal to set.
 * @param[in] State The state to set to.
 */
void comexec_SetSignal(jtag_Signal Signal, bool State)
{
	jtag_Set(Signal, State);
	comexec_SendReply(true);
}

/**
 * @brief Get a signal state
 *
 * @param[in] Signal The signal to display.
 */
void comexec_GetSignal(jtag_Signal Signal)
{
	bool val = jtag_Get(Signal);
	message_Write(MESSAGE_LEVEL_GENERAL, "%s: %i\r\n",  jtag_SignalNames[Signal], val);
	comexec_SendReply(true);
}

/**
 * @brief Send a reply message
 *
 * The reply message indicates the successful execution of the command
 * `OK` or failure `ERR`. The command handlers can provide additional
 * information in both cases.
 */
void comexec_SendReply(bool Success)
{
	if(Success)
	{
		message_Write(MESSAGE_LEVEL_REQUIRED, "OK\r\n");
	}
	else
	{
		message_Write(MESSAGE_LEVEL_REQUIRED, "ERROR\r\n");
	}
	//issue a new prompt
	message_Write(MESSAGE_LEVEL_REQUIRED, "> ");
}

/**
 * @brief Display the list of available commands
 *
 */
void comexec_Help()
{

}

/**
 * @brief Execute the given command
 *
 * Parses the supplied command and converts any arguments into the data types
 * expected by the various handler functions.
 *
 * @param[in] Buffer The command to execute in string format
 */
void comexec_Execute(char *Buffer)
{
	char *pSaveToken;
	char *Token;
	bool parseSuccess = false;

	Token = strtok_r(Buffer, " ", &pSaveToken);

	if(strcmp(Token, "help") == 0)
	{
		comexec_Help();
	}
	else if(strcmp(Token, "chain") == 0)
	{
		comexec_Chain();
	}
	else if(strcmp(Token, "clock") == 0)
	{
		unsigned int count;

		//check and convert the required counts value
		if((Token = strtok_r(NULL, " ", &pSaveToken)) != NULL)
		{
			errno = 0;
			count = strtoul(Token, NULL, 10);
			if(errno == 0)
			{
				//execute the clock command
				comexec_Clock(count);
			}
			else
			{
				message_Write(MESSAGE_LEVEL_GENERAL, "n needs to be a number.\r\n");
				comexec_SendReply(false);
			}
		}
		else
		{
			message_Write(MESSAGE_LEVEL_GENERAL, "missing parameter count.\r\n");
			comexec_SendReply(false);
		}
	}
	else if(strcmp(Token, "message") == 0)
	{
		message_Levels level = MESSAGE_LEVEL_MAX;
		parseSuccess = true;
		//check and convert the required counts value
		if((Token = strtok_r(NULL, " ", &pSaveToken)) != NULL)
		{
			errno = 0;
			level = strtoul(Token, NULL, 10);
			if(errno != 0)
			{
				parseSuccess = false;
				message_Write(MESSAGE_LEVEL_GENERAL, "level needs to be a number.\r\n");
				comexec_SendReply(false);
			}
		}
		if(parseSuccess)
		{
			comexec_MessageLevel(level);
		}
	}
	else if(strcmp(Token, "scan") == 0)
	{
		knock_Mode scanMode = KNOCK_MODE_RESET;
		unsigned int pins;

		//check and convert the required npins value
		if((Token = strtok_r(NULL, " ", &pSaveToken)) != NULL)
		{
			errno = 0;
			pins = strtoul(Token, NULL, 10);
			if(errno == 0)
			{
				parseSuccess = true;
				if((Token = strtok_r(NULL, " ", &pSaveToken)) != NULL)
				{
					//handle the mode if supplied
					if(strcmp(Token, "reset") == 0)
					{
						scanMode = KNOCK_MODE_RESET;
					}
					else if(strcmp(Token, "bypass") == 0)
					{
						scanMode = KNOCK_MODE_BYPASS;
					}
					else
					{
						message_Write(MESSAGE_LEVEL_GENERAL, "invalid mode.\r\n");
						comexec_SendReply(false);
						parseSuccess = false;
					}
				}

				if(parseSuccess)
				{
					//execute the scan command
					comexec_ScanForJTAG(pins, scanMode);
				}
			}
			else
			{
				message_Write(MESSAGE_LEVEL_GENERAL, "npins needs to be a number.\r\n");
				comexec_SendReply(false);
				parseSuccess = false;
			}

		}
		else
		{
			message_Write(MESSAGE_LEVEL_GENERAL, "missing parameter npins.\r\n");
			comexec_SendReply(false);
		}
	}
}
