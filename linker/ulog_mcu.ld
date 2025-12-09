/**
 * @file ulog_mcu.ld
 * @brief Linker script fragment for bare-metal microcontrollers
 * 
 * For systems with fixed addresses (AVR, ARM Cortex-M, RISC-V, etc.)
 * The .logs section contains metadata only (not stored in flash/ROM).
 * 
 * Usage: Add to linker flags: -Wl,-T,ulog_mcu.ld
 * 
 * This fragment defines its own LOGMETA memory region, so it works
 * without modifying your MCU's existing linker script.
 * 
 * Adjust ORIGIN to a suitable address for your target:
 * - AVR: Use high address beyond program memory
 * - ARM: Use address beyond flash/RAM regions
 * - RISC-V: Use address in unused memory space
 */

/* Define metadata-only memory region (not loaded to flash) */
MEMORY
{
  LOGMETA (r) : ORIGIN = 0x00810000, LENGTH = 64K
}

SECTIONS
{
  /* Create a contiguous, 256-byte aligned .logs section with start/end symbols */
  .logs ALIGN(256) :
  {
    __ulog_logs_start = .;
    KEEP(*(.logs))
    __ulog_logs_end = .;
  } > LOGMETA
}
