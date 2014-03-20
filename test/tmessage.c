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
#include "tmessage.h"

#define serial_Send	message_Mock_serial_Send
#include "../source/message.c"

static unsigned int callCount_Send;

/**
 * @brief Test that the message module initializes correctly
 *
 * The initial message level is MESSAGE_LEVEL_GENERAL.
 */
bool message_TestInitialization()
{
	message_Level = 99;

	message_Init();

	ASSERT(message_Level == MESSAGE_LEVEL_GENERAL, "Message level incorrect: %i should be %i", message_Level, MESSAGE_LEVEL_GENERAL);	

	return true;
}

/**
 * @brief Test level setting
 *
 * The level can be set to any of the predefined values (REQUIRED -> DEBUG). 
 * A call to set with an invalid vaule will result in no change of the current
 * level.
 */
bool message_TestSetLevel()
{
	message_Level = 10;

	//test a correct set
	message_SetLevel(MESSAGE_LEVEL_GENERAL);
	ASSERT(message_Level == MESSAGE_LEVEL_GENERAL, "Message level incorrect: %i should be %i", message_Level, MESSAGE_LEVEL_GENERAL);

	//test an invalid set +ve
	message_SetLevel(10);
	ASSERT(message_Level == MESSAGE_LEVEL_GENERAL, "Message level incorrect: %i should be %i", message_Level, MESSAGE_LEVEL_GENERAL);

	//test an invalid set -ve
	message_SetLevel(-6);
	ASSERT(message_Level == MESSAGE_LEVEL_GENERAL, "Message level incorrect: %i should be %i", message_Level, MESSAGE_LEVEL_GENERAL);

	return true;
}

/**
 * @brief Test messages are displayed at the correct level
 *
 * A message must only be displayed if the provided message level is less 
 * than or equal to the current message level. Otherwise it is ignored.
 */
bool message_TestMessages()
{
	//setup
	callCount_Send = 0;
	message_Level = MESSAGE_LEVEL_GENERAL;

	message_Write(MESSAGE_LEVEL_GENERAL, "General Message");
	ASSERT(callCount_Send == 1, "Call count incorrect: %i should be %i", callCount_Send, 1);
	message_Write(MESSAGE_LEVEL_VERBOSE, "Verbose Message");
	ASSERT(callCount_Send == 1, "Call count incorrect: %i should be %i", callCount_Send, 1);
	message_Write(MESSAGE_LEVEL_REQUIRED, "Required Message");
	ASSERT(callCount_Send == 2, "Call count incorrect: %i should be %i", callCount_Send, 2);

	return true;
}

/**
 * @brief Mock serial_Send
 * 
 * Records the number of times it was called
 */
void message_Mock_serial_Send(const char *buffer, unsigned int len)
{
	callCount_Send += 1;
}

