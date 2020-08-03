/*************************************************************************
 *
 * GAMEBOY PRINTER EMULATION PROJECT (Arduino) (Dev Version)
 *
 * Creation Date: 2017-4-6
 * PURPOSE: To capture gameboy printer images without a gameboy printer.
 *           This version is to investigate gameboy behaviour.
 * AUTHOR: Brian Khuu
 *
 */
#include <stdint.h> // uint8_t
#include <stddef.h> // size_t
#include <stdbool.h> // bool

/*******************************************************************************
*******************************************************************************/

//#define MINIMUM_PRINT_MODE // Skip C compatible capture for speed boost

/*******************************************************************************
*******************************************************************************/

/* Gameboy Link Cable Mapping to Arduino Pin */
// Note: Serial Clock Pin must be attached to an interrupt pin of the arduino
//  ___________
// |  6  4  2  |
//  \_5__3__1_/   (at cable)
//

#ifdef ESP8266
// Pin Setup for ESP8266 Devices
//                  | Arduino Pin | Gameboy Link Pin  |
#define GBP_VCC_PIN               // Pin 1            : 5.0V (Unused)
#define GBP_SO_PIN       13       // Pin 2            : ESP-pin 7 MOSI (Serial OUTPUT) -> Arduino 13
#define GBP_SI_PIN       12       // Pin 3            : ESP-pin 6 MISO (Serial INPUT)  -> Arduino 12
#define GBP_SD_PIN                // Pin 4            : Serial Data  (Unused)
#define GBP_SC_PIN       14       // Pin 5            : ESP-pin 5 CLK  (Serial Clock)  -> Arduino 14
#define GBP_GND_PIN               // Pin 6            : GND (Attach to GND Pin)
#define LED_STATUS_PIN    2       // Internal LED blink on packet reception
#else
// Pin Setup for Arduinos
//                  | Arduino Pin | Gameboy Link Pin  |
#define GBP_VCC_PIN               // Pin 1            : 5.0V (Unused)
#define GBP_SO_PIN        4       // Pin 2            : Serial OUTPUT
#define GBP_SI_PIN        3       // Pin 3            : Serial INPUT
#define GBP_SD_PIN                // Pin 4            : Serial Data  (Unused)
#define GBP_SC_PIN        2       // Pin 5            : Serial Clock (Interrupt)
#define GBP_GND_PIN               // Pin 6            : GND (Attach to GND Pin)
#define LED_STATUS_PIN   13       // Internal LED blink on packet reception
#endif

/*******************************************************************************
*******************************************************************************/

#define GPB_BUFFER_SIZE 300
uint8_t gbp_buffer_Gameboy[GPB_BUFFER_SIZE] = {0};
uint8_t gbp_buffer_Printer[GPB_BUFFER_SIZE] = {0};


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

static inline size_t gpb_cbuff_Capacity(gpb_cbuff_t *cb){ return cb->capacity;}
static inline size_t gpb_cbuff_Count(gpb_cbuff_t *cb)   { return cb->count;}
static inline bool gpb_cbuff_IsFull(gpb_cbuff_t *cb)    { return (cb->count >= cb->capacity);}
static inline bool gpb_cbuff_IsEmpty(gpb_cbuff_t *cb)   { return (cb->count == 0);}


/**************************************************************
 * SNIFFER ISR
 **************************************************************/
#define GBP_SYNC_WORD_0       0x88    // 0b10001000
#define GBP_SYNC_WORD_1       0x33    // 0b00110011
#define GBP_SYNC_WORD         0x8833  // 0b1000100000110011
#define GBP_COMMAND_INIT      0x01    // 0b00000001 // Typically 10 bytes packet
#define GBP_COMMAND_PRINT     0x02    // 0b00000010 // Print instructions
#define GBP_COMMAND_DATA      0x04    // 0b00000100 // Typically 650 bytes packet (10byte header + 640byte image)
#define GBP_COMMAND_BREAK     0x08    // 0b00001000 // Means to forcibly stops printing
#define GBP_COMMAND_INQUIRY   0x0F    // 0b00001111 // Always reports current status
#define GBP_STATUS_MASK_LOWBAT      (1<<7) // Battery Too Low
#define GBP_STATUS_MASK_ER2         (1<<6) // Other Error
#define GBP_STATUS_MASK_ER1         (1<<5) // Paper Jam
#define GBP_STATUS_MASK_ER0         (1<<4) // Packet Error (e.g. likely gameboy program failure)
#define GBP_STATUS_MASK_UNTRAN      (1<<3) // Unprocessed Data
#define GBP_STATUS_MASK_FULL        (1<<2) // Image Data Full
#define GBP_STATUS_MASK_BUSY        (1<<1) // Printer Busy
#define GBP_STATUS_MASK_SUM         (1<<0) // Checksum Error

