/*******************************************************************************
 *
 * GAMEBOY PRINTER EMULATION PROJECT V2
 * Copyright (C) 2020 Brian Khuu
 *
 * PURPOSE: Header file for gameboy printer
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
 *
 * HISTORY:
 *   - Creation Date: 2017-4-6
 *   - Revised 1 Date: 2017-11-27
 *   - Revised 2 Date: 2020-08-16
 *   - PURPOSE:
 *   - AUTHOR: Brian Khuu
 *
 *   # Rev1:
 *   Below header file was revised again in 2017-11-27 because I found the
 *   gameboy programming manual, which points out exactly how the communication
 *   works.
 *
 *   # Rev2:
 *   Revised again to strip out stuff that didn't end up being used for V2 of the
 *   gameboy printer emulation project for arduino.
 *
------------------------------------------------------------------------------*/
#ifndef GAMEBOY_PRINTER_PROTOCOL_H
#define GAMEBOY_PRINTER_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
    GAMEBOY LINK SIGNALING
********************************************************************************

  # Manual Measurement With Oscilloscope

    - Clock Frequency (Normal Speed Mode): 8kHz (127.63 us)
    - Clock Frequency (Double Speed Mode): 16kHz (63.81 us)
    - Transmission Speed: 867 baud (1.153ms per 8bit symbol) on normal mode
    - Between Symbol Period: 229.26 us on normal mode

    ```
                           1.153ms
            <--------------------------------------->
             0   1   2   3   4   5   6   7             0   1   2   3   4   5   6   7
         __   _   _   _   _   _   _   _   ___________   _   _   _   _   _   _   _   _
    CLK:   |_| |_| |_| |_| |_| |_| |_| |_|           |_| |_| |_| |_| |_| |_| |_| |_|
    DAT: ___XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX____________XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX_
           <-->                           <---------->
           127.63 us                         229.26 us
    ```

    Based on SIO Timing Chart. Page 30 of GameBoy PROGRAMMING MANUAL Version 1.0:
    * CPOL=1 : Clock Polarity 1. Idle on high.
    * CPHA=1 : Clock Phase 1. Change on falling. Check bit on rising edge.

  # Gameboy Link Pinout (Gameboy Original/Color Link Cable Pinout)

    - Pin 1 :  GBP_VCC_PIN  : VDD35
    - Pin 2 :  GBP_SO_PIN   : SO
    - Pin 3 :  GBP_SI_PIN   : SI
    - Pin 4 :  GBP_SD_PIN   : SD
    - Pin 5 :  GBP_SC_PIN   : SC
    - Pin 6 :  GBP_GND_PIN  : GND

    ```
     ___________
    |  6  4  2  |
     \_5__3__1_/   (at cable)
    ```


------------------------------------------------------------------------------*/

/*******************************************************************************
    GAMEBOY PRINTER PROTOCOL
*******************************************************************************/

// GameBoy Printer Packet Structure
/*
  | BYTE POS :    |     0     |     1     |     2     |      3      |     4     |     5     |  6 + X    | 6 + X + 1 | 6 + X + 2 | 6 + X + 3 | 6 + X + 4 |
  |---------------|-----------|-----------|-----------|-------------|-----------|-----------|-----------|-----------|-----------|-----------|-----------|
  | SIZE          |        2 Bytes        |  1 Byte   |   1 Byte    |  1 Bytes  |  1 Bytes  | Variable  |        2 Bytes        |  1 Bytes  |  1 Bytes  |
  | DESCRIPTION   |       SYNC_WORD       | COMMAND   | COMPRESSION |     DATA_LENGTH(X)    | Payload   |       CHECKSUM        |  DEVICEID |  STATUS   |
  | GB TO PRINTER |    0x88   |    0x33   | See Below | See Below   | Low Byte  | High Byte | See Below |       See Below       |    0x00   |    0x00   |
  | TO PRINTER    |    0x00   |    0x00   |    0x00   |   0x00      |    0x00   |    0x00   |    0x00   |    0x00   |    0x00   |    0x81   | See Below |

  * Header is the Command, Compression and Data Length
  * Command field may be either Initialize (0x01), Data (0x04), Print (0x02), or Inquiry (0x0F).
  * Compression field is a compression indicator. No compression (0x00), Yes Compression (0x01)
  * Payload byte count size depends on the value of the `DATA_LENGTH` field.
  * Checksum is 2 bytes of data representing the sum of the header + all data in the data portion of the packet
  * Status byte is a bitfield byte indicating various status of the printer itself. (e.g. If it is still printing)
*/

