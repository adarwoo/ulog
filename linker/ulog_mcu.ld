/**
 * @file ulog_mcu.ld
 * @brief Linker script fragment for bare-metal microcontrollers
 * 
 * For systems with fixed addresses (AVR, ARM Cortex-M, RISC-V, etc.)
 * The .logs section contains metadata only (not stored in flash/ROM).
 * 
 * Usage: Include this fragment: -Wl,-T,ulog_mcu.ld
 * 
 * This is a standalone fragment - NO patching of your main linker script needed!
 * 
 * Places .logs at address 0x000000 which is:
 * - 256-byte aligned (perfect for ID extraction)
 * - Simple: first log at 0x000000, second at 0x000100, etc.
 * - No conflicts: .logs is non-allocatable metadata, won't interfere with .text
 * - Universal: works on all MCU architectures
 * 
 * The .logs section is pure metadata - it exists only in the ELF file for
 * host-side tools to parse. It's never programmed to the device.
 */

<<<<<<< HEAD
/* Define metadata-only memory region (not loaded to flash) */
MEMORY
{
  logmeta (r) : ORIGIN = 0x00010000, LENGTH = 64K
}

=======
>>>>>>> f4fe48dbff0f89cc808b133de05df5e509d854b5
SECTIONS
{
  /* Place .logs at address 0 for simplest ID computation */
  /* Log ID = (record_address >> 8) */
  /* Record 0 at 0x000000, record 1 at 0x000100, record 2 at 0x000200, etc. */
  .logs 0x000000 :
  {
    __ulog_logs_start = .;
    KEEP(*(.logs))
    __ulog_logs_end = .;
<<<<<<< HEAD
  } > logmeta
=======
  }
>>>>>>> f4fe48dbff0f89cc808b133de05df5e509d854b5
}
