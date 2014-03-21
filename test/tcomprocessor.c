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
#include "tcomprocessor.h"

#define comexec_Execute		comproc_Mock_comexec_Execute

#include "../source/comprocessor.c"

/** 
 * A pointer to what should be received by the mock execute function
 */
static char *expected_Execute;
/**
 * The expected length of the buffer passed for execution
 */
static unsigned int expected_ExecuteLen;	

/**
 * -2 wrong length was passed 
 * -1 if the wrong data was passed
 * 0 if not called
 * 1 if correct
 */
static int result_Execute;

/**
 * @brief Test that execute is being called with the expected values
 */
void comproc_Mock_comexec_Execute(char *buffer, unsigned int length)
{
	if(strncmp(buffer, expected_Execute, length) == 0)	
	{
		result_Execute = 1;
	}
	else if(length != expected_ExecuteLen)
	{
		result_Execute = -2;
	}
	else
	{
		result_Execute = -1;
	}
}

/**
 * @brief Test the command processor initialization
 *
 * The command buffer length should be set to zero
 */
bool comproc_TestInitialization()
{
	comproc_BufferLength = 99;

	comproc_Init();
	ASSERT(comproc_BufferLength == 0, "Buffer length not initialized correctly");

	return true;
}

/**
 * @brief Test the command processor
 *
 * The command processor should copy bytes into the command buffer until a
 * terminator is found (LF or CRLF) and then hand it off for execution.
 */
bool comproc_TestProcess()
{
	//test CRLF terminator
	comproc_Init();
	expected_Execute = "test one\r\n";
	expected_ExecuteLen = 10;
	result_Execute = 0;
	
	comproc_Process("test one\r\n", 10);
	ASSERT(result_Execute == 1, "execute failed: %i", result_Execute);
	ASSERT(comproc_BufferLength == 0, "buffer length not reset");

	//test CR terminator
	comproc_Init();
	expected_Execute = "test one\n";
	expected_ExecuteLen = 10;
	result_Execute = 0;

	comproc_Process("test one\n", 9);
	ASSERT(result_Execute == 1, "execute failed: %i", result_Execute);

	return true;
}

/**
 * @brief Test the command processor converts bytes to lower case
 *
 * The command processor should convert uppercase characters to lowercase
 */
bool comproc_TestProcessToLower()
{
	comproc_Init();
	expected_Execute = "test one 2 three\r\n";
	expected_ExecuteLen = 18;
	result_Execute = 0;
	
	comproc_Process("tEst ONe 2 THREE\r\n", 18);
	ASSERT(result_Execute == 1, "execute failed: %i", result_Execute);

	return true;
}
