/*
  # Gameboy Printer Packet
  * Author:  Brian Khuu (2020-08-09)
  * Licence: GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
  * Purpose: This module focus on parsing a stream of bytes as gbp packets

  ******************************************************************************

*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "gameboy_printer_protocol.h"
#include "gbp_serial_io.h"
#include "gbp_pkt.h"

bool gbp_pkt_init(gbp_pktBuff_t *_pktBuff)
{
  _pktBuff->pktByteIndex = 0;
  return true;
}

// returns true if packet is received
bool gbp_pkt_processByte(const uint8_t _byte, gbp_pktBuff_t *_pktBuff)
{
  /*
    [ 00 ][ 01 ][ 02 ][ 03 ][ 04 ][ 05 ][5+X ][5+X+1][5+X+2][5+X+3][5+X+4]
    [SYNC][SYNC][COMM][COMP][LEN0][LEN1][DATA][CSUM0][CSUM1][DUMMY][DUMMY]
  */
  _pktBuff->received = GBP_REC_NONE;

  // Parsing fixed packet header
  if(_pktBuff->pktByteIndex <= 5)
  {
    if (_pktBuff->pktByteIndex == 1)
    {
      _pktBuff->command     = 0;
      _pktBuff->compression = 0;
      _pktBuff->dataLength  = 0;
      _pktBuff->printerID   = 0;
      _pktBuff->status      = 0;
      _pktBuff->payloadBuffOffset = 0;
      _pktBuff->payloadBuffSize = 0;
    }

    switch (_pktBuff->pktByteIndex)
    {
      case 0: _pktBuff->pktByteIndex = (_byte == 0x88) ? 1 : 0; break;
      case 1: _pktBuff->pktByteIndex = (_byte == 0x33) ? 2 : 0; break;
      case 2: _pktBuff->pktByteIndex++; _pktBuff->command = _byte; break;
      case 3: _pktBuff->pktByteIndex++; _pktBuff->compression = _byte; break;
      case 4: _pktBuff->pktByteIndex++; _pktBuff->dataLength =  ((uint16_t)_byte<<0)&0x00FF; break;
      case 5: _pktBuff->pktByteIndex++; _pktBuff->dataLength |= ((uint16_t)_byte<<8)&0xFF00; break;
      default: break;
    }

    // Data packets are streamed
    if ((_pktBuff->pktByteIndex == 6)&&(_pktBuff->command == GBP_COMMAND_DATA)&&(_pktBuff->dataLength!=0))
    {
      // Indicate received packet
      _pktBuff->received = GBP_REC_GOT_DATA_HEADER;
      return true;
    }

    return false;
  }

  if ((_pktBuff->command == GBP_COMMAND_DATA)&&(_pktBuff->dataLength!=0))
  {
    // Parse tile
    if (_pktBuff->pktByteIndex < (8+_pktBuff->dataLength))
    {
      const uint16_t payloadIndex = _pktBuff->pktByteIndex - 6;
      const uint16_t offset  = (payloadIndex/GBP_PKT_TILE_SIZE_IN_BYTE)*GBP_PKT_TILE_SIZE_IN_BYTE;
      const uint8_t buffsize = payloadIndex%GBP_PKT_TILE_SIZE_IN_BYTE + 1;
      _pktBuff->payloadBuff[buffsize-1] = _byte;
      if (buffsize == GBP_PKT_TILE_SIZE_IN_BYTE)
      {
        _pktBuff->payloadBuffOffset = offset;
        _pktBuff->payloadBuffSize = buffsize;
        _pktBuff->received = GBP_REC_GOT_DATA_TILE;
      }
    }

    // Increment
    if (_pktBuff->pktByteIndex == (8+_pktBuff->dataLength))
    {
      // Printer ID Found
      _pktBuff->printerID = _byte;
    }
    else if (_pktBuff->pktByteIndex == (8+_pktBuff->dataLength + 1))
    {
      // Status Byte Found
      // End of packet reached
      _pktBuff->status = _byte;
      _pktBuff->pktByteIndex = 0;
      return false;
    }
    _pktBuff->pktByteIndex++;
    return _pktBuff->received != GBP_REC_NONE;
  }
  else
  {
    if (_pktBuff->command == GBP_COMMAND_PRINT)
    {
      if ((_pktBuff->payloadBuffSize < _pktBuff->dataLength) &&
        (_pktBuff->payloadBuffSize < GBP_PKT_PAYLOAD_BUFF_SIZE_IN_BYTE))
      {
        _pktBuff->payloadBuff[_pktBuff->payloadBuffSize] = _byte;
        _pktBuff->payloadBuffSize = _pktBuff->payloadBuffSize + 1;
      }
    }

    // Increment
    if (_pktBuff->pktByteIndex == (8+_pktBuff->dataLength))
    {
      _pktBuff->printerID = _byte;
    }
    else if (_pktBuff->pktByteIndex == (8+_pktBuff->dataLength + 1))
    {
      // End of packet reached
      _pktBuff->status = _byte;
      _pktBuff->pktByteIndex = 0;
      // Indicate received packet
      _pktBuff->received = GBP_REC_GOT_PACKET;
      return true;
    }
    _pktBuff->pktByteIndex++;
    return false;
  }

  return false;
}