/* Sync Word */
#define GBP_SYNC_WORD_0       0x88    // 0b10001000
#define GBP_SYNC_WORD_1       0x33    // 0b00110011
#define GBP_SYNC_WORD         0x8833  // 0b1000100000110011

/* Command Byte */
// General Sequence: INIT --> DATA --> INQY --> ... --> DATA --> INQY --> ...
#define GBP_COMMAND_INIT      0x01    // 0b00000001 // Typically 10 bytes packet
#define GBP_COMMAND_PRINT     0x02    // 0b00000010 // Print instructions
#define GBP_COMMAND_DATA      0x04    // 0b00000100 // Typically 650 bytes packet (10byte header + 640byte image)
#define GBP_COMMAND_BREAK     0x08    // 0b00001000 // Means to forcibly stops printing
#define GBP_COMMAND_INQUIRY   0x0F    // 0b00001111 // Always reports current status

/* Compression Flag */
#define GBP_COMPRESSION_DISABLED  0x00
#define GBP_COMPRESSION_ENABLED   0x01 // Run Length Encoded Compression

/* Device ID Byte */
// According to the GB programming manual. This is a device ID number.
// [1bit:MSB Always'0x1'][7bits: Device Number ID ]
// Where the MSB is always 1 and the lower 7 bits is the device number.
#define GBP_DEVICE_ID         0x81    // 0b10000001 // Gameboy Pocket Printer ID = 0x1

/* Print Instruction Payload (4 data bytes) (Section 4.2 Print Instruction Packet In Document DMG-06-4216-001-A) */
#define GBP_PRINT_INSTRUCT_PAYLOAD_SIZE 4 //  (4 data bytes)
#define GBP_PRINT_INSTRUCT_INDEX_NUM_OF_SHEETS    0 // 0-255 (1 in the example). 0 means line feed only. 1 feed = 2.64 mm
#define GBP_PRINT_INSTRUCT_INDEX_NUM_OF_LINEFEED  1 // High Nibble 4 bits represents the number of feeds before printing. Lower Nibble is 4 bits, representing the number of feeds after printing.
#define GBP_PRINT_INSTRUCT_INDEX_PALETTE_VALUE    2 // Default is 00. Palettes are defined by every 2 bits beginning from high bit. (See Chapter 2, Section 2.3, Character RAM. In Document DMG-06-4216-001-A)
#define GBP_PRINT_INSTRUCT_INDEX_PRINT_DENSITY    3 // 0x00-0x7F. Default values are 0x40 and 0x80 or greater.

/* State Byte Bit Position */
#define GBP_STATUS_BIT_LOWBAT      7 // Battery Too Low
#define GBP_STATUS_BIT_ER2         6 // Other Error
#define GBP_STATUS_BIT_ER1         5 // Paper Jam
#define GBP_STATUS_BIT_ER0         4 // Packet Error (e.g. likely gameboy program failure)
#define GBP_STATUS_BIT_UNTRAN      3 // Unprocessed Data
#define GBP_STATUS_BIT_FULL        2 // Image Data Full
#define GBP_STATUS_BIT_BUSY        1 // Printer Busy
#define GBP_STATUS_BIT_SUM         0 // Checksum Error

