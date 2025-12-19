#include "ulog.h"
#include <cstdio>

int main() {
    printf("Testing ulog with multiple arguments (up to 8)\n");
    
    // Test with 5 arguments - this was NOT possible before!
    uint8_t a = 10, b = 20, c = 30, d = 40, e = 50;
    ULOG_INFO("Five args: {}, {}, {}, {}, {}", a, b, c, d, e);
    ulog_flush();
    
    // Test with 6 arguments
    uint8_t f = 60;
    ULOG_INFO("Six args: {}, {}, {}, {}, {}, {}", a, b, c, d, e, f);
    ulog_flush();
    
    // Test with 7 arguments
    uint8_t g = 70;
    ULOG_INFO("Seven args: {}, {}, {}, {}, {}, {}, {}", a, b, c, d, e, f, g);
    ulog_flush();
    
    // Test with 8 arguments - maximum!
    uint8_t h = 80;
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
    ULOG_INFO("Sensor {} status: {}, temps: {}, {}", sensor, status, temp1, temp2);
    ulog_flush();
    
    printf("All tests completed successfully!\n");
    return 0;
}
