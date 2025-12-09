#include "ulog.h"
#include <cstdio>

int main() {
    extern const unsigned char __ulog_logs_start[];
    printf("__ulog_logs_start runtime = %p\n", (void*)__ulog_logs_start);
    
    ULOG_INFO("Hello, ULog!");
    return 0;
}
