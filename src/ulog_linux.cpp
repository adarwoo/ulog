/**
 * @file ulog_linux.cpp
 * @brief Linux implementation of the ULOG logging framework.
 * @author
 */
#include "ulog_port.h"
#include <pthread.h>
#include <cstdio>


// C Linkage for the ULOG functions called from ulog.c
extern "C" {
   void _ulog_transmit();
   void _ulog_init();
   void _ulog_linux_send_data(const uint8_t *data, size_t len) {
        // For demonstration, print the data as hex to stdout
        for (size_t i = 0; i < len; ++i) {
        printf("%02x ", data[i]);
        }

        printf("\n");
        fflush(stdout);
    }

    pthread_mutex_t ulog_tx_semaphore = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t ulog_tx_condition = PTHREAD_COND_INITIALIZER;
}

namespace {
    static pthread_t _ulog_tx_thread;
    static bool stop_thread = false;

    /**
    * Initialize the ULOG system
    * The logging can start before this is called, but no data will be sent
    */
    __attribute__((constructor()))
    void _ulog_linux_init() {
        // Start a thread to push logs when notified
        
        auto _ulog_tx_thread_func = [](void*) -> void* {
            while (true) {
                // Wait for notification
                pthread_mutex_lock(&ulog_tx_semaphore);
                
                // Check stop flag BEFORE waiting
                if (stop_thread) {
                    pthread_mutex_unlock(&ulog_tx_semaphore);
                    break;
                }
                
                pthread_cond_wait(&ulog_tx_condition, &ulog_tx_semaphore);
                
                if (stop_thread) {
                    pthread_mutex_unlock(&ulog_tx_semaphore);
                    break;
                }

                pthread_mutex_unlock(&ulog_tx_semaphore);

                // Transmit logs
                _ulog_transmit();
            }
            return nullptr;
        };

        pthread_create(&_ulog_tx_thread, NULL, _ulog_tx_thread_func, NULL);

        // Complete the initialisation
        _ulog_init();
    }

    __attribute__((destructor()))
    void _ulog_linux_deinit() {
        ulog_flush();
        // Join the thread (never happens in practice)
        pthread_mutex_lock(&ulog_tx_semaphore);
        stop_thread = true;
        pthread_cond_signal(&ulog_tx_condition);
        pthread_mutex_unlock(&ulog_tx_semaphore);
        pthread_join(_ulog_tx_thread, NULL);
    }
}
