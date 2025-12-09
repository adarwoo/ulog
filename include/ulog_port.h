#pragma once
/**
 * @file ulog_port.h
 * @brief Porting layer for the ulog logging framework.
 */
#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Private prototypes used by the ULOG framework
// ---------------------------------------------------------------------------

void _ulog_on_transmit();
void _ulog_init();

#ifdef __cplusplus
}
#endif

// ---------------------------------------------------------------------------
// Start of platform detection
// ---------------------------------------------------------------------------

#ifdef __AVR__
   // Check for ASX framework
#  if defined(USE_ASX) || defined(__ASX__)
#    include "ulog_avr_asx_gnu.h"
#  else
#    include "ulog_avr_none_gnu.h"
#  endif
#elif defined(__linux__)
#  include "ulog_linux_gnu.h"
#else
#  error "ULog: Unsupported platform. Please define porting layer."
#endif

// ---------------------------------------------------------------------------
// End of platform detection
// ---------------------------------------------------------------------------

#include "ulog.h"
