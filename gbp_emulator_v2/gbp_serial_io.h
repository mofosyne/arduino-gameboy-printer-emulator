/*************************************************************************
 *
 * Gameboy Printer Serial IO
 * Part of GAMEBOY PRINTER EMULATION PROJECT V2 (Arduino)
 * Copyright (C) 2020 Brian Khuu
 *
 * PURPOSE: This module focus on capturing packets from a gameboy to a virtual printer
 * LICENCE:
 *   This file is part of Arduino Gameboy Printer Emulator.
 *
 *   Arduino Gameboy Printer Emulator is free software:
 *   you can redistribute it and/or modify it under the terms of the
 *   GNU General Public License as published by the Free Software Foundation,
 *   either version 3 of the License, or (at your option) any later version.
 *
 *   Arduino Gameboy Printer Emulator is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Arduino Gameboy Printer Emulator.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef GBP_SERIAL_IO_H
#define GBP_SERIAL_IO_H
#include <stdint.h> // uint8_t
#include <stddef.h> // size_t
#include <stdbool.h> // bool

#define GBP_FEATURE_USING_RISING_CLOCK_ONLY_ISR // Away from technical accuracy towards double speed mode compatibility

/******************************************************************************/

/* Init/Reset/ISR Functions */
bool gpb_serial_io_init(size_t buffSize, uint8_t *buffPtr);
bool gpb_serial_io_reset(void);
#ifdef GBP_FEATURE_USING_RISING_CLOCK_ONLY_ISR
bool gpb_serial_io_OnRising_ISR(const bool GBP_SOUT);
#else
bool gpb_serial_io_OnChange_ISR(const bool GBP_SCLK, const bool GBP_SOUT);
#endif

/* Timeout */
bool gbp_serial_io_timeout_handler(uint32_t elapsed_ms);

/* Output */
size_t  gbp_serial_io_dataBuff_getByteCount(void);
uint8_t gbp_serial_io_dataBuff_getByte(void);
uint8_t gbp_serial_io_dataBuff_getByte_Peek(uint32_t offset);
uint16_t gbp_serial_io_dataBuff_waterline(bool resetWaterline);
uint16_t gbp_serial_io_dataBuff_max(void);

/******************************************************************************/
#endif
