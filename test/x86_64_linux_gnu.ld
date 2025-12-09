
/* ulog_linux_rodata.ld */
SECTIONS
{
  /* Create a contiguous, 256-byte aligned .logs section with start/end symbols */
  .logs ALIGN(256) :
  {
    __ulog_logs_start = .;
    KEEP(*(.logs))
    __ulog_logs_end = .;
  }
}
/* Place .logs right after .rodata (RO PT_LOAD); non-PIE/PIE both fine */
INSERT AFTER .rodata
