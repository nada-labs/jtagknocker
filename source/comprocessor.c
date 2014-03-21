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
 * buffer off for execution.
 *
 */
void comproc_Process(const char * buffer, unsigned int len)
{
	const char *src = buffer;
	char *dest = &comproc_Buffer[comproc_BufferLength];

	while((comproc_BufferLength < COMPROC_BUFFER_LENGTH) && (len > 0))
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

			//have we reached the end of a command
			if(*src == '\n')
			{
				comexec_Execute(comproc_Buffer, comproc_BufferLength);
				//reset the index and pointer
				comproc_BufferLength = 0;
				dest = comproc_Buffer;
			}

			//increment the dest pointer
			++dest;
		}
		//a byte was consumed from the input stream, update length
		//and the pointer
		++src;
		--len;
	}
}
