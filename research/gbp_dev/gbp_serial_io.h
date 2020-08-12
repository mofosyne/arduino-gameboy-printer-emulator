#include <stdint.h> // uint8_t
#include <stddef.h> // size_t
#include "gbp_cbuff.h"

#define GBP_FEATURE_USING_RISING_CLOCK_ONLY_ISR // Away from technical accuracy towards double speed mode compatibility

/* Output Options */
#define GBP_FEATURE_RAW_DUMP // Output is a raw stream of packet data.

/******************************************************************************/

//#define GPB_BUFFER_SIZE 650  // 640 bytes usually
#define GPB_BUFFER_SIZE 700  // 640 bytes usually

/******************************************************************************/

/* Init/Reset/ISR Functions */
bool gpb_serial_io_init(size_t buffSize, uint8_t *buffPtr);
bool gpb_serial_io_reset(void);
#ifdef GBP_FEATURE_USING_RISING_CLOCK_ONLY_ISR
bool gpb_serial_io_OnRising_ISR(const bool GBP_SOUT);
#else
bool gpb_serial_io_OnChange_ISR(const bool GBP_SCLK, const bool GBP_SOUT);
#endif

/* Timeout */
bool gbp_serial_io_timeout_handler(uint32_t elapsed_ms);

// Dev Note: Is used during testing and simpler implementations
size_t gbp_serial_io_dataBuff_getByteCount(void);
uint8_t gbp_serial_io_dataBuff_getByte(void);
uint8_t gbp_serial_io_dataBuff_getByte_Peek(uint32_t offset);

/******************************************************************************/

#if 0 // Not sure if required yet
/* Instruct */
int gbp_printInstruction_num_of_sheets(void);
int gbp_printInstruction_num_of_linefeed_before_print(void);
int gbp_printInstruction_num_of_linefeed_after_print(void);
int gbp_printInstruction_palette_value(void);
int gbp_printInstruction_print_density(void);

/* printer status update (Set) */
void gbp_set_low_battery(bool val);
void gbp_set_other_error(bool val);
void gbp_set_paper_jam(bool val);
void gbp_set_packet_error(bool val);
void gbp_set_unprocessed_data(bool val);
void gbp_set_print_buffer_full(bool val);
void gbp_set_printer_busy(bool val);
void gbp_set_checksum_error(bool val);

/* printer status update (Get) */
bool gbp_get_low_battery(void);
bool gbp_get_other_error(void);
bool gbp_get_paper_jam(void);
bool gbp_get_packet_error(void);
bool gbp_get_unprocessed_data(void);
bool gbp_get_print_buffer_full(void);
bool gbp_get_printer_busy(void);
bool gbp_get_checksum_error(void);
#endif