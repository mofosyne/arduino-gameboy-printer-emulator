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

#include <stdint.h>
#include <stdbool.h>

#include "gameboy_printer_protocol.h"
#include "gbp_serial_io.h"
#include "gbp_pkt.h"

bool gbp_pkt_init(gbp_pkt_t *_pkt)
{
  _pkt->pktByteIndex = 0;
  _pkt->buffIndex    = 0;
  return true;
}

bool gbp_pkt_reset(gbp_pkt_t *_pkt)
{
  _pkt->pktByteIndex = 0;
  _pkt->buffIndex    = 0;
  return true;
}

// returns true if packet is received
bool gbp_pkt_processByte(gbp_pkt_t *_pkt, const uint8_t _byte, uint8_t buffer[], uint8_t *bufferSize, const size_t bufferMax)
{
  /*
    [ 00 ][ 01 ][ 02 ][ 03 ][ 04 ][ 05 ][ 5+X ][5+X+1][5+X+2][5+X+3][5+X+4]
    [SYNC][SYNC][COMM][COMP][LEN0][LEN1][DATAX][CSUM0][CSUM1][DUMMY][DUMMY]
  */

  // Dev Note: Minimum required size of 4 bytes for printer instruction packet
  //  data payload can be streamed so doesn't have to fit full size
  if (bufferMax < 4)
    return false;

  _pkt->received = GBP_REC_NONE;

  // Parsing fixed packet header
  if (_pkt->pktByteIndex <= 5)
  {
    if (_pkt->pktByteIndex == 1)
    {
      _pkt->command     = 0;
      _pkt->compression = 0;
      _pkt->dataLength  = 0;
      _pkt->printerID   = 0;
      _pkt->status      = 0;
      *bufferSize       = 0;
    }

    switch (_pkt->pktByteIndex)
    {
      case 0: _pkt->pktByteIndex = (_byte == 0x88) ? 1 : 0; break;
      case 1: _pkt->pktByteIndex = (_byte == 0x33) ? 2 : 0; break;
      case 2:
        _pkt->pktByteIndex++;
        _pkt->command = _byte;
        break;
      case 3:
        _pkt->pktByteIndex++;
        _pkt->compression = _byte;
        break;
      case 4:
        _pkt->pktByteIndex++;
        _pkt->dataLength = ((uint16_t)_byte << 0) & 0x00FF;
        break;
      case 5:
        _pkt->pktByteIndex++;
        _pkt->dataLength |= ((uint16_t)_byte << 8) & 0xFF00;
        break;
      default: break;
    }

    // Data packets are streamed
    if (_pkt->pktByteIndex == 6)
    {
      if (bufferMax > _pkt->dataLength)
      {
        // Payload fits into buffer
        return false;
      }
      else
      {
        // Must stream...
        _pkt->received = GBP_REC_GOT_PACKET;
        return true;
      }
    }

    return false;
  }

  // Capture Bytes to buffer if needed
  if ((6 <= _pkt->pktByteIndex) && (_pkt->pktByteIndex < (6 + _pkt->dataLength)))
  {
    // Byte is from payload... add to buffer
    const uint16_t payloadIndex = _pkt->pktByteIndex - 6;
    //const uint16_t offset       = (payloadIndex/bufferMax) * bufferMax;
    const uint16_t bufferUsage = payloadIndex % bufferMax + 1;
    buffer[bufferUsage - 1]    = _byte;
    *bufferSize                = bufferUsage;
    if (bufferUsage == _pkt->dataLength)
    {
      // Fits fully in buffer
    }
    else if (bufferUsage == bufferMax)
    {
      _pkt->received = GBP_REC_GOT_PAYLOAD_PARTAL;
    }
  }
  else if (_pkt->pktByteIndex == (6 + _pkt->dataLength))
  {
    *bufferSize = _pkt->dataLength % bufferMax;
  }

  // Increment
  if (_pkt->pktByteIndex == (8 + _pkt->dataLength))
  {
    _pkt->printerID = _byte;
  }
  else if (_pkt->pktByteIndex == (8 + _pkt->dataLength + 1))
  {
    // End of packet reached
    _pkt->status       = _byte;
    _pkt->pktByteIndex = 0;
    // Indicate received packet
    if (bufferMax > _pkt->dataLength)
    {
      // Payload fits into buffer
      _pkt->received = GBP_REC_GOT_PACKET;
      return true;
    }
    else
    {
      // Finished streaming
      _pkt->received = GBP_REC_GOT_PACKET_END;
      return true;
    }
  }

  _pkt->pktByteIndex++;
  return _pkt->received != GBP_REC_NONE;
}


