unsigned char test_label(void) {
    __asm__ volatile(
        ".pushsection .logs,\"a\",@progbits\n\t"
        ".balign 256\n\t"
        "1:\n\t"
        ".byte 42\n\t"
        ".popsection"
        ::: "memory"
    );
    extern const char label_ref[] asm("1b");
    return (unsigned int)label_ref & 0xFF;
}
