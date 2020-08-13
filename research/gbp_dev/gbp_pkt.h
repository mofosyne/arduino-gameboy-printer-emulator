
/*

  Refactor Notes:
    * If want compression, then we need a buffer of 0x81 = 129 Bytes. This is for the loop data.

*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "gameboy_printer_protocol.h"

#define GBP_PKT_TILE_SIZE_IN_BYTE 16
#define GBP_PKT_PAYLOAD_BUFF_SIZE_IN_BYTE GBP_PKT_TILE_SIZE_IN_BYTE

typedef enum
{
  GBP_REC_NONE,
  GBP_REC_GOT_PACKET,
  /* Streaming Packet */
  GBP_REC_GOT_PAYLOAD_PARTAL,
  GBP_REC_GOT_PACKET_END
} gbp_received_t;

typedef struct
{
  gbp_received_t received;
  uint16_t pktByteIndex;

  /* Packet Information */
  uint8_t command;
  uint8_t compression;
  uint16_t dataLength;
  uint8_t printerID;
  uint8_t status;
} gbp_pkt_t;

bool gbp_pkt_init(gbp_pkt_t *_pkt);
bool gbp_pkt_processByte( const uint8_t _byte, gbp_pkt_t *_pkt, uint8_t buffer[], uint8_t *bufferSize, const uint8_t bufferMax);


/*******************************************************************************
 * Print Instruction
*******************************************************************************/

static inline int gbp_pkt_printInstruction_num_of_sheets(uint8_t payloadBuff[GBP_PRINT_INSTRUCT_PAYLOAD_SIZE])
{
  return (payloadBuff[GBP_PRINT_INSTRUCT_INDEX_NUM_OF_SHEETS  ]);
}

static inline int gbp_pkt_printInstruction_num_of_linefeed_before_print(uint8_t payloadBuff[GBP_PRINT_INSTRUCT_PAYLOAD_SIZE])
{
  return (payloadBuff[GBP_PRINT_INSTRUCT_INDEX_NUM_OF_LINEFEED] >> 4) & 0x0F;
}

static inline int gbp_pkt_printInstruction_num_of_linefeed_after_print(uint8_t payloadBuff[GBP_PRINT_INSTRUCT_PAYLOAD_SIZE])
{
  return (payloadBuff[GBP_PRINT_INSTRUCT_INDEX_NUM_OF_LINEFEED]) & 0x0F;
}

static inline int gbp_pkt_printInstruction_palette_value(uint8_t payloadBuff[GBP_PRINT_INSTRUCT_PAYLOAD_SIZE])
{
  return (payloadBuff[GBP_PRINT_INSTRUCT_INDEX_PALETTE_VALUE  ]);
}

static inline int gbp_pkt_printInstruction_print_density(uint8_t payloadBuff[GBP_PRINT_INSTRUCT_PAYLOAD_SIZE])
{
  return (payloadBuff[GBP_PRINT_INSTRUCT_INDEX_PRINT_DENSITY  ]);
}
