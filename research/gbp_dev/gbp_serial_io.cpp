#include <stdint.h> // uint8_t
#include <stddef.h> // size_t
//#include <Arduino.h> // Just for Serial.Print() only

#include "gameboy_printer_protocol.h"
#include "gpb_serial_io.h"

#define GBP_PKT10_TIMEOUT_MS 5000

// Testing
//#define TEST_CHECKSUM_FORCE_FAIL
//#define TEST_PRETEND_BUFFER_FULL


// Feature
//#define FEATURE_CHECKSUM_SUPPORTED ///< WIP

#define GPB_BUSY_PACKET_COUNT 3 // 68 Inquiry packets is generally approximately how long it takes for a real printer to print. This is not a real printer so can be shorter



/******************************************************************************/
// # Circular Byte Buffer For Embedded Applications (Index Based)
// Author: Brian Khuu (July 2020) (briankhuu.com) (mofosyne@gmail.com)
// This Gist (Pointer): https://gist.github.com/mofosyne/d7a4a8d6a567133561c18aaddfd82e6f
// This Gist (Index): https://gist.github.com/mofosyne/82020d5c0e1e11af0eb9b05c73734956
#include <stdint.h> // uint8_t
#include <stddef.h> // size_t
#include <stdbool.h> // bool

typedef struct gpb_cbuff_t
{
  size_t capacity; ///< Maximum number of items in the buffer
  size_t count;    ///< Number of items in the buffer
  uint8_t *buffer; ///< Data Buffer
  size_t head;     ///< Head Index
  size_t tail;     ///< Tail Index

#ifdef FEATURE_CHECKSUM_SUPPORTED
  // Temp
  size_t countTemp;    ///< Number of items in the buffer
  size_t headTemp;     ///< Head Index
#endif // FEATURE_CHECKSUM_SUPPORTED
} gpb_cbuff_t;

static inline bool gpb_cbuff_Init(gpb_cbuff_t *cb, size_t capacity, uint8_t *buffPtr)
{
  gpb_cbuff_t emptyCB = {0};
  if ((cb == NULL) || (buffPtr == NULL))
    return false; ///< Failed
  // Init Struct
  *cb = emptyCB;
  cb->capacity = capacity;
  cb->buffer   = buffPtr;
  cb->count    = 0;
  cb->head     = 0;
  cb->tail     = 0;
  return true; ///< Successful
}

static inline bool gpb_cbuff_Reset(gpb_cbuff_t *cb)
{
  cb->count = 0;
  cb->head = 0;
  cb->tail = 0;

  return true; ///< Successful
}

static inline bool gpb_cbuff_Enqueue(gpb_cbuff_t *cb, uint8_t b)
{
  // Full
  if (cb->count >= cb->capacity)
    return false; ///< Failed
  // Push value
  cb->buffer[cb->head] = b;
  // Increment head
  cb->head = (cb->head + 1) % cb->capacity;
  cb->count = cb->count + 1;
  return true; ///< Successful
}

static inline bool gpb_cbuff_Dequeue(gpb_cbuff_t *cb, uint8_t *b)
{
  // Empty
  if (cb->count == 0)
    return false; ///< Failed
  // Pop value
  *b = cb->buffer[cb->tail];
  // Increment tail
  cb->tail = (cb->tail + 1) % cb->capacity;
  cb->count = cb->count - 1;
  return true; ///< Successful
}

static inline bool gpb_cbuff_Dequeue_Peek(gpb_cbuff_t *cb, uint8_t *b, uint32_t offset)
{
  // Empty
  if (cb->count == 0)
    return false; ///< Failed
  if (cb->count < offset)
    return false; ///< Failed
  // Pop value
  *b = cb->buffer[(cb->tail + offset) % cb->capacity];
  return true; ///< Successful
}

static inline size_t gpb_cbuff_Capacity(gpb_cbuff_t *cb) { return cb->capacity;}
static inline size_t gpb_cbuff_Count(gpb_cbuff_t *cb)   { return cb->count;}
static inline bool gpb_cbuff_IsFull(gpb_cbuff_t *cb)    { return (cb->count >= cb->capacity);}
static inline bool gpb_cbuff_IsEmpty(gpb_cbuff_t *cb)   { return (cb->count == 0);}

