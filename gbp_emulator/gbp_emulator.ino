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

#define SC_PIN 2 // Serial Clock   (INPUT)  // Interrupt Pin
#define SO_PIN 3 // Serial OUTPUT  (INPUT)
#define SI_PIN 4 // Serial INPUT   (OUTPUT)
#define SD_PIN 0 // Serial Data    (?)

#define GBP_PACKET_TIMEOUT_MS 100 // ms timeout period to wait for next byte in a packet

typedef enum gbp_parse_state_t
{ // Indicates the stage of the parsing processing (syncword is not parsed)
    GBP_PARSE_STATE_COMMAND                   = 0,
    GBP_PARSE_STATE_COMPRESSION               = 1,
    GBP_PARSE_STATE_DATA_LENGTH_LOW           = 2,
    GBP_PARSE_STATE_PACKET_DATA_LENGTH_HIGH   = 3,
    GBP_PARSE_STATE_VARIABLE_PAYLOAD          = 4,
    GBP_PARSE_STATE_CHECKSUM_LOW              = 5,
    GBP_PARSE_STATE_CHECKSUM_HIGH             = 6,
    GBP_PARSE_STATE_ACK                       = 7,
    GBP_PARSE_STATE_PRINTER_STATUS            = 8,
    GBP_PARSE_STATE_PACKET_RECEIVED           = 9,
    GBP_PARSE_STATE_DIAGNOSTICS               = 10
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

// GameBoy Printer Packet Structure
/*
  | BYTE POS :   |     0     |     1     |     2     |     3     |     4     |  4 + X    | 4 + X + 1 | 4 + X + 2 | 4 + X + 3 | 4 + X + 4 |
  |--------------|-----------|-----------|-----------|-----------|-----------|-----------|-----------|-----------|-----------|-----------|
  | SIZE         |        2 Bytes        |  1 Bytes  |  1 Bytes  |  1 Bytes  | Variable  |        2 Bytes        |  1 Bytes  |  1 Bytes  |
  | DESCRIPTION  |       SYNC_WORD       | COMMAND   |     DATA_LENGTH(X)    | Payload   |       CHECKSUM        |    ACK    |  STATUS   |
  | FROM PRINTER |    0x88   |    0x33   | See Below | Low Byte  | High Byte | See Below |       See Below       |    0x00   |    0x00   |
  | TO PRINTER   |    0x00   |    0x00   |    0x00   |    0x00   |    0x00   |    0x00   |    0x00   |    0x00   |    0x81   | See Below |

  * Command may be either Initialize (0x01), Data (0x04), Print (0x02), or Inquiry (0x0F).
  * Payload byte count size depends on the value of the `DATA_LENGTH` field.
  * Checksum is a simple sum of bytes in command, data length, and the data payload.
  * Status byte is a bitfield byte indicating various status of the printer itself. (e.g. If it is still printing)
*/

// Sync Word
#define GBP_SYNC_WORD_0       0x88    // 0b10001000
#define GBP_SYNC_WORD_1       0x33    // 0b00110011
#define GBP_SYNC_WORD         0x8833  // 0b1000100000110011

// Command Byte Values
#define GBP_COMMAND_INIT      0x01 // 0b00000001 // Typically 10 bytes packet
#define GBP_COMMAND_DATA      0x04 // 0b00000100 // Typically 650 bytes packet (10byte header + 640byte image)
#define GBP_COMMAND_PRINT     0x02 // 0b00000010 // Typically 14 bytes (10 + 4 printer setting)
#define GBP_COMMAND_INQUIRY   0x0F // 0b00001111 // Always reports current status. Typically 10 bytes packet 

// ACKNOWLEGEMENT BYTE VALUE
#define GBP_ACK               0x81 // 0b10000001 // Recommended by "GB Printer interface specification"
#define GBP_ACK_2             0x80 // 0b10000000 // Used by esp8266-gameboy-printer

// GAMEBOY PRINTER STATUS BYTE BIT POSITION
#define GBP_STATUS_TEMP_WARN_BIT_POS          7
#define GBP_STATUS_PAPER_JAM_BIT_POS          6
#define GBP_STATUS_TIMEOUT_BIT_POS            5
#define GBP_STATUS_BATTERY_LOW_BIT_POS        4
#define GBP_STATUS_READY_TO_PRINT_BIT_POS     3
#define GBP_STATUS_PRINT_REQUESTED_BIT_POS    2
#define GBP_STATUS_CURRENTLY_PRINTING_BIT_POS 1
#define GBP_STATUS_CHECKSUM_ERROR_BIT_POS     0

// Gameboy Printer Packet Structure
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

// Gameboy Printer Status Code Structure
typedef struct gbp_printer_status_t
{
  bool too_hot_or_cold;
  bool paper_jam;
  bool timeout;
  bool battery_low;
  bool ready_to_print;
  bool print_reqested;
  bool currently_printing;
  bool checksum_error;
} gbp_printer_status_t;

static uint8_t gbp_status_byte(struct gbp_printer_status_t *printer_status_ptr)
{ // This is returns a gameboy printer status byte (Based on description in http://gbdev.gg8.se/wiki/articles/Gameboy_Printer )
  /*        | BITFLAG NAME                                |BIT POS| */
  return 
          ( ( printer_status_ptr->too_hot_or_cold     ?1:0) <<  GBP_STATUS_TEMP_WARN_BIT_POS          )
        | ( ( printer_status_ptr->paper_jam           ?1:0) <<  GBP_STATUS_PAPER_JAM_BIT_POS          )
        | ( ( printer_status_ptr->timeout             ?1:0) <<  GBP_STATUS_TIMEOUT_BIT_POS            )
        | ( ( printer_status_ptr->battery_low         ?1:0) <<  GBP_STATUS_BATTERY_LOW_BIT_POS        )
        | ( ( printer_status_ptr->ready_to_print      ?1:0) <<  GBP_STATUS_READY_TO_PRINT_BIT_POS     )
        | ( ( printer_status_ptr->print_reqested      ?1:0) <<  GBP_STATUS_PRINT_REQUESTED_BIT_POS    )
        | ( ( printer_status_ptr->currently_printing  ?1:0) <<  GBP_STATUS_CURRENTLY_PRINTING_BIT_POS )
        | ( ( printer_status_ptr->checksum_error      ?1:0) <<  GBP_STATUS_CHECKSUM_ERROR_BIT_POS     )
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



// Reads the bitstream and outputs a bytes stream after sync
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

// This deals with interpreting bytes stream as a packet structure
typedef struct gbp_packet_parser_t
{
  gbp_parse_state_t parse_state;
  uint16_t data_index;
  uint16_t  calculated_checksum;
} gbp_packet_parser_t;

// Printer Status and other stuff
typedef struct gbp_printer_t
{ // This is the overall information about the printer
  bool      initialized;

  gbp_printer_status_t    gbp_printer_status;
  gbp_rx_tx_byte_buffer_t gbp_rx_tx_byte_buffer;
  gbp_packet_parser_t     gbp_packet_parser;
  gbp_packet_t            gbp_packet;

  // Triggered upon successful read of a packet
  bool packet_ready_flag;

  // Buffers
  uint8_t                 gbp_print_settings_buffer[4];
  uint8_t                 gbp_print_buffer[800];  // 640 bytes usually

  // Timeout if bytes not received in time
  unsigned long uptime_til_timeout_ms;
} gbp_printer_t;


/*
    Global Vars
*/
gbp_printer_t gbp_printer; // Overall Structure

/*
    Static Functions
*/


/*------------------------- BYTE STREAMER --------------------------*/

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
#if 1
          Serial.println("-");
#endif
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

/*------------------------- MESSAGE PARSER --------------------------*/


static bool gbp_parse_message_reset(struct gbp_packet_parser_t *ptr)
{
  *ptr = 
  {
    (gbp_parse_state_t) 0,
    0
  };
}


static bool gbp_parse_message_update
(
  struct gbp_packet_parser_t *ptr,             // Parser Variables
  bool *packet_ready_flag,                     // OUTPUT: Packet Ready Semaphore
  struct gbp_packet_t *packet_ptr,             // INPUT/OUTPUT: Packet Data Buffer
  struct gbp_printer_t *printer_ptr,           // INPUT/OUTPUT: Printer Variables
  const bool new_rx_byte,                      // INPUT: New Incoming Byte Flag
  const uint8_t rx_byte,                       // INPUT: New Incoming Byte Value
  bool *new_tx_byte,                           // OUTPUT: New Outgoing Byte Ready
  uint8_t *tx_byte                             // OUTPUT: New Outgoing Byte Value
)
{ // Return false if there was no error detected
  bool error_status = false;
  gbp_parse_state_t parse_state_prev = ptr->parse_state;

  *new_tx_byte = false;

  //-------------------------- NEW BYTES

  if (new_rx_byte)
  {
    // This keeps track of each stage and how to handle each incoming byte
    switch (ptr->parse_state)
    {
      case GBP_PARSE_STATE_COMMAND:
      {
        ptr->parse_state = GBP_PARSE_STATE_COMPRESSION;

        packet_ptr->command = rx_byte;

        switch (packet_ptr->command)
        {
          case GBP_COMMAND_INIT:
            packet_ptr->data_ptr = NULL;
            break;
          case GBP_COMMAND_DATA:
            packet_ptr->data_ptr = printer_ptr->gbp_print_buffer;
            break;
          case GBP_COMMAND_PRINT:
            packet_ptr->data_ptr = printer_ptr->gbp_print_settings_buffer;
            break;
          case GBP_COMMAND_INQUIRY:
            packet_ptr->data_ptr = NULL;
            break;
          default:
            packet_ptr->data_ptr = NULL;
        }

        // Checksum Tally
        ptr->calculated_checksum = 0; // Initialise Count
        ptr->calculated_checksum += rx_byte;
      } break;
      case GBP_PARSE_STATE_COMPRESSION:
      {
        ptr->parse_state = GBP_PARSE_STATE_DATA_LENGTH_LOW;
        packet_ptr->compression = rx_byte;

        // Checksum Tally
        ptr->calculated_checksum += rx_byte;
      } break;
      case GBP_PARSE_STATE_DATA_LENGTH_LOW:
      {
        ptr->parse_state = GBP_PARSE_STATE_PACKET_DATA_LENGTH_HIGH;
        packet_ptr->data_length |= ( (rx_byte << 0) & 0x00FF );

        // Checksum Tally
        ptr->calculated_checksum += rx_byte;
      } break;
      case GBP_PARSE_STATE_PACKET_DATA_LENGTH_HIGH:
      {
        packet_ptr->data_length |= ( (rx_byte << 8) & 0xFF00 );

        // Check data length
        if (packet_ptr->data_length > 0)
        {
          if (packet_ptr->data_ptr == NULL)
          { // SIMPLE ASSERT
            Serial.println("ERROR: Serial data length should be non zero");
            while(1);
          }
          ptr->parse_state = GBP_PARSE_STATE_VARIABLE_PAYLOAD;
        }
        else
        { // Skip variable payload stage if data_length is zero
          ptr->parse_state = GBP_PARSE_STATE_CHECKSUM_LOW;
        }

        // Checksum Tally
        ptr->calculated_checksum += rx_byte;
      } break;
      case GBP_PARSE_STATE_VARIABLE_PAYLOAD:
      {
        /*
          The logical flow of this section is similar to 
          `for (data_index = 0 ; (data_index > packet_ptr->data_length) ; data_index++ )`
        */
        // Record Byte
        packet_ptr->data_ptr[ptr->data_index] = rx_byte;

        // Checksum Tally
        ptr->calculated_checksum += rx_byte;

        // Increment to next byte position in the data field
        ptr->data_index++;

        // Escape and move to next stage
        if (ptr->data_index > packet_ptr->data_length)
        {
          ptr->parse_state = GBP_PARSE_STATE_CHECKSUM_LOW;
        }
      } break;
      case GBP_PARSE_STATE_CHECKSUM_LOW:
      {
        ptr->parse_state = GBP_PARSE_STATE_CHECKSUM_HIGH;
        packet_ptr->checksum = 0;
        packet_ptr->checksum |= ( (rx_byte << 0) & 0x00FF );
      } break;
      case GBP_PARSE_STATE_CHECKSUM_HIGH:
      {
        ptr->parse_state = GBP_PARSE_STATE_ACK;
        packet_ptr->checksum |= ( (rx_byte << 8) & 0xFF00 );
      } break;
      case GBP_PARSE_STATE_ACK:
      {
        ptr->parse_state = GBP_PARSE_STATE_PRINTER_STATUS;
      } break;
      case GBP_PARSE_STATE_PRINTER_STATUS:
      {
        ptr->parse_state = GBP_PARSE_STATE_PACKET_RECEIVED;
      } break;
      case GBP_PARSE_STATE_PACKET_RECEIVED:
      {
      } break;
      case GBP_PARSE_STATE_DIAGNOSTICS:
      {
      } break;
    }
  } // New Byte Detected

  //-------------------------- INIT FOR NEXT STAGE
  /*
    This section commonly deals with initialising the next parsing state.
    e.g. Initialising variables in addition to also staging the next response byte.
  */

  // Indicates if there was a change in state on last cycle
  if (ptr->parse_state != parse_state_prev)
  {
    // This keeps track of each stage and how to handle each incoming byte
    switch (ptr->parse_state)
    {
      case GBP_PARSE_STATE_COMMAND:
      {
      } break;
      case GBP_PARSE_STATE_COMPRESSION:
      {
      } break;
      case GBP_PARSE_STATE_DATA_LENGTH_LOW:
      {
        packet_ptr->data_length = 0;
      } break;
      case GBP_PARSE_STATE_PACKET_DATA_LENGTH_HIGH:
      {
      } break;
      case GBP_PARSE_STATE_VARIABLE_PAYLOAD:
      {
        ptr->data_index = 0;
      } break;
      case GBP_PARSE_STATE_CHECKSUM_LOW:
      {
      } break;
      case GBP_PARSE_STATE_CHECKSUM_HIGH:
      {
      } break;
      case GBP_PARSE_STATE_ACK:
      {
        /* 
          # "GB Printer interface specification" [Source](https://www.mikrocontroller.net/attachment/34801/gb-printer.txt)
          > An acknowledgement code is set (by GB Printer) to either 0x80 or 0x81. 
          > The difference of those two values is unsure at this moment.
          > However, any other values are unaccepted and should be avoided.
          > When writing a GB Printer alternative (e.g., emulator,) it is safe to return 0x81 always.
        */
        *new_tx_byte = true;
        *tx_byte = GBP_ACK;

        packet_ptr->acknowledgement = *tx_byte;
      } break;
      case GBP_PARSE_STATE_PRINTER_STATUS:
      {

        // Checksum Verification
        if (ptr->calculated_checksum == packet_ptr->checksum)
        { // Checksum Passed
          printer_ptr->gbp_printer_status.checksum_error = false;
        }
        else
        { // Checksum Failed
          printer_ptr->gbp_printer_status.checksum_error = true;
        }

        switch (packet_ptr->command)
        {
          case GBP_COMMAND_INIT:
            break;
          case GBP_COMMAND_DATA:
            printer_ptr->gbp_printer_status.ready_to_print      = true;
            break;
          case GBP_COMMAND_PRINT:
            printer_ptr->gbp_printer_status.ready_to_print      = false;
            printer_ptr->gbp_printer_status.print_reqested      = true;
            printer_ptr->gbp_printer_status.currently_printing  = true;
            break;
          case GBP_COMMAND_INQUIRY:
            break;
        }

        *new_tx_byte = true;
        *tx_byte = gbp_status_byte(&(printer_ptr->gbp_printer_status));
        packet_ptr->printer_status = *tx_byte;
      } break;
      case GBP_PARSE_STATE_PACKET_RECEIVED:
      {
        *packet_ready_flag = true;
      } break;
      case GBP_PARSE_STATE_DIAGNOSTICS:
      {
      } break;

    }
  } // Init Next State


  return error_status;
}

/*------------------------- Gameboy Printer --------------------------*/

static bool gbp_printer_init(struct gbp_printer_t *ptr)
{
  ptr->initialized = true;
  ptr->gbp_printer_status = {0};

  gbp_rx_tx_byte_reset(&(ptr->gbp_rx_tx_byte_buffer));
  gbp_parse_message_reset(&(ptr->gbp_packet_parser));
}



/**************************************************************
 **************************************************************/

static void gbp_status_byte_print(struct gbp_printer_status_t *printer_status_ptr);


void serialClock_ISR(void)
{
  int rx_bitState;

  uint8_t rx_byte;
  bool new_rx_byte;

  uint8_t tx_byte;
  bool new_tx_byte;

  /***************** BYTE PARSER ***********************/

  new_rx_byte = gbp_rx_tx_byte_update(&(gbp_printer.gbp_rx_tx_byte_buffer), &rx_byte,  &rx_bitState);

  if (new_rx_byte)
  {
    // Update Timeout State
    if (gbp_printer.gbp_rx_tx_byte_buffer.syncronised)
    { // Push forward timeout since a byte was received.
      gbp_printer.uptime_til_timeout_ms = millis() + GBP_PACKET_TIMEOUT_MS;
    }
  }

  /***************** TX/RX BIT->BYTE DIAGNOSTICS ***********************/

#if 0 // Bit Scanning
  if ( NO_NEW_BIT != rx_bitState )
  { // New bit detected
    Serial.print(rx_bitState);
  }
#endif

#if 0 // Byte Scanning
  if (new_rx_byte)
  {
    // Diagnostics
    Serial.print(gbp_printer.gbp_packet_parser.parse_state,HEX);
    Serial.print(":");
    Serial.println(rx_byte,HEX);
  }
#endif

  /***************** PACKET PARSER ***********************/

  gbp_parse_message_update(
                            &(gbp_printer.gbp_packet_parser), 
                            &gbp_printer.packet_ready_flag,
                            &(gbp_printer.gbp_packet), 
                            &(gbp_printer),
                            new_rx_byte, rx_byte,
                            &new_tx_byte, &tx_byte
                          );

  /***************** TX BYTE SET ***********************/

  // Byte to be tranmitted to the gameboy received
  if (new_tx_byte)
  {
    gbp_rx_tx_byte_set(&(gbp_printer.gbp_rx_tx_byte_buffer), tx_byte);
  }

  /***************** TX BYTE SET DIAGNOSTICS ***********************/

#if 0 // warning: effects timing
  if (new_tx_byte)
  {
    Serial.print("T");
    Serial.println(new_tx_byte,HEX);
  }
#endif

}

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
  gbp_printer_init(&gbp_printer);

  // attach ISR
  attachInterrupt( digitalPinToInterrupt(SC_PIN), serialClock_ISR, CHANGE);  // attach interrupt handler


} // setup()

void loop() {  

  // Packet received from gameboy
  if (gbp_printer.packet_ready_flag)
  {
#if 1
    Serial.println("#");
    switch (gbp_printer.gbp_packet.command)
    {
      case GBP_COMMAND_INIT:
        fprintf(&serialout, "INIT");
        break;
      case GBP_COMMAND_DATA:
        fprintf(&serialout, "DATA");
        break;
      case GBP_COMMAND_PRINT:
        fprintf(&serialout, "PRNT");
        break;
      case GBP_COMMAND_INQUIRY:
        fprintf(&serialout, "INQY");
        break;
      default:
        fprintf(&serialout, "UKNO");
    }
    fprintf(&serialout, ": %u,%u | %u,%u | %u\n", 
               gbp_printer.gbp_packet.data_length,
               gbp_printer.gbp_packet.checksum,
               gbp_printer.gbp_packet.acknowledgement,
               gbp_printer.gbp_packet.printer_status,
               gbp_printer.gbp_printer_status.checksum_error
            );
#endif
    gbp_rx_tx_byte_reset(&(gbp_printer.gbp_rx_tx_byte_buffer));
    gbp_parse_message_reset(&(gbp_printer.gbp_packet_parser));

    gbp_printer.packet_ready_flag = false; // Packet Processed
  }


  // Trigger Timeout and reset the printer if byte stopped being recieved.
  if ( (gbp_printer.gbp_rx_tx_byte_buffer.syncronised) )
  {
    if ( (0 != gbp_printer.uptime_til_timeout_ms) && (millis() > gbp_printer.uptime_til_timeout_ms) )
    { // reset printer byte and packet processor
      Serial.println("ERROR: Timed Out");
      gbp_rx_tx_byte_reset(&(gbp_printer.gbp_rx_tx_byte_buffer));
      gbp_parse_message_reset(&(gbp_printer.gbp_packet_parser));
    }
  }
  else
  { // During scanning phase timeout is not required.
    gbp_printer.uptime_til_timeout_ms = 0;
  }


  // Diagnostics Console
  while (Serial.available() > 0) 
  {
    switch (Serial.read())
    {
      case '?':
        Serial.print("parse_state:");
        Serial.println(gbp_printer.gbp_packet_parser.parse_state,HEX);
        Serial.print("data_index:");
        Serial.println(gbp_printer.gbp_packet_parser.data_index,HEX);
        Serial.print("next timeout in:");
        Serial.println(gbp_printer.uptime_til_timeout_ms);
        Serial.print("milis():");
        Serial.println(millis());
        Serial.print("synced:");
        Serial.println(gbp_printer.gbp_rx_tx_byte_buffer.syncronised);
        break;

      case '!':
        gbp_status_byte_print(&(gbp_printer.gbp_printer_status));
        break;
    }
  };


} // loop()




static void gbp_status_byte_print(struct gbp_printer_status_t *printer_status_ptr)
{ // This is returns a gameboy printer status byte (Based on description in http://gbdev.gg8.se/wiki/articles/Gameboy_Printer )  
  fprintf(&serialout, "Printer Status: %s,%s,%s,%s|%s,%s,%s,%s\n", 
   ( printer_status_ptr->too_hot_or_cold     ?"Too Hot/Cold":"_"),
   ( printer_status_ptr->paper_jam           ?"Paper Jam":"_"),
   ( printer_status_ptr->timeout             ?"Timeout":"_"),
   ( printer_status_ptr->battery_low         ?"Batt Low":"_"),
   ( printer_status_ptr->ready_to_print      ?"Ready To Print":"_"),
   ( printer_status_ptr->print_reqested      ?"Print Reqested":"_"),
   ( printer_status_ptr->currently_printing  ?"Currently Printing":"_"),
   ( printer_status_ptr->checksum_error      ?"Checksum Error":"_")
  );
}


#ifdef __cplusplus
}
#endif