/*******************************************************************************
  Tile Accumulator
*******************************************************************************/

static bool gbp_pkt_tileAccu_insertByte(gbp_pkt_tileAcc_t *tileBuff, const uint8_t byte)
{
  if (tileBuff->count == GBP_TILE_SIZE_IN_BYTE)
    return true;

  tileBuff->tile[tileBuff->count] = byte;
  tileBuff->count++;
  return tileBuff->count == GBP_TILE_SIZE_IN_BYTE;
}

bool gbp_pkt_tileAccu_tileReadyCheck(gbp_pkt_tileAcc_t *tileBuff)
{
  if (tileBuff->count < GBP_TILE_SIZE_IN_BYTE)
    return false;

  tileBuff->count = 0;
  return true;
}


/*******************************************************************************
*******************************************************************************/

bool gbp_pkt_decompressor(gbp_pkt_t *_pkt, const uint8_t buff[], const size_t buffSize, gbp_pkt_tileAcc_t *tileBuff)
{
  if (!_pkt->compression)
  {
    // Uncompressed payload // e.g. Gameboy Camera
    while (1)
    {
      // for (buffIndex = 0; buffIndex < buffSize ; buffIndex++)
      if (_pkt->buffIndex < buffSize)
      {
        if (gbp_pkt_tileAccu_insertByte(tileBuff, buff[_pkt->buffIndex++]))
        {
          return true;  // Got tile
        }
      }
      else
      {
        _pkt->buffIndex = 0;  // Reset for next buffer
        return false;
      }
    }
  }
  else
  {
    // Compressed payload (Run length encoding) // e.g. Pokemon Trading Card
    while (1)
    {
      // for (buffIndex = 0; buffIndex < buffSize ; buffIndex++)
      // Dev Note: Need to also check if we have completed adding looped byte even if all incoming bytes have been read
      if ((_pkt->buffIndex < buffSize) || (_pkt->compressedRun && !_pkt->repeatByteGet && (_pkt->loopRunLength != 0)))
      {
        // Incoming Bytes Avaliable
        if (_pkt->loopRunLength == 0)
        {
          // Start of either a raw run of byte or compressed run of byte
          uint8_t b = buff[_pkt->buffIndex++];
          if (b < 128)
          {
            // (0x7F=127) its a classical run, read the n bytes after (Raphael-Boichot)
            _pkt->loopRunLength = b + 1;
            _pkt->compressedRun = false;
          }
          else if (b >= 128)
          {
            // (0x80 = 128) its a compressed run, read the next byte and repeat (Raphael-Boichot)
            _pkt->loopRunLength = b - 128 + 2;
            _pkt->compressedRun = true;
            _pkt->repeatByteGet = true;
          }
        }
        else if (_pkt->repeatByteGet)
        {
          // Grab loop byte
          uint8_t b           = buff[_pkt->buffIndex++];
          _pkt->repeatByte    = b;
          _pkt->repeatByteGet = false;
        }
        else
        {
          const uint8_t b = (_pkt->compressedRun) ? _pkt->repeatByte : buff[_pkt->buffIndex++];
          _pkt->loopRunLength--;
          if (gbp_pkt_tileAccu_insertByte(tileBuff, b))
          {
            return true;  // Got tile
          }
        }
      }
      else
      {
        _pkt->buffIndex = 0;  // Reset for next buffer
        return false;
      }
    }
  }
  return false;
}