#define GBP_STATUS_MASK_LOWBAT      (1<<7) // Battery Too Low
#define GBP_STATUS_MASK_ER2         (1<<6) // Other Error
#define GBP_STATUS_MASK_ER1         (1<<5) // Paper Jam
#define GBP_STATUS_MASK_ER0         (1<<4) // Packet Error (e.g. likely gameboy program failure)
#define GBP_STATUS_MASK_UNTRAN      (1<<3) // Unprocessed Data
#define GBP_STATUS_MASK_FULL        (1<<2) // Image Data Full
#define GBP_STATUS_MASK_BUSY        (1<<1) // Printer Busy
#define GBP_STATUS_MASK_SUM         (1<<0) // Checksum Error


/*******************************************************************************
 * Gameboy Tile
*******************************************************************************/
#define GBP_TILE_SIZE_IN_BYTE 16


/*******************************************************************************
  MACROS
*******************************************************************************/

#define gpb_setBit(X,BITPOS)   (X | (1 << BITPOS))
#define gpb_unsetBit(X,BITPOS) (X & (~(1 << BITPOS)))
#define gpb_getBit(X,BITPOS)   ((X & (1 << BITPOS)) > 0)

#define gpb_status_bit_update_low_battery(X,SET)       X = SET ? gpb_setBit(X, GBP_STATUS_BIT_LOWBAT) : gpb_unsetBit(X, GBP_STATUS_BIT_LOWBAT)
#define gpb_status_bit_update_other_error(X,SET)       X = SET ? gpb_setBit(X, GBP_STATUS_BIT_ER2)    : gpb_unsetBit(X, GBP_STATUS_BIT_ER2)
#define gpb_status_bit_update_paper_jam(X,SET)         X = SET ? gpb_setBit(X, GBP_STATUS_BIT_ER1)    : gpb_unsetBit(X, GBP_STATUS_BIT_ER1)
#define gpb_status_bit_update_packet_error(X,SET)      X = SET ? gpb_setBit(X, GBP_STATUS_BIT_ER0)    : gpb_unsetBit(X, GBP_STATUS_BIT_ER0)
#define gpb_status_bit_update_unprocessed_data(X,SET)  X = SET ? gpb_setBit(X, GBP_STATUS_BIT_UNTRAN) : gpb_unsetBit(X, GBP_STATUS_BIT_UNTRAN)
#define gpb_status_bit_update_print_buffer_full(X,SET) X = SET ? gpb_setBit(X, GBP_STATUS_BIT_FULL)   : gpb_unsetBit(X, GBP_STATUS_BIT_FULL)
#define gpb_status_bit_update_printer_busy(X,SET)      X = SET ? gpb_setBit(X, GBP_STATUS_BIT_BUSY)   : gpb_unsetBit(X, GBP_STATUS_BIT_BUSY)
#define gpb_status_bit_update_checksum_error(X,SET)    X = SET ? gpb_setBit(X, GBP_STATUS_BIT_SUM)    : gpb_unsetBit(X, GBP_STATUS_BIT_SUM)

#define gpb_status_bit_getbit_low_battery(X)       gpb_getBit(X, GBP_STATUS_BIT_LOWBAT)
#define gpb_status_bit_getbit_other_error(X)       gpb_getBit(X, GBP_STATUS_BIT_ER2)
#define gpb_status_bit_getbit_paper_jam(X)         gpb_getBit(X, GBP_STATUS_BIT_ER1)
#define gpb_status_bit_getbit_packet_error(X)      gpb_getBit(X, GBP_STATUS_BIT_ER0)
#define gpb_status_bit_getbit_unprocessed_data(X)  gpb_getBit(X, GBP_STATUS_BIT_UNTRAN)
#define gpb_status_bit_getbit_print_buffer_full(X) gpb_getBit(X, GBP_STATUS_BIT_FULL)
#define gpb_status_bit_getbit_printer_busy(X)      gpb_getBit(X, GBP_STATUS_BIT_BUSY)
#define gpb_status_bit_getbit_checksum_error(X)    gpb_getBit(X, GBP_STATUS_BIT_SUM)


#ifdef __cplusplus
}
#endif
#endif // GAMEBOY_PRINTER_PROTOCOL_H