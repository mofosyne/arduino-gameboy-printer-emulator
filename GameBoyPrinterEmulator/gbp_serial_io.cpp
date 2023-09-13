/*************************************************************************
 *
 * Gameboy Printer Serial IO
 * Part of GAMEBOY PRINTER EMULATION PROJECT V2 (Arduino)
 *  Copyright (C) 2020 Brian Khuu
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
/*******************************************************************************
  ## Dev Note (2020-08-09):
    I was trying to do most of the processing here, however I think it makes
    more sense to simply grab a stream of packets then process that in a separate
    module. This will also make maintainance easier this way.
*******************************************************************************/

#include <stdint.h>  // uint8_t
#include <stddef.h>  // size_t

#include "gameboy_printer_protocol.h"
#include "gbp_serial_io.h"
#include "gbp_cbuff.h"

/******************************************************************************/

#define GBP_PKT10_TIMEOUT_MS 500

// Testing
//#define TEST_CHECKSUM_FORCE_FAIL
//#define TEST_PRETEND_BUFFER_FULL

// Feature
//#define FEATURE_CHECKSUM_SUPPORTED ///< WIP

#define GBP_BUSY_PACKET_COUNT 20  // 68 Inquiry packets is generally approximately how long it takes for a real printer to print. This is not a real printer so can be shorter


/******************************************************************************/

typedef enum
{
  GBP_SIO_MODE_RESET,
  GBP_SIO_MODE_8BITS,
  GBP_SIO_MODE_16BITS_BIG_ENDIAN,
  GBP_SIO_MODE_16BITS_LITTLE_ENDIAN,
} gpb_sio_mode_t;

// SIO Serial Input Output Psudo SPI
static struct
{
  bool SINOutputPinState;  /// GPIO state of output
  // Preamble Sync
  bool syncronised;   ///< True When Preamble Found
  uint16_t preamble;  ///< Scanning for Preamble
  // Byte Tx/Rx
  uint16_t bitMaskMap;  // gpb_sio_bitmaskmaps_t
  gpb_sio_mode_t mode;
  uint16_t rx_buff;
  uint16_t tx_buff;
} gpb_sio;


/******************************************************************************/
/******************************************************************************/

typedef enum gbp_pktIO_parse_state_t
{
  // Indicates the stage of the parsing processing (syncword is not parsed)
  // [PREAMBLE][HEADER][DATA][CHECKSUM][DUMMY]
  // [GBP_SYNC_WORD][GBP_COMMAND][DATA][CRC][GBP_STATUS]
  GBP_PKT10_PARSE_HEADER_COMMAND_AND_COMPRESSION,
  GBP_PKT10_PARSE_HEADER_DATA_LENGTH,
  GBP_PKT10_PARSE_DATA_PAYLOAD,
  GBP_PKT10_PARSE_CHECKSUM,
  GBP_PKT10_PARSE_DUMMY
} gbp_pktIO_parse_state_t;

static struct
{
  // Initialized Command
  bool initReceived;
  uint32_t timeout_ms;

  // Circular Buffer : To store raw packet stream for packet processor
  gpb_cbuff_t dataBuffer;

  // What packets was received for internal processing
  bool printInstructionReceived;  ///< Print Instruction Command
  bool dataPacketReceived;        ///< Data Packet Command
  bool dataEndPacketReceived;     ///< Data End Packet Command (Data size of 0)
  bool breakPacketReceived;       ///< Break Packet Command
  bool nulPacketReceived;         ///< Inquiry Packet Command

  // Packet Parsing
  gbp_pktIO_parse_state_t packetState;
  uint8_t command;
  uint8_t compression;
  uint16_t data_length;
  uint16_t data_i;
  uint16_t checksum;      ///< For data integrity check
  uint16_t checksumCalc;  ///< For data integrity check
  uint16_t statusBuffer;  ///< This is send on every packet in the dummy data region

  // Status Packet Sequencing (For faking the printer for more advance games)
  int busyPacketCountdown;
  int untransPacketCountdown;
  int dataPacketCountdown;

  // Dev
  uint16_t dataBufferWaterline;
} gpb_pktIO;


/*******************************************************************************
 * Serial IO
*******************************************************************************/

