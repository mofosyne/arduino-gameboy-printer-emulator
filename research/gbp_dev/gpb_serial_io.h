#include <stdint.h> // uint8_t
#include <stddef.h> // size_t

#define GBP_FEATURE_RAW_DUMP

//#define GPB_BUFFER_SIZE 650  // 640 bytes usually
#define GPB_BUFFER_SIZE 1000  // 640 bytes usually

#ifdef GBP_FEATURE_RAW_DUMP
size_t gbp_sio_lastPacketByteCount(void);
size_t gbp_sio_lastPacketChecksum(void);
uint16_t gbp_sio_lastPacketStatus(void);
#endif

bool gpb_pktIO_init(size_t buffSize, uint8_t *buffPtr);
bool gpb_pktIO_reset(void);
bool gpb_pktIO_OnChange_ISR(const bool GBP_SCLK, const bool GBP_SOUT);

/* Parse status */
bool gbp_timeout_handler(uint32_t elapsed_ms);
bool gbp_packetWasReceived(bool reset);

/* Incoming packet */
bool gbp_init_CheckReceived(bool clear);
bool gbp_printInstruction_CheckReceived(bool clear);
bool gbp_dataPacket_CheckReceived(bool clear);
bool gbp_dataEndPacket_CheckReceived(bool clear);
bool gbp_breakPacket_CheckReceived(bool clear);
bool gbp_nullPacket_CheckReceived(bool clear);

/* Instruct */
int gbp_printInstruction_num_of_sheets(void);
int gbp_printInstruction_num_of_linefeed_before_print(void);
int gbp_printInstruction_num_of_linefeed_after_print(void);
int gbp_printInstruction_palette_value(void);
int gbp_printInstruction_print_density(void);

/* Buff */
size_t gbp_dataBuff_getByteCount(void);
uint8_t gbp_dataBuff_getByte(void);
uint8_t gbp_dataBuff_getByte_Peek(uint32_t offset);


/* printer status update */
void gbp_set_low_battery(bool val);
void gbp_set_other_error(bool val);
void gbp_set_paper_jam(bool val);
void gbp_set_packet_error(bool val);
void gbp_set_unprocessed_data(bool val);
void gbp_set_print_buffer_full(bool val);
void gbp_set_printer_busy(bool val);
void gbp_set_checksum_error(bool val);
bool gbp_get_low_battery(void);
bool gbp_get_other_error(void);
bool gbp_get_paper_jam(void);
bool gbp_get_packet_error(void);
bool gbp_get_unprocessed_data(void);
bool gbp_get_print_buffer_full(void);
bool gbp_get_printer_busy(void);
bool gbp_get_checksum_error(void);
