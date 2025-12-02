/**
 * @file ulog_port_header_avr_asx.h
 * @brief Porting layer for the AVR ASX platform
 * @note This header is included by ulog.h to define porting macros.
 * The following macros must be defined:
 * - ULOG_PORT_LDI : Assembly instruction to load the log ID register
 * - ULOG_ID_REGISTER : Register name used to store the log ID
 */

// Assembly instruction to load the log ID into the receiving register
#define ULOG_PORT_LDI "<YOUR CPU ASSEMBLY INSTRUCTION TO LOAD IMMEDIATE> %0, hi8(1b)\n\t"

// Register used for ULOG ID storage
// Use a general purpose register that is not used by the compiler for parameter passing
#define ULOG_ID_REGISTER <YOUR_REGISTER_HERE>
