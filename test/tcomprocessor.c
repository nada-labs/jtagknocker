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

/**
 * @brief Test backspace and delete remove characters
 *
 * The backspace and delete characters should remove previous input from the
 * command buffer, one byte per instance. The length of the buffer should
 * never go below zero
 */
bool comproc_TestProcessBSDel()
{
	//test backspace and delete
	comproc_Init();
	expected_Execute = "test one 2 three\r\n";
	expected_ExecuteLen = 18;
	result_Execute = 0;

	comproc_Process("tEstc6\b\b Op\x7FNe 2 THREE\r\n", 24);
	ASSERT(result_Execute == 1, "execute failed: %i", result_Execute);

	comproc_Init();
	comproc_Process("test\b\b\b\b\b\b\b\b", 12);
	ASSERT(comproc_BufferLength == 0, "Buffer length incorrect: %i, should be 0", comproc_BufferLength);

	return true;
}

/**
 * @brief Test many small packets combine into a valid command
 *
 * Many small packets should be able to be added together to make a
 * valid command
 */
bool comproc_TestProcessSmallPackets()
{
	comproc_Init();
	expected_Execute = "test one 2 three\r\n";
	expected_ExecuteLen = 18;
	result_Execute = 0;

	comproc_Process("t", 1);
	comproc_Process("est", 3);
	comproc_Process(" one", 4);
	comproc_Process(" ", 1);
	comproc_Process("2 thr", 5);
	comproc_Process("ee", 2);
	comproc_Process("\r\n", 2);
	ASSERT(result_Execute == 1, "execute failed: %i", result_Execute);

	return true;
}

/**
 * @brief Test that commands process correctly one after the other
 *
 * Commands should process one after another, no matter where the packet
 * boundaries lie
 */
bool comproc_TestProcessMultiCommands()
{
	comproc_Init();
	expected_Execute = "test one 2 three\r\n";
	expected_ExecuteLen = 18;
	result_Execute = 0;

	comproc_Process("test one 2 three\r\n", 18);
	ASSERT(result_Execute == 1, "execute failed: %i", result_Execute);
	result_Execute = 0;
	comproc_Process("test one 2 three\r\n", 18);
	ASSERT(result_Execute == 1, "execute failed: %i", result_Execute);

	result_Execute = 0;
	comproc_Process("test one 2 three\r\ntest one 2 three", 34);
	ASSERT(result_Execute == 1, "execute failed: %i", result_Execute);
	result_Execute = 0;
	comproc_Process("\r\n", 2);
	ASSERT(result_Execute == 1, "execute failed: %i", result_Execute);

	return true;
}

/**
 * @brief Test that an oversized command is handled correctly
 *
 * An oversized command is any incoming data that has more that
 * @ref COMPROC_BUFFER_LENGTH bytes in it before the terminator. In this case
 * comproc_BufferLength should never exceede the total size allowed. All data
 * in the buffer will be silently discarded when a terminator is finally seen
 * and normal processing will resume.
 */
bool comproc_TestProcessHugePacket()
{
	unsigned int index = 0;
	comproc_Init();
	result_Execute = 0;

	for(index = 0; index <= COMPROC_BUFFER_LENGTH; index += 10)
	{
		//fill the buffer up
		comproc_Process("0123456789", 10);
	}
	ASSERT(comproc_BufferLength == COMPROC_BUFFER_LENGTH, "Internal buffer length wrong: %i should be %i", comproc_BufferLength, COMPROC_BUFFER_LENGTH);

	comproc_Process("\n", 1);
	ASSERT(comproc_BufferLength == 0, "Internal buffer length wrong: %i should be %i", comproc_BufferLength, 0);
	ASSERT(result_Execute == 0, "Execute was called and it shouldn't have been");

	return true;
}