gpb_cbuff_t gbp_snifferDataBuffer_Gameboy = {0};
gpb_cbuff_t gbp_snifferDataBuffer_Printer = {0};

// returns boolean value for led activity indicator
bool gpb_sniffer_OnChange_ISR(const bool GBP_SC, const bool GBP_SO, const bool GBP_SI)
{
  static uint16_t preambleSO; ///< Scanning for Preamble
  static uint16_t preambleSI; ///< Scanning for Preamble
  static bool gameboyConnectedToSO; // Gameboy connected to SO (Typical)
  static bool gameboyConnectedToSI; // Gameboy connected to SI (Swapped)
  // Psudo 8bit SPI
  static uint8_t bitMaskMap;
  static uint8_t rx_buff; // From gameboy
  static uint8_t tx_buff; // From printer

  // Detect which side of the cable the gameboy is connected to
  if (!(gameboyConnectedToSO || gameboyConnectedToSI))
  {
    // Expecting rising edge
    if (!GBP_SC)
      return false;
    // Clocking bits on rising edge
    preambleSO |= GBP_SO ? 1 : 0;
    preambleSI |= GBP_SI ? 1 : 0;
    // Check if found
    gameboyConnectedToSO = (preambleSO & 0xFFFF) == GBP_SYNC_WORD;
    gameboyConnectedToSI = (preambleSI & 0xFFFF) == GBP_SYNC_WORD;
    // Sync Not Found? Keep scanning
    if (!(gameboyConnectedToSO || gameboyConnectedToSI))
    {
      preambleSO <<= 1;
      preambleSI <<= 1;
      return false;
    }
    // Preamble Found
    preambleSO = 0;
    preambleSI = 0;
    // Start capturing bytes
    bitMaskMap = 1<<7; ///< Reset Psudo SPI engine
    rx_buff = 0;
    tx_buff = 0;
    // Add bytes we found
    gpb_cbuff_Enqueue(&gbp_snifferDataBuffer_Gameboy, GBP_SYNC_WORD_0);
    gpb_cbuff_Enqueue(&gbp_snifferDataBuffer_Gameboy, GBP_SYNC_WORD_1);
    gpb_cbuff_Enqueue(&gbp_snifferDataBuffer_Printer, 0x00); // Ignore initial 2 bytes from printer
    gpb_cbuff_Enqueue(&gbp_snifferDataBuffer_Printer, 0x00); // Ignore initial 2 bytes from printer
    return true;
  }
  /* Swap bit if cable swapped */
  const bool rx_bit = (gameboyConnectedToSO) ? GBP_SO : GBP_SI; // From gameboy
  const bool tx_bit = (gameboyConnectedToSO) ? GBP_SI : GBP_SO; // From printer
  /* Psudo SPI Engine */
  // Basically I have one bit acting as a mask moving across a word sized buffer
  if (bitMaskMap > 0)
  {
    // Serial Transaction Is Active
    if (GBP_SC)
    {
      // Rising Edge Clock (Rx Bit)
      //gpb_cbuff_Enqueue(&gbp_snifferDataBuffer_Gameboy, (tx_bit<<4)|(rx_bit)); // Dev: Check bit by bit
      rx_buff |= rx_bit ? (bitMaskMap & 0xFF) : 0; ///< Clocking bits on rising edge
      tx_buff |= tx_bit ? (bitMaskMap & 0xFF) : 0; ///< Clocking bits on rising edge
      bitMaskMap >>= 1; ///< One tx/rx bit cycle complete, next bit now
      if (bitMaskMap > 0)
        return true;
    }
    else
    {
      // Falling Edge Clock (Tx Bit)
      return false;
    }
  }
  // Byte found
  gpb_cbuff_Enqueue(&gbp_snifferDataBuffer_Gameboy, rx_buff);
  gpb_cbuff_Enqueue(&gbp_snifferDataBuffer_Printer, tx_buff);
  // Search for next byte
  bitMaskMap = 1<<7; ///< Reset Psudo SPI engine
  rx_buff = 0;
  tx_buff = 0;
  return true;
}


