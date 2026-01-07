/**
 * @file ulog.c
 * @brief Ultra-lightweight logging framework for embedded systems.
 * Common flavourless implementation of the library.
 * @author software@arreckx.com
 */
#include "ulog.h"
#include <stdio.h>
//
// Apply configuration defaults
//
#ifndef ULOG_QUEUE_SIZE
#  define ULOG_QUEUE_SIZE 64
#endif

/** COBS framing char */
#define COBS_EOF 0xA6

/** Special ID for application start */
#define ULOG_ID_START 0x7FFE

/** Special ID for overrun */
#define ULOG_ID_OVERRUN 0x7FFF

/** Continuation flag - MSB of 16-bit ID */
#define ULOG_ID_CONTINUATION 0x8000

// Circular buffer (stubbed here)
#define MAX_PAYLOAD 4

#ifndef ULOG_ID_TYPE
#  define ULOG_ID_TYPE uint16_t
#endif

// ----------------------------------------------------------------------
// Private types
// ----------------------------------------------------------------------
typedef struct __attribute__((packed)) {
   // Length of the data in the buffer (including the id)
   uint8_t payload_len;

   union __attribute__((packed)) {
      struct __attribute__((packed)) {
         // Id of the log message (16-bit, stored in native little-endian)
         uint16_t id;
         // Data to send over
         uint8_t data[MAX_PAYLOAD];
      };

      uint8_t payload[2 + MAX_PAYLOAD];
   };
} LogPacket;

// ----------------------------------------------------------------------
// Private data
// ----------------------------------------------------------------------

// Circular buffer pointers
uint8_t log_head = 0, log_tail = 0;

// Circular buffer itself
LogPacket logs_circular_buffer[ULOG_QUEUE_SIZE];


// Scratch buffer for encoded output. Worse case
// The payload(COBS adds +2 overhead worse case)
static union __attribute__((packed)) {
   struct {
      uint8_t len;
      uint16_t id;
      uint8_t eof;
   };
   uint8_t bytes[MAX_PAYLOAD + sizeof(ULOG_ID_TYPE) + 2];
} tx_encoded = {{0x03, ULOG_ID_START, COBS_EOF}};

// Flag to indicate a buffer overrun masked into the trait of the log
// This is a simple way to indicate that some logs were lost
// due to buffer full condition
static uint8_t buffer_overrun = 0;


// ----------------------------------------------------------------------
// Internal functions
// ----------------------------------------------------------------------

/**
 * Reserve a log packet in the circular buffer
 * @return Pointer to the reserved packet, or nullptr if the buffer is full
 */
static LogPacket *reserve_log_packet() {
    LogPacket *retval = NULL;

    // As required - disable interrupt - but save since this could be used from within an interrupt
    _ULOG_PORT_ENTER_CRITICAL_SECTION();

    // Don't accept new logs while we are in overrun state
    // Rationale: Allow the buffer to clean, process logs faster to recover properly
    if ( buffer_overrun == 0 ) {
        uint8_t next = (uint8_t)((log_head + 1) % ULOG_QUEUE_SIZE);

        if (next != log_tail) {
            retval = &logs_circular_buffer[log_head];
            log_head = next;
        } else {
            // Buffer full - set the overrun flag
            buffer_overrun = 1;
        }
    } else {
        if ( buffer_overrun < 255 ) {
            // Count the number of overruns - saturate at 255
            ++buffer_overrun;
        }
    }

    // Restore the context
    _ULOG_PORT_EXIT_CRITICAL_SECTION();

    return retval;
}

/**
 * COBS encoder: encodes input into output, returns encoded length
 * @param input Input data
 * @param length Length of input data
 * @return Length of encoded data
 */
static uint8_t cobs_encode(const uint8_t* input, uint8_t length) {
    uint8_t read_index = 0;
    uint8_t write_index = 1;
    uint8_t code_index = 0;
    uint8_t code = 1;

    while (read_index < length) {
        uint8_t input_byte = input[read_index];

        if (input_byte == COBS_EOF) {
            tx_encoded.bytes[code_index] = code;
            code_index = write_index++;
            code = 1;
        } else {
            tx_encoded.bytes[write_index++] = input_byte;
            ++code;
        }

        ++read_index;
    }

    tx_encoded.bytes[code_index] = code;
    tx_encoded.bytes[write_index++] = COBS_EOF; // end frame

    return write_index;
}

