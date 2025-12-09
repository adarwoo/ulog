//
// Porting guide macros
//
#include <pthread.h>
#include <stdbool.h>

// Define these macros or functions in ulog_porting.h to adapt to your platform

#ifdef __cplusplus
extern "C" {
#endif

extern pthread_mutex_t ulog_tx_semaphore;
extern pthread_cond_t ulog_tx_condition;

void _ulog_linux_send_data(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

/**
 * Enter critical section
 */
#define _ULOG_PORT_ENTER_CRITICAL_SECTION() \
    pthread_mutex_lock(&ulog_tx_semaphore)

/**
 * Exit critical section
 */
#define _ULOG_PORT_EXIT_CRITICAL_SECTION() \
    pthread_mutex_unlock(&ulog_tx_semaphore)

/**
 * Optional - ring a bell to notify that data is available
 */
#define _ULOG_PORT_NOTIFY() { \
    pthread_mutex_lock(&ulog_tx_semaphore); \
    pthread_cond_signal(&ulog_tx_condition); \
    pthread_mutex_unlock(&ulog_tx_semaphore); \
}
    
/**
 * Send data over the transport
 */
#define _ULOG_PORT_SEND_DATA(tx_encoded, encoded_len) \
    _ulog_linux_send_data(tx_encoded, encoded_len)

#define _ULOG_UART_TX_READY() true
