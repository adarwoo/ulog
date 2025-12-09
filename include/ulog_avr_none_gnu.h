/**
 * @file ulog_avr_none_gnu.h
 * @brief Minimal AVR implementation without framework dependencies.
 * @note This is a bare-metal implementation for code size testing.
 */
#include <avr/interrupt.h>
#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void _ulog_avr_send_data(const uint8_t *data, size_t len);
bool _ulog_avr_tx_ready(void);

#ifdef __cplusplus
}
#endif

//
// Macro definitions
//

/**
 * Enter critical section (disable interrupts)
 */
#define _ULOG_PORT_ENTER_CRITICAL_SECTION() \
    uint8_t _sreg_save = SREG; cli()

/**
 * Exit critical section (restore interrupts)
 */
#define _ULOG_PORT_EXIT_CRITICAL_SECTION() \
    SREG = _sreg_save; sei();

/**
 * Notify transmitter (polling mode - no notification needed)
 */
#define _ULOG_PORT_NOTIFY() \
    do { } while(0)

/**
 * Send data over UART
 */
#define _ULOG_PORT_SEND_DATA(tx_encoded, encoded_len) \
    _ulog_avr_send_data(tx_encoded, encoded_len)

/**
 * Check if UART is ready to transmit
 */
#define _ULOG_UART_TX_READY() \
    _ulog_avr_tx_ready()

/**
 * AVR-optimized ID computation (no subtraction!).
 * AVR has fixed addresses, so we extract ID directly from address bits 8-15.
 * This saves one SUB instruction - critical for AVR performance.
 */
#define _ULOG_ID_REL_DEFINED
static inline uint8_t ulog_id_rel(const void *p) {
   uintptr_t addr = (uintptr_t)p;
   return (uint8_t)((addr >> 8) & 0xFF);
}