#ifdef FEATURE_CHECKSUM_SUPPORTED
/* Temp Enqeue */
static inline bool gpb_cbuff_ResetTemp(gpb_cbuff_t *cb)
{
  cb->countTemp = 0;
  cb->headTemp  = cb->head ;
}

static inline bool gpb_cbuff_AcceptTemp(gpb_cbuff_t *cb)
{
  cb->count += cb->countTemp;
  cb->head  = cb->headTemp  ;
  return true; ///< Successful
}

static inline bool gpb_cbuff_EnqueueTemp(gpb_cbuff_t *cb, uint8_t b)
{
  // Full
  if (cb->countTemp >= (cb->capacity - cb->count))
    return false; ///< Failed
  // Push value
  cb->buffer[cb->headTemp] = b;
  // Increment headTemp
  cb->headTemp = (cb->headTemp + 1) % cb->capacity;
  cb->countTemp = cb->countTemp + 1;
  return true; ///< Successful
}
#else
#define gpb_cbuff_EnqueueTemp(CB, B) gpb_cbuff_Enqueue(CB, B)
#endif // FEATURE_CHECKSUM_SUPPORTED

/******************************************************************************/
/******************************************************************************/

#ifdef GBP_FEATURE_RAW_DUMP
static struct
{
  size_t totalBytesReceived;
  size_t lastPacketByteCount;
  size_t lastPacketChecksum;
  uint16_t lastPacketStatus;
} packetRawDump;
#endif


/******************************************************************************/
/******************************************************************************/

typedef enum
{
  GPB_SIO_MODE_RESET,
  GPB_SIO_MODE_8BITS,
  GPB_SIO_MODE_16BITS_BIG_ENDIAN,
  GPB_SIO_MODE_16BITS_LITTLE_ENDIAN,
} gpb_sio_mode_t;

// SIO Serial Input Output Psudo SPI
static struct
{
  bool     SINOutputPinState; /// GPIO state of output
  // Preamble Sync
  bool     syncronised; ///< True When Preamble Found
  uint16_t preamble; ///< Scanning for Preamble
  // Byte Tx/Rx
  uint16_t  bitMaskMap; // gpb_sio_bitmaskmaps_t
  gpb_sio_mode_t  mode;
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

  // Print Instruction Command
  bool printInstructionReceived;
  uint8_t printInstructionBuffer[4];

  // Data Packet Command
  bool dataPacketReceived;
  uint16_t dataPacketPayloadSize;
  uint16_t dataPacketIndex; ///< Keep track during parsing, if enough bytes received
  gpb_cbuff_t dataBuffer;

  // Data End Packet Command (Data size of 0)
  bool dataEndPacketReceived;

  // Break Packet Command
  bool breakPacketReceived;

  // Inquiry Packet Command
  bool nulPacketReceived;

  // Packet Parsing
  gbp_pktIO_parse_state_t packetState;
  uint8_t command;
  uint8_t compression;
  uint16_t data_length;
  uint16_t data_i;
  uint16_t checksum; ///< For data integrity check (Ignored for this implementation)
  uint16_t checksumCalc; ///< For data integrity check (Ignored for this implementation)
  uint16_t statusBuffer; ///< This is send on every packet in the dummy data region

  // Notify
  bool packetReceivedNotify;
  uint32_t timeout_ms;

  // Status Packet Sequencing (For faking the printer)
  int busyPacketCountdown;
  int untransPacketCountdown;
  int dataPacketCountdown;
} gpb_pktIO;

/******************************************************************************/

bool gbp_timeout_handler(uint32_t elapsed_ms)
{
  if (gpb_pktIO.breakPacketReceived)
  {
    gpb_pktIO_reset();
    return true;
  }

  if (gpb_pktIO.timeout_ms > 0)
  {
    gpb_pktIO.timeout_ms = (gpb_pktIO.timeout_ms > elapsed_ms) ? (gpb_pktIO.timeout_ms - elapsed_ms) : 0;
    if (gpb_pktIO.timeout_ms == 0)
    {
      gpb_pktIO_reset();
      return true;
    }
  }
  return false;
}

bool gbp_packetWasReceived(bool reset)
{
  bool received = gpb_pktIO.packetReceivedNotify;
  if (reset)
    gpb_pktIO.packetReceivedNotify = false;
  return received;
}

