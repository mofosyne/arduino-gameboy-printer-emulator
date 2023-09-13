/*************************************************************************
 *
 * Gameboy Printer Packet Parser
 * Part of GAMEBOY PRINTER EMULATION PROJECT V2 (Arduino)
 * Copyright (C) 2020 Brian Khuu
 *
 * PURPOSE: This module focus on parsing a stream of bytes as gbp packets
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

#include <stdint.h>   // uint8_t
#include <stddef.h>   // size_t
#include <stdbool.h>  // bool
#include "gameboy_printer_protocol.h"

#define GBP_PKT_PAYLOAD_BUFF_SIZE_IN_BYTE GBP_TILE_SIZE_IN_BYTE

typedef enum
{
  GBP_REC_NONE,
  GBP_REC_GOT_PACKET,
  /* Streaming Packet */
  GBP_REC_GOT_PAYLOAD_PARTAL,
  GBP_REC_GOT_PACKET_END
} gbp_received_t;

typedef struct
{
  gbp_received_t received;
  uint16_t pktByteIndex;

  /* Packet Information */
  uint8_t command;
  uint8_t compression;
  uint16_t dataLength;
  uint8_t printerID;
  uint8_t status;

  /* Decompressor */
  size_t buffIndex;
  bool compressedRun;
  bool repeatByteGet;
  uint8_t repeatByte;
  uint8_t loopRunLength;

} gbp_pkt_t;

typedef struct
{
  // This is the tile data accumulator
  unsigned char count;
  unsigned char tile[GBP_TILE_SIZE_IN_BYTE];
} gbp_pkt_tileAcc_t;


bool gbp_pkt_init(gbp_pkt_t *_pkt);
bool gbp_pkt_reset(gbp_pkt_t *_pkt);
bool gbp_pkt_processByte(gbp_pkt_t *_pkt, const uint8_t _byte, uint8_t buffer[], uint8_t *bufferSize, const size_t bufferMax);
bool gbp_pkt_decompressor(gbp_pkt_t *_pkt, const uint8_t buff[], const size_t buffSize, gbp_pkt_tileAcc_t *tileBuff);
bool gbp_pkt_tileAccu_tileReadyCheck(gbp_pkt_tileAcc_t *tileBuff);

/*******************************************************************************
 * Print Instruction
*******************************************************************************/

static inline int gbp_pkt_printInstruction_num_of_sheets(uint8_t payloadBuff[GBP_PRINT_INSTRUCT_PAYLOAD_SIZE])
{
  return (payloadBuff[GBP_PRINT_INSTRUCT_INDEX_NUM_OF_SHEETS]);
}

static inline int gbp_pkt_printInstruction_num_of_linefeed_before_print(uint8_t payloadBuff[GBP_PRINT_INSTRUCT_PAYLOAD_SIZE])
{
  return (payloadBuff[GBP_PRINT_INSTRUCT_INDEX_NUM_OF_LINEFEED] >> 4) & 0x0F;
}

static inline int gbp_pkt_printInstruction_num_of_linefeed_after_print(uint8_t payloadBuff[GBP_PRINT_INSTRUCT_PAYLOAD_SIZE])
{
  return (payloadBuff[GBP_PRINT_INSTRUCT_INDEX_NUM_OF_LINEFEED]) & 0x0F;
}

static inline int gbp_pkt_printInstruction_palette_value(uint8_t payloadBuff[GBP_PRINT_INSTRUCT_PAYLOAD_SIZE])
{
  return (payloadBuff[GBP_PRINT_INSTRUCT_INDEX_PALETTE_VALUE]);
}

static inline int gbp_pkt_printInstruction_print_density(uint8_t payloadBuff[GBP_PRINT_INSTRUCT_PAYLOAD_SIZE])
{
  return (payloadBuff[GBP_PRINT_INSTRUCT_INDEX_PRINT_DENSITY]);
}
