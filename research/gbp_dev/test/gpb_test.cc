#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "gameboy_printer_protocol.h"
#include "gpb_serial_io.h"

#define FEATURE_PACKET_DUMP
#define FEATURE_PACKET_COMMUNICATION_SUMMARY

/*******************************************************************************
 * Test Vectors Variables
*******************************************************************************/
uint8_t testVector[] = {
  //#include "2020-08-02_GameboyPocketCameraJP.txt"
  #include "2020-08-02_PokemonSpeciallPicachuEdition.txt"
};

uint8_t testResponse[sizeof(testVector)+100] = {0};

typedef struct
{
  bool packetFound;
  size_t startOfPacketBytePos;
  size_t packetMismatch; // Byte parsed does not match test vector
  size_t statusMismatch; // Status byte received does not match test vector
  uint8_t printerID;
  uint8_t printerIDExpected;
  uint8_t status;
  uint8_t statusExpected;
  uint8_t command;
  uint16_t pktDataLength;
} packetResult_t;

packetResult_t testPacketResult[sizeof(testVector)] = {{0}};


/*******************************************************************************
 * Variable for gbp
*******************************************************************************/
uint8_t gbp_buffer[sizeof(testVector)+100] = {0};



/*******************************************************************************
 * Utilites
*******************************************************************************/

const char *gbpCommand_toStr(int val)
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


void dummy_ISR(const bool GBP_SCLK, const bool GBP_SOUT)
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

#ifdef GBP_FEATURE_USING_RISING_CLOCK_ONLY_ISR
  if (GBP_SCLK)
  {
    txBit = gpb_serial_io_OnRising_ISR(GBP_SOUT);
  }
#else
  txBit = gpb_serial_io_OnChange_ISR(GBP_SCLK, GBP_SOUT);
#endif

}


/*******************************************************************************
 * Main Test Routine
*******************************************************************************/
int main(void)
{
  printf("/* GBP Testing (Test Vector Size: %lu) */", (long unsigned) sizeof(testVector));
  // Prep
  gpb_serial_io_init(sizeof(gbp_buffer), gbp_buffer);

  // Process
  for (size_t i = 0 ; i < sizeof(testVector) ; i++)
  {
    const uint8_t byte = testVector[i];
    for (int bi = 7 ; bi >= 0 ; bi--)
    {
      // One clock cycle
      dummy_ISR(0, (byte >> bi) & 0x01);
      dummy_ISR(1, (byte >> bi) & 0x01);
    }
  }

  // Display
  while (gbp_serial_io_dataBuff_getByteCount() > 0)
  {
    /* tiles received */
    static uint32_t byteTotal = 0;
    static uint16_t pktTotalCount = 0;
    static uint16_t pktByteIndex = 0;
    static uint16_t pktDataLength = 0;
    const size_t dataBuffCount = gbp_serial_io_dataBuff_getByteCount();
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
          uint8_t commandTypeByte = gbp_serial_io_dataBuff_getByte_Peek(2);
          uint8_t dataLengthByte0 = gbp_serial_io_dataBuff_getByte_Peek(4);
          uint8_t dataLengthByte1 = gbp_serial_io_dataBuff_getByte_Peek(5);
          pktDataLength = 0;
          pktDataLength |= ((uint16_t)dataLengthByte0<<0)&0x00FF;
          pktDataLength |= ((uint16_t)dataLengthByte1<<8)&0xFF00;
#ifdef FEATURE_PACKET_DUMP
          printf("/* %3lu : %s (dLen:%lu) */\r\n",
            (unsigned long) pktTotalCount,
            (char *) gbpCommand_toStr(commandTypeByte),
            (unsigned long) pktDataLength
            );
#endif
          // For packetResult_t
          testPacketResult[pktTotalCount].command = commandTypeByte;
          testPacketResult[pktTotalCount].pktDataLength = pktDataLength;
        }
        // Print Hex Byte
        uint8_t rxByte = gbp_serial_io_dataBuff_getByte();
#ifdef FEATURE_PACKET_DUMP
        printf((pktByteIndex == (8+pktDataLength + 0))?"/*(*/ ":"");
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
            printf((testResponse[byteTotal] & GBP_STATUS_MASK_UNTRAN) ? "UNTRAN " : "");
            printf((testResponse[byteTotal] & GBP_STATUS_MASK_FULL  ) ? "FULL "   : "");
            printf((testResponse[byteTotal] & GBP_STATUS_MASK_BUSY  ) ? "BUSY "   : "");
            printf((testResponse[byteTotal] & GBP_STATUS_MASK_SUM   ) ? "SUM "    : "");
            printf(" */");
          }
        }
