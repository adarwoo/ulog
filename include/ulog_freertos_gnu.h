//
// FreeRTOS porting macros
//
#include <FreeRTOS.h>
#include <task.h>
#include <event_groups.h>

#ifdef __cplusplus
extern "C" {
#endif

extern EventGroupHandle_t ulog_tx_event;
extern TaskHandle_t ulog_idle_task_handle;

#define ULOG_TX_EVENT_BIT (1 << 0)

void _ulog_freertos_send_data(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

/**
 * Enter critical section
 * FreeRTOS: disable interrupts or use a mutex depending on context
 */
#define _ULOG_PORT_ENTER_CRITICAL_SECTION() \
    taskENTER_CRITICAL()

/**
 * Exit critical section
 */
#define _ULOG_PORT_EXIT_CRITICAL_SECTION() \
    taskEXIT_CRITICAL()

/**
 * Notify that data is available using event groups.
 * The idle hook will check this event and transmit when CPU is idle.
 */
#define _ULOG_PORT_NOTIFY() \
    xEventGroupSetBits(ulog_tx_event, ULOG_TX_EVENT_BIT)
    
/**
 * Send data over the transport
 */
#define _ULOG_PORT_SEND_DATA(tx_encoded, encoded_len) \
    _ulog_freertos_send_data(tx_encoded, encoded_len)

/**
 * Check if UART is ready to transmit
 * Platform-specific implementation required
 */
#define _ULOG_UART_TX_READY() true

