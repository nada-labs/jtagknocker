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
#if !defined(_TJTAG_H_)
#define _TJTAG_H_

#include <stdbool.h>

//Test functions
extern bool jtag_TestInitSignalAlloc();
extern bool jtag_TestInitRegisterSetup();
extern bool jtag_TestSignalConfigSet();
extern bool jtag_TestSignalConfigSetInput();
extern bool jtag_TestSignalConfigSetInvalid();
extern bool jtag_TestSignalConfigUnSet();
extern bool jtag_TestSignalConfigAlreadySetPin();
extern bool jtag_TestSignalConfigAlreadySetSig();

extern bool jtag_TestSetAndClear();
extern bool jtag_TestSetUnallocatedSignal();
extern bool jtag_TestSetInput();
extern bool jtag_TestGet();
extern bool jtag_TestGetUnallocated();
extern bool jtag_TestIsAllocated();

#endif
