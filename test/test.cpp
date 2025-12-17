#include "ulog.h"
#include "test.hpp"
#include <cstdio>

extern "C" {
    void test_log_in_c_function();
}

int main() {
    ULOG_INFO("Hello, ULog!");
    ULOG_MILE("Starting!");
    ULOG_ERROR("An error occurred: %%u", 42u);

    // Test string!
    const char test_str1[] = "Test1";
    const char test_str2[] = "Test string for ULog!";
    ULOG_DEBUG1("Logging a string: {}", test_str1);
    ULOG_DEBUG2("Logging another string: {}", test_str2);
    ulog_flush();
    
    printf("Logging another string: %s\n", "1");
    ULOG_DEBUG2("Logging another string: {}", "1");
    ulog_flush();

    printf("Logging another string: %s\n", "12");
    ULOG_DEBUG2("Logging another string: {}", "12");
    ulog_flush();
    
    printf("Logging another string: %s\n", "123");
    ULOG_DEBUG2("Logging another string: {}", "123");
    ulog_flush();
    
    ULOG_DEBUG2("Logging another string: {}", "1234");
    printf("Logging another string: %s\n", "1234");
    ulog_flush();
    
    ULOG_DEBUG2("Logging another string: {}", "12345");
    printf("Logging another string: %s\n", "12345");
    ulog_flush();
    
    ULOG_DEBUG2("Logging another string: {}", "123456");    
    printf("Logging another string: %s\n", "123456");
    ulog_flush();
    
    ULOG_DEBUG2("Logging another string: {}", "1234567");
    printf("Logging another string: %s\n", "1234567");
    ulog_flush();
    
    ULOG_DEBUG2("Logging another string: {}", "12345678");
    printf("Logging another string: %s\n", "12345678");
    ulog_flush();
    
    ULOG_DEBUG2("Logging another string: {}", "123456789");
    printf("Logging another string: %s\n", "123456789");
    ulog_flush();
    
    ULOG_DEBUG2("Logging another string: {}", "1234567890");
    printf("Logging another string: %s\n", "1234567890");
    ulog_flush();
    
    ULOG_DEBUG2("Logging another string: {}", "12345678901");
    printf("Logging another string: %s\n", "12345678901");
    ulog_flush();
    
    ULOG_DEBUG2("Logging another string: {}", "123456789012");
    printf("Logging another string: %s\n", "123456789012");
    ulog_flush();
    
    ULOG_DEBUG2("Logging another string: {}", "1234567890123");
    printf("Logging another string: %s\n", "1234567890123");
    ulog_flush();
    
    ULOG_DEBUG2("Logging another string: {}", "12345678901234");
    printf("Logging another string: %s\n", "12345678901234");
    ulog_flush();
    
    ULOG_DEBUG2("Logging another string: {}", "123456789012345");
    printf("Logging another string: %s\n", "123456789012345");
    ulog_flush();
    
    ULOG_DEBUG2("Logging another string: {}", "1234567890123456");
    printf("Logging another string: %s\n", "1234567890123456");
    ulog_flush();
    
    ULOG_DEBUG2("Logging another string: {}", "12345678901234567");
    printf("Logging another string: %s\n", "12345678901234567");
    ulog_flush();
    
    ULOG_DEBUG2("Logging another string: {}", "123456789012345678");
    printf("Logging another string: %s\n", "123456789012345678");
    ulog_flush();
    
    ULOG_DEBUG2("Logging another string: {}", "1234567890123456789");
    printf("Logging another string: %s\n", "1234567890123456789");
    ulog_flush();
    
    printf("Logging another string: %s\n", "12345678901234567890");
    ULOG_DEBUG2("Logging another string: {}", "12345678901234567890");
    ulog_flush();
    
    printf("Logging from C function\n");
    test_log_in_c_function();

    return 0;
}
