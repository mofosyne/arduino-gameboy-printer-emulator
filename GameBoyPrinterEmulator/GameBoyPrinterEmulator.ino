/*************************************************************************
 *
 * GAMEBOY PRINTER EMULATION PROJECT V3.2.1 (Arduino)
 * Copyright (C) 2022 Brian Khuu
 *
 * PURPOSE: To capture gameboy printer images without a gameboy printer
 *          via the arduino platform. (Tested on the arduino nano)
 *          This version is to investigate gameboy behaviour.
 *          This was originally started on 2017-4-6 but updated on 2020-08-16
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
 *
 */

// See /WEBUSB.md for details
#if USB_VERSION == 0x210
#include <WebUSB.h>
WebUSB WebUSBSerial(1, "herrzatacke.github.io/gb-printer-web/#/webusb");
#define Serial WebUSBSerial
#endif

#define GAME_BOY_PRINTER_MODE      true   // to use with https://github.com/Mraulio/GBCamera-Android-Manager and https://github.com/Raphael-Boichot/PC-to-Game-Boy-Printer-interface
#define GBP_OUTPUT_RAW_PACKETS     true   // by default, packets are parsed. if enabled, output will change to raw data packets for parsing and decompressing later
#define GBP_USE_PARSE_DECOMPRESSOR false  // embedded decompressor can be enabled for use with parse mode but it requires fast hardware (SAMD21, SAMD51, ESP8266, ESP32)

#include <stdint.h>  // uint8_t
#include <stddef.h>  // size_t

#include "gameboy_printer_protocol.h"
#include "gbp_serial_io.h"

#if GBP_OUTPUT_RAW_PACKETS
#define GBP_FEATURE_PACKET_CAPTURE_MODE
#else
#define GBP_FEATURE_PARSE_PACKET_MODE
#if GBP_USE_PARSE_DECOMPRESSOR
#define GBP_FEATURE_PARSE_PACKET_USE_DECOMPRESSOR
#endif
#endif

#ifdef GBP_FEATURE_PARSE_PACKET_MODE
#include "gbp_pkt.h"
#endif




/* Gameboy Link Cable Mapping to Arduino Pin */
// Note: Serial Clock Pin must be attached to an interrupt pin of the arduino
//  ___________
// |  6  4  2  |
//  \_5__3__1_/   (at cable)
//

// clang-format off
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
// clang-format on

/*******************************************************************************
*******************************************************************************/

// Dev Note: Gamboy camera sends data payload of 640 bytes usually

#ifdef GBP_FEATURE_PARSE_PACKET_MODE
#define GBP_BUFFER_SIZE 400
#else
#define GBP_BUFFER_SIZE 650
#endif

/* Serial IO */
// This circular buffer contains a stream of raw packets from the gameboy
uint8_t gbp_serialIO_raw_buffer[GBP_BUFFER_SIZE] = { 0 };

#ifdef GBP_FEATURE_PARSE_PACKET_MODE
/* Packet Buffer */
gbp_pkt_t gbp_pktState                                 = { GBP_REC_NONE, 0 };
uint8_t gbp_pktbuff[GBP_PKT_PAYLOAD_BUFF_SIZE_IN_BYTE] = { 0 };
uint8_t gbp_pktbuffSize                                = 0;
#ifdef GBP_FEATURE_PARSE_PACKET_USE_DECOMPRESSOR
gbp_pkt_tileAcc_t tileBuff = { 0 };
#endif
#endif

#ifdef GBP_FEATURE_PACKET_CAPTURE_MODE
inline void gbp_packet_capture_loop();
#endif
#ifdef GBP_FEATURE_PARSE_PACKET_MODE
inline void gbp_parse_packet_loop();
#endif

/*******************************************************************************
  Utility Functions
*******************************************************************************/

const char *gbpCommand_toStr(int val)
{
  switch (val)
  {
    case GBP_COMMAND_INIT: return "INIT";
    case GBP_COMMAND_PRINT: return "PRNT";
    case GBP_COMMAND_DATA: return "DATA";
    case GBP_COMMAND_BREAK: return "BREK";
    case GBP_COMMAND_INQUIRY: return "INQY";
    default: return "?";
  }
}