static bool gpb_sio_next(const gpb_sio_mode_t mode, const uint16_t txdata)
{
  gpb_sio.rx_buff = 0;
  gpb_sio.mode    = mode;
  switch (mode)
  {
    case GBP_SIO_MODE_RESET:
      gpb_sio.bitMaskMap        = 0;
      gpb_sio.SINOutputPinState = false;
      gpb_sio.tx_buff           = 0xFFFF;
      gpb_sio.syncronised       = false;
      break;
    case GBP_SIO_MODE_8BITS:
      gpb_sio.bitMaskMap = (uint16_t)1 << (8 - 1);
      gpb_sio.tx_buff    = txdata;
      break;
    case GBP_SIO_MODE_16BITS_BIG_ENDIAN:
      gpb_sio.bitMaskMap = (uint16_t)1 << (16 - 1);
      gpb_sio.tx_buff    = txdata;
      break;
    case GBP_SIO_MODE_16BITS_LITTLE_ENDIAN:
      gpb_sio.bitMaskMap = (uint16_t)1 << (16 - 1);
      gpb_sio.tx_buff    = 0;
      gpb_sio.tx_buff |= ((txdata >> 8) & 0x00FF);
      gpb_sio.tx_buff |= ((txdata << 8) & 0xFF00);
      break;
  }
  return true;
}

static uint16_t gpb_sio_getWord()
{
  uint16_t temp = 0;
  switch (gpb_sio.mode)
  {
    case GBP_SIO_MODE_RESET:
      break;
    case GBP_SIO_MODE_8BITS:
      temp |= ((gpb_sio.rx_buff >> 0) & 0x00FF);
      break;
    case GBP_SIO_MODE_16BITS_BIG_ENDIAN:
      temp |= ((gpb_sio.rx_buff >> 0) & 0xFFFF);
      break;
    case GBP_SIO_MODE_16BITS_LITTLE_ENDIAN:
      temp |= ((gpb_sio.rx_buff >> 8) & 0x00FF);
      temp |= ((gpb_sio.rx_buff << 8) & 0xFF00);
      break;
  }
  return temp;
}

static uint8_t gpb_sio_getByte(const int bytePos)
{
  switch (bytePos)
  {
    case 0: return ((gpb_sio.rx_buff >> 0) & 0xFF);
    case 1: return ((gpb_sio.rx_buff >> 8) & 0xFF);
    default: return 0;
  }
}


/******************************************************************************/

bool gbp_serial_io_timeout_handler(uint32_t elapsed_ms)
{
#if 0  // This redundancy causes an infinite loop in (Tsuri Seensei 2) GH-57
  if (gpb_pktIO.breakPacketReceived)
  {
    gpb_serial_io_reset();
    return true;
  }
#endif
  if (gpb_pktIO.timeout_ms > 0)
  {
    gpb_pktIO.timeout_ms = (gpb_pktIO.timeout_ms > elapsed_ms) ? (gpb_pktIO.timeout_ms - elapsed_ms) : 0;
    if (gpb_pktIO.timeout_ms == 0)
    {
      gpb_serial_io_reset();
      return true;
    }
  }
  return false;
}

size_t gbp_serial_io_dataBuff_getByteCount(void)
{
  return gpb_cbuff_Count(&gpb_pktIO.dataBuffer);
}

uint8_t gbp_serial_io_dataBuff_getByte(void)
{
  uint8_t b = 0;

  if (!gpb_cbuff_Dequeue(&gpb_pktIO.dataBuffer, &b))
    return 0;

  /* Packet Timeout Reset (Still Processing) */
  gpb_pktIO.timeout_ms = GBP_PKT10_TIMEOUT_MS;

  return b;
}

uint8_t gbp_serial_io_dataBuff_getByte_Peek(uint32_t offset)
{
  uint8_t b = 0;
  gpb_cbuff_Dequeue_Peek(&gpb_pktIO.dataBuffer, &b, offset);
  return b;
}

uint16_t gbp_serial_io_dataBuff_waterline(bool resetWaterline)
{
  uint16_t retval = gpb_pktIO.dataBufferWaterline;
  if (resetWaterline)
  {
    gpb_pktIO.dataBufferWaterline = 0;
  }
  return retval;
}

uint16_t gbp_serial_io_dataBuff_max(void)
{
  return gpb_cbuff_Capacity(&gpb_pktIO.dataBuffer);
}