#endif
        // Check if packet bytes match test vector
        if (pktByteIndex < (8+pktDataLength))
        {
          if (pktByteIndex == 0)
          {
            testPacketResult[pktTotalCount].packetFound = true;
            testPacketResult[pktTotalCount].startOfPacketBytePos = byteTotal;
          }
          // Check if we have captured the byte correctly
          if (rxByte != testVector[byteTotal])
          {
            testPacketResult[pktTotalCount].packetMismatch = true;
          }
        }
        else if (pktByteIndex == (8+pktDataLength + 0))
        {
          // Do not care about status byte relating to printer ID
          testPacketResult[pktTotalCount].printerID = testResponse[byteTotal];
          testPacketResult[pktTotalCount].printerIDExpected = testVector[byteTotal];
        }
        else if (pktByteIndex == (8+pktDataLength + 1))
        {
          // Check if our status reply is correct
          testPacketResult[pktTotalCount].status = testResponse[byteTotal];
          testPacketResult[pktTotalCount].statusExpected = testVector[byteTotal];
          if (testResponse[byteTotal] != testVector[byteTotal])
          {
            testPacketResult[pktTotalCount].statusMismatch = true;
          }
        }
        // Splitting packets for convenience
        if ((pktByteIndex>5)&&(pktByteIndex>=(9+pktDataLength)))
        {
#ifdef FEATURE_PACKET_DUMP
          printf("\r\n");
#endif
          pktByteIndex = 0;
          pktTotalCount++;
        }
        else
        {
#ifdef FEATURE_PACKET_DUMP
          printf( ((pktByteIndex+1)%16 == 0) ? "\n" : " "); ///< Insert Newline Periodically
#endif
          pktByteIndex++; // Byte hex split counter
        }
        byteTotal++; // Byte total counter
      }
    }
  }


#ifdef FEATURE_PACKET_COMMUNICATION_SUMMARY
  printf("\r\n/* Communication Summary */\r\n");
  for (size_t i = 0 ; i < (sizeof(testPacketResult)/sizeof(testPacketResult[0])) ; i++)
  {
    if (!testPacketResult[i].packetFound)
      continue;

    //if (testPacketResult[i].packetMismatch || testPacketResult[i].statusMismatch)
    {
      printf("/* pkt %3lu %s (dLen:%3lu) ",
          (unsigned long) i,
          (char *)gbpCommand_toStr(testPacketResult[i].command),
          (unsigned long) testPacketResult[i].pktDataLength
        );
      if (testPacketResult[i].printerID != testPacketResult[i].printerIDExpected)
      {
        printf("(ID: Got=0x%02X, Exp=0x%02X) ", (unsigned) testPacketResult[i].printerID, (unsigned) testPacketResult[i].printerIDExpected);
      }
      else
      {
        printf("(ID:0x%02X) ", (unsigned) testPacketResult[i].printerID);
      }
      printf("(Status: Got=");
      if (testPacketResult[i].status)
      {
        printf((testPacketResult[i].status & GBP_STATUS_MASK_LOWBAT) ? "LOWBAT " : "");
        printf((testPacketResult[i].status & GBP_STATUS_MASK_ER2   ) ? "ER2 "    : "");
        printf((testPacketResult[i].status & GBP_STATUS_MASK_ER1   ) ? "ER1 "    : "");
        printf((testPacketResult[i].status & GBP_STATUS_MASK_ER0   ) ? "ER0 "    : "");
        printf((testPacketResult[i].status & GBP_STATUS_MASK_UNTRAN) ? "UNTRAN "  : "");
        printf((testPacketResult[i].status & GBP_STATUS_MASK_FULL  ) ? "FULL "   : "");
        printf((testPacketResult[i].status & GBP_STATUS_MASK_BUSY  ) ? "BUSY "   : "");
        printf((testPacketResult[i].status & GBP_STATUS_MASK_SUM   ) ? "SUM "    : "");
      }
      printf(", Exp=");
      if (testPacketResult[i].statusExpected)
      {
        printf((testPacketResult[i].statusExpected & GBP_STATUS_MASK_LOWBAT) ? "LOWBAT " : "");
        printf((testPacketResult[i].statusExpected & GBP_STATUS_MASK_ER2   ) ? "ER2 "    : "");
        printf((testPacketResult[i].statusExpected & GBP_STATUS_MASK_ER1   ) ? "ER1 "    : "");
        printf((testPacketResult[i].statusExpected & GBP_STATUS_MASK_ER0   ) ? "ER0 "    : "");
        printf((testPacketResult[i].statusExpected & GBP_STATUS_MASK_UNTRAN) ? "UNTRAN "  : "");
        printf((testPacketResult[i].statusExpected & GBP_STATUS_MASK_FULL  ) ? "FULL "   : "");
        printf((testPacketResult[i].statusExpected & GBP_STATUS_MASK_BUSY  ) ? "BUSY "   : "");
        printf((testPacketResult[i].statusExpected & GBP_STATUS_MASK_SUM   ) ? "SUM "    : "");
      }
      printf(") ");
      printf("%s%s",
          (char *) testPacketResult[i].packetMismatch ? "[Packet Mismatch] " : "",
          (char *) testPacketResult[i].statusMismatch ? "[Status Mismatch] " : ""
        );
      printf("*/\r\n");
    }
  }
#endif

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