size_t gbp_dataBuff_getByteCount(void)
{
  return gpb_cbuff_Count(&gpb_pktIO.dataBuffer);
}

uint8_t gbp_dataBuff_getByte(void)
{
  uint8_t b = 0;
  gpb_cbuff_Dequeue(&gpb_pktIO.dataBuffer, &b);
  if (gpb_cbuff_Count(&gpb_pktIO.dataBuffer) == 0)
  {
    gbp_set_unprocessed_data(false);
  }
  return b;
}

uint8_t gbp_dataBuff_getByte_Peek(uint32_t offset)
{
  uint8_t b = 0;
  gpb_cbuff_Dequeue_Peek(&gpb_pktIO.dataBuffer, &b, offset);
  return b;
}

/******************************************************************************/

bool gbp_init_CheckReceived(bool clear)
{
  bool ret = gpb_pktIO.initReceived;
  if (clear)
    gpb_pktIO.initReceived = false;
  return ret;
}

bool gbp_printInstruction_CheckReceived(bool clear)
{
  bool ret = gpb_pktIO.printInstructionReceived;
  if (clear)
    gpb_pktIO.printInstructionReceived = false;
  return ret;
}

bool gbp_dataPacket_CheckReceived(bool clear)
{
  bool ret = gpb_pktIO.dataPacketReceived;
  if (clear)
    gpb_pktIO.dataPacketReceived = false;
  return ret;
}

bool gbp_dataEndPacket_CheckReceived(bool clear)
{
  bool ret = gpb_pktIO.dataEndPacketReceived;
  if (clear)
    gpb_pktIO.dataEndPacketReceived = false;
  return ret;
}

bool gbp_breakPacket_CheckReceived(bool clear)
{
  bool ret = gpb_pktIO.breakPacketReceived;
  if (clear)
    gpb_pktIO.breakPacketReceived = false;
  return ret;
}

bool gbp_nullPacket_CheckReceived(bool clear)
{
  bool ret = gpb_pktIO.nulPacketReceived;
  if (clear)
    gpb_pktIO.nulPacketReceived = false;
  return ret;
}


/******************************************************************************/

int gbp_printInstruction_num_of_sheets(void)
{
  if (gpb_pktIO.printInstructionReceived)
    return -1;
  return (gpb_pktIO.printInstructionBuffer[GBP_PRINT_INSTRUCT_INDEX_NUM_OF_SHEETS  ]);
}

int gbp_printInstruction_num_of_linefeed_before_print(void)
{
  if (gpb_pktIO.printInstructionReceived)
    return -1;
  return (gpb_pktIO.printInstructionBuffer[GBP_PRINT_INSTRUCT_INDEX_NUM_OF_LINEFEED] >> 4) & 0x0F;
}

int gbp_printInstruction_num_of_linefeed_after_print(void)
{
  if (gpb_pktIO.printInstructionReceived)
    return -1;
  return (gpb_pktIO.printInstructionBuffer[GBP_PRINT_INSTRUCT_INDEX_NUM_OF_LINEFEED]) & 0x0F;
}

int gbp_printInstruction_palette_value(void)
{
  if (gpb_pktIO.printInstructionReceived)
    return -1;
  return (gpb_pktIO.printInstructionBuffer[GBP_PRINT_INSTRUCT_INDEX_PALETTE_VALUE  ]);
}

int gbp_printInstruction_print_density(void)
{
  if (gpb_pktIO.printInstructionReceived)
    return -1;
  return (gpb_pktIO.printInstructionBuffer[GBP_PRINT_INSTRUCT_INDEX_PRINT_DENSITY  ]);
}


/******************************************************************************/

void gbp_set_low_battery(bool val) {gpb_status_bit_update_low_battery(gpb_pktIO.statusBuffer, val);}
void gbp_set_other_error(bool val) {gpb_status_bit_update_other_error(gpb_pktIO.statusBuffer, val);}
void gbp_set_paper_jam(bool val) {gpb_status_bit_update_paper_jam(gpb_pktIO.statusBuffer, val);}
void gbp_set_packet_error(bool val) {gpb_status_bit_update_packet_error(gpb_pktIO.statusBuffer, val);}
void gbp_set_unprocessed_data(bool val) {gpb_status_bit_update_unprocessed_data(gpb_pktIO.statusBuffer, val);}
void gbp_set_print_buffer_full(bool val) {gpb_status_bit_update_print_buffer_full(gpb_pktIO.statusBuffer, val);}
void gbp_set_printer_busy(bool val) {gpb_status_bit_update_printer_busy(gpb_pktIO.statusBuffer, val);}
void gbp_set_checksum_error(bool val) {gpb_status_bit_update_checksum_error(gpb_pktIO.statusBuffer, val);}

