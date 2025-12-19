# Porting Guide for ulog

## Overview

The ulog framework is designed for portability across different platforms (AVR, ARM, x86_64, etc.) and operating systems. This guide explains how to port ulog to a new platform.

## Platform Naming Convention

To ensure consistency across all supported platforms, use this standardized naming convention:

**Format:** `<os>_<abi>[_<vendor>]`

### Components:

* **os**: Operating system or runtime environment  
  Examples: `linux`, `baremetal`, `windows`, `freertos`, `asx`
* **abi**: Application Binary Interface or libc variant  
  Examples: `gnueabi`, `gnu`, `eabi`, `mingw`
* **vendor** (optional): Board or toolchain vendor  
  Examples: `nvidia`, `raspberry`, `nxp`

### Examples

* `baremetal_eabi`
* `linux_gnueabi`
* `asx_gnu`

### Rules

* Use lowercase for all components
* Separate components with underscores (`_`)
* Include arch, os, and abi at minimum
* Add vendor only if necessary to distinguish variants
* Avoid distribution-specific names unless required

## Porting Steps

### 1. Create OS specific Header

Create a new header file named `ulog_<platform_name>.h` in the `include/` directory.

Example: `ulog_mysuperos_gnu.h`

### 2. Define Platform Macros

Your platform header must define the following macros:

```c
/**
 * Enter critical section (disable interrupts)
 */
#define _ULOG_PORT_ENTER_CRITICAL_SECTION() \
    <YOUR IMPLEMENTATION>

/**
 * Exit critical section (restore interrupts)
 */
#define _ULOG_PORT_EXIT_CRITICAL_SECTION() \
    <YOUR IMPLEMENTATION>

/**
 * Notify transmitter that log data is available (optional)
 */
#define _ULOG_PORT_NOTIFY() \
    <YOUR IMPLEMENTATION>

/**
 * Send data over the transport (UART, USB, etc.)
 */
#define _ULOG_PORT_SEND_DATA(tx_encoded, encoded_len) \
    <YOUR IMPLEMENTATION>

/**
 * Check if transmitter is ready
 */
#define _ULOG_UART_TX_READY() \
    <YOUR IMPLEMENTATION>
```

### 4. Add CPU Detection

The file ulog_port.h takes care of the CPU specific part used by ulog. This part allows packing the log data into the .logs sections.
The packing macro use assembly language for an optimum code generation, but requires a CPU specific instruction to move the ID into
a register of the CPU.

This can be done by setting the macro:
   #define _ULOG_LOAD_ID "leaq 1b(%%rip), %0"

Optionally, for complex cases like with Intel CPU using PIE and ASLR, you can specify:
   #define _ULOG_EMIT_RECORD_PROLOGUE const void *_ulog_index
   #define _ULOG_EMIT_RECORD_EPILOGUE uint8_t id = ulog_id_rel(_ulog_index)

### 5. Create Linker Script

The linker must create a `.logs` section to store log metadata. Pre-configured scripts are available in the `linker/` directory.

#### Option A: Use Pre-configured Scripts

**For PIE/ASLR systems (Linux, macOS, etc.):**
```bash
# Add to your build flags
LDFLAGS += -Wl,-T,linker/ulog_pie.ld
```

**For bare-metal microcontrollers (AVR, ARM, RISC-V, etc.):**
```bash
# Add to your build flags
LDFLAGS += -Wl,-T,linker/ulog_mcu.ld
```

The `ulog_mcu.ld` script defines a `LOGMETA` memory region at `0x00810000`. Adjust the `ORIGIN` address if needed for your target.

#### Option B: Manual Linker Script Integration

For **AVR** or embedded platforms with fixed memory layout:

```ld
MEMORY
{
  text   (rx)   : ORIGIN = 0x0000, LENGTH = 0x10000
  data   (rw!x) : ORIGIN = 0x800000, LENGTH = 0x2000
  LOGMETA (r)   : ORIGIN = 0x00810000, LENGTH = 64K  /* Metadata only, not loaded */
}

SECTIONS
{
  .logs ALIGN(256) :
  {
    __ulog_logs_start = .;
    KEEP(*(.logs))
    __ulog_logs_end = .;
  } > LOGMETA
}
```

For **Linux** or platforms using standard ELF layout:

```ld
SECTIONS
{
  .logs ALIGN(256) :
  {
    __ulog_logs_start = .;
    KEEP(*(.logs))
    __ulog_logs_end = .;
  }
}
INSERT AFTER .rodata
```

### 6. Implement Platform-Specific Functions

Create a `.c` or `.cpp` file to implement the platform initialization:

```c
#include "ulog_port.h"

void _ulog_init() {
    // Initialize UART, USB, or other transport
    // Set up transmitter thread/task if needed
}

void _ulog_transmit() {
    // Flush pending logs to the transport
    // This is called when logs are ready to send
}

void _ulog_platform_send_data(const uint8_t *data, size_t len) {
    // Low-level transmit function
    // Send bytes to UART, USB, network, etc.
}
```

### 7. Enable Compiler Flags

For proper operation, compile with `-fdata-sections`:

```makefile
CFLAGS += -fdata-sections
CXXFLAGS += -fdata-sections
```

This ensures that function-local static log records are placed in the `.logs` section correctly.

**For questions or issues, please refer to the main README.md or open an issue on the project repository.**

