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

#include "gameboy_printer_protocol.h"
#include "gpb_serial_io.h"
#include "gbp_pkt.h"

// GBP_FEATURE_PACKET_CAPTURE_MODE
#define GBP_FEATURE_PARSE_PACKET_MODE

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


#define GBP_PACKET_TIMEOUT_MS 100 // ms timeout period to wait for next byte in a packet

uint8_t gbp_buffer[GPB_BUFFER_SIZE] = {0};

/**************************************************************
 **************************************************************/

#ifdef GBP_FEATURE_PARSE_PACKET_MODE
gbp_pktBuff_t gbp_pktBuff = {GBP_REC_NONE,0};
#endif

#ifdef ESP8266
void ICACHE_RAM_ATTR serialClock_ISR(void)
#else
void serialClock_ISR(void)
#endif
{
  // Serial Clock (1 = Rising Edge) (0 = Falling Edge); Master Output Slave Input (This device is slave)
#ifdef GBP_FEATURE_USING_RISING_CLOCK_ONLY_ISR
  const bool txBit = gpb_serial_io_OnRising_ISR(digitalRead(GBP_SO_PIN));
#else
  const bool txBit = gpb_serial_io_OnChange_ISR(digitalRead(GBP_SC_PIN), digitalRead(GBP_SO_PIN));
#endif
  digitalWrite(GBP_SI_PIN, txBit ? HIGH : LOW);
}

void setup(void)
{
  // Config Serial
  // Has to be fast or it will not transfer the image fast enough to the computer
  Serial.begin(115200);

  /* Pins from gameboy link cable */
  pinMode(GBP_SC_PIN, INPUT);
  pinMode(GBP_SO_PIN, INPUT);
  pinMode(GBP_SI_PIN, OUTPUT);

  /* Default link serial out pin state */
  digitalWrite(GBP_SI_PIN, LOW);

  /* LED Indicator */
  pinMode(LED_STATUS_PIN, OUTPUT);
  digitalWrite(LED_STATUS_PIN, LOW);

  /* Welcome Message */
#ifdef GBP_FEATURE_PACKET_CAPTURE_MODE
  Serial.print("/* GAMEBOY PRINTER EMULATION PROJECT (Packet Capture Mode) */\n");
  Serial.print("/* By Brian Khuu (2020) */\n");
  Serial.print("// Note: Each byte is from each GBP packet is from the gameboy\n");
  Serial.print("//       except for the last two bytes which is from the printer\n");
#endif

#ifdef GBP_FEATURE_PARSE_PACKET_MODE
  Serial.print("# GAMEBOY PRINTER EMULATION PROJECT\n");
  Serial.print("# By Brian Khuu (2020)\n");
  gbp_pkt_init(&gbp_pktBuff);
#endif

  /* Setup */
  gpb_serial_io_init(sizeof(gbp_buffer), gbp_buffer);

  /* attach ISR */
#ifdef GBP_FEATURE_USING_RISING_CLOCK_ONLY_ISR
  attachInterrupt( digitalPinToInterrupt(GBP_SC_PIN), serialClock_ISR, RISING);  // attach interrupt handler
#else
  attachInterrupt( digitalPinToInterrupt(GBP_SC_PIN), serialClock_ISR, CHANGE);  // attach interrupt handler
#endif
} // setup()

char *gbpCommand_toStr(int val)
{
  switch (val)
  {
    case GBP_COMMAND_INIT    : return "INIT";
    case GBP_COMMAND_PRINT   : return "PRNT";
    case GBP_COMMAND_DATA    : return "DATA";
    case GBP_COMMAND_BREAK   : return "BREK";
    case GBP_COMMAND_INQUIRY : return "INQY";
    default: return "?";
  }
}