/*******************************************************************************
  Interrupt Service Routine
*******************************************************************************/

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


/*******************************************************************************
  Main Setup and Loop
*******************************************************************************/

void setup(void)
{
  // Config Serial
  // Has to be fast or it will not transfer the image fast enough to the computer
  Serial.begin(115200);

  // Wait for Serial to be ready
  while (!Serial) { ; }

  Connect_to_printer();  //makes an attempt to switch in printer mode

  /* Pins from gameboy link cable */
  pinMode(GBP_SC_PIN, INPUT);
  pinMode(GBP_SO_PIN, INPUT);
  pinMode(GBP_SI_PIN, OUTPUT);

  /* Default link serial out pin state */
  digitalWrite(GBP_SI_PIN, LOW);

  /* LED Indicator */
  pinMode(LED_STATUS_PIN, OUTPUT);
  digitalWrite(LED_STATUS_PIN, LOW);

  /* Setup */
  gpb_serial_io_init(sizeof(gbp_serialIO_raw_buffer), gbp_serialIO_raw_buffer);

  /* Attach ISR */
#ifdef GBP_FEATURE_USING_RISING_CLOCK_ONLY_ISR
  attachInterrupt(digitalPinToInterrupt(GBP_SC_PIN), serialClock_ISR, RISING);  // attach interrupt handler
#else
  attachInterrupt(digitalPinToInterrupt(GBP_SC_PIN), serialClock_ISR, CHANGE);  // attach interrupt handler
#endif

  /* Packet Parser */
#ifdef GBP_FEATURE_PARSE_PACKET_MODE
  gbp_pkt_init(&gbp_pktState);
#endif

#define VERSION_STRING "V3.2.1 (Copyright (C) 2022 Brian Khuu)"

  /* Welcome Message */
#ifdef GBP_FEATURE_PACKET_CAPTURE_MODE
  Serial.println(F("// GAMEBOY PRINTER Packet Capture " VERSION_STRING));
  Serial.println(F("// Note: Each byte is from each GBP packet is from the gameboy"));
  Serial.println(F("//       except for the last two bytes which is from the printer"));
  Serial.println(F("// JS Raw Packet Decoder: https://mofosyne.github.io/arduino-gameboy-printer-emulator/GameBoyPrinterDecoderJS/gameboy_printer_js_raw_decoder.html"));
#endif
#ifdef GBP_FEATURE_PARSE_PACKET_MODE
  Serial.println(F("// GAMEBOY PRINTER Emulator " VERSION_STRING));
  Serial.println(F("// Note: Each hex encoded line is a gameboy tile"));
  Serial.println(F("// JS Decoder: https://mofosyne.github.io/arduino-gameboy-printer-emulator/GameBoyPrinterDecoderJS/gameboy_printer_js_decoder.html"));
#endif
  Serial.println(F("// --- GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007 ---"));
  Serial.println(F("// This program comes with ABSOLUTELY NO WARRANTY;"));
  Serial.println(F("// This is free software, and you are welcome to redistribute it"));
  Serial.println(F("// under certain conditions. Refer to LICENSE file for detail."));
  Serial.println(F("// ---"));

  Serial.flush();
}  // setup()

