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


enum gbp_parse_state 
{ // Indicates the stage of the parsing processing
    IDLING,
    GBP_MAGIC_BYTE_1, // 0x88
    GBP_MAGIC_BYTE_2, // 0x33
    GBP_COMMAND_TYPE,
    GBP_COMPRESSION_TYPE,
    GBP_PACKET_LENGTH,
    GBP_CHECKSUM,
    GBP_ACKNOWLEGEMENT,
    GBP_STATUS
};








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

static bool gbp_get_byte(uint8_t *byte_output, uint8_t *byte_buffer, int bit_input, int *bit_received)
{ // This returns a byte if avaiable
  *bit_received++;
  if (*bit_received < 8)
  { // Byte Ready
    gbp_scan_byte(byte_output, byte_buffer, bit_input);  
    return true;
  }
  return false;  
}

static bool gbp_parse_message(int data_bit)
{ // Returns true when a message is fully parsed (This is timing critical. Avoid serial prints)
  // Byte Wise Buffer State
  static uint8_t byte_output = 0;
  static uint8_t byte_buffer = 0x00;
  static int bit_received = 0;

  gbp_scan_byte(&byte_output, &byte_buffer, data_bit);

  if ( byte_output == 0x88 )
  {
    Serial.print("0x88 detected\n");
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

