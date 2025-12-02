/**
 * @file ulog_port.h
 * @brief Porting layer for the ulog logging framework.
 */
#include "ulog.h"

#ifdef __cplusplus
extern "C" {
#endif

// Private prototypes used by the ULOG framework
void _ulog_on_transmit();
void _ulog_init();

// Include the port-specific header
#include _ULOG_PORT_IMPL_PATH

#ifdef __cplusplus
}
#endif