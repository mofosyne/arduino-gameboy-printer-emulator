/*************************************************************************
 * 
 * GAMEBOY PRINTER EMULATION PROJECT
 * 
 * Creation Date: 2017-4-6
 * PURPOSE: To capture gameboy printer images without a gameboy printer 
 * AUTHOR: Brian Khuu
 * 
 */

#ifdef __cplusplus
extern "C" {
#endif

#define NO_NEW_BIT -1
#define NO_NEW_BYTE -1

#define SC_PIN 13 // Serial Clock   (INPUT)
#define SD_PIN 0 // Serial Data    (?)
#define SO_PIN 12 // Serial OUTPUT  (INPUT)
#define SI_PIN 11 // Serial INPUT   (OUTPUT)


typedef enum gbp_parse_state_t
{ // Indicates the stage of the parsing processing (syncword is not parsed)
    GBP_PARSE_STATE_COMMAND,
    GBP_PARSE_STATE_COMPRESSION,
    GBP_PARSE_STATE_DATA_LENGTH_LOW,
    GBP_PARSE_STATE_PACKET_DATA_LENGTH_HIGH,
    GBP_PARSE_STATE_VARIABLE_PAYLOAD,
    GBP_PARSE_STATE_CHECKSUM_LOW,
    GBP_PARSE_STATE_CHECKSUM_HIGH,
    GBP_PARSE_STATE_ACK,
    GBP_PARSE_STATE_PRINTER_STATUS,
    GBP_PARSE_STATE_DIAGNOSTICS
} gbp_parse_state_t;



/************************************************************************/

// include the library code:
#include <LiquidCrystal.h>

/*
  LCD OBJECT
*/

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(9,8,7,6,5,4);

/*
  Serial Printf (http://playground.arduino.cc/Main/Printf)
*/
static FILE serialout = {0} ;      // serialout FILE structure

static int serial_putchar(char ch, FILE* stream)
{
    Serial.write(ch) ;
    return (0) ;
}

/*
  Gameboy TX RX
*/

typedef struct {
  uint8_t data;
  uint8_t bitmask;
} gbp_tx_byte_t;

gbp_tx_byte_t gbp_tx_byte_buffer;

/**************************************************************
 * 
 *  GAMEBOY PRINTER PROTOCOL
 * 
 **************************************************************/

#define GBP_SYNC_WORD_BYTE_1 0x88
#define GBP_SYNC_WORD_BYTE_2 0x33
#define GBP_SYNC_WORD 0x8833

// Initialize(0x01), Data (0x04), Print (0x02), and Inquiry (0x0F).
#define GBP_COMMAND_INIT 0x01
#define GBP_COMMAND_DATA 0x04
#define GBP_COMMAND_PRINT 0x02
#define GBP_COMMAND_INQUIRY 0x0F

static uint8_t gbp_status_byte(
                                bool too_hot_or_cold, bool paper_jam, bool timeout, bool battery_low,
                                bool ready_to_print, bool print_reqested, bool currently_printing, bool checksum_error
                                )
{ // This is returns a gameboy printer status byte (Based on description in http://gbdev.gg8.se/wiki/articles/Gameboy_Printer )
  /*        | BITFLAG NAME            |BIT POS| */
  return 
          ( ( too_hot_or_cold     ?1:0) <<  7 )
        | ( ( paper_jam           ?1:0) <<  6 )
        | ( ( timeout             ?1:0) <<  5 )
        | ( ( battery_low         ?1:0) <<  4 )
        | ( ( ready_to_print      ?1:0) <<  3 )
        | ( ( print_reqested      ?1:0) <<  2 )
        | ( ( currently_printing  ?1:0) <<  1 )
        | ( ( checksum_error      ?1:0) <<  0 )
        ;
}

/**************************************************************
 * 
 *  GAMEBOY PRINTER FUNCTIONS (Stream Byte Version)
 * 
 **************************************************************/

/*
    Structs
*/

typedef struct gbp_rx_tx_byte_buffer_t
{
  bool      initialized;

  // Bit State
  int       serial_clock_state_prev;  // Previous Serial Clock State

  // Sync word
  bool      syncronised; // Is true when byte is aligned
  uint16_t  sync_word;   // Sync word to match against
  uint16_t  sync_buffer; // Streaming sync buffer

  // Bit position within a byte frame
  uint8_t   byte_frame_bit_pos;  

  // Used for receiving byte
  uint8_t   rx_byte_buffer;

  // Used for transmitting byte
  uint8_t   tx_byte_staging;
  uint8_t   tx_byte_buffer;
} gbp_rx_tx_byte_buffer_t;

typedef struct gbp_packet_parser_t
{
  gbp_parse_state_t parse_state_prev;
  gbp_parse_state_t parse_state;
  uint16_t data_index;
} gbp_packet_parser_t;

typedef struct gbp_packet_t
{ // This represents the structure of a gbp printer packet (excluding the sync word)
  // Received
  uint8_t   command;  
  uint8_t   compression;  
  uint16_t  data_length;
  uint8_t   *data_ptr;   // Variable length field determined by data_length
  uint16_t  checksum;

  // Send
  uint8_t   acknowledgement;
  uint8_t   printer_status;
} gbp_packet_t;



/*
    Global Vars
*/
uint8_t                 gbp_data_buffer[400];   // Variable length field determined by data_length (Convert to pointer if mallocing this)
gbp_rx_tx_byte_buffer_t gbp_rx_tx_byte_buffer;
gbp_packet_parser_t     gbp_packet_parser;
gbp_packet_t            gbp_packet = 
{
  0,  //  command
  0,  //  compression
  0,  //  data_length
  gbp_data_buffer,  // data
  0,  //  checksum
  0,  //  ack
  0   //  status
};

/*
    Static Functions
*/

static bool gbp_rx_tx_byte_reset(struct gbp_rx_tx_byte_buffer_t *ptr)
{ // Resets the byte reader, back into scanning for the next packet.
  *ptr = {0}; // Clear 

  ptr->initialized  = true;
  ptr->syncronised  = false;
  ptr->sync_word    = GBP_SYNC_WORD;
}

static bool gbp_rx_tx_byte_set(struct gbp_rx_tx_byte_buffer_t *ptr, const uint8_t tx_byte)
{ // Stages the next byte to be transmitted
  ptr->tx_byte_staging = tx_byte;
}

static bool gbp_rx_tx_byte_update(struct gbp_rx_tx_byte_buffer_t *ptr, uint8_t *rx_byte,  int *rx_bitState)
{ // This is a byte scanner to allow this to read gameboy printer protocol formatted messages
  bool byte_ready = false;

  int serial_clock_state  = digitalRead(SC_PIN);
  int serial_data_state   = digitalRead(SD_PIN);
  int serial_out_state    = digitalRead(SO_PIN);

  *rx_bitState = NO_NEW_BIT;

  if (!(ptr->initialized))
  { // Record initial clock pin state
    gbp_rx_tx_byte_reset(ptr);
    ptr->initialized = true;
    ptr->serial_clock_state_prev = serial_clock_state;
    return byte_ready;
  }

  // Clock Edge Detection
  if (serial_clock_state != ptr->serial_clock_state_prev)
  { // Clock Pin Transition Detected

    if (serial_clock_state)
    { // Rising Clock (Bit Rx Read)

      // Current Bit State (Useful for diagnostics)
      *rx_bitState = serial_out_state;

      // Is this syncronised to a byte frame yet?
      if (!(ptr->syncronised))
      { // Preamble Sync Scan

        // The sync buffer is seen as a FIFO stream of bits
        (ptr->sync_buffer) <<= 1;

        // Push in a `1` else leave as `0`
        if(serial_out_state)
        { 
          (ptr->sync_buffer) |= 1;
        }

        // Check if Sync Word is found
        if (ptr->sync_buffer == ptr->sync_word)
        { // Syncword detected
          ptr->syncronised = true;
          ptr->byte_frame_bit_pos = 7;
        }

      }
      else
      { // Byte Read Mode

        if(serial_out_state)
        { // Get latest incoming bit and insert to next bit position in a byte
          ptr->rx_byte_buffer |= (1 << ptr->byte_frame_bit_pos);
        }

        if (ptr->byte_frame_bit_pos > 0)
        { // Need to read a few more bits to make a byte
          ptr->byte_frame_bit_pos--;
        }
        else
        { // All bits in a byte frame has been received
          byte_ready = true;
          
          // Set Byte Result
          *rx_byte = ptr->rx_byte_buffer;
          // Reset Rx Buffer
          ptr->byte_frame_bit_pos = 7;
          ptr->rx_byte_buffer = 0;
        }
      }
    }
    else
    { // Falling Clock (Bit Tx Set)

      if ((ptr->syncronised) );
      { // Only start transmitting when syncronised

        // Loading new TX Bytes on new byte frames
        if(7 == ptr->byte_frame_bit_pos)
        { // Start of a new byte cycle
            if (ptr->tx_byte_staging)
            { // Byte ready to be sent
              ptr->tx_byte_buffer = ptr->tx_byte_staging;
            }
            else
            { // No new bytes present. Just keep transmitting zeros.
              ptr->tx_byte_buffer = 0;
            }
        }

        // Send next bit in a byte
        if(ptr->tx_byte_buffer & (1 << ptr->byte_frame_bit_pos))
        { // Send High Bit
          digitalWrite(SI_PIN, HIGH);
        }
        else
        { // Send Low Bit
          digitalWrite(SI_PIN, LOW);
        }

      }

    }

  }

  // Save current state for next edge detection
  ptr->serial_clock_state_prev = serial_clock_state;
  return byte_ready;
}

static bool gbp_parse_message_reset(struct gbp_packet_parser_t *ptr)
{
  *ptr = 
  {
    (gbp_parse_state_t) 0,
    (gbp_parse_state_t) 0,
    0
  };
}

static bool gbp_parse_message_update(struct gbp_packet_parser_t *ptr, struct gbp_packet_t *packet_ptr, const bool new_rx_byte, const uint8_t rx_byte, bool *new_tx_byte, uint8_t *tx_byte)
{
  static uint16_t data_index;
  bool   packet_ready_flag = false;

  // Indicates if there was a change in state on last cycle
  bool   init_state_flag = (ptr->parse_state != ptr->parse_state_prev);

  // This keeps track of each stage and how to handle each incoming byte
  switch (ptr->parse_state)
  {
    case GBP_PARSE_STATE_COMMAND:
    {
      if (new_rx_byte)
      {
        packet_ptr->command = rx_byte;
        ptr->parse_state = (gbp_parse_state_t)((int)ptr->parse_state + 1); // Move to next parse state ( Would usally be written as `parse_state++` in plain C )
      }
    } break;
    case GBP_PARSE_STATE_COMPRESSION:
    {
      if (new_rx_byte)
      {
        packet_ptr->compression = rx_byte;
        ptr->parse_state = (gbp_parse_state_t)((int)ptr->parse_state + 1); // Move to next parse state ( Would usally be written as `parse_state++` in plain C )
      }
    } break;
    case GBP_PARSE_STATE_DATA_LENGTH_LOW:
    {
      if (init_state_flag)
      {
        packet_ptr->data_length = 0;
      }
      if (new_rx_byte)
      {
        packet_ptr->data_length |= ( (rx_byte << 0) & 0x00FF );
        ptr->parse_state = (gbp_parse_state_t)((int)ptr->parse_state + 1); // Move to next parse state ( Would usally be written as `parse_state++` in plain C )
      }
    } break;
    case GBP_PARSE_STATE_PACKET_DATA_LENGTH_HIGH:
    {
      if (new_rx_byte)
      {
        packet_ptr->data_length |= ( (rx_byte << 8) & 0xFF00 );

        // Check data length
        if (packet_ptr->data_length > 0)
        {
          ptr->parse_state = (gbp_parse_state_t)((int)ptr->parse_state + 1); // Move to next parse state ( Would usally be written as `parse_state++` in plain C )
        }
        else
        { // Skip variable payload stage if data_length is zero
          ptr->parse_state = GBP_PARSE_STATE_CHECKSUM_LOW;
        }
      }
    } break;
    case GBP_PARSE_STATE_VARIABLE_PAYLOAD:
    {
      /*
        The logical flow of this section is similar to 
        `for (data_index = 0 ; (data_index > packet_ptr->data_length) ; data_index++ )`
      */

      if (init_state_flag)
      {
        ptr->data_index = 0;
      }

      if (new_rx_byte)
      {
        // Record Byte
        packet_ptr->data_ptr[ptr->data_index] = rx_byte;

        // Increment to next byte position in the data field
        ptr->data_index++;

        // Escape and move to next stage
        if (ptr->data_index > packet_ptr->data_length)
        {
          ptr->parse_state = (gbp_parse_state_t)((int)ptr->parse_state + 1); // Move to next parse state ( Would usally be written as `parse_state++` in plain C )
        }
      }
    } break;
    case GBP_PARSE_STATE_CHECKSUM_LOW:
    {
      if (new_rx_byte)
      {
        packet_ptr->checksum = 0;
        packet_ptr->checksum |= ( (rx_byte << 0) & 0x00FF );
        ptr->parse_state = (gbp_parse_state_t)((int)ptr->parse_state + 1); // Move to next parse state ( Would usally be written as `parse_state++` in plain C )
      }
    } break;
    case GBP_PARSE_STATE_CHECKSUM_HIGH:
    {
      if (new_rx_byte)
      {
        packet_ptr->checksum |= ( (rx_byte << 8) & 0xFF00 );
        ptr->parse_state = (gbp_parse_state_t)((int)ptr->parse_state + 1); // Move to next parse state ( Would usally be written as `parse_state++` in plain C )
      }
    } break;
    case GBP_PARSE_STATE_ACK:
    {
      if (init_state_flag)
      {
        *new_tx_byte = true;
        *tx_byte = 0x81;
        /* 
          # "GB Printer interface specification" [Source](https://www.mikrocontroller.net/attachment/34801/gb-printer.txt)
          > An acknowledgement code is set (by GB Printer) to either 0x80 or 0x81. 
          > The difference of those two values is unsure at this moment.
          > However, any other values are unaccepted and should be avoided.
          > When writing a GB Printer alternative (e.g., emulator,) it is safe to return 0x81 always.
        */
      }
      if (new_rx_byte)
      {
        ptr->parse_state = (gbp_parse_state_t)((int)ptr->parse_state + 1); // Move to next parse state ( Would usally be written as `parse_state++` in plain C )
      }
    } break;
    case GBP_PARSE_STATE_PRINTER_STATUS:
    {
      if (init_state_flag)
      {
        *new_tx_byte = true;
        *tx_byte = gbp_status_byte( 0,0,0,0 , 0,0,0,0 );
      }
      if (new_rx_byte)
      {
        ptr->parse_state = (gbp_parse_state_t)((int)ptr->parse_state + 1); // Move to next parse state ( Would usally be written as `parse_state++` in plain C )
        packet_ready_flag = true;
      }
    } break;
    case GBP_PARSE_STATE_DIAGNOSTICS:
    {
      if (init_state_flag)
      {
        Serial.println("Received:");
        Serial.println(packet_ptr->command,HEX);
        Serial.println(packet_ptr->compression,HEX);
        Serial.println(packet_ptr->data_length,HEX);
        Serial.println(packet_ptr->checksum,HEX);
        Serial.println(packet_ptr->acknowledgement,HEX);
        Serial.println(packet_ptr->printer_status,HEX);
      }
    } break;

  }

  // Keeping track of change in state
  ptr->parse_state == ptr->parse_state_prev;
  return packet_ready_flag;
}

/**************************************************************
 **************************************************************/

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("GameBody Link");

  // Config Serial
  Serial.begin(9600);
  // Serial fprintf setup
  fdev_setup_stream (&serialout, serial_putchar, NULL, _FDEV_SETUP_WRITE);
  //e.g. fprintf(&serialout, ">> SD:%d SO:%d SI:%d\n", serial_data_state, serial_out_state, serial_input_state ) ;

  pinMode(SC_PIN, INPUT);
  pinMode(SO_PIN, INPUT);
  pinMode(SI_PIN, OUTPUT);

  // Default output value
  digitalWrite(SI_PIN, LOW);

  Serial.print("GAMEBOY PRINTER EMULATION PROJECT\n");

  // Clear Byte Scanner and Parser
  gbp_rx_tx_byte_reset(&gbp_rx_tx_byte_buffer);
  gbp_parse_message_reset(&gbp_packet_parser);
} // setup()