void loop()
{
  static uint16_t sioWaterline = 0;

#ifdef GBP_FEATURE_PACKET_CAPTURE_MODE
  gbp_packet_capture_loop();
#endif
#ifdef GBP_FEATURE_PARSE_PACKET_MODE
  gbp_parse_packet_loop();
#endif

  // Trigger Timeout and reset the printer if byte stopped being received.
  static uint32_t last_millis = 0;
  uint32_t curr_millis        = millis();
  if (curr_millis > last_millis)
  {
    uint32_t elapsed_ms = curr_millis - last_millis;
    if (gbp_serial_io_timeout_handler(elapsed_ms))
    {
      Serial.println("");
      Serial.print("// Completed ");
      Serial.print("(Memory Waterline: ");
      Serial.print(gbp_serial_io_dataBuff_waterline(false));
      Serial.print("B out of ");
      Serial.print(gbp_serial_io_dataBuff_max());
      Serial.println("B)");
      Serial.flush();
      digitalWrite(LED_STATUS_PIN, LOW);

#ifdef GBP_FEATURE_PARSE_PACKET_MODE
      gbp_pkt_reset(&gbp_pktState);
#ifdef GBP_FEATURE_PARSE_PACKET_USE_DECOMPRESSOR
      tileBuff.count = 0;
#endif
#endif
    }
  }
  last_millis = curr_millis;

  // Diagnostics Console
  while (Serial.available() > 0)
  {
    switch (Serial.read())
    {
      case '?':
        Serial.println("d=debug, ?=help");
        break;

      case 'd':
        Serial.print("waterline: ");
        Serial.print(gbp_serial_io_dataBuff_waterline(false));
        Serial.print("B out of ");
        Serial.print(gbp_serial_io_dataBuff_max());
        Serial.println("B");
        break;
    }
  };
}  // loop()

/******************************************************************************/

#ifdef GBP_FEATURE_PARSE_PACKET_MODE
inline void gbp_parse_packet_loop(void)
{
  const char nibbleToCharLUT[] = "0123456789ABCDEF";
  for (int i = 0; i < gbp_serial_io_dataBuff_getByteCount(); i++)
  {
    if (gbp_pkt_processByte(&gbp_pktState, (const uint8_t)gbp_serial_io_dataBuff_getByte(), gbp_pktbuff, &gbp_pktbuffSize, sizeof(gbp_pktbuff)))
    {
      if (gbp_pktState.received == GBP_REC_GOT_PACKET)
      {
        digitalWrite(LED_STATUS_PIN, HIGH);
        Serial.print((char)'{');
        Serial.print("\"command\":\"");
        Serial.print(gbpCommand_toStr(gbp_pktState.command));
        Serial.print("\"");
        if (gbp_pktState.command == GBP_COMMAND_INQUIRY)
        {
          // !{"command":"INQY","status":{"lowbatt":0,"jam":0,"err":0,"pkterr":0,"unproc":1,"full":0,"bsy":0,"chk_err":0}}
          Serial.print(", \"status\":{");
          Serial.print("\"LowBat\":");
          Serial.print(gpb_status_bit_getbit_low_battery(gbp_pktState.status) ? '1' : '0');
          Serial.print(",\"ER2\":");
          Serial.print(gpb_status_bit_getbit_other_error(gbp_pktState.status) ? '1' : '0');
          Serial.print(",\"ER1\":");
          Serial.print(gpb_status_bit_getbit_paper_jam(gbp_pktState.status) ? '1' : '0');
          Serial.print(",\"ER0\":");
          Serial.print(gpb_status_bit_getbit_packet_error(gbp_pktState.status) ? '1' : '0');
          Serial.print(",\"Untran\":");
          Serial.print(gpb_status_bit_getbit_unprocessed_data(gbp_pktState.status) ? '1' : '0');
          Serial.print(",\"Full\":");
          Serial.print(gpb_status_bit_getbit_print_buffer_full(gbp_pktState.status) ? '1' : '0');
          Serial.print(",\"Busy\":");
          Serial.print(gpb_status_bit_getbit_printer_busy(gbp_pktState.status) ? '1' : '0');
          Serial.print(",\"Sum\":");
          Serial.print(gpb_status_bit_getbit_checksum_error(gbp_pktState.status) ? '1' : '0');
          Serial.print((char)'}');
        }
        if (gbp_pktState.command == GBP_COMMAND_PRINT)
        {
          //!{"command":"PRNT","sheets":1,"margin_upper":1,"margin_lower":3,"pallet":228,"density":64 }
          Serial.print(", \"sheets\":");
          Serial.print(gbp_pkt_printInstruction_num_of_sheets(gbp_pktbuff));
          Serial.print(", \"margin_upper\":");
          Serial.print(gbp_pkt_printInstruction_num_of_linefeed_before_print(gbp_pktbuff));
          Serial.print(", \"margin_lower\":");
          Serial.print(gbp_pkt_printInstruction_num_of_linefeed_after_print(gbp_pktbuff));
          Serial.print(", \"pallet\":");
          Serial.print(gbp_pkt_printInstruction_palette_value(gbp_pktbuff));
          Serial.print(", \"density\":");
          Serial.print(gbp_pkt_printInstruction_print_density(gbp_pktbuff));
        }
        if (gbp_pktState.command == GBP_COMMAND_DATA)
        {
          //!{"command":"DATA", "compressed":0, "more":0}
#ifdef GBP_FEATURE_PARSE_PACKET_USE_DECOMPRESSOR
          Serial.print(", \"compressed\":0");  // Already decompressed by us, so no need to do so
#else
          Serial.print(", \"compressed\":");
          Serial.print(gbp_pktState.compression);
#endif
          Serial.print(", \"more\":");
          Serial.print((gbp_pktState.dataLength != 0) ? '1' : '0');
        }
        Serial.println((char)'}');
        Serial.flush();
      }
      else
      {
#ifdef GBP_FEATURE_PARSE_PACKET_USE_DECOMPRESSOR
        // Required for more complex games with compression support
        while (gbp_pkt_decompressor(&gbp_pktState, gbp_pktbuff, gbp_pktbuffSize, &tileBuff))
        {
          if (gbp_pkt_tileAccu_tileReadyCheck(&tileBuff))
          {
            // Got Tile
            for (int i = 0; i < GBP_TILE_SIZE_IN_BYTE; i++)
            {
              const uint8_t data_8bit = tileBuff.tile[i];
              if (i == GBP_TILE_SIZE_IN_BYTE - 1)
              {
                Serial.print((char)nibbleToCharLUT[(data_8bit >> 4) & 0xF]);
                Serial.println((char)nibbleToCharLUT[(data_8bit >> 0) & 0xF]);  // use println on last byte to reduce serial calls
              }
              else
              {
                Serial.print((char)nibbleToCharLUT[(data_8bit >> 4) & 0xF]);
                Serial.print((char)nibbleToCharLUT[(data_8bit >> 0) & 0xF]);
                Serial.print((char)' ');
              }
            }
            Serial.flush();
          }
        }
#else
        // Simplified support for gameboy camera only application
        // Dev Note: Good for checking if everything above decompressor is working
        if (gbp_pktbuffSize > 0)
        {
          // Got Tile
          for (int i = 0; i < gbp_pktbuffSize; i++)
          {
            const uint8_t data_8bit = gbp_pktbuff[i];
            if (i == gbp_pktbuffSize - 1)
            {
              Serial.print((char)nibbleToCharLUT[(data_8bit >> 4) & 0xF]);
              Serial.println((char)nibbleToCharLUT[(data_8bit >> 0) & 0xF]);  // use println on last byte to reduce serial calls
            }
            else
            {
              Serial.print((char)nibbleToCharLUT[(data_8bit >> 4) & 0xF]);
              Serial.print((char)nibbleToCharLUT[(data_8bit >> 0) & 0xF]);
              Serial.print((char)' ');
            }
          }
          Serial.flush();
        }
#endif
      }
    }
  }
}
#endif

