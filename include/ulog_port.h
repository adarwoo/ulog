/**
 * @file ulog_port.h
 * @brief Porting layer for the ulog logging framework.
 */

#ifdef __cplusplus
extern "C" {
#endif

// Common prototypes used by the ULOG framework
void _ulog_on_transmit();

#ifdef __AVR__
#  include "sysclk.h"
#  include <interrupt.h>
#  include <reactor.h>

   extern reactor_handle_t _ulog_asx_react_to_initiate_transmit;
   extern void _ulog_asx_send_data(const uint8_t *data, size_t len);

#  define _ULOG_PORT_ENTER_CRITICAL_SECTION() \
    irqflags_t save_flags = cpu_irq_save()

#  define _ULOG_PORT_EXIT_CRITICAL_SECTION() \
    cpu_irq_restore(save_flags)

#  define _ULOG_PORT_NOTIFY() \
    reactor_null_notify_from_isr(_ulog_asx_react_to_initiate_transmit);

#  define _ULOG_PORT_SEND_DATA(tx_encoded, encoded_len) \
    _ulog_asx_send_data(tx_encoded, encoded_len)
#else
#  error "Please define the porting macros for your platform in ulog_porting.h"
#endif

#ifdef __cplusplus
}
#endif