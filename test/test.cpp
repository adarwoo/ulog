#include "ulog.h"
#include "test.hpp"

extern "C" {
    void test_log_in_c_function();
}

int main() {
    ULOG_INFO("Hello, ULog!");

    ULOG_MILE("Starting!");
    ULOG_MILE("Starting!");
    ULOG_ERROR("An error occurred: %u", 42u);

    test_log_in_c_function();

    return 0;
}
