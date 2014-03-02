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
#if !defined(_CHAIN_H_)
#define _CHAIN_H_

#include <stdbool.h>

#define CHAIN_MAX_DEVICES		(20)	///< Maximum number of devices in a chain supported
#define CHAIN_MAX_IRLEN			(CHAIN_MAX_DEVICES * 32)	///< Maximum chain IR length supported for autodetection

extern void chain_Init();
extern bool chain_Detect();

#endif
