/**
 * @file ulog_freertos.cpp
 * @brief FreeRTOS implementation of the ULOG logging framework.
 * @author
 */
#include "ulog_port.h"
#include <FreeRTOS.h>
#include <task.h>
#include <event_groups.h>
#include <cstdio>


// C Linkage for the ULOG functions called from ulog.c
extern "C" {
   void _ulog_transmit();
   void _ulog_init();
   
   /**
    * Platform-specific data transmission function.
    * Override this to send data via UART, USB, etc.
    */
   void _ulog_freertos_send_data(const uint8_t *data, size_t len) {
         // Default implementation: print as hex
         // Replace with actual UART/transport code
         for (size_t i = 0; i < len; ++i) {
            printf("%02x ", data[i]);
         }
         printf("\n");
    }

    EventGroupHandle_t ulog_tx_event = NULL;
    TaskHandle_t ulog_idle_task_handle = NULL;
}

namespace {
    /**
     * FreeRTOS idle hook - called when all tasks are idle.
     * Checks for pending log data and transmits it.
     */
    extern "C" void vApplicationIdleHook(void) {
        // Check if there's data to transmit
        EventBits_t bits = xEventGroupGetBits(ulog_tx_event);
        
        if (bits & ULOG_TX_EVENT_BIT) {
            // Clear the event bit
            xEventGroupClearBits(ulog_tx_event, ULOG_TX_EVENT_BIT);
            
            // Transmit logs in idle time
            _ulog_transmit();
        }
    }

    /**
     * Initialize the ULOG system for FreeRTOS.
     * Called automatically before main() using constructor attribute.
     */
    __attribute__((constructor()))
    void _ulog_freertos_init() {
        // Create event group for notification
        ulog_tx_event = xEventGroupCreate();
        
        configASSERT(ulog_tx_event != NULL);

        // Complete the initialization
        _ulog_init();
    }

    /**
     * Cleanup function (rarely used in embedded systems).
     */
    __attribute__((destructor()))
    void _ulog_freertos_deinit() {
        ulog_flush();
        
        if (ulog_tx_event != NULL) {
            vEventGroupDelete(ulog_tx_event);
            ulog_tx_event = NULL;
        }
    }
}