/**************************************************************
 **************************************************************/

#ifdef ESP8266
void ICACHE_RAM_ATTR serialClock_ISR(void)
#else
void serialClock_ISR(void)
#endif
{
  // Serial Clock (1 = Rising Edge) (0 = Falling Edge); Master Output Slave Input (This device is slave)
  const bool activity = gpb_sniffer_OnChange_ISR(digitalRead(GBP_SC_PIN), digitalRead(GBP_SO_PIN), digitalRead(GBP_SI_PIN));
  digitalWrite(LED_STATUS_PIN, (activity)?HIGH:LOW);
}

void setup(void)
{
  // Config Serial
  // Has to be fast or it will not transfer the image fast enough to the computer
  Serial.begin(115200);

  /* Pins from gameboy link cable */
  pinMode(GBP_SC_PIN, INPUT);
  pinMode(GBP_SO_PIN, INPUT);
  pinMode(GBP_SI_PIN, INPUT);

  /* LED Indicator */
  pinMode(LED_STATUS_PIN, OUTPUT);
  digitalWrite(LED_STATUS_PIN, LOW);

  /* Welcome Message */
  Serial.print("/* GAMEBOY PRINTER EMULATION PROJECT (Packet Capture Mode) */\n");
  Serial.print("/* By Brian Khuu (2020) */\n");
#ifdef MINIMUM_PRINT_MODE
  Serial.print("// Note: Every byte is from the gameboy except the last two byte of each packet \n");
  Serial.print("//       which be the response from the printer (Minimum Mode)\n");
#else
  Serial.print("// Note: Every byte is from the gameboy except when indicated by '(' or ')' to \n");
  Serial.print("//       which it would indicate a response from the printer (C source format)\n");
#endif
  Serial.print("//------------------------------------------------------------------------------\n");
  Serial.print("// REAL GAMEBOY TO GAMEBOY PRINTER PACKET CAPTURE \n");
  Serial.print("// GAME: <Game name> \n");
  Serial.print("// DATE: <Date of capture in ISO 8601 format e.g. 2020-08-02 >\n");
  Serial.print("// AUTHOR: <Your name and contact details if you want>\n");
  Serial.print("// Please send your capture to the issue page below to assist in development \n");
  Serial.print("// https://github.com/mofosyne/arduino-gameboy-printer-emulator/issues \n");

  /* Setup */
  gpb_cbuff_Init(&gbp_snifferDataBuffer_Gameboy, sizeof(gbp_buffer_Gameboy), gbp_buffer_Gameboy);
  gpb_cbuff_Init(&gbp_snifferDataBuffer_Printer, sizeof(gbp_buffer_Printer), gbp_buffer_Printer);

  /* attach ISR */
  attachInterrupt( digitalPinToInterrupt(GBP_SC_PIN), serialClock_ISR, CHANGE);  // attach interrupt handler
} // setup()


char *gbpCommand_toStr(int val)
{
  switch (val)
  {
    case GBP_COMMAND_INIT    : return "INIT";
    case GBP_COMMAND_PRINT   : return "PRINT";
    case GBP_COMMAND_DATA    : return "DATA";
    case GBP_COMMAND_BREAK   : return "BREAK";
    case GBP_COMMAND_INQUIRY : return "INQUIRY";
    default: return "?";
  }
}

