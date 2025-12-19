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