/******************************************************************************/

bool gpb_serial_io_reset(void)
{
  gpb_sio.syncronised       = false;
  gpb_sio.rx_buff           = 0;
  gpb_sio.tx_buff           = 0;
  gpb_sio.SINOutputPinState = false;
  gpb_sio.bitMaskMap        = 0;

  // Clear all device status bits
  gpb_status_bit_update_low_battery(gpb_pktIO.statusBuffer, false);
  gpb_status_bit_update_other_error(gpb_pktIO.statusBuffer, false);
  gpb_status_bit_update_paper_jam(gpb_pktIO.statusBuffer, false);
  gpb_status_bit_update_packet_error(gpb_pktIO.statusBuffer, false);
  gpb_status_bit_update_unprocessed_data(gpb_pktIO.statusBuffer, false);
  gpb_status_bit_update_print_buffer_full(gpb_pktIO.statusBuffer, false);
  gpb_status_bit_update_printer_busy(gpb_pktIO.statusBuffer, false);
  gpb_status_bit_update_checksum_error(gpb_pktIO.statusBuffer, false);

  // Reset data buffer
  gpb_cbuff_Reset(&gpb_pktIO.dataBuffer);

#ifdef FEATURE_CHECKSUM_SUPPORTED
  // Reset temp Buffer
  gpb_cbuff_ResetTemp(&gpb_pktIO.dataBuffer);
#endif  // FEATURE_CHECKSUM_SUPPORTED

  return true;
}

bool gpb_serial_io_init(size_t buffSize, uint8_t *buffPtr)
{
  // reset status data
  gpb_pktIO.statusBuffer        = 0x0000;
  gpb_pktIO.statusBuffer        = GBP_DEVICE_ID << 8;
  gpb_pktIO.busyPacketCountdown = 0;

  // print data buffer
  gpb_cbuff_Init(&gpb_pktIO.dataBuffer, buffSize, buffPtr);

  // Packet Parsing Subsystem
  gpb_serial_io_reset();

  return true;
}


/******************************************************************************/