void loop() {  
  int rx_bitState;

  uint8_t rx_byte;
  bool new_rx_byte;

  uint8_t tx_byte;
  bool new_tx_byte;

  bool packet_ready_flag;

  new_rx_byte = gbp_rx_tx_byte_update(&gbp_rx_tx_byte_buffer, &rx_byte,  &rx_bitState);

#if 0 // Bit Scanning
  if ( NO_NEW_BIT != rx_bitState )
  { // New bit detected
    Serial.print(rx_bitState);
  }
#endif

#if 1 // Byte Scanning
  if (new_rx_byte)
  {
    Serial.println(rx_byte,HEX);
  }
#endif

  packet_ready_flag = gbp_parse_message_update(
                                              &gbp_packet_parser, &gbp_packet, 
                                              new_rx_byte, rx_byte,
                                              &new_tx_byte, &tx_byte
                                              );
  if (new_tx_byte)
  {
    gbp_rx_tx_byte_set(&gbp_rx_tx_byte_buffer, tx_byte);
  }

  if (packet_ready_flag)
  {
#if 1
    Serial.println("#");
#else
    Serial.println("packet received");
    switch (gbp_packet.command)
    {
      case GBP_COMMAND_INIT:
        Serial.println("INIT");
        break;
      case GBP_COMMAND_DATA:
        Serial.println("DATA");
        break;
      case GBP_COMMAND_PRINT:
        Serial.println("PRNT");
        break;
      case GBP_COMMAND_INQUIRY:
        Serial.println("INQY");
        break;
    }
    Serial.print("cmp:");
    Serial.println(gbp_packet.compression,HEX);
    Serial.print("len:");
    Serial.println(gbp_packet.data_length,HEX);
    Serial.print("csum:");
    Serial.println(gbp_packet.checksum,HEX);
    Serial.println("---");
#endif
    gbp_rx_tx_byte_reset(&gbp_rx_tx_byte_buffer);
    gbp_parse_message_reset(&gbp_packet_parser);
  }


} // loop()

#ifdef __cplusplus
}
#endif
