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

#if !defined(_COMMANDS_H_)
#define _COMMANDS_H_

#include "jtagtap.h"
#include "jtag.h"

#define CMD_ARG_NONE		(-1)
#define CMD_CFGCLK_ADAPTIVE (-2)
#define CMD_DEVICE_IDCODE 	(1)
#define CMD_DEVICE_IRLEN  	(2)
#define CMD_SCAN_RESET		(1)
#define CMD_SCAN_BYPASS		(2)

extern void cmd_Init();
extern void cmd_Process(char *Buffer, unsigned int Length);

//Command processors
extern void cmd_Help();
extern void cmd_Binary ();
extern void cmd_Shift(); 
extern void cmd_Device(int Device, int Attribute, int Value);
extern void cmd_Debug(int Level);
extern void cmd_Tap(jtagTAP_TAPState State);
extern void cmd_Clock(int Counts);
extern void cmd_Config(); 
extern void cmd_SignalConfig(jtag_Pin Signal, int Pin);
extern void cmd_ConfigClock(int Rate);
extern void cmd_IREnum(int Device);
extern void cmd_Chain();
extern void cmd_Scan(int Pins, int Mode);
extern void cmd_Signal(jtag_Pin Signal, int State);

#endif