bool gbp_get_low_battery(void) {return gpb_status_bit_getbit_low_battery(gpb_pktIO.statusBuffer);}
bool gbp_get_other_error(void) {return gpb_status_bit_getbit_other_error(gpb_pktIO.statusBuffer);}
bool gbp_get_paper_jam(void) {return gpb_status_bit_getbit_paper_jam(gpb_pktIO.statusBuffer);}
bool gbp_get_packet_error(void) {return gpb_status_bit_getbit_packet_error(gpb_pktIO.statusBuffer);}
bool gbp_get_unprocessed_data(void) {return gpb_status_bit_getbit_unprocessed_data(gpb_pktIO.statusBuffer);}
bool gbp_get_print_buffer_full(void) {return gpb_status_bit_getbit_print_buffer_full(gpb_pktIO.statusBuffer);}
bool gbp_get_printer_busy(void) {return gpb_status_bit_getbit_printer_busy(gpb_pktIO.statusBuffer);}
bool gbp_get_checksum_error(void) {return gpb_status_bit_getbit_checksum_error(gpb_pktIO.statusBuffer);}


/******************************************************************************/

bool gpb_pktIO_reset(void)
{
  gpb_sio.syncronised = false;
  gpb_sio.rx_buff = 0;
  gpb_sio.tx_buff = 0;
  gpb_sio.SINOutputPinState = false;
  gpb_sio.bitMaskMap = 0;

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
#endif // FEATURE_CHECKSUM_SUPPORTED

  return true;
}

bool gpb_pktIO_init(size_t buffSize, uint8_t *buffPtr)
{
  // reset status data
  gpb_pktIO.statusBuffer = 0x0000;
  gpb_pktIO.statusBuffer = GBP_DEVICE_ID << 8;
  gpb_pktIO.busyPacketCountdown = 0;

  // print data buffer
  gpb_cbuff_Init(&gpb_pktIO.dataBuffer, buffSize, buffPtr);

  // Packet Parsing Subsystem
  gpb_pktIO_reset();

  return true;
}


/******************************************************************************/

bool gpb_sio_next(const gpb_sio_mode_t mode, const uint16_t txdata)
{
  gpb_sio.rx_buff = 0;
  gpb_sio.mode = mode;
  switch (mode)
  {
    case GPB_SIO_MODE_RESET            :
      gpb_sio.bitMaskMap = 0;
      gpb_sio.SINOutputPinState = false;
      gpb_sio.tx_buff = 0xFFFF;
      gpb_sio.syncronised = false;
#ifdef GBP_FEATURE_RAW_DUMP
      packetRawDump.lastPacketByteCount = packetRawDump.totalBytesReceived;
#endif
      break;
    case GPB_SIO_MODE_8BITS                :
      gpb_sio.bitMaskMap = (uint16_t)1 << (8 - 1);
      gpb_sio.tx_buff = txdata;
      break;
    case GPB_SIO_MODE_16BITS_BIG_ENDIAN    :
      gpb_sio.bitMaskMap = (uint16_t)1 << (16 - 1);
      gpb_sio.tx_buff = txdata;
      break;
    case GPB_SIO_MODE_16BITS_LITTLE_ENDIAN :
      gpb_sio.bitMaskMap = (uint16_t)1 << (16 - 1);
      gpb_sio.tx_buff = 0;
      gpb_sio.tx_buff |= ((txdata >> 8) & 0x00FF);
      gpb_sio.tx_buff |= ((txdata << 8) & 0xFF00);
      break;
  }
  return true;
}

