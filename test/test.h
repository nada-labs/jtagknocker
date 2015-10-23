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
#if !defined(_TEST_H_)
#define _TEST_H_

extern void test_Write(const char *fmt, ...);
#define NULL ((void *) 0)
/**
 * @brief Test an operation and report message
 *
 * If the boolean test is false the provided message and arguments will be
 * sent to the serial port.
 *
 * @note Assumes all tests return boolean
 */
#define ASSERT(test, message, ...) do{ 								\
	if(!(test))										\
	{											\
		test_Write("%s:%i Test Failed: " message "\r\n", __FUNCTION__, __LINE__, ##__VA_ARGS__);	\
		return false;										\
	}											\
}while(0)


#endif