/**
 * Initiate a transmission reactor handler
 * Invoked at every insertion and once a transmit is complete
 * Checks for pending data
 * If found, encode and initiates the transmission on the UART
 * This function should be called when the system is idle.
 */
void _ulog_transmit() {
   uint8_t encoded_len = 0;

   // Avoid race condition since an interrupt could be logging
   // Keep critical section for the entire encode+send to protect tx_encoded buffer
   _ULOG_PORT_ENTER_CRITICAL_SECTION();

   if ( _ULOG_UART_TX_READY() ) {
      // Check the UART is ready and we have data to send
      if (log_tail != log_head) {
         // Data to send - encode while still in critical section
         LogPacket pkt = logs_circular_buffer[log_tail];
         log_tail = (uint8_t)((log_tail + 1) % ULOG_QUEUE_SIZE);

         encoded_len = cobs_encode(pkt.payload, pkt.payload_len);
         _ULOG_PORT_SEND_DATA(tx_encoded.bytes, encoded_len);
      } else if (buffer_overrun > 0) {
         // Send overrun notification
         struct {
            uint16_t id;
            uint8_t count;
         } overrun_payload = {ULOG_ID_OVERRUN, buffer_overrun};

         buffer_overrun = 0;

         encoded_len = cobs_encode((const uint8_t*)&overrun_payload, sizeof(overrun_payload));
         _ULOG_PORT_SEND_DATA(tx_encoded.bytes, encoded_len);
      }
   }

   // Restore SREG
   _ULOG_PORT_EXIT_CRITICAL_SECTION();
}

/**
 * Internal init function to be called by the porting layer
 */
void _ulog_init() {
   _ULOG_PORT_SEND_DATA(tx_encoded.bytes, 4); // Send the application start frame (3 bytes + EOF)
}


// ----------------------------------------------------------------------
// C Linkage functions called from the inline assembly
// Let the compiler optimize the obvious factorization
// ----------------------------------------------------------------------

void ulog_detail_enqueue(uint16_t id) {
   LogPacket* dst = reserve_log_packet();

   if (dst) {
      dst->id = id;
      dst->payload_len = 2+0;
   }

   _ULOG_PORT_NOTIFY();
}

void ulog_detail_enqueue_1(uint16_t id, uint8_t v0) {
   LogPacket* dst = reserve_log_packet();

   if (dst) {
      dst->id = id;
      dst->payload_len = 2+1;
      dst->data[0] = v0;
   }

   _ULOG_PORT_NOTIFY();
}

void ulog_detail_enqueue_2(uint16_t id, uint8_t v0, uint8_t v1) {
   LogPacket* dst = reserve_log_packet();

   if (dst) {
      dst->id = id;
      dst->payload_len = 2+2;
      dst->data[0] = v0;
      dst->data[1] = v1;
   }

   _ULOG_PORT_NOTIFY();
}

void ulog_detail_enqueue_3(uint16_t id, uint8_t v0, uint8_t v1, uint8_t v2) {
   LogPacket* dst = reserve_log_packet();

   if (dst) {
      dst->id = id;
      dst->payload_len = 2+3;
      dst->data[0] = v0;
      dst->data[1] = v1;
      dst->data[2] = v2;
   }

   _ULOG_PORT_NOTIFY();
}

void ulog_detail_enqueue_4(uint16_t id, uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3) {
   LogPacket* dst = reserve_log_packet();

   if (dst) {
      dst->id = id;
      dst->payload_len = 2+4;
      dst->data[0] = v0;
      dst->data[1] = v1;
      dst->data[2] = v2;
      dst->data[3] = v3;
   }

   _ULOG_PORT_NOTIFY();
}

void ulog_flush(void) {
   while (true) {
      _ULOG_PORT_ENTER_CRITICAL_SECTION();
      bool empty = (log_head == log_tail);
      _ULOG_PORT_EXIT_CRITICAL_SECTION();

      if ( empty ) {
         break;
      }

      _ulog_transmit();
   }
}