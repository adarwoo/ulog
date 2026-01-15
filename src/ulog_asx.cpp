/**
 * @file ulog.cpp
 * @brief ASX implementation of the ULog utrace
 * Provides asynchronous logging over UART using the ASX framework.
 * @author software@arreckx.com
 */
#include "ulog_port.h"

#if !defined(ULOG_IS_DISABLED) || (ULOG_IS_DISABLED == 0)
#include <string_view>
#include <cstring>

#include <avr/interrupt.h>

#include <asx/uart.hpp>
#include <asx/reactor.hpp>

// Allow overriding the UART number used for ULOG
// This should be defined in conf_ulog.h if needed
#ifndef ULOG_UART
#  define ULOG_UART 0
#endif

// Define the baud rate for ULOG - uLogger must be configured to match this rate
// We use a high baud rate to minimize the time spent transmitting logs
// The link length is usually very short (PCB traces + short cables) allowing
//  for high baud rates with minimal errors.
constexpr auto BAUD_RATE = 921600;

reactor_handle_t _ulog_asx_react_to_initiate_transmit = REACTOR_NULL_HANDLE;

namespace {
   using namespace asx;

   // UART type
   using uart = uart::Uart<
      ULOG_UART,
      uart::CompileTimeConfig<
         BAUD_RATE,
         uart::width::_8,
         uart::parity::none,
         uart::stop::_1
      >
   >;

   /**
    * Initialize the ULOG system
    * The logging can start before this is called, but no data will be sent
    */
   __attribute__((constructor()))
   void init() {
      uart::init();
      uart::disable_rx();
      uart::get().CTRLA = 0; // No interrupt at this stage

      // Register a reactor for initiating transmissions
      _ulog_asx_react_to_initiate_transmit = asx::reactor::bind(_ulog_transmit, reactor_prio_low);

      // When the UART completes a transmission,
      // trigger the reactor to check if more data needs sending
      uart::react_on_send_complete(_ulog_asx_react_to_initiate_transmit);

      // Initialize ULOG port and send the START frame
      _ulog_init();
   }

   extern "C" void _ulog_asx_send_data(const uint8_t *data, size_t len) {
      uart::send(std::span<const uint8_t>(data, len));
   }

   extern "C" bool _ulog_asx_tx_ready() {
      return uart::tx_ready();
   }
}
#endif // ULOG_IS_DISABLED