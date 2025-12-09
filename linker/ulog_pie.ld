/**
 * @file ulog_pie.ld
 * @brief Linker script fragment for PIE/ASLR systems (Linux, modern OSes)
 * 
 * For position-independent executables where .logs section address changes at runtime.
 * Requires subtraction from base address in ulog_id_rel().
 * 
 * Usage: Add to linker flags: -Wl,-T,ulog_pie.ld
 */

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

/* Place .logs right after .rodata (read-only PT_LOAD segment) */
/* Works for both PIE and non-PIE executables */
INSERT AFTER .rodata
