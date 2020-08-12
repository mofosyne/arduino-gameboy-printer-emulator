
/*

  Refactor Notes:
    * If want compression, then we need a buffer of 0x81 = 129 Bytes. This is for the loop data.

*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define GBP_PKT_TILE_SIZE_IN_BYTE 16
#define GBP_PKT_PAYLOAD_BUFF_SIZE_IN_BYTE GBP_PKT_TILE_SIZE_IN_BYTE

typedef enum
{
  GBP_REC_NONE,

  /* Packet Arrived */
  GBP_REC_GOT_PACKET,

  /* Data Payload Streaming */
  GBP_REC_GOT_DATA_HEADER,
  GBP_REC_GOT_DATA_TILE,
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

  /* Payload */
  // For data packets, this is a tile at each offset of the packet data payload
  // For everything else, it should be large enough to contain it. Offset=0;
  uint16_t payloadBuffOffset;
  uint16_t payloadBuffSize;
  uint8_t payloadBuff[GBP_PKT_PAYLOAD_BUFF_SIZE_IN_BYTE];
} gbp_pktBuff_t;

const char *gbp_pkt_commandType_toStr(int val);
bool gbp_pkt_init(gbp_pktBuff_t *_pktBuff);
bool gbp_pkt_processByte(const uint8_t _byte, gbp_pktBuff_t *_pktBuff);
