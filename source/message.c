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
#include "message.h"
#include "serial.h"
#include <strings.h>
#include <stdarg.h>

#define MESSAGE_WRITE_BUFFER	256	///< The maximum length of a message

static message_Levels message_Level;	///< The current message level

/**
 * @brief Initialize the message module
 *
 * Set the initial message level to MESSAGE_LEVEL_GENERAL
 */
void message_Init()
{
	message_Level = MESSAGE_LEVEL_GENERAL;
}

/**
 * @brief Set the current message level
 *
 * @param[in] level The level to set, has to be one of the message_Levels
 */
void message_SetLevel(message_Levels level)
{
	if(level < MESSAGE_LEVEL_MAX)
	{
		message_Level = level;
	}
}

/**
 * @brief Get the current message level
 *
 */
message_Levels message_GetLevel()
{
	return message_Level;
}

/**
 * @brief Format and display a message
 *
 * The message should only be displayed to the user if the message level
 * is less than or equal to the current message level.
 *
 * @param level The message level
 * @param fmt A format string for the level
 * @return The number of bytes written or -1 if an error occured,
 * -2 for incorrect level
 */
int message_Write(message_Levels level, const char *fmt, ...)
{
	char buffer[MESSAGE_WRITE_BUFFER];
	int n;
	va_list args;
	if(level <= message_Level)
	{
		va_start(args, fmt);
		n = vsnprintf(buffer, MESSAGE_WRITE_BUFFER, fmt, args);
		if((n > 0) && (n < MESSAGE_WRITE_BUFFER))
		{
			serial_Send(buffer, n);
		}
		else
		{
			n = -1;
		}
		va_end(args);
	}
	else
	{
		n = -2;
	}
}
