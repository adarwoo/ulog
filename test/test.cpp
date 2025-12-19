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

    printf("Testing ulog with multiple arguments (up to 8)\n");
    
    // Test with 5 arguments - this was NOT possible before!
    uint8_t a = 10, b = 20, c = 30, d = 40, e = 50;
    printf("Five args: %u, %u, %u, %u, %u\n", a, b, c, d, e);
    ULOG_INFO("Five args: {}, {}, {}, {}, {}", a, b, c, d, e);
    ulog_flush();
    
    // Test with 6 arguments
    uint8_t f = 60;
    printf("Six args: %u, %u, %u, %u, %u, %u\n", a, b, c, d, e, f);
    ULOG_INFO("Six args: {}, {}, {}, {}, {}, {}", a, b, c, d, e, f);
    ulog_flush();
    
    // Test with 7 arguments
    uint8_t g = 70;
    printf("Seven args: %u, %u, %u, %u, %u, %u, %u\n", a, b, c, d, e, f, g);
    ULOG_INFO("Seven args: {}, {}, {}, {}, {}, {}, {}", a, b, c, d, e, f, g);
    ulog_flush();
    
    // Test with 8 arguments - maximum!
    uint8_t h = 80;
    printf("Eight args: %u, %u, %u, %u, %u, %u, %u, %u\n", a, b, c, d, e, f, g, h);
    ULOG_INFO("Eight args: {}, {}, {}, {}, {}, {}, {}, {}", a, b, c, d, e, f, g, h);
    ulog_flush();
    
    // Test mixing types - previously impossible with 4-byte limit
    uint16_t u16_1 = 1000, u16_2 = 2000;
    uint32_t u32_1 = 0xDEADBEEF, u32_2 = 0xCAFEBABE;
    ULOG_INFO("Mixed: u16={}, u32={}, u16={}, u32={}", u16_1, u32_1, u16_2, u32_2);
    ulog_flush();
    
    // Test with floats - previously couldn't combine float with other args
    float temp1 = 36.6f, temp2 = 98.6f;
    uint8_t sensor = 5;
    char status[] = "OK";
    printf("Sensor %u status: %s, temps: %.2f, %.2f\n", sensor, status, temp1, temp2);
    ULOG_INFO("Sensor {} status: {}, temps: {}, {}", sensor, status, temp1, temp2);
    ulog_flush();
    
    printf("\n=== All tests completed successfully! ===\n");

    return 0;
}
