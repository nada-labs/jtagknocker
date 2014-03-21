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

#if !defined(_COMEXECUTE_H_)
#define _COMEXECUTE_H_
#include "knock.h"
#include "jtag.h"
#include "jtagtap.h"
#include "message.h"

extern void comexec_Execute(char *Buffer, unsigned int Length);

//Command handlers
extern void comexec_MessageLevel(message_Levels Level);
extern void comexec_Chain();
extern void comexec_ScanForJTAG(unsigned int Pins, knock_Mode Mode);
extern void comexec_SignalConfig(jtag_Signal Signal, int Pin);
extern void comexec_Config();
extern void comexec_TAP(jtagTAP_TAPState State);
extern void comexec_Clock(unsigned int Counts);
extern void comexec_SetSignal(jtag_Signal Signal, bool State);
extern void comexec_GetSignal(jtag_Signal Signal);

#endif
