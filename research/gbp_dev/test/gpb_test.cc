#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "../gameboy_printer_protocol.h"
#include "gpb_serial_io.h"

uint8_t testVector[] = {
  #include "2020-08-02_GameboyPocketCameraJP.txt"
};

uint8_t testResponse[sizeof(testVector)+100] = {0};

uint8_t gbp_buffer[sizeof(testVector)+100] = {0};



/*******************************************************************************
 * Utilites
*******************************************************************************/

const char *gbpCommand_toStr(int val)
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


void dummy_OnChange_ISR(const bool GBP_SCLK, const bool GBP_SOUT)
{
  static bool txBit = false;

  if (GBP_SCLK)
  {
    // Gameboy will read printer's bit on rise
    static uint8_t byteMask = 1<<7;
    static size_t tx_byteCount = 0;
    // On rising, record test
    testResponse[tx_byteCount] |= (txBit ? 0xFF : 0x00) & byteMask;
    byteMask >>= 1;
    if (byteMask == 0)
    {
      byteMask = 1<<7;
      tx_byteCount++;
    }
  }

  txBit = gpb_pktIO_OnChange_ISR(GBP_SCLK, GBP_SOUT);
}


/*******************************************************************************
 * Main Test Routine
*******************************************************************************/
int main(void)
{
  printf("/* GBP Testing (Test Vector Size: %lu) */", (long unsigned) sizeof(testVector));
  // Prep
  gpb_pktIO_init(sizeof(gbp_buffer), gbp_buffer);

  // Process
  for (size_t i = 0 ; i < sizeof(testVector) ; i++)
  {
    const uint8_t byte = testVector[i];
    for (int bi = 7 ; bi >= 0 ; bi--)
    {
      // One clock cycle
      dummy_OnChange_ISR(0, (byte >> bi) & 0x01);
      dummy_OnChange_ISR(1, (byte >> bi) & 0x01);
    }
  }

  // Display
  while (gbp_dataBuff_getByteCount() > 0)
  {
    /* tiles received */
    static uint32_t byteTotal = 0;
    static uint16_t pktTotalCount = 0;
    static uint16_t pktByteIndex = 0;
    static uint16_t pktDataLength = 0;
    const size_t dataBuffCount = gbp_dataBuff_getByteCount();
    if (
        ((pktByteIndex != 0)&&(dataBuffCount>0))||
        ((pktByteIndex == 0)&&(dataBuffCount>=6))
        )
    {
      for (size_t i = 0 ; i < dataBuffCount ; i++)
      { // Display the data payload encoded in hex
        // Start of a new packet
        if (pktByteIndex == 0)
        {
          /*
            [ 00 ][ 01 ][ 02 ][ 03 ][ 04 ][ 05 ][ 5+X  ][ 5+X+1 ][ 5+X+2 ][ 5+X+3 ][ 5+X+4 ]
            [SYNC][SYNC][COMM][COMP][LEN0][LEN1][ DATA ][ CSUM0 ][ CSUM1 ][ DUMMY ][ DUMMY ]
          */
          uint8_t commandTypeByte = gbp_dataBuff_getByte_Peek(2);
          uint8_t dataLengthByte0 = gbp_dataBuff_getByte_Peek(4);
          uint8_t dataLengthByte1 = gbp_dataBuff_getByte_Peek(5);
          pktDataLength = 0;
          pktDataLength |= ((uint16_t)dataLengthByte0<<0)&0x00FF;
          pktDataLength |= ((uint16_t)dataLengthByte1<<8)&0xFF00;
          printf("\r\n/* %02X : %s */\r\n", pktTotalCount, gbpCommand_toStr(commandTypeByte));
        }
        // Print Hex Byte
        uint8_t rxByte = gbp_dataBuff_getByte();
        printf((pktByteIndex == (8+pktDataLength + 0))?"/*(*/ ":"");
        //printf("(%d)", byteTotal);
        if (pktByteIndex < (8+pktDataLength))
        {
          printf("0x%02X,", rxByte);
        }
        else
        {
          printf("0x%02X,", testResponse[byteTotal]);
        }
        if (pktByteIndex == (8+pktDataLength + 1))
        {
          printf("/*)*/");
          if (testResponse[byteTotal])
          {
            printf(" /* Printer Status: ");
            printf((testResponse[byteTotal] & GBP_STATUS_MASK_LOWBAT) ? "LOWBAT " : "");
            printf((testResponse[byteTotal] & GBP_STATUS_MASK_ER2   ) ? "ER2 "    : "");
            printf((testResponse[byteTotal] & GBP_STATUS_MASK_ER1   ) ? "ER1 "    : "");
            printf((testResponse[byteTotal] & GBP_STATUS_MASK_ER0   ) ? "ER0 "    : "");
            printf((testResponse[byteTotal] & GBP_STATUS_MASK_UNTRAN) ? "UNTRAN"  : "");
            printf((testResponse[byteTotal] & GBP_STATUS_MASK_FULL  ) ? "FULL "   : "");
            printf((testResponse[byteTotal] & GBP_STATUS_MASK_BUSY  ) ? "BUSY "   : "");
            printf((testResponse[byteTotal] & GBP_STATUS_MASK_SUM   ) ? "SUM "    : "");
            printf(" */");
          }
        }
        // Splitting packets for convenience
        if ((pktByteIndex>5)&&(pktByteIndex>=(9+pktDataLength)))
        {
          printf("\r\n");
          pktByteIndex = 0;
          pktTotalCount++;
        }
        else
        {
          printf( ((pktByteIndex+1)%16 == 0) ? "\n" : " "); ///< Insert Newline Periodically
          pktByteIndex++; // Byte hex split counter
        }
        byteTotal++; // Byte total counter
      }
    }
  }

#if 0 // Dev: testResponse Dump
  printf("\r\n/* testResponse Dump */\r\n");
  for (size_t i = 0 ; i < sizeof(testResponse) ; i++)
  {
    const int charperline = 16*3;
    printf("%02X", testResponse[i]);
    printf((i%(charperline) == (charperline-1))?"\r\n":" ");
  }
#endif

  printf("\r\n/* Done */\r\n");
}