void loop()
{
#ifdef GBP_FEATURE_PACKET_CAPTURE_MODE
  /* tiles received */
  static uint32_t byteTotal = 0;
  static uint32_t pktTotalCount = 0;
  static uint32_t pktByteIndex = 0;
  static uint16_t pktDataLength = 0;
  const size_t dataBuffCount = gbp_serial_io_dataBuff_getByteCount();
  if (
      ((pktByteIndex != 0)&&(dataBuffCount>0))||
      ((pktByteIndex == 0)&&(dataBuffCount>=6))
      )
  {
    const char nibbleToCharLUT[] = "0123456789ABCDEF";
    uint8_t data_8bit = 0;
    for (int i = 0 ; i < dataBuffCount ; i++)
    { // Display the data payload encoded in hex
      // Start of a new packet
      if (pktByteIndex == 0)
      {
        pktDataLength = gbp_serial_io_dataBuff_getByte_Peek(4);
        pktDataLength |= (gbp_serial_io_dataBuff_getByte_Peek(5)<<8)&0xFF00;
        Serial.print("/* ");
        Serial.print(pktTotalCount);
        Serial.print(" : ");
        Serial.print(gbpCommand_toStr(gbp_serial_io_dataBuff_getByte_Peek(2)));
        Serial.print(" */\n");
        digitalWrite(LED_STATUS_PIN, HIGH);
      }
      // Print Hex Byte
      data_8bit = gbp_serial_io_dataBuff_getByte();
      Serial.print((char)'0');
      Serial.print((char)'x');
      Serial.print((char)nibbleToCharLUT[(data_8bit>>4)&0xF]);
      Serial.print((char)nibbleToCharLUT[(data_8bit>>0)&0xF]);
      Serial.print((char)',');
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
#endif


#ifdef GBP_FEATURE_PARSE_PACKET_MODE
  const char nibbleToCharLUT[] = "0123456789ABCDEF";
  for (int i = 0 ; i < gbp_serial_io_dataBuff_getByteCount() ; i++)
  {
    if (gbp_pkt_processByte((const uint8_t) gbp_serial_io_dataBuff_getByte(), &gbp_pktBuff))
    {
      switch (gbp_pktBuff.received)
      {
        case GBP_REC_GOT_DATA_HEADER :
        case GBP_REC_GOT_PACKET      :
        {
          Serial.print((char)'!');
          Serial.print((char)'{');
          Serial.print("\"command\":\"");
          Serial.print(gbpCommand_toStr(gbp_pktBuff.command));
          Serial.print("\"");
          if (gbp_pktBuff.command == GBP_COMMAND_INQUIRY)
          {
            // !{"command":"INQY","status":{"lowbatt":0,"jam":0,"err":0,"pkterr":0,"unproc":1,"full":0,"bsy":0,"chk_err":0}}
            Serial.print(", \"status\":{");
            Serial.print("\"LowBat\":");
            Serial.print(gpb_status_bit_getbit_low_battery(gbp_pktBuff.status)      ? '1' : '0');
            Serial.print(",\"ER2\":");
            Serial.print(gpb_status_bit_getbit_other_error(gbp_pktBuff.status)      ? '1' : '0');
            Serial.print(",\"ER1\":");
            Serial.print(gpb_status_bit_getbit_paper_jam(gbp_pktBuff.status)        ? '1' : '0');
            Serial.print(",\"ER0\":");
            Serial.print(gpb_status_bit_getbit_packet_error(gbp_pktBuff.status)     ? '1' : '0');
            Serial.print(",\"Untran\":");
            Serial.print(gpb_status_bit_getbit_unprocessed_data(gbp_pktBuff.status) ? '1' : '0');
            Serial.print(",\"Full\":");
            Serial.print(gpb_status_bit_getbit_print_buffer_full(gbp_pktBuff.status)? '1' : '0');
            Serial.print(",\"Busy\":");
            Serial.print(gpb_status_bit_getbit_printer_busy(gbp_pktBuff.status)     ? '1' : '0');
            Serial.print(",\"Sum\":");
            Serial.print(gpb_status_bit_getbit_checksum_error(gbp_pktBuff.status)   ? '1' : '0');
            Serial.print((char)'}');
          }
          if (gbp_pktBuff.command == GBP_COMMAND_PRINT)
          {
            //!{"command":"PRNT","sheets":1,"margin_upper":1,"margin_lower":3,"pallet":228,"density":64 }
            Serial.print(", \"sheets\":");
            Serial.print(gbp_pkt_printInstruction_num_of_sheets(&gbp_pktBuff));
            Serial.print(", \"margin_upper\":");
            Serial.print(gbp_pkt_printInstruction_num_of_linefeed_before_print(&gbp_pktBuff));
            Serial.print(", \"margin_lower\":");
            Serial.print(gbp_pkt_printInstruction_num_of_linefeed_after_print(&gbp_pktBuff));
            Serial.print(", \"pallet\":");
            Serial.print(gbp_pkt_printInstruction_palette_value(&gbp_pktBuff));
            Serial.print(", \"density\":");
            Serial.print(gbp_pkt_printInstruction_print_density(&gbp_pktBuff));
          }
          if (gbp_pktBuff.command == GBP_COMMAND_DATA)
          {
            Serial.print(", \"compressed\":");
            Serial.print(gbp_pktBuff.compression);
            Serial.print(", \"more\":");
            Serial.print((gbp_pktBuff.dataLength != 0)?'1':'0');
          }
          Serial.print((char)'}');
          Serial.print("\r\n");
        } break;
        case GBP_REC_GOT_DATA_TILE   :
        {
          for (int i = 0 ; i < gbp_pktBuff.payloadBuffSize ; i++)
          {
            const uint8_t data_8bit = gbp_pktBuff.payloadBuff[i];
            Serial.print((char)nibbleToCharLUT[(data_8bit>>4)&0xF]);
            Serial.print((char)nibbleToCharLUT[(data_8bit>>0)&0xF]);
            Serial.print((char)' ');
          }
          Serial.print("\r\n");
        } break;
        default: break;
      }
    }
  }
#endif


#if 0
  /* tiles received */
  if (gbp_serial_io_dataBuff_getByteCount() >= 16)
  {
    for (uint16_t i = 0 ; i < 16 ; i++)
    { // Display the data payload encoded in hex
      const uint8_t data_8bit = gbp_serial_io_dataBuff_getByte();
      const char nibbleToCharLUT[] = "0123456789ABCDEF";
      Serial.print((char)nibbleToCharLUT[(data_8bit>>4)&0xF]);
      Serial.print((char)nibbleToCharLUT[(data_8bit>>0)&0xF]);
      Serial.print(((i+1)%16 == 0)?"\n":" "); ///< Insert Newline Periodically
    }
  }
#endif


  // Trigger Timeout and reset the printer if byte stopped being received.
  {
    static uint32_t last_millis = 0;
    uint32_t curr_millis = millis();
    if (curr_millis > last_millis)
    {
      uint32_t elapsed_ms = curr_millis - last_millis;
      if (gbp_serial_io_timeout_handler(elapsed_ms))
      {
#if defined(GBP_FEATURE_PARSE_PACKET_MODE)
        Serial.println("\n\n# Timed Out\n\n");
#elif defined(GBP_FEATURE_PACKET_CAPTURE_MODE)
        Serial.println("\n\n// Timed Out\n\n");
#else
        Serial.println("\n\n// Timed Out\n\n");
#endif
        digitalWrite(LED_STATUS_PIN, LOW);
      }
    }
    last_millis = curr_millis;
  }

#if 0
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
        gbp_status_byte_print_as_json_fields(&(gbp_printer.gbp_printer_status));
        Serial.print("\n");
        break;
    }
  };
#endif
} // loop()
