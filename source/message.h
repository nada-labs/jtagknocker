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
#if !defined(_MESSAGE_H_)
#define _MESSAGE_H_

/**
 * @brief Available message levels
 *
 * Each additional level also displays the levels below it
 */
typedef enum message_eLevels
{
	MESSAGE_LEVEL_REQUIRED = 0,	///< Required messages only, always displayed
	MESSAGE_LEVEL_GENERAL,		///< General informational messages
	MESSAGE_LEVEL_VERBOSE,		///< General and additional informational messages
	MESSAGE_LEVEL_DEBUG,		///< Lots of messages
	MESSAGE_LEVEL_MAX		///< Maximum level number
}message_Levels;

//Message functions

extern void message_Init();
extern void message_SetLevel(message_Levels level);
extern message_Levels message_GetLevel();
extern int message_Write(message_Levels level, const char *fmt, ...);
#endif
