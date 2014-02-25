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
#if !defined(_JTAG_H_)
#define _JTAG_H_

#include <stdbool.h>
#define JTAG_PIN_NOT_ALLOCATED (16U)

typedef enum jtag_ePin
{
	JTAG_PIN_TCK = 0,
	JTAG_PIN_TMS,
	JTAG_PIN_TDI,
	JTAG_PIN_TDO,
	JTAG_PIN_MAX
} jtag_Pin;

extern void jtag_Init();

extern void jtag_Cfg(jtag_Pin pin, unsigned int num);
extern void jtag_Set(jtag_Pin pin, bool val);
extern void jtag_Clock();

#endif