// Assumption: Only one gameboy printer connection required
// Return: pin state of GBP_SIN
#ifdef GBP_FEATURE_USING_RISING_CLOCK_ONLY_ISR
bool gpb_serial_io_OnRising_ISR(const bool GBP_SOUT)
#else
bool gpb_serial_io_OnChange_ISR(const bool GBP_SCLK, const bool GBP_SOUT)
#endif
{
  // Based on SIO Timing Chart. Page 30 of GameBoy PROGRAMMING MANUAL Version 1.0:
  // * CPOL=1 : Clock Polarity 1. Idle on high.
  // * CPHA=1 : Clock Phase 1. Change on falling. Check bit on rising edge.

  // # Pin input state
  // * GBP_SCLK : Serial Clock (1 = Rising Edge) (0 = Falling Edge)
  // * GBP_SOUT : Master Output Slave Input (This device is slave)

  // Scan for preamble
  if (!gpb_sio.syncronised)
  {
#ifndef GBP_FEATURE_USING_RISING_CLOCK_ONLY_ISR
    // Expecting rising edge
    if (!GBP_SCLK)
      return false;
#endif

    // Clocking bits on rising edge
    gpb_sio.preamble |= GBP_SOUT ? 1 : 0;

    // Sync Not Found? Keep scanning
    if ((gpb_sio.preamble & 0xFFFF) != GBP_SYNC_WORD)
    {
      gpb_sio.preamble <<= 1;
      return false;
    }

    // Preamble Found... Currently at rising edge
    // Start reading the packet header
    gpb_pktIO.packetState = GBP_PKT10_PARSE_HEADER_COMMAND_AND_COMPRESSION;
    gpb_sio.preamble      = 0;
    gpb_sio.syncronised   = true;
    gpb_sio_next(GBP_SIO_MODE_16BITS_BIG_ENDIAN, 0);
    return false;
  }

  /* Psudo SPI Engine */
  // Basically I have one bit acting as a mask moving across a word sized buffer
  if (gpb_sio.bitMaskMap > 0)
  {
    // Serial Transaction Is Active
#ifdef GBP_FEATURE_USING_RISING_CLOCK_ONLY_ISR
    // Rising Edge Clock (Rx Bit)
    gpb_sio.rx_buff |= GBP_SOUT ? (gpb_sio.bitMaskMap & 0xFFFF) : 0;  ///< Clocking bits on rising edge
    gpb_sio.bitMaskMap >>= 1;                                         ///< One tx/rx bit cycle complete, next bit now
    // Falling Edge Clock (Tx Bit) (Prep now for next rising edge)
    gpb_sio.SINOutputPinState = (gpb_sio.bitMaskMap & gpb_sio.tx_buff) > 0;
    if (gpb_sio.bitMaskMap > 0)
      return gpb_sio.SINOutputPinState;
#else
    if (GBP_SCLK)
    {
      // Rising Edge Clock (Rx Bit)
      gpb_sio.rx_buff |= GBP_SOUT ? (gpb_sio.bitMaskMap & 0xFFFF) : 0;  ///< Clocking bits on rising edge
      gpb_sio.bitMaskMap >>= 1;                                         ///< One tx/rx bit cycle complete, next bit now

      if (gpb_sio.bitMaskMap > 0)
        return gpb_sio.SINOutputPinState;
    }
    else
    {
      // Falling Edge Clock (Tx Bit)
      gpb_sio.SINOutputPinState = (gpb_sio.bitMaskMap & gpb_sio.tx_buff) > 0;
      return gpb_sio.SINOutputPinState;
    }
#endif
  }

  /****************************************************************************/

  /* There is uncaptured sync bytes so add it in */
  if (gpb_pktIO.packetState == GBP_PKT10_PARSE_HEADER_COMMAND_AND_COMPRESSION)
  {
    gpb_cbuff_EnqueueTemp(&gpb_pktIO.dataBuffer, GBP_SYNC_WORD_0);
    gpb_cbuff_EnqueueTemp(&gpb_pktIO.dataBuffer, GBP_SYNC_WORD_1);
  }

  /* Byte captured so send it downstream to packet processor */
  switch (gpb_sio.mode)
  {
    case GBP_SIO_MODE_8BITS:
      gpb_cbuff_EnqueueTemp(&gpb_pktIO.dataBuffer, (uint8_t)((gpb_sio.rx_buff >> 0) & 0xFF));
      break;
    case GBP_SIO_MODE_16BITS_BIG_ENDIAN:
    case GBP_SIO_MODE_16BITS_LITTLE_ENDIAN:
      if (gpb_pktIO.packetState == GBP_PKT10_PARSE_DUMMY)
      {
        // Virtual Printer --> Gameboy
        // Dev Notes: This is for dumping status byte. This is only done during
        //            the dummy buffer byte phase so might as well use these
        //            bytes for documenting response of the status byte
        gpb_cbuff_EnqueueTemp(&gpb_pktIO.dataBuffer, (uint8_t)((gpb_sio.tx_buff >> 8) & 0xFF));
        gpb_cbuff_EnqueueTemp(&gpb_pktIO.dataBuffer, (uint8_t)((gpb_sio.tx_buff >> 0) & 0xFF));
      }
      else
      {
        // Gameboy --> Virtual Printer
        gpb_cbuff_EnqueueTemp(&gpb_pktIO.dataBuffer, (uint8_t)((gpb_sio.rx_buff >> 8) & 0xFF));
        gpb_cbuff_EnqueueTemp(&gpb_pktIO.dataBuffer, (uint8_t)((gpb_sio.rx_buff >> 0) & 0xFF));
      }
      break;
    default:
      break;
  }

  // Track upper usage of buffer
  uint16_t waterline = gpb_cbuff_Count(&gpb_pktIO.dataBuffer);
  if (waterline > gpb_pktIO.dataBufferWaterline)
  {
    gpb_pktIO.dataBufferWaterline = waterline;
  }

  /* Packet Timeout Reset */
  gpb_pktIO.timeout_ms = GBP_PKT10_TIMEOUT_MS;

  /****************************************************************************/
  /* Packet State */
  switch (gpb_pktIO.packetState)
  {
    case GBP_PKT10_PARSE_HEADER_COMMAND_AND_COMPRESSION:
      {
        // Parse
        gpb_pktIO.command      = gpb_sio_getByte(1);
        gpb_pktIO.compression  = gpb_sio_getByte(0);
        gpb_pktIO.checksumCalc = 0;
        // Next Header Segment
        gpb_pktIO.packetState = GBP_PKT10_PARSE_HEADER_DATA_LENGTH;
        gpb_sio_next(GBP_SIO_MODE_16BITS_LITTLE_ENDIAN, 0);
      }
      break;
    case GBP_PKT10_PARSE_HEADER_DATA_LENGTH:
      {
        // Parse
        // GBP Data Length and Checksum is sent in little-endian format
        gpb_pktIO.data_length = gpb_sio_getWord();
        // Dev Note: For robustness, we know only data and print have data payload
        // Prep data parsing
        gpb_pktIO.data_i = 0;
        switch (gpb_pktIO.command)
        {
          case GBP_COMMAND_DATA:
            if (gpb_pktIO.data_length != 0)
            {
              gpb_pktIO.packetState = GBP_PKT10_PARSE_DATA_PAYLOAD;
              gpb_sio_next(GBP_SIO_MODE_8BITS, 0);
            }
            else
            {
              gpb_pktIO.packetState = GBP_PKT10_PARSE_CHECKSUM;
              gpb_sio_next(GBP_SIO_MODE_16BITS_LITTLE_ENDIAN, 0);
            }
            break;
          case GBP_COMMAND_PRINT:
            gpb_pktIO.packetState = GBP_PKT10_PARSE_DATA_PAYLOAD;
            gpb_sio_next(GBP_SIO_MODE_8BITS, 0);
            // Size limit guard
            gpb_pktIO.data_length = gpb_pktIO.data_length > 4 ? 4 : gpb_pktIO.data_length;
            break;
          default:
            gpb_pktIO.packetState = GBP_PKT10_PARSE_CHECKSUM;
            gpb_sio_next(GBP_SIO_MODE_16BITS_LITTLE_ENDIAN, 0);
            break;
        }
      }
      break;
    case GBP_PKT10_PARSE_DATA_PAYLOAD:
      {
        switch (gpb_pktIO.command)
        {
          case GBP_COMMAND_DATA:
            // Dev Note: Previous naive approach was to capture byte here
            // e.g. gpb_cbuff_EnqueueTemp(&gpb_pktIO.dataBuffer, (uint8_t)(gpb_sio.rx_buff & 0xFF));
            break;
          case GBP_COMMAND_PRINT:
            // Dev Note: But now we are doing packet processing later on...
            //           so now we are focusing only on capturing bytes in ISR
            //gpb_pktIO.printInstructionBuffer[gpb_pktIO.data_i] = (uint8_t)(gpb_sio.rx_buff & 0xFF);
            break;
          default:
            break;
        }

        gpb_pktIO.checksumCalc += (uint16_t)gpb_sio_getByte(0);

        // Increment to next byte position in the data field
        gpb_pktIO.data_i++;

        // Escape and move to next stage
        if (gpb_pktIO.data_i >= gpb_pktIO.data_length)
        {
          gpb_pktIO.packetState = GBP_PKT10_PARSE_CHECKSUM;
          gpb_sio_next(GBP_SIO_MODE_16BITS_LITTLE_ENDIAN, 0);
        }
        else
        {
          gpb_pktIO.packetState = GBP_PKT10_PARSE_DATA_PAYLOAD;
          gpb_sio_next(GBP_SIO_MODE_8BITS, 0);
        }
      }
      break;
    case GBP_PKT10_PARSE_CHECKSUM:
      {
        // GBP Data Length and Checksum is sent in little-endian format. Swap
        gpb_pktIO.checksum = gpb_sio_getWord();

        // Checksum
        gpb_pktIO.checksumCalc += gpb_pktIO.command;
        gpb_pktIO.checksumCalc += gpb_pktIO.compression;
        gpb_pktIO.checksumCalc += (gpb_pktIO.data_length >> 8) & 0xFF;
        gpb_pktIO.checksumCalc += (gpb_pktIO.data_length >> 0) & 0xFF;

#ifdef FEATURE_CHECKSUM_SUPPORTED
        // Dev Note: Was used to confirm that packetizer was working
        // This will cause the printer to retry sending this packet
        if (gpb_pktIO.checksum != gpb_pktIO.checksumCalc)
        {
          gpb_status_bit_update_checksum_error(gpb_pktIO.statusBuffer, true);
        }
#endif  // FEATURE_CHECKSUM_SUPPORTED

#ifdef TEST_CHECKSUM_FORCE_FAIL
        // WIP investigation of checksum based flow control
        static int checksumFailToggle = 0;
        if (checksumFailToggle > 2)
        {
          checksumFailToggle = 0;
          gpb_status_bit_update_checksum_error(gpb_pktIO.statusBuffer, true);
        }
        checksumFailToggle++;
#endif  // TEST_CHECKSUM_FORCE_FAIL

#ifdef TEST_PRETEND_BUFFER_FULL
        // WIP investigation of buffer full behaviour in the interest of
        // adding flow control
        static int fakeFullToggle = 0;
        if (fakeFullToggle > 5)
        {
          fakeFullToggle = 0;
          gpb_status_bit_update_print_buffer_full(gpb_pktIO.statusBuffer, true);
        }
        else
        {
          gpb_status_bit_update_print_buffer_full(gpb_pktIO.statusBuffer, false);
        }
        fakeFullToggle++;
#endif  // TEST_PRETEND_BUFFER_FULL

        // Update status data : Device Status
        switch (gpb_pktIO.command)
        {
          // INIT --> DATA --> ENDDATA --> PRINT
          case GBP_COMMAND_INIT:
            gpb_pktIO.dataPacketCountdown    = 6;
            gpb_pktIO.untransPacketCountdown = 0;
            gpb_pktIO.busyPacketCountdown    = 0;
            gpb_status_bit_update_print_buffer_full(gpb_pktIO.statusBuffer, false);
            gpb_status_bit_update_printer_busy(gpb_pktIO.statusBuffer, false);
            break;
          case GBP_COMMAND_PRINT:
            gpb_pktIO.busyPacketCountdown = GBP_BUSY_PACKET_COUNT;
            break;
          case GBP_COMMAND_DATA:
            gpb_pktIO.untransPacketCountdown = 3;
            break;
          case GBP_COMMAND_BREAK:
            gpb_status_bit_update_low_battery(gpb_pktIO.statusBuffer, false);
            gpb_status_bit_update_other_error(gpb_pktIO.statusBuffer, false);
            gpb_status_bit_update_paper_jam(gpb_pktIO.statusBuffer, false);
            gpb_status_bit_update_packet_error(gpb_pktIO.statusBuffer, false);
            gpb_status_bit_update_unprocessed_data(gpb_pktIO.statusBuffer, false);
            gpb_status_bit_update_print_buffer_full(gpb_pktIO.statusBuffer, true);
            gpb_status_bit_update_printer_busy(gpb_pktIO.statusBuffer, true);
            gpb_status_bit_update_checksum_error(gpb_pktIO.statusBuffer, false);
            break;
          case GBP_COMMAND_INQUIRY:
            if (gpb_pktIO.untransPacketCountdown > 0)
            {
              gpb_pktIO.untransPacketCountdown--;
              if (gpb_pktIO.untransPacketCountdown == 0)
              {
                gpb_status_bit_update_unprocessed_data(gpb_pktIO.statusBuffer, false);
                if (gpb_pktIO.busyPacketCountdown > 0)
                {
                  gpb_status_bit_update_printer_busy(gpb_pktIO.statusBuffer, true);
                  gpb_status_bit_update_print_buffer_full(gpb_pktIO.statusBuffer, true);
                }
              }
            }
            else if (gpb_pktIO.busyPacketCountdown > 0)
            {
              gpb_pktIO.busyPacketCountdown--;
              if (gpb_pktIO.busyPacketCountdown == 0)
              {
                gpb_status_bit_update_printer_busy(gpb_pktIO.statusBuffer, false);
              }
            }
            break;
          default:
            break;
        }

        // Start sending device id and status byte
        gpb_pktIO.packetState = GBP_PKT10_PARSE_DUMMY;
        gpb_sio_next(GBP_SIO_MODE_16BITS_BIG_ENDIAN, gpb_pktIO.statusBuffer);
      }
      break;
    case GBP_PKT10_PARSE_DUMMY:
      {
        // Update status data : Device Status
        switch (gpb_pktIO.command)
        {
          // INIT --> DATA --> ENDDATA --> PRINT
          case GBP_COMMAND_INIT:
            break;
          case GBP_COMMAND_PRINT:
            break;
          case GBP_COMMAND_DATA:
            if (gpb_pktIO.dataPacketCountdown > 0)
            {
              gpb_pktIO.dataPacketCountdown--;
              if (gpb_pktIO.dataPacketCountdown == 0)
              {
                gpb_status_bit_update_unprocessed_data(gpb_pktIO.statusBuffer, false);
              }
            }
            gpb_status_bit_update_print_buffer_full(gpb_pktIO.statusBuffer, false);
            gpb_status_bit_update_unprocessed_data(gpb_pktIO.statusBuffer, false);
            if (gpb_pktIO.data_length == 0)
            {
              gpb_status_bit_update_unprocessed_data(gpb_pktIO.statusBuffer, false);
              gpb_status_bit_update_print_buffer_full(gpb_pktIO.statusBuffer, true);
            }
            break;
          case GBP_COMMAND_BREAK:
            break;
          case GBP_COMMAND_INQUIRY:
            gpb_status_bit_update_unprocessed_data(gpb_pktIO.statusBuffer, false);
            if ((gpb_pktIO.untransPacketCountdown == 0) && (gpb_pktIO.busyPacketCountdown == 0))
            {
              gpb_status_bit_update_print_buffer_full(gpb_pktIO.statusBuffer, false);
            }
            break;
          default:
            break;
        }

        switch (gpb_pktIO.command)
        {
          case GBP_COMMAND_INIT:
            gpb_pktIO.initReceived = true;
            break;
          case GBP_COMMAND_PRINT:
            gpb_pktIO.printInstructionReceived = true;
            break;
          case GBP_COMMAND_DATA:
            if (gpb_pktIO.data_length > 0)
              gpb_pktIO.dataPacketReceived = true;
            else
              gpb_pktIO.dataEndPacketReceived = true;
            break;
          case GBP_COMMAND_BREAK:
            gpb_pktIO.breakPacketReceived = true;
            break;
          case GBP_COMMAND_INQUIRY:
            gpb_pktIO.nulPacketReceived = true;
            break;
          default:
            break;
        }

#ifdef FEATURE_CHECKSUM_SUPPORTED
        // temp buff handling
        if (gpb_status_bit_getbit_checksum_error(gpb_pktIO.statusBuffer))
        {
          // On checksum error, throw away old data. GBP will resend
          gpb_cbuff_ResetTemp(&gpb_pktIO.dataBuffer);
        }
        else
        {
          // Checksum ok, keep the new data
          gpb_cbuff_AcceptTemp(&gpb_pktIO.dataBuffer);
        }
#endif  // FEATURE_CHECKSUM_SUPPORTED

        // Cleanup
        gpb_pktIO.packetState = GBP_PKT10_PARSE_HEADER_COMMAND_AND_COMPRESSION;
        gpb_sio_next(GBP_SIO_MODE_RESET, 0);
        gpb_sio.SINOutputPinState = false;
      }
      break;
    default:
      {
        // ? Should not reach here
        gpb_pktIO.packetState = GBP_PKT10_PARSE_HEADER_COMMAND_AND_COMPRESSION;
        gpb_sio_next(GBP_SIO_MODE_RESET, 0);
        gpb_sio.SINOutputPinState = false;
      }
  }

#ifdef GBP_FEATURE_USING_RISING_CLOCK_ONLY_ISR
  /*
    We have processed a byte at bit pos 7, now need to prep txBit line for next
    byte at bit Pos 0 of next byte so lets change now so that we have the correct
    tx bit state on next rise.
             0   1   2   3   4   5   6   7             0   1   2   3   4   5   6   7
         __   _   _   _   _   _   _   _   ___________   _   _   _   _   _   _   _   _
    CLK:   |_| |_| |_| |_| |_| |_| |_| |_|           |_| |_| |_| |_| |_| |_| |_| |_|
    DAT: ___XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX____________XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX_
  */
  gpb_sio.SINOutputPinState = (gpb_sio.bitMaskMap & gpb_sio.tx_buff) > 0;
#endif

  return gpb_sio.SINOutputPinState;
}


/******************************************************************************/
