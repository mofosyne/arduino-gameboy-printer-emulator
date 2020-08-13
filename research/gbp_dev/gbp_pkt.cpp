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

bool gbp_pkt_init(gbp_pkt_t *_pkt)
{
  _pkt->pktByteIndex = 0;
  return true;
}

// returns true if packet is received
bool gbp_pkt_processByte( const uint8_t _byte, gbp_pkt_t *_pkt, uint8_t buffer[], uint8_t *bufferSize, const uint8_t bufferMax)
{
  /*
    [ 00 ][ 01 ][ 02 ][ 03 ][ 04 ][ 05 ][ 5+X ][5+X+1][5+X+2][5+X+3][5+X+4]
    [SYNC][SYNC][COMM][COMP][LEN0][LEN1][DATAX][CSUM0][CSUM1][DUMMY][DUMMY]
  */
  _pkt->received = GBP_REC_NONE;

  // Parsing fixed packet header
  if(_pkt->pktByteIndex <= 5)
  {
    if (_pkt->pktByteIndex == 1)
    {
      _pkt->command     = 0;
      _pkt->compression = 0;
      _pkt->dataLength  = 0;
      _pkt->printerID   = 0;
      _pkt->status      = 0;
      *bufferSize = 0;
    }

    switch (_pkt->pktByteIndex)
    {
      case 0: _pkt->pktByteIndex = (_byte == 0x88) ? 1 : 0; break;
      case 1: _pkt->pktByteIndex = (_byte == 0x33) ? 2 : 0; break;
      case 2: _pkt->pktByteIndex++; _pkt->command = _byte; break;
      case 3: _pkt->pktByteIndex++; _pkt->compression = _byte; break;
      case 4: _pkt->pktByteIndex++; _pkt->dataLength =  ((uint16_t)_byte<<0)&0x00FF; break;
      case 5: _pkt->pktByteIndex++; _pkt->dataLength |= ((uint16_t)_byte<<8)&0xFF00; break;
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
  if ((6 <= _pkt->pktByteIndex) && (_pkt->pktByteIndex < (6+_pkt->dataLength)))
  {
    // Byte is from payload... add to buffer
    const uint16_t payloadIndex = _pkt->pktByteIndex - 6;
    //const uint16_t offset       = (payloadIndex/bufferMax) * bufferMax;
    const uint16_t bufferUsage  = payloadIndex%bufferMax + 1;
    buffer[bufferUsage - 1] = _byte;
    *bufferSize = bufferUsage;
    if (bufferUsage == _pkt->dataLength)
    {
      // Fits fully in buffer
    }
    else if (bufferUsage == bufferMax)
    {
      _pkt->received = GBP_REC_GOT_PAYLOAD_PARTAL;
    }
  } else if (_pkt->pktByteIndex == (6+_pkt->dataLength))
  {
    *bufferSize = _pkt->dataLength%bufferMax;
  }

  // Increment
  if (_pkt->pktByteIndex == (8+_pkt->dataLength))
  {
    _pkt->printerID = _byte;
  }
  else if (_pkt->pktByteIndex == (8+_pkt->dataLength + 1))
  {
    // End of packet reached
    _pkt->status = _byte;
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
