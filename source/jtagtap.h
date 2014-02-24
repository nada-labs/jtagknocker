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
#if !defined(_JTAGTAP_H_)
#define _JTAGTAP_H_

typedef enum jtagTAP_eTAPState {
	JTAGTAP_STATE_UNKNOWN = 0,
	JTAGTAP_STATE_RESET,
	JTAGTAP_STATE_IDLE,
	JTAGTAP_STATE_DR_SCAN,
	JTAGTAP_STATE_DR_CAPTURE,
	JTAGTAP_STATE_DR_SHIFT,
	JTAGTAP_STATE_DR_EXIT1,
	JTAGTAP_STATE_DR_PAUSE,
	JTAGTAP_STATE_DR_EXIT2,
	JTAGTAP_STATE_DR_UPDATE,
	JTAGTAP_STATE_IR_SCAN,
	JTAGTAP_STATE_IR_CAPTURE,
	JTAGTAP_STATE_IR_SHIFT,
	JTAGTAP_STATE_IR_EXIT1,
	JTAGTAP_STATE_IR_PAUSE,
	JTAGTAP_STATE_IR_EXIT2,
	JTAGTAP_STATE_IR_UPDATE,
} jtagTAP_TAPState;

void jtagTAP_Init();
void jtagTAP_SetState(jtagTAP_TAPState target);

#endif
