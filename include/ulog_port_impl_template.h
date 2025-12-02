//
// Porting guide macros
//
#include <YOUR HEADERS_IF_NEEDED>

// Define these macros or functions in ulog_porting.h to adapt to your platform

#ifdef __cplusplus
extern "C" {
#endif

<YOUR GLOLBAL_DECLARATIONS_IF_NEEDED>

#ifdef __cplusplus
}
#endif

/**
 * Enter critical section
 */
#define _ULOG_PORT_ENTER_CRITICAL_SECTION()
    <YOUR IMPLEMENTATION TO ENTER CRITICAL SECTION>

/**
 * Exit critical section
 */
#define _ULOG_PORT_EXIT_CRITICAL_SECTION()
    <YOUR IMPLEMENTATION TO EXIT CRITICAL SECTION>

/**
 * Optional - ring a bell to notify that data is available
 */
#define _ULOG_PORT_NOTIFY()
    <YOUR IMPLEMENTATION TO NOTIFY TRANSMITTER>

/**
 * Send data over the transport
 */
#define _ULOG_PORT_SEND_DATA(tx_encoded, encoded_len) \
    <YOUR IMPLEMENTATION TO SEND DATA>

#define _ULOG_UART_TX_READY() \
    <YOUR IMPLEMENTATION TO CHECK IF TRANSMITTER IS READY>

