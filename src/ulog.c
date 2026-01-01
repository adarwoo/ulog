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
typedef struct {
    // Length of the data in the buffer (including the id)
    uint8_t payload_len;

    union {
        struct {
            // Id of the log message (16-bit, stored in big-endian)
            uint8_t id_msb;
            uint8_t id_lsb;
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
// Pre-fill with the encoded application start frame (big-endian)
static uint8_t tx_encoded[MAX_PAYLOAD + sizeof(ULOG_ID_TYPE) + 2] =
   {0x03, (uint8_t)(ULOG_ID_START >> 8), (uint8_t)(ULOG_ID_START & 0xFF), COBS_EOF};

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
            tx_encoded[code_index] = code;
            code_index = write_index++;
            code = 1;
        } else {
            tx_encoded[write_index++] = input_byte;
            ++code;
        }

        ++read_index;
    }

    tx_encoded[code_index] = code;
    tx_encoded[write_index++] = COBS_EOF; // end frame

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
   // Avoid race condition since an interrupt could be logging
   _ULOG_PORT_ENTER_CRITICAL_SECTION();

   // Check the UART is ready and we have data to send
   // No race expected: The flag could clear after we check it, but in that
   // case, given the critical section here, the UART call this function again when ready
   if ( _ULOG_UART_TX_READY() ) {
      if (log_tail != log_head) {
         // Data to send
         LogPacket pkt = logs_circular_buffer[log_tail];
         log_tail = (uint8_t)((log_tail + 1) % ULOG_QUEUE_SIZE);

         uint8_t encoded_len = cobs_encode(pkt.payload, pkt.payload_len);

         _ULOG_PORT_SEND_DATA(tx_encoded, encoded_len);
      } else if (buffer_overrun > 0) {
         struct {
            uint8_t id_msb;
            uint8_t id_lsb;
            uint8_t count;
         } overrun_payload = {(uint8_t)(ULOG_ID_OVERRUN >> 8), (uint8_t)(ULOG_ID_OVERRUN & 0xFF), 0};

         uint8_t encoded_len = cobs_encode((const uint8_t*)&overrun_payload, sizeof(overrun_payload));

         // Send an overrun notification packet
         _ULOG_PORT_SEND_DATA(tx_encoded, encoded_len);

         // Clear the overrun flag
         buffer_overrun = 0;
      }
   }

   // Restore SREG
   _ULOG_PORT_EXIT_CRITICAL_SECTION();
}

/**
 * Internal init function to be called by the porting layer
 */
void _ulog_init() {
   _ULOG_PORT_SEND_DATA(tx_encoded, 4); // Send the application start frame (3 bytes + EOF)
}


// ----------------------------------------------------------------------
// C Linkage functions called from the inline assembly
// Let the compiler optimize the obvious factorization
// ----------------------------------------------------------------------

void ulog_detail_enqueue(uint16_t id) {
   LogPacket* dst = reserve_log_packet();

   if (dst) {
      dst->id_msb = (uint8_t)(id >> 8);
      dst->id_lsb = (uint8_t)(id & 0xFF);
      dst->payload_len = 2+0;
   }
}

void ulog_detail_enqueue_1(uint16_t id, uint8_t v0) {
   LogPacket* dst = reserve_log_packet();

   if (dst) {
      dst->id_msb = (uint8_t)(id >> 8);
      dst->id_lsb = (uint8_t)(id & 0xFF);
      dst->payload_len = 2+1;
      dst->data[0] = v0;

      // Notify that data is available so a transmission can be initiated
      // This function must be callable from an interrupt context
      _ULOG_PORT_NOTIFY();
   }
}

void ulog_detail_enqueue_2(uint16_t id, uint8_t v0, uint8_t v1) {
   LogPacket* dst = reserve_log_packet();

   if (dst) {
      dst->id_msb = (uint8_t)(id >> 8);
      dst->id_lsb = (uint8_t)(id & 0xFF);
      dst->payload_len = 2+2;
      dst->data[0] = v0;
      dst->data[1] = v1;

      // Notify that data is available so a transmission can be initiated
      // This function must be callable from an interrupt context
      _ULOG_PORT_NOTIFY();
   }
}

void ulog_detail_enqueue_3(uint16_t id, uint8_t v0, uint8_t v1, uint8_t v2) {
   LogPacket* dst = reserve_log_packet();

   if (dst) {
      dst->id_msb = (uint8_t)(id >> 8);
      dst->id_lsb = (uint8_t)(id & 0xFF);
      dst->payload_len = 2+3;
      dst->data[0] = v0;
      dst->data[1] = v1;
      dst->data[2] = v2;

      // Notify that data is available so a transmission can be initiated
      // This function must be callable from an interrupt context
      _ULOG_PORT_NOTIFY();

   }
}

void ulog_detail_enqueue_4(uint16_t id, uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3) {
   LogPacket* dst = reserve_log_packet();

   if (dst) {
      dst->id_msb = (uint8_t)(id >> 8);
      dst->id_lsb = (uint8_t)(id & 0xFF);
      dst->payload_len = 2+4;
      dst->data[0] = v0;
      dst->data[1] = v1;
      dst->data[2] = v2;
      dst->data[3] = v3;

      // Notify that data is available so a transmission can be initiated
      // This function must be callable from an interrupt context
      _ULOG_PORT_NOTIFY();

   }
}

void ulog_flush(void) {
   // Keep transmitting until the buffer is empty
   _ULOG_PORT_ENTER_CRITICAL_SECTION();

   while (log_tail != log_head) {
      _ULOG_PORT_EXIT_CRITICAL_SECTION();
      _ulog_transmit();
      _ULOG_PORT_ENTER_CRITICAL_SECTION();
   }

   _ULOG_PORT_EXIT_CRITICAL_SECTION();
}