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
#if !defined(_TCHAIN_H_)
#define _TCHAIN_H_
#include <stdbool.h>

extern bool chain_TestFakeChain();
extern bool chain_TestInitilization();
extern bool chain_TestDeviceCount();
extern bool chain_TestChainIRLength();
extern bool chain_TestResetDRIDCode();
extern bool chain_TestResetDRBypass();
extern bool chain_TestResetDRIDCodes();
extern bool chain_TestDeviceIRLen();

#endif
