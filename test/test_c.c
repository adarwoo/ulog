#include <stdint.h>
#include <stdbool.h>
#include "ulog.h"

void test_log_in_c_function() {
    // Test all integral types
    uint8_t u8 = 255;
    int8_t s8 = -128;
    bool b = true;
    uint16_t u16 = 65535;
    int16_t s16 = -32768;
    uint32_t u32 = 0xDEADBEEF;
    int32_t s32 = -123456;
    float f = 3.14159f;
    
    // 0 args
    ULOG_INFO("No arguments");
    
    // 1 arg - all types
    ULOG_INFO("uint8_t", u8);
    ULOG_INFO("int8_t", s8);
    ULOG_INFO("bool", b);
    ULOG_INFO("uint16_t", u16);
    ULOG_INFO("int16_t", s16);
    ULOG_INFO("uint32_t", u32);
    ULOG_INFO("int32_t", s32);
    ULOG_INFO("float", f);
    
    // 2 args - various combinations
    ULOG_INFO("u8,u8", u8, u8);
    ULOG_INFO("u8,u16", u8, u16);
    ULOG_INFO("u16,u8", u16, u8);
    ULOG_INFO("u16,u16", u16, u16);
    ULOG_INFO("s8,s8", s8, s8);
    ULOG_INFO("bool,u8", b, u8);
    
    // 3 args - combinations that fit in 4 bytes
    ULOG_INFO("u8,u8,u8", u8, u8, u8);
    ULOG_INFO("u8,u16,u8", u8, u16, u8);  // 4 bytes total
    ULOG_INFO("u16,u8", u16, u8);         // 3 bytes, 3rd arg ignored if provided
    ULOG_INFO("u16,u16", u16, u16);       // 4 bytes, 3rd arg ignored if provided
    
    // 4 args - only u8 types fit
    ULOG_INFO("u8,u8,u8,u8", u8, u8, u8, u8);
}