void loop()
{
  /* tiles received */
  static uint32_t byteTotal = 0;
  static uint16_t pktTotalCount = 0;
  static uint16_t pktByteIndex = 0;
  static uint16_t pktDataLength = 0;
  const size_t dataBuffCount = gpb_cbuff_Count(&gbp_snifferDataBuffer_Gameboy);
  if (
      ((pktByteIndex != 0)&&(dataBuffCount>0))||
      ((pktByteIndex == 0)&&(dataBuffCount>=6))
      )
  {
    const char nibbleToCharLUT[] = "0123456789ABCDEF";
    uint8_t rx_8bit = 0;
    uint8_t tx_8bit = 0;
    for (size_t i = 0 ; i < dataBuffCount ; i++)
    { // Display the data payload encoded in hex
      // Start of a new packet
      if (pktByteIndex == 0)
      {
        /*
          [ 00 ][ 01 ][ 02 ][ 03 ][ 04 ][ 05 ][ 5+X  ][ 5+X+1 ][ 5+X+2 ][ 5+X+3 ][ 5+X+4 ]
          [SYNC][SYNC][COMM][COMP][LEN0][LEN1][ DATA ][ CSUM0 ][ CSUM1 ][ DUMMY ][ DUMMY ]
        */
        uint8_t commandTypeByte = 0;
        uint8_t dataLengthByte0 = 0;
        uint8_t dataLengthByte1 = 0;
        gpb_cbuff_Dequeue_Peek(&gbp_snifferDataBuffer_Gameboy, &commandTypeByte, 2);
        gpb_cbuff_Dequeue_Peek(&gbp_snifferDataBuffer_Gameboy, &dataLengthByte0, 4);
        gpb_cbuff_Dequeue_Peek(&gbp_snifferDataBuffer_Gameboy, &dataLengthByte1, 5);
        pktDataLength = 0;
        pktDataLength |= ((uint16_t)dataLengthByte0<<0)&0x00FF;
        pktDataLength |= ((uint16_t)dataLengthByte1<<8)&0xFF00;
#ifdef MINIMUM_PRINT_MODE
        Serial.print("# ");
        Serial.print(pktTotalCount);
        Serial.print(" : ");
        Serial.print(gbpCommand_toStr(commandTypeByte));
#else
        Serial.print("/* ");
        Serial.print(pktTotalCount);
        Serial.print(" : ");
        Serial.print(gbpCommand_toStr(commandTypeByte));
        Serial.print(" */\n");
#endif
        digitalWrite(LED_STATUS_PIN, HIGH);
      }
      // Print Hex Byte
      gpb_cbuff_Dequeue(&gbp_snifferDataBuffer_Gameboy, &rx_8bit);
      gpb_cbuff_Dequeue(&gbp_snifferDataBuffer_Printer, &tx_8bit);
#ifndef MINIMUM_PRINT_MODE
      Serial.print((pktByteIndex == (8+pktDataLength + 0))?"/*(*/ ":"");
      Serial.print((char)'0');
      Serial.print((char)'x');
#endif
      if (pktByteIndex < (8+pktDataLength))
      {
        // Send bytes from gameboy to printer. This is the main packet
        Serial.print((char)nibbleToCharLUT[(rx_8bit>>4)&0xF]);
        Serial.print((char)nibbleToCharLUT[(rx_8bit>>0)&0xF]);
      }
      else
      {
        // Send bytes from printer to gameboy. This is the status response.
        Serial.print((char)nibbleToCharLUT[(tx_8bit>>4)&0xF]);
        Serial.print((char)nibbleToCharLUT[(tx_8bit>>0)&0xF]);
      }
#ifdef MINIMUM_PRINT_MODE
      Serial.print((char)' ');
#else
      Serial.print((char)',');
      if (pktByteIndex == (8+pktDataLength + 1))
      {
        Serial.print(" /*)*/");
        if (tx_8bit)
        {
          Serial.print(" /* Printer Status: ");
          if (tx_8bit & GBP_STATUS_MASK_LOWBAT) { Serial.print("LOWBAT ");}
          if (tx_8bit & GBP_STATUS_MASK_ER2   ) { Serial.print("ER2 ");}
          if (tx_8bit & GBP_STATUS_MASK_ER1   ) { Serial.print("ER1 ");}
          if (tx_8bit & GBP_STATUS_MASK_ER0   ) { Serial.print("ER0 ");}
          if (tx_8bit & GBP_STATUS_MASK_UNTRAN) { Serial.print("UNTRAN ");}
          if (tx_8bit & GBP_STATUS_MASK_FULL  ) { Serial.print("FULL ");}
          if (tx_8bit & GBP_STATUS_MASK_BUSY  ) { Serial.print("BUSY ");}
          if (tx_8bit & GBP_STATUS_MASK_SUM   ) { Serial.print("SUM ");}
          Serial.print(" */");
        }
      }
#endif
      // Splitting packets for convenience
      if ((pktByteIndex>5)&&(pktByteIndex>=(9+pktDataLength)))
      {
        digitalWrite(LED_STATUS_PIN, LOW);
        Serial.print("\n");
        pktByteIndex = 0;
        pktTotalCount++;
      }
      else
      {
        Serial.print(((pktByteIndex+1)%16 == 0)?'\n':' '); ///< Insert Newline Periodically
        pktByteIndex++; // Byte hex split counter
        byteTotal++; // Byte total counter
      }
    }
  }

} // loop()
