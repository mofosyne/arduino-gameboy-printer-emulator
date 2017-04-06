/*************************************************************************
 * 
 * GAMEBOY PRINTER EMULATION PROJECT
 * 
 * Creation Date: 2017-4-6
 * PURPOSE: To capture gameboy printer images without a gameboy printer 
 * AUTHOR: Brian Khuu
 * 
 */

#define NO_NEW_BIT -1

#define SC_PIN 13 // Serial Clock   (INPUT)
#define SD_PIN 0 // Serial Data    (?)
#define SO_PIN 12 // Serial OUTPUT  (INPUT)
#define SI_PIN 11 // Serial INPUT   (OUTPUT)


typedef enum gbp_parse_state_t
{ // Indicates the stage of the parsing processing
    GBP_PARSE_STATE_IDLING,
    GBP_PARSE_STATE_MAGIC_BYTE,
    GBP_PARSE_STATE_COMMAND,
    GBP_PARSE_STATE_COMPRESSION,
    GBP_PARSE_STATE_PACKET_LENGTH_LOW,
    GBP_PARSE_STATE_PACKET_LENGTH_HIGH,
    GBP_PARSE_STATE_CHECKSUM_LOW,
    GBP_PARSE_STATE_CHECKSUM_HIGH,
    /** This could be sent seperately perhaps **/
    //GBP_PARSE_STATE_ACKNOWLEGEMENT,
    //GBP_PARSE_STATE_STATUS,
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

/**************************************************************
 * 
 *  GAMEBOY PRINTER PROTOCOL
 * 
 **************************************************************/

#define GBP_MAGIC_BYTE_VALUE_1 0x88
#define GBP_MAGIC_BYTE_VALUE_2 0x33

/**************************************************************
 * 
 *  GAMEBOY PRINTER FUNCTIONS
 * 
 **************************************************************/
/*
 * Checks for any new incoming data bit from the gameboy. 
 *  Every bit is received on the rising edge of the serial clock.
 * 
 * Returns:
 *         * NO_NEW_BIT if no new bit detected.
 *         * 1 : High Bit Received
 *         * 0 : Low Bit Received
 */
static int gbp_get_bit()
{ // Checks for any new bits
  static bool first_start = true;
  static int serial_clock_state_prev = 0;
  int bit_status = NO_NEW_BIT;

  int serial_clock_state  = digitalRead(SC_PIN);
  int serial_data_state   = digitalRead(SD_PIN);
  int serial_out_state    = digitalRead(SO_PIN);
  int serial_input_state  = digitalRead(SI_PIN);
  
  if (first_start)
  { // Just need to record initial state
    first_start = false;
  }
  else
  { // Check for new bits
    // Clock Edge Detection
    if (serial_clock_state != serial_clock_state_prev)
    { // Clock Pin Transition Detected
      if (serial_clock_state)
      { // Rising Edge
        // fprintf(&lcdout, ">> SD:%d SO:%d SI:%d\n", serial_data_state, serial_out_state, serial_input_state ) ;
        // Serial.print(serial_out_state);
        if(serial_out_state)
        { // High Bit Recieved
          bit_status = 1;
        }
        else
        { // Low Bit Recieved
          bit_status = 0;
        }
      }
    }
  }

  // Save current state for next edge detection
  serial_clock_state_prev = serial_clock_state;
  return bit_status;
}


static bool gbp_scan_byte(uint8_t *byte_output, uint8_t *byte_buffer, int bit_input)
{ // During Idling, scan for the initial magic byte
  *byte_buffer <<= 1;

  if(bit_input)
  { // Insert a `1` else leave as `0`
    *byte_buffer |= 0b0001;
  }

  *byte_output = *byte_buffer;

  return true;  // There is always a byte avaiable to scan
}

static bool gbp_get_byte(uint8_t *byte_output, uint8_t *byte_buffer, int bit_input, int *bit_count_ptr)
{ // This returns a byte if avaiable
  gbp_scan_byte(byte_output, byte_buffer, bit_input);
  
  (*bit_count_ptr) = (*bit_count_ptr) + 1;
  if (*bit_count_ptr >= 8)
  { // Byte Ready
    return true;
  }
  return false;  
}

static gbp_parse_state_t gbp_parse_message(int data_bit)
{ // Returns true when a message is fully parsed (This is timing critical. Avoid serial prints)
  static gbp_parse_state_t parse_state = GBP_PARSE_STATE_IDLING;

  // Byte Wise Buffer State
  static bool clear_byte_buffer_flag = true;
  static uint8_t byte_output = 0;
  static uint8_t byte_buffer = 0x00;
  static int bit_received = 0;

  // Packet Field
  static uint8_t command;
  static uint8_t compression;
  static uint16_t packet_data_length;
  static uint16_t checksum;

  static uint16_t packet_data_length_low;
  static uint16_t packet_data_length_high;
  static uint16_t checksum_low;
  static uint16_t checksum_high;

#if 1
  if (clear_byte_buffer_flag)
  {
    clear_byte_buffer_flag = false;
    byte_output = 0;
    byte_buffer = 0x00;
    bit_received = 0;
  }
#endif

  switch (parse_state)
  {
    /********************* MAGIC BYTES **************************/
    case GBP_PARSE_STATE_IDLING:
    {
      // Scan for magic byte
      gbp_scan_byte(&byte_output, &byte_buffer, data_bit);
      if ( byte_output == GBP_MAGIC_BYTE_VALUE_1 )
      { // First Magic Byte Found
        parse_state = GBP_PARSE_STATE_MAGIC_BYTE;
        clear_byte_buffer_flag = true;
      }
    } break;
    case GBP_PARSE_STATE_MAGIC_BYTE:
    {
      // Scan for magic byte
      gbp_scan_byte(&byte_output, &byte_buffer, data_bit);  // Should probbly be a proper bit by bit count. Too lazy
      if ( byte_output == GBP_MAGIC_BYTE_VALUE_2 )
      { // Second Magic Byte Found
        parse_state = GBP_PARSE_STATE_COMMAND;
        clear_byte_buffer_flag = true;
        //Serial.print("magic_byte_detected\n"); // Note: will cause bit miss
      }
    } break;
    /********************* COMMAND **************************/
    case GBP_PARSE_STATE_COMMAND:
    {
      if (gbp_get_byte(&byte_output, &byte_buffer, data_bit, &bit_received))
      { // Command Byte Received
        command = byte_output;
        parse_state = GBP_PARSE_STATE_COMPRESSION;
        clear_byte_buffer_flag = true;
      }      
    } break;
    /********************* COMPRESSION **************************/
    case GBP_PARSE_STATE_COMPRESSION:
    {
      if (gbp_get_byte(&byte_output, &byte_buffer, data_bit, &bit_received))
      { // Command Byte Received
        compression = byte_output;
        parse_state = GBP_PARSE_STATE_PACKET_LENGTH_LOW;
        clear_byte_buffer_flag = true;
      }
    } break;
    /********************* PACKET_LENGTH **************************/
    case GBP_PARSE_STATE_PACKET_LENGTH_LOW:
    {
      if (gbp_get_byte(&byte_output, &byte_buffer, data_bit, &bit_received))
      { // Command Byte Received
        packet_data_length_low = byte_output;
        //packet_data_length = byte_output & 0x0F;
        parse_state = GBP_PARSE_STATE_PACKET_LENGTH_HIGH;
        clear_byte_buffer_flag = true;
      }
    } break;
    case GBP_PARSE_STATE_PACKET_LENGTH_HIGH:
    {
      if (gbp_get_byte(&byte_output, &byte_buffer, data_bit, &bit_received))
      { // Command Byte Received
        packet_data_length_high = byte_output;
        //packet_data_length = (byte_output << 4) & 0xF0;
        parse_state = GBP_PARSE_STATE_CHECKSUM_LOW;
        clear_byte_buffer_flag = true;
      }
    } break;
    /********************* CHECKSUM **************************/
    case GBP_PARSE_STATE_CHECKSUM_LOW:
    {
      if (gbp_get_byte(&byte_output, &byte_buffer, data_bit, &bit_received))
      { // Command Byte Received
        checksum_low = byte_output;
        //checksum = byte_output & 0x0F;
        parse_state = GBP_PARSE_STATE_CHECKSUM_HIGH;
        clear_byte_buffer_flag = true;
      }
    } break;
    case GBP_PARSE_STATE_CHECKSUM_HIGH:
    {
      if (gbp_get_byte(&byte_output, &byte_buffer, data_bit, &bit_received))
      { // Command Byte Received
        checksum_high = byte_output;
        //checksum = (byte_output << 4) & 0xF0;
        parse_state = GBP_PARSE_STATE_DIAGNOSTICS;
        clear_byte_buffer_flag = true;
      }
    } break;
    /********************* DIAGNOSTICS **************************/
    case GBP_PARSE_STATE_DIAGNOSTICS:
    { // print out the headers
      Serial.println("\n commandbyte: "); // Note: will cause bit miss
      Serial.println(command,HEX);
      Serial.println("\n compression: "); // Note: will cause bit miss
      Serial.println(compression,HEX);
      Serial.println("\n packet_data_length: "); // Note: will cause bit miss
      Serial.println(packet_data_length_low,HEX);
      Serial.println(packet_data_length_high,HEX);
      packet_data_length = ( (packet_data_length_high << 8) & 0xFF00 ) | ( (packet_data_length_low << 0) & 0x00FF );
      Serial.println(packet_data_length,HEX);
      //Serial.println(packet_data_length,HEX);
      Serial.println("\n checksum: "); // Note: will cause bit miss
      Serial.println(checksum_low,HEX);
      Serial.println(checksum_high,HEX);
      checksum = ( (checksum_high << 8) & 0xFF00 ) | ( (checksum_low << 0) & 0x00FF );
      Serial.println(checksum,HEX);

      parse_state = GBP_PARSE_STATE_IDLING;
    } break;
    default:
    { // Unknown state. Revert to idle state
      parse_state = GBP_PARSE_STATE_IDLING;
    } break;
  }
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
  pinMode(SI_PIN, INPUT);

  // Default output value
  //digitalWrite(SI_PIN, LOW);

  Serial.print("GAMEBOY PRINTER EMULATION PROJECT\n");
}

void loop() {  
  int data_bit;

  // Get next Bit
  data_bit = gbp_get_bit();

#if 1 // Bit Scanning
  if ( NO_NEW_BIT != data_bit )
  { // New bit detected
    Serial.print(data_bit);
  }
#endif

  if ( NO_NEW_BIT != data_bit )
  {
    gbp_parse_message(data_bit);
    // Parse Message
  
    /// Scanning for Magic Header


  }

}

