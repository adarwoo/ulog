/**
 * @file ulog_port_impl_avr_asx_gnu.h
 * @brief Porting layer for the AVR microcontroller using the ASX framework.
 */
#include <reactor.h>
#include <avr/interrupt.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern reactor_handle_t _ulog_asx_react_to_initiate_transmit;
void _ulog_avr_asx_send_data(const uint8_t *data, size_t len);
bool _ulog_avr_asx_tx_ready();

#ifdef __cplusplus
}
#endif

//
// Macro definitions
//

#define _ULOG_PORT_ENTER_CRITICAL_SECTION() \
    cli()

#define _ULOG_PORT_EXIT_CRITICAL_SECTION() \
    sei()

#define _ULOG_PORT_NOTIFY() \
    reactor_null_notify_from_isr(_ulog_asx_react_to_initiate_transmit);

#define _ULOG_PORT_SEND_DATA(tx_encoded, encoded_len) \
    _ulog_avr_asx_send_data(tx_encoded, encoded_len)

#define _ULOG_UART_TX_READY() \
    _ulog_avr_asx_tx_ready()

/**
 * AVR-specific log record emission using direct register loading.
 * Only the high byte (bits 8-15) of the label address is needed for the ID.
 * Each invocation gets a unique local label (1:) which is guaranteed unique
 * per inline asm block by the assembler.
 * 
 * NOTE: .logs section is metadata-only (parsed from ELF by host tools),
 * so it uses NO "a" flag to prevent it from being counted in device memory.
 */
#define _ULOG_EMIT_RECORD(level, fmt, typecode)                       \
   uint8_t id;                                                        \
   __asm__ volatile(                                                  \
      ".pushsection .logs,\"\",@progbits\n\t"                          \
      ".balign 256\n\t"                                               \
      "1:\n\t"                                                        \
      ".byte %c1\n\t"                                                 \
      ".long %c2\n\t"                                                 \
      ".long %c3\n\t"                                                 \
      ".asciz \"" __FILE__ "\"\n\t"                                   \
      ".asciz \"" fmt "\"\n\t"                                        \
      ".popsection\n\t"                                               \
      "ldi %0, hi8(1b)"                                               \
      : "=r" (id)                                                     \
      : "i" ((uint8_t)(level)),                                       \
        "i" ((uint32_t)(__LINE__)),                                   \
        "i" ((uint32_t)(typecode))                                    \
   );