#ifdef GBP_FEATURE_PACKET_CAPTURE_MODE
inline void gbp_packet_capture_loop()
{
  /* tiles received */
  static uint32_t byteTotal     = 0;
  static uint32_t pktTotalCount = 0;
  static uint32_t pktByteIndex  = 0;
  static uint16_t pktDataLength = 0;
  const size_t dataBuffCount    = gbp_serial_io_dataBuff_getByteCount();
  if (
    ((pktByteIndex != 0) && (dataBuffCount > 0)) || ((pktByteIndex == 0) && (dataBuffCount >= 6)))
  {
    const char nibbleToCharLUT[] = "0123456789ABCDEF";
    uint8_t data_8bit            = 0;
    for (int i = 0; i < dataBuffCount; i++)
    {  // Display the data payload encoded in hex
      // Start of a new packet
      if (pktByteIndex == 0)
      {
        pktDataLength = gbp_serial_io_dataBuff_getByte_Peek(4);
        pktDataLength |= (gbp_serial_io_dataBuff_getByte_Peek(5) << 8) & 0xFF00;
#if 0
        Serial.print("// ");
        Serial.print(pktTotalCount);
        Serial.print(" : ");
        Serial.println(gbpCommand_toStr(gbp_serial_io_dataBuff_getByte_Peek(2)));
#endif
        digitalWrite(LED_STATUS_PIN, HIGH);
      }
      // Print Hex Byte
      data_8bit = gbp_serial_io_dataBuff_getByte();
      Serial.print((char)nibbleToCharLUT[(data_8bit >> 4) & 0xF]);
      Serial.print((char)nibbleToCharLUT[(data_8bit >> 0) & 0xF]);
      // Splitting packets for convenience
      if ((pktByteIndex > 5) && (pktByteIndex >= (9 + pktDataLength)))
      {
        digitalWrite(LED_STATUS_PIN, LOW);
        Serial.println("");
        pktByteIndex = 0;
        pktTotalCount++;
      }
      else
      {
        Serial.print((char)' ');
        pktByteIndex++;  // Byte hex split counter
        byteTotal++;     // Byte total counter
      }
    }
    Serial.flush();
  }
}
#endif

