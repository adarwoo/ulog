/**
 * @file ulog_asx.cpp
 * @brief ASX for AVR implementation of the ULOG logging framework.
 * @author software@arreckx.com
 */
#include <interrupt.h>

#include <string_view>
#include <asx/uart.hpp>
#include <asx/reactor.hpp>

#include "ulog.h"

// Allow selecting the UART to use for ULOG
// To be added to conf_ulog.h if needed
#ifndef ULOG_UART
#  define ULOG_UART 0
#endif

// C Linkage for the ULOG functions called from ulog.c
extern "C" {
   void _ulog_transmit();
   void _ulog_init();
   reactor_handle_t _ulog_asx_react_to_initiate_transmit = REACTOR_NULL_HANDLE;
}

namespace {
   // UART type
   using uart = asx::uart::Uart<
      ULOG_UART,
      asx::uart::CompileTimeConfig<
         115200,
         asx::uart::width::_8,
         asx::uart::parity::none,
         asx::uart::stop::_1
      >
   >;

   /**
    * Initialize the ULOG system
    * The logging can start before this is called, but no data will be sent
    */
   __attribute__((constructor()))
   void _ulog_asx_init() {
      uart::init();
      uart::disable_rx();
      uart::get().CTRLA = 0; // No interrupt at this stage

      // Register a reactor for filling the buffer
      _ulog_asx_react_to_initiate_transmit = asx::reactor::bind(_ulog_transmit);

      // When the uart is done sending a frame, we can send the next one
      uart::react_on_send_complete(_ulog_asx_react_to_initiate_transmit);

      // Complete the initialisation
      _ulog_init();
   }
} // End of anonymous namespace

extern "C" void _ulog_asx_send_data(const uint8_t *data, size_t len) {
   uart::send(std::span<const uint8_t>(data, len));
}

extern "C" bool _ulog_asx_tx_ready() {
   return uart::tx_ready();
}

