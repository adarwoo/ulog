/**
 * @file ulog_avr_none_gnu.cpp
 * @brief Minimal AVR implementation without framework dependencies.
 * @author software@arreckx.com
 * @note This is a bare-metal implementation for code size testing.
 */
#include "ulog_port.h"
#include <avr/io.h>

// UART configuration - adjust for your AVR device
#ifndef F_CPU
#  define F_CPU 16000000UL
#endif

#ifndef ULOG_BAUD
#  define ULOG_BAUD 115200
#endif

// Calculate UBRR value
#define ULOG_UBRR_VALUE ((F_CPU / (16UL * ULOG_BAUD)) - 1)

// C Linkage for the ULOG functions
extern "C" {
   void _ulog_transmit();
   void _ulog_init();
}

namespace {
   /**
    * Initialize the UART for ULOG transmission
    */
   __attribute__((constructor()))
   void _ulog_avr_init() {
      // Set baud rate
      UBRR0H = (uint8_t)(ULOG_UBRR_VALUE >> 8);
      UBRR0L = (uint8_t)ULOG_UBRR_VALUE;
      
      // Enable transmitter
      UCSR0B = (1 << TXEN0);
      
      // Set frame format: 8 data bits, 1 stop bit, no parity
      UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

      // Complete the ULOG initialization
      _ulog_init();
   }

   /**
    * Transmit a single byte via UART (blocking)
    */
   inline void uart_putc(uint8_t data) {
      // Wait for empty transmit buffer
      while (!(UCSR0A & (1 << UDRE0)));
      
      // Put data into buffer, sends the data
      UDR0 = data;
   }
} // namespace

extern "C" void _ulog_avr_send_data(const uint8_t *data, size_t len) {
   for (size_t i = 0; i < len; i++) {
      uart_putc(data[i]);
   }
}

extern "C" bool _ulog_avr_tx_ready() {
   // In polling mode, always ready after data is sent
   return (UCSR0A & (1 << UDRE0)) != 0;
}

// Implement the transmit function that will be called by ULOG
extern "C" void _ulog_on_transmit() {
   // In polling mode, just call transmit directly
   _ulog_transmit();
}
