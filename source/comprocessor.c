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

#include "comprocessor.h"
#include "comexecute.h"

#define COMPROC_BUFFER_LENGTH	(80)			///< Maximum command length supported

static unsigned int comproc_BufferLength;		///< Length of the command in the buffer
static char comproc_Buffer[COMPROC_BUFFER_LENGTH+1];	///< Command buffer

/**
 * @brief Initialize the command processor
 *
 */
void comproc_Init()
{
	//Initialize the buffer length
	comproc_BufferLength = 0;
}

/**
 * @brief Process an incoming command
 *
 * The incoming buffer may provide a few bytes to a valid command, process
 * them into the command buffer, changing case and handling backspace and
 * delete as required. When a valid terminator is found, hand the command
 * buffer off for execution. Commands should process one after another, no
 * matter where the packet boundaries lie.
 *
 * The backspace and delete characters should remove previous input from the
 * command buffer, one byte per instance. The length of the buffer should
 * never go below zero
 *
 * An oversized command is any incoming data that has more that
 * @ref COMPROC_BUFFER_LENGTH bytes in it before the terminator. In this case
 * comproc_BufferLength should never exceede the total size allowed. All data
 * in the buffer will be silently discarded when a terminator is finally seen
 * and normal processing will resume.
 *
 */
void comproc_Process(const char * buffer, unsigned int len)
{
	const char *src = buffer;
	char *dest = &comproc_Buffer[comproc_BufferLength];

	while(len > 0)
	{
		if(comproc_BufferLength < COMPROC_BUFFER_LENGTH)
		{
			//process backspace and delete first
			if((*src == '\b') || (*src == 0x7F))
			{
				if(comproc_BufferLength > 0)
				{
					//go back one
					--comproc_BufferLength;
					--dest;
				}
			}
			else
			{
				//copy the byte from the incomming buffer into the command buffer
				//converting to lowercase if needed
				if((*src >= 'A') && (*src <= 'Z'))
				{
					*dest = (*src | 0x20);		//convert to lowercase
				}
				else
				{
					*dest = *src;
				}
				++comproc_BufferLength;
				++dest;

				//have we reached the end of a command
				if(*src == '\n')
				{
					//NULL terminate the buffer
					comproc_Buffer[comproc_BufferLength] = '\x00';

					comexec_Execute(comproc_Buffer);
					//reset the index and pointer
					comproc_BufferLength = 0;
					dest = comproc_Buffer;
				}

			}
		}
		else
		{
			//the buffer is full, wait for a terminator and then reset
			//execute isn't called as the command is invalid
			if(*src == '\n')
			{
				comproc_BufferLength = 0;
				dest = comproc_Buffer;
			}
		}

		//a byte was consumed from the input stream, update length
		//and the pointer
		++src;
		--len;
	}
}
