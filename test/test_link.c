
// test.c
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>

struct ulog_record {
    uint8_t  level;
    uint32_t line;
    uint32_t typecode;
    const char *file;
    const char *fmt;
};

// Two records in .logs, each aligned to 256 bytes
__attribute__((section(".logs"), used, aligned(256)))
static const struct ulog_record A = {
    3, __LINE__, 0, __FILE__, "Hello A"
};

__attribute__((section(".logs"), used, aligned(256)))
static const struct ulog_record B = {
    3, __LINE__, 0, __FILE__, "Hello B"
};

// Start/end symbols provided by the linker script
extern const unsigned char __ulog_logs_start[];
extern const unsigned char __ulog_logs_end[];

static inline uint8_t ulog_id_rel(const void *p) {
    uintptr_t base = (uintptr_t)__ulog_logs_start;
    uintptr_t addr = (uintptr_t)p;
    return (uint8_t)(((addr - base) >> 8) & 0xFF);
}

int main(void) {
    printf(".logs start   = %p\n", (void*)__ulog_logs_start);
    printf(".logs end     = %p (size=%zu bytes)\n",
           (void*)__ulog_logs_end,
           (size_t)(__ulog_logs_end - __ulog_logs_start));

    printf("A addr        = %p, id_rel=%u, id_abs=%u\n",
           (void*)&A,
           ulog_id_rel(&A),
           (unsigned)(((uintptr_t)&A >> 8) & 0xFF));

    printf("B addr        = %p, id_rel=%u, id_abs=%u\n",
           (void*)&B,
           ulog_id_rel(&B),
           (unsigned)(((uintptr_t)&B >> 8) & 0xFF));

    return 0;
}
