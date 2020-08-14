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
bool gbp_pkt_processByte(gbp_pkt_t *_pkt,  const uint8_t _byte, uint8_t buffer[], uint8_t *bufferSize, const uint8_t bufferMax)
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


/*******************************************************************************
*******************************************************************************/

static bool gbp_pkt_tileAccumulator(gbp_pkt_tileAcc_t *tileBuff, const uint8_t byte)
{
  if (tileBuff->count == 16)
  {
    return true;
  }

  tileBuff->tile[tileBuff->count] = byte;
  tileBuff->count++;
  return tileBuff->count == 16;
}

bool gbp_pkt_get_tile(gbp_pkt_tileAcc_t *tileBuff)
{
  if (tileBuff->count < 16)
    return false;

  tileBuff->count = 0;
  return true;
}

/*******************************************************************************
*******************************************************************************/

bool gbp_pkt_decompressor(gbp_pkt_t *_pkt, const uint8_t buff[], const size_t buffSize, gbp_pkt_tileAcc_t *tileBuff)
{
  static size_t buffIndex = 0;

#if 0
  if (buffIndex == 0)
  {
    for (size_t i = 0 ; i < buffSize ; i++)
    {
      printf("%02X ", buff[i]);
    }
    printf("\r\n");
  }
#endif


  if (!_pkt->compression)
  {
    while (1)
    {
      // for (buffIndex = 0; buffIndex < buffSize ; buffIndex++)
      if (buffIndex < buffSize)
      {
        if (gbp_pkt_tileAccumulator(tileBuff, buff[buffIndex++]))
        {
          return true; // Got tile
        }
      }
      else
      {
        buffIndex = 0; // Reset for next buffer
        return false;
      }
    }
  }
  else
  {
    while (1)
    {
      // for (buffIndex = 0; buffIndex < buffSize ; buffIndex++)
      if (buffIndex < buffSize)
      {
        // Compressed payload (Run length encoding)
        // Refactor: Move to struct
        static bool compressedRun;
        static bool repeatByteGet;
        static uint8_t repeatByte;
        static uint8_t loopRunLength;
        if (loopRunLength == 0)
        {
          // Start of either a raw run of byte or compressed run of byte
          uint8_t b = buff[buffIndex++];
          if (b <= 0x7F)
          {
            // (0x7F=127) its a classical run, read the n bytes after (Raphael-Boichot)
            loopRunLength = b + 1;
            compressedRun = false;
          }
          else
          {
            // (0x80 = 128) for (b>128) its a compressed run, read the next byte and repeat (Raphael-Boichot)
            loopRunLength = b - 128 + 2;
            compressedRun = true;
            repeatByteGet = true;
          }
        }
        else if (repeatByteGet)
        {
          // Grab loop byte
          uint8_t b = buff[buffIndex++];
          repeatByte = b;
          repeatByteGet = false;
        }
        else
        {
          const uint8_t b = (compressedRun) ? repeatByte : buff[buffIndex++];
#if 0
          static int ii = 0;
          if (ii++ < 100)
          {
            if (compressedRun)
              printf("[com len=%3d b='%02X'] = %02X \r\n", loopRunLength, repeatByte, b);
            else
              printf("[raw len=%3d ] = %02X \r\n", loopRunLength, b);
          }
#endif
          loopRunLength--;
          if (gbp_pkt_tileAccumulator(tileBuff, b))
          {
            return true; // Got tile
          }
        }
      }
      else
      {
        buffIndex = 0; // Reset for next buffer
        return false;
      }
    }
  }
  return false;
}

#if 0
bool gbp_pkt_decompressor(gbp_pkt_t *_pkt, const uint8_t buff[], const size_t buffSize, uint8_t tileBuff[16])
{
  // Assumption: No mixure of compression and non compression packets
  static size_t i = 0;
  while (1)
  {
    if (!(i < buffSize))
    {
      // Done processing incoming buffer
      i = 0;
      break;
    }

    // Get Byte
    const uint8_t b = buff[i++];

    // Process Byte
    if (_pkt->compression)
    {
      // Compressed payload (Run length encoding)
      // Refactor: Move to struct
      static bool compressedRun;
      static bool repeatByteGet;
      static uint8_t repeatByte;
      static uint8_t loopRunLength;
      if (loopRunLength == 0)
      {
        // Start of either a raw run of byte or compressed run of byte
        if (b <= 0x7F)
        {
          // (0x7F=127) its a classical run, read the n bytes after
          loopRunLength = b+1;
          compressedRun = false;
          repeatByteGet = true;
        }
        else
        {
          // for (b>128) its a compressed run, read the next byte and repeat (Raphael-Boichot)
          loopRunLength = b-128+2;
          compressedRun = true;
        }
      }
      else if (compressedRun && repeatByteGet)
      {
        // Grab loop byte
        repeatByte = b;
        repeatByteGet = false;
      }
      else
      {
        loopRunLength--;
        printf("[%s %3d '%02X' %02X] \r\n", compressedRun?"c":"r", loopRunLength, repeatByte, b);

        if (compressedRun)
        {
          if (gbp_pkt_tileAccumulator(&(_pkt->tileCounter), repeatByte, tileBuff))
          {
            return true;
          }
        }
        else
        {
          if (gbp_pkt_tileAccumulator(&(_pkt->tileCounter), b, tileBuff))
          {
            return true;
          }
        }
      }
    }
    else
    {
      // Raw payload passthough
      if(gbp_pkt_tileAccumulator(&(_pkt->tileCounter), b, tileBuff))
      {
        return true;
      }
    }
  }

  return false;
}
#endif