uint16_t gpb_sio_getWord()
{
  uint16_t temp = 0;
  switch (gpb_sio.mode)
  {
    case GPB_SIO_MODE_RESET            :
      break;
    case GPB_SIO_MODE_8BITS                :
      temp |= ((gpb_sio.rx_buff >> 0) & 0x00FF);
      break;
    case GPB_SIO_MODE_16BITS_BIG_ENDIAN    :
      temp |= ((gpb_sio.rx_buff >> 0) & 0xFFFF);
      break;
    case GPB_SIO_MODE_16BITS_LITTLE_ENDIAN :
      temp |= ((gpb_sio.rx_buff >> 8) & 0x00FF);
      temp |= ((gpb_sio.rx_buff << 8) & 0xFF00);
      break;
  }
  return temp;
}

uint8_t gpb_sio_getByte(const int bytePos)
{
  switch (bytePos)
  {
    case 0: return ((gpb_sio.rx_buff >> 0) & 0xFF);
    case 1: return ((gpb_sio.rx_buff >> 8) & 0xFF);
    default: return 0;
  }
}

#ifdef GBP_FEATURE_RAW_DUMP
size_t gbp_sio_lastPacketByteCount(void)
{
  return packetRawDump.lastPacketByteCount;
}
size_t gbp_sio_lastPacketChecksum(void)
{
  return packetRawDump.lastPacketChecksum;
}
uint16_t gbp_sio_lastPacketStatus(void)
{
  return packetRawDump.lastPacketStatus;
}
#endif

/******************************************************************************/

