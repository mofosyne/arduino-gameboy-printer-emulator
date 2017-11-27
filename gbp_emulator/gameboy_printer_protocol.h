/*******************************************************************************
 *
 * GAMEBOY PRINTER EMULATION PROJECT
 *
 * Creation Date: 2017-11-27
 * PURPOSE: Standardised header file for gameboy printer
 * AUTHOR: Brian Khuu

  Source Documentation:
    DMG-06-4216-001-A
    Released 11/09/1999

 */

/*******************************************************************************
    GAMEBOY LINK SIGNALING
********************************************************************************

  - Clock Frequency: 8kHz (127.63 us)
  - Transmission Speed: 867 baud (1.153ms per 8bit symbol)
  - Between Symbol Period: 229.26 us

------------------------------------------------------------------------------*/


/*******************************************************************************
    GAMEBOY PRINTER PROTOCOL
*******************************************************************************/

// GameBoy Printer Packet Structure
/*
  | BYTE POS :    |     0     |     1     |     2     |      3      |     4     |     5     |  6 + X    | 6 + X + 1 | 6 + X + 2 | 6 + X + 3 | 6 + X + 4 |
  |---------------|-----------|-----------|-----------|-------------|-----------|-----------|-----------|-----------|-----------|-----------|-----------|
  | SIZE          |        2 Bytes        |  1 Byte   |   1 Byte    |  1 Bytes  |  1 Bytes  | Variable  |        2 Bytes        |  1 Bytes  |  1 Bytes  |
  | DESCRIPTION   |       SYNC_WORD       | COMMAND   | COMPRESSION |     DATA_LENGTH(X)    | Payload   |       CHECKSUM        |    ACK    |  STATUS   |
  | GB TO PRINTER |    0x88   |    0x33   | See Below | See Below   | Low Byte  | High Byte | See Below |       See Below       |    0x00   |    0x00   |
  | TO PRINTER    |    0x00   |    0x00   |    0x00   |   0x00      |    0x00   |    0x00   |    0x00   |    0x00   |    0x00   |    0x81   | See Below |

  * Header is the Command, Compression and Data Length
  * Command field may be either Initialize (0x01), Data (0x04), Print (0x02), or Inquiry (0x0F).
  * Compression field is a compression indicator. No compression (0x00)
  * Payload byte count size depends on the value of the `DATA_LENGTH` field.
  * Checksum is 2 bytes of data representing the sum of the header + all data in the data portion of the packet
  * Status byte is a bitfield byte indicating various status of the printer itself. (e.g. If it is still printing)
*/

/* Sync Word */
#define GBP_SYNC_WORD_0       0x88    // 0b10001000
#define GBP_SYNC_WORD_1       0x33    // 0b00110011
#define GBP_SYNC_WORD         0x8833  // 0b1000100000110011

/* Command Byte */
#define GBP_COMMAND_INIT      0x01    // 0b00000001 // Typically 10 bytes packet
#define GBP_COMMAND_PRINT     0x02    // 0b00000010 // Print instructions
#define GBP_COMMAND_DATA      0x04    // 0b00000100 // Typically 650 bytes packet (10byte header + 640byte image)
#define GBP_COMMAND_BREAK     0x08    // // Means to forcibly stops printing
#define GBP_COMMAND_INQUIRY   0x0F    // 0b00001111 // Always reports current status




/* Ack Byte */
// According to the GB programming manual. This is actually a status byte.
// Where the MSB is always 1 and the lower 7 bits is the device number.
// Pocket printer is always 1
#define GBP_ACK               0x81    // 0b10000001 // Recommended by "GB Printer interface specification"
#define GBP_ACK_2             0x80    // 0b10000000 // Used by esp8266-gameboy-printer


/* Print Instruction Payload (4 data bytes) */
#define GBP_PRINT_BYTE_PRINT_DENSITY    3
#define GBP_PRINT_BYTE_PALETTE_VALUE    2
#define GBP_PRINT_BYTE_NUM_OF_LINEFEED  1 // High Nibble 4 bits represents the number of feeds before printing.
                                          // Lower Nibble is 4 bits, representing the number of feeds after printing.
#define GBP_PRINT_BYTE_NUM_OF_SHEETS    0 // 0-255 (1 in the example). 0 means line feed only.


/* State Byte Bit Position */
#define GBP_STATUS_BIT_LOWBAT      7 // Battery Too Low
#define GBP_STATUS_BIT_ER2         6 // Other Error
#define GBP_STATUS_BIT_ER1         5 // Paper Jam
#define GBP_STATUS_BIT_ER0         4 // Packet Error (e.g. likely gameboy program failure)
#define GBP_STATUS_BIT_UNTRAN      3 // Unprocessed Data
#define GBP_STATUS_BIT_FULL        2 // Image Data Full
#define GBP_STATUS_BIT_BUSY        1 // Printer Busy
#define GBP_STATUS_BIT_SUM         0 // Checksum Error

/*******************************************************************************
  Structures
*******************************************************************************/

// Gameboy Printer Status Code Structure
typedef struct gbp_printer_status_t
{
  bool low_battery;
  bool paper_jam;
  bool other_error;
  bool packet_error;
  bool unprocessed_data;
  bool print_buffer_full;
  bool printer_busy;
  bool checksum_error;
} gbp_printer_status_t;



/*******************************************************************************
  UTILITIES
*******************************************************************************/

inline uint8_t gbp_status_byte(struct gbp_printer_status_t *printer_status_ptr)
{ // This is returns a gameboy printer status byte
  //(Based on description in http://gbdev.gg8.se/wiki/articles/Gameboy_Printer )

  /*        | BITFLAG NAME                                |BIT POS            */
  return
          ( ((printer_status_ptr->low_battery       )?1:0) <<  GBP_STATUS_BIT_LOWBAT )
        | ( ((printer_status_ptr->other_error       )?1:0) <<  GBP_STATUS_BIT_ER2    )
        | ( ((printer_status_ptr->paper_jam         )?1:0) <<  GBP_STATUS_BIT_ER1    )
        | ( ((printer_status_ptr->packet_error      )?1:0) <<  GBP_STATUS_BIT_ER0    )
        | ( ((printer_status_ptr->unprocessed_data  )?1:0) <<  GBP_STATUS_BIT_UNTRAN )
        | ( ((printer_status_ptr->print_buffer_full )?1:0) <<  GBP_STATUS_BIT_FULL   )
        | ( ((printer_status_ptr->printer_busy      )?1:0) <<  GBP_STATUS_BIT_BUSY   )
        | ( ((printer_status_ptr->checksum_error    )?1:0) <<  GBP_STATUS_BIT_SUM    )
        ;
}

/*******************************************************************************
  DEBUG UTILITIES
*******************************************************************************/

/* Uncomment to disable these functions */
// #define GAMEBOY_PRINTER_PROTO_DEBUG 0

#if (!defined(GAMEBOY_PRINTER_PROTO_DEBUG)) || (GAMEBOY_PRINTER_PROTO_DEBUG)

inline const char* gbp_command_byte_to_str(uint8_t value)
{
  switch(value)
  {
    case GBP_COMMAND_INIT   : return "INIT";
    case GBP_COMMAND_PRINT  : return "PRNT";
    case GBP_COMMAND_DATA   : return "DATA";
    case GBP_COMMAND_INQUIRY: return "INQY";
    default: return "?";
  }
}

#endif
