#include "ulog.h"

void test_log_in_c_function() {
    ULOG_WARN("Logging from C function: %u", 42u);
}