// Assumption: Only one gameboy printer connection required
// Return: pin state of GBP_SIN
#ifdef GBP_FEATURE_USING_RISING_CLOCK_ONLY_ISR
bool gpb_pktIO_OnRising_ISR(const bool GBP_SOUT)
#else
bool gpb_pktIO_OnChange_ISR(const bool GBP_SCLK, const bool GBP_SOUT)
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
    gpb_sio.preamble = 0;
    gpb_sio.syncronised = true;
    gpb_sio_next(GPB_SIO_MODE_16BITS_BIG_ENDIAN, 0);
    return false;
  }

  /* Psudo SPI Engine */
  // Basically I have one bit acting as a mask moving across a word sized buffer
  if (gpb_sio.bitMaskMap > 0)
  {
    // Serial Transaction Is Active
#ifdef GBP_FEATURE_USING_RISING_CLOCK_ONLY_ISR
      // Rising Edge Clock (Rx Bit)
      gpb_sio.rx_buff |= GBP_SOUT ? (gpb_sio.bitMaskMap & 0xFFFF) : 0; ///< Clocking bits on rising edge
      gpb_sio.bitMaskMap >>= 1; ///< One tx/rx bit cycle complete, next bit now
      // Falling Edge Clock (Tx Bit) (Prep now for next rising edge)
      gpb_sio.SINOutputPinState = (gpb_sio.bitMaskMap & gpb_sio.tx_buff) > 0;
      if (gpb_sio.bitMaskMap > 0)
        return gpb_sio.SINOutputPinState;
#else
    if (GBP_SCLK)
    {
      // Rising Edge Clock (Rx Bit)
      gpb_sio.rx_buff |= GBP_SOUT ? (gpb_sio.bitMaskMap & 0xFFFF) : 0; ///< Clocking bits on rising edge
      gpb_sio.bitMaskMap >>= 1; ///< One tx/rx bit cycle complete, next bit now

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

#ifdef GBP_FEATURE_RAW_DUMP
  if (gpb_pktIO.packetState == GBP_PKT10_PARSE_HEADER_COMMAND_AND_COMPRESSION)
  {
    gpb_cbuff_EnqueueTemp(&gpb_pktIO.dataBuffer, GBP_SYNC_WORD_0);
    gpb_cbuff_EnqueueTemp(&gpb_pktIO.dataBuffer, GBP_SYNC_WORD_1);
    packetRawDump.totalBytesReceived += 2;
  }
  switch (gpb_sio.mode)
  {
    case GPB_SIO_MODE_8BITS                :
      gpb_cbuff_EnqueueTemp(&gpb_pktIO.dataBuffer, (uint8_t)((gpb_sio.rx_buff >> 0) & 0xFF));
      packetRawDump.totalBytesReceived += 1;
      break;
    case GPB_SIO_MODE_16BITS_BIG_ENDIAN    :
    case GPB_SIO_MODE_16BITS_LITTLE_ENDIAN :
      if (gpb_pktIO.packetState == GBP_PKT10_PARSE_DUMMY)
      {
        // This is for dumping status byte. This is only done during the dummy buffer byte phase
        // so might as well use these bytes for documenting response of the status byte
        gpb_cbuff_EnqueueTemp(&gpb_pktIO.dataBuffer, (uint8_t)((gpb_sio.tx_buff >> 8) & 0xFF));
        gpb_cbuff_EnqueueTemp(&gpb_pktIO.dataBuffer, (uint8_t)((gpb_sio.tx_buff >> 0) & 0xFF));
        packetRawDump.totalBytesReceived += 2;
        break;
      }
      gpb_cbuff_EnqueueTemp(&gpb_pktIO.dataBuffer, (uint8_t)((gpb_sio.rx_buff >> 8) & 0xFF));
      gpb_cbuff_EnqueueTemp(&gpb_pktIO.dataBuffer, (uint8_t)((gpb_sio.rx_buff >> 0) & 0xFF));
      packetRawDump.totalBytesReceived += 2;
      break;
    default:
      break;
  }
#endif

  /* Packet Timeout Reset */
  gpb_pktIO.timeout_ms = GBP_PKT10_TIMEOUT_MS;

  /****************************************************************************/
  /* Packet State */
  switch (gpb_pktIO.packetState)
  {
    case GBP_PKT10_PARSE_HEADER_COMMAND_AND_COMPRESSION   :
    {
      // Parse
      gpb_pktIO.command     = gpb_sio_getByte(1);
      gpb_pktIO.compression = gpb_sio_getByte(0);
      gpb_pktIO.checksumCalc = 0;
      // Next Header Segment
      gpb_pktIO.packetState = GBP_PKT10_PARSE_HEADER_DATA_LENGTH;
      gpb_sio_next(GPB_SIO_MODE_16BITS_LITTLE_ENDIAN, 0);
    } break;
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
            gpb_sio_next(GPB_SIO_MODE_8BITS, 0);
          }
          else
          {
            gpb_pktIO.packetState = GBP_PKT10_PARSE_CHECKSUM;
            gpb_sio_next(GPB_SIO_MODE_16BITS_LITTLE_ENDIAN, 0);
          }
          break;
        case GBP_COMMAND_PRINT:
          gpb_pktIO.packetState = GBP_PKT10_PARSE_DATA_PAYLOAD;
          gpb_sio_next(GPB_SIO_MODE_8BITS, 0);
          // Size limit guard
          gpb_pktIO.data_length = gpb_pktIO.data_length > 4 ? 4 : gpb_pktIO.data_length;
          break;
        default:
          gpb_pktIO.packetState = GBP_PKT10_PARSE_CHECKSUM;
          gpb_sio_next(GPB_SIO_MODE_16BITS_LITTLE_ENDIAN, 0);
          break;
      }
    } break;
    case GBP_PKT10_PARSE_DATA_PAYLOAD     :
    {
      switch (gpb_pktIO.command)
      {
        case GBP_COMMAND_DATA:
#ifndef GBP_FEATURE_RAW_DUMP
          gpb_cbuff_EnqueueTemp(&gpb_pktIO.dataBuffer, (uint8_t)(gpb_sio.rx_buff & 0xFF));
#endif
          break;
        case GBP_COMMAND_PRINT:
          gpb_pktIO.printInstructionBuffer[gpb_pktIO.data_i] = (uint8_t)(gpb_sio.rx_buff & 0xFF);
          break;
        default:
          // Don't record payload
          break;
      }

      gpb_pktIO.checksumCalc += (uint16_t)gpb_sio_getByte(0);

      // Increment to next byte position in the data field
      gpb_pktIO.data_i++;

      // Escape and move to next stage
      if (gpb_pktIO.data_i >= gpb_pktIO.data_length)
      {
        gpb_pktIO.packetState = GBP_PKT10_PARSE_CHECKSUM;
        gpb_sio_next(GPB_SIO_MODE_16BITS_LITTLE_ENDIAN, 0);
      }
      else
      {
        gpb_pktIO.packetState = GBP_PKT10_PARSE_DATA_PAYLOAD;
        gpb_sio_next(GPB_SIO_MODE_8BITS, 0);
      }
    } break;
    case GBP_PKT10_PARSE_CHECKSUM :
    {
      // GBP Data Length and Checksum is sent in little-endian format. Swap
      gpb_pktIO.checksum = gpb_sio_getWord();

      // Checksum
      gpb_pktIO.checksumCalc += gpb_pktIO.command                ;
      gpb_pktIO.checksumCalc += gpb_pktIO.compression            ;
      gpb_pktIO.checksumCalc += (gpb_pktIO.data_length >> 8) & 0xFF;
      gpb_pktIO.checksumCalc += (gpb_pktIO.data_length >> 0) & 0xFF;

#ifdef FEATURE_CHECKSUM_SUPPORTED
      // Dev Note: Was used to confirm that packetizer was working
      // This will cause the printer to retry sending this packet
      if (gpb_pktIO.checksum != gpb_pktIO.checksumCalc)
      {
        gpb_status_bit_update_checksum_error(gpb_pktIO.statusBuffer, true);
      }
#endif // FEATURE_CHECKSUM_SUPPORTED

#ifdef TEST_CHECKSUM_FORCE_FAIL
      static int checksumFailToggle = 0;
      if (checksumFailToggle > 2)
      {
        checksumFailToggle = 0;
        gpb_status_bit_update_checksum_error(gpb_pktIO.statusBuffer, true);
      }
      checksumFailToggle++;
#endif // TEST_CHECKSUM_FORCE_FAIL

#ifdef TEST_PRETEND_BUFFER_FULL
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
#endif



      // Update status data : Device Status
      switch (gpb_pktIO.command)
      {
        // INIT --> DATA --> ENDDATA --> PRINT
        case GBP_COMMAND_INIT:
            gpb_pktIO.dataPacketCountdown = 6;
            gpb_pktIO.untransPacketCountdown = 0;
            gpb_pktIO.busyPacketCountdown = 0;
            gpb_status_bit_update_print_buffer_full(gpb_pktIO.statusBuffer, false);
          break;
        case GBP_COMMAND_PRINT:
          gpb_pktIO.busyPacketCountdown = GPB_BUSY_PACKET_COUNT;
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
          gpb_status_bit_update_print_buffer_full(gpb_pktIO.statusBuffer, false);
          gpb_status_bit_update_printer_busy(gpb_pktIO.statusBuffer, false);
          gpb_status_bit_update_checksum_error(gpb_pktIO.statusBuffer, false);
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

#ifdef GBP_FEATURE_RAW_DUMP
      packetRawDump.lastPacketChecksum = gpb_pktIO.checksumCalc;
      packetRawDump.lastPacketStatus = gpb_pktIO.statusBuffer;
#endif

      // Start sending device id and status byte
      gpb_pktIO.packetState = GBP_PKT10_PARSE_DUMMY;
      gpb_sio_next(GPB_SIO_MODE_16BITS_BIG_ENDIAN, gpb_pktIO.statusBuffer);
    } break;
    case GBP_PKT10_PARSE_DUMMY    :
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
              gpb_status_bit_update_print_buffer_full(gpb_pktIO.statusBuffer, true);
            }
          }
          gpb_status_bit_update_unprocessed_data(gpb_pktIO.statusBuffer, true);
          if (gpb_pktIO.data_length == 0)
          {
            gpb_status_bit_update_unprocessed_data(gpb_pktIO.statusBuffer, false);
          }
          break;
        case GBP_COMMAND_BREAK:
          break;
        case GBP_COMMAND_INQUIRY:
          if ((gpb_pktIO.untransPacketCountdown == 0)&&(gpb_pktIO.busyPacketCountdown == 0))
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
#endif // FEATURE_CHECKSUM_SUPPORTED

      // Cleanup
      gpb_pktIO.packetState = GBP_PKT10_PARSE_HEADER_COMMAND_AND_COMPRESSION;
      gpb_sio_next(GPB_SIO_MODE_RESET, 0);
      gpb_sio.SINOutputPinState = false;

      // device id and device status sent. Packet done
      gpb_pktIO.packetReceivedNotify = true;
    } break;
    default:
    {
      // ? Should not reach here
      gpb_pktIO.packetState = GBP_PKT10_PARSE_HEADER_COMMAND_AND_COMPRESSION;
      gpb_sio_next(GPB_SIO_MODE_RESET, 0);
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
