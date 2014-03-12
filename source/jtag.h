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
#define JTAG_SIGNAL_NOT_ALLOCATED	(-1)	///< Flag for deallocating a signal
#define JTAG_PIN_MAX			(16)	///< Maximum number of signals supported

typedef enum jtag_eSignal
{
	JTAG_SIGNAL_TCK = 0,
	JTAG_SIGNAL_TMS,
	JTAG_SIGNAL_TDI,
	JTAG_SIGNAL_TDO,
	JTAG_SIGNAL_TRST,
	JTAG_SIGNAL_SRST,
	JTAG_SIGNAL_RTCK,
	JTAG_SIGNAL_MAX
} jtag_Signal;

extern void jtag_Init();

extern void jtag_Cfg(jtag_Signal sig, int num);
extern void jtag_Set(jtag_Signal sig, bool val);
extern bool jtag_Get(jtag_Signal sig);
extern bool jtag_IsAllocated(jtag_Signal sig);
extern void jtag_Clock();

#endif