void Connect_to_printer()
{
#if GAME_BOY_PRINTER_MODE  //Printer mode
  pinMode(GBP_SC_PIN, OUTPUT);
  pinMode(GBP_SO_PIN, INPUT_PULLUP);
  pinMode(GBP_SI_PIN, OUTPUT);
  pinMode(LED_STATUS_PIN, OUTPUT);
  const char INIT[] = { 0x88, 0x33, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 };  //INIT command
  uint8_t junk, status;
  for (uint8_t i = 0; i < 10; i++)
  {
    junk = (printing(INIT[i]));
    if (i == 8)
    {
      status = junk;
    }
  }
  if (status == 0X81)  //Printer connected !
  {
    digitalWrite(GBP_SC_PIN, HIGH);  //acts like a real Game Boy
    digitalWrite(GBP_SI_PIN, LOW);   //acts like a real Game Boy
    Serial.println(F("// A printer is connected to the serial cable !!!"));
    Serial.println(F("// GAME BOY PRINTER I/O INTERFACE Made By Raphaël BOICHOT, 2023"));
    Serial.println(F("// Use with the GBCamera-Android-Manager: https://github.com/Mraulio/GBCamera-Android-Manager"));
    Serial.println(F("// Or with the PC-to-Game-Boy-Printer-interface: https://github.com/Raphael-Boichot/PC-to-Game-Boy-Printer-interface"));
    delay(100);
    Serial.begin(9600);
    while (!Serial) { ; }
    while (Serial.available() > 0)
    {  //flush the buffer from any remaining data
      Serial.read();
    }
    digitalWrite(LED_STATUS_PIN, HIGH);  //LED ON = PRINTER INTERFACE mode
    while (true)
    {
      if (Serial.available() > 0)
      {
        Serial.write(printing(Serial.read()));
      }
    }
  }
#endif
}

#if GAME_BOY_PRINTER_MODE      //Printer mode
char printing(char byte_sent)  // this function prints bytes to the serial
{
  bool bit_sent, bit_read;
  char byte_read;
  for (int i = 0; i <= 7; i++)
  {
    bit_sent = bitRead(byte_sent, 7 - i);
    digitalWrite(GBP_SC_PIN, LOW);
    digitalWrite(GBP_SI_PIN, bit_sent);  //GBP_SI_PIN is SOUT for the printer
    digitalWrite(LED_STATUS_PIN, bit_sent);
    delayMicroseconds(30);  //double speed mode
    digitalWrite(GBP_SC_PIN, HIGH);
    bit_read = (digitalRead(GBP_SO_PIN));  //GBP_SO_PIN is SIN for the printer
    bitWrite(byte_read, 7 - i, bit_read);
    delayMicroseconds(30);  //double speed mode
  }
  delayMicroseconds(0);  //optionnal delay between bytes, may be less than 1490 µs
  //  Serial.println(byte_sent, HEX);
  //  Serial.println(byte_read, HEX);
  return byte_read;
}
#endif
