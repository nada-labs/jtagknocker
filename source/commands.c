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
#include "commands.h"
#include "serial.h"

/**
 * @brief Initalize the command module
 *
 */
void cmd_Init()
{

}

/**
 * @brief Process a command
 *
 * Process the provided command, The buffer must have two null terminators and lenth must include them.
 *
 * @param[in] Buffer A buffer containing the command. Double NULL terminated
 * @param[in] Length Size of the buffer, including terminators
 */
void cmd_Process(char *Buffer, unsigned int Length)
{

}

/**
 * @brief Display the help
 *
 * Help is just a list of commands
 */
void cmd_Help()
{
	serial_Write("cmd_Help();\n");
}

/**
 * @brief Enter binary mode
 *
 * Binary mode is used by OpenOCD etc for fater transfer rates. Any invalid
 * packet in binary mode returns to text mode.
 */
void cmd_Binary ()
{
	serial_Write("cmd_Binary();\n");
}

/**
 * @brief Enter data shift mode
 *
 * Data to be shifted out is provided in hexadecimal format, LSB first
 * Received data is displayed in hexadecimal, LSB first. Any invalid character
 * exits data shift mode.
 */
void cmd_Shift()
{
	serial_Write("cmd_Shift();\n");
}

/**
 * @brief Set/Display a device attribute
 *
 * Attributes are currently IDCODE and IR Length
 *
 * @param[in] Device The device to query/set. CMD_ARG_NONE for all devices
 * @param[in] Attribute The attribute to set. CMD_DEVICE_xxx
 * @param[in] Value The value to set the attribute to. 
 */
void cmd_Device(int Device, int Attribute, int Value)
{
	serial_Write("cmd_Device(%i, %i, %i);\n", Device, Attribute, Value);
}

/**
 * @brief Sets the debug level
 *
 * The higher the level, the more information is displayed.
 *
 * @param[in] Level Debug level to set, defaults to 1.
 */
void cmd_Debug(int Level)
{
	serial_Write("cmd_Debug(%i);\n", Level);
}

/**
 * @brief Move the TAP state machine to the provided state
 *
 * Sets or displays the current TAP state.
 * @param[in] State the target state.
 */
void cmd_Tap(jtagTAP_TAPState State)
{
	serial_Write("cmd_Tap(%i);\n", State);
}

/**
 * @brief Toggle the TCK line
 *
 * Toggles the TCK line using the set speed or adaptave clocking if configured.
 *
 * @param[in] Counts the number of clock ticks to send
 */
void cmd_Clock(int Counts)
{
	serial_Write("cmd_Clock(%i);\n", Counts);
}

/**
 * @brief Displays the current configuration
 *
 */
void cmd_Config()
{
	serial_Write("cmd_Config();\n");
}

/**
 * @brief Configure a JTAG signal.
 *
 * Displays the current allocation for the signal, or sets it to the provided pin.
 * Returns an error if the provided pin is already allocated to another pin or it
 * is out of range.
 *
 * @param[in] Signal The JTAG signal to configure
 * @param[in] The pin to assign the signal to, or CMD_ARG_NONE to display the
 * current assigned pin.
 */
void cmd_SignalConfig(jtag_Signal Signal, int Pin)
{
	serial_Write("cmd_SignalConfig(%i, %i);\n", Signal, Pin);
}

/**
 * @brief Configure the clocking mode/frequency
 *
 * Displays the clock mode or rate if CMD_ARG_NONE and sets the clock to 
 * adaptive mode if Rate is CMD_ARG_NONE.
 *
 * @param[in] Rate The clock rate in kHz.
 */
void cmd_ConfigClock(int Rate)
{
	serial_Write("cmd_ConfigClock(%i);\n", Rate);
}

/**
 * @brief Enumerate instructions for a device
 *
 * Walks through all possible IR values for a given device and determines 
 * the DR length, displaying its contents in hexadecimal, LSB first.
 *
 * @warning This could potentially damage your hardware, use at own risk.
 * @note It is advisable to have TRST configured to skip the DR_UPDATE phase.
 *
 * @param[in] Device The device to enumerate, or CMD_ARG_NONE for all devices
 * on the chain.
 */
void cmd_IREnum(int Device)
{
	serial_Write("cmd_IREnum(%i);\n", Device);
}

/**
 * @brief Displays information about the devices connected on the JTAG chain
 *
 */
void cmd_Chain()
{
	serial_Write("cmd_Chain();\n");
}

/**
 * @brief Scan for a JTAG port
 *
 * @param[in] Pins The number of pins to enumerate through. Has to be 4 or greater.
 * @param[in] Mode CMD_SCAN_RESET For TAP reset scanning, 
 * CMD_SCAN_BYPASS for TAP IR Bypass scanning.
 */
void cmd_Scan(int Pins, int Mode)
{
	serial_Write("cmd_Scan(%i, %i);\n", Pins, Mode);
}

/**
 * @brief Sets or displays the state of a JTAG signal
 *
 * @param[in] Signal The signal to display.
 * @param[in] State The state to set the provided signal to.
 */
void cmd_Signal(jtag_Signal Signal, int State)
{
	serial_Write("cmd_Signal(%i, %i);\n", Signal, State);
}



