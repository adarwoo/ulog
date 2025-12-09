# Porting Guide for ulog

## Overview

The ulog framework is designed for portability across different platforms (AVR, ARM, x86_64, etc.) and operating systems. This guide explains how to port ulog to a new platform.

## Platform Naming Convention

To ensure consistency across all supported platforms, use this standardized naming convention:

**Format:** `<arch>_<os>_<abi>[_<vendor>]`

### Components:

* **arch**: CPU architecture  
  Examples: `avr`, `arm`, `aarch64`, `x86_64`, `riscv64`
* **os**: Operating system or runtime environment  
  Examples: `linux`, `baremetal`, `windows`, `freertos`, `asx`
* **abi**: Application Binary Interface or libc variant  
  Examples: `gnueabi`, `gnu`, `eabi`, `mingw`
* **vendor** (optional): Board or toolchain vendor  
  Examples: `nvidia`, `raspberry`, `nxp`

### Examples

* `arm_baremetal_eabi`
* `arm_linux_gnueabi`
* `aarch64_linux_gnu`
* `x86_64_linux_gnu`
* `avr_asx_gnu`

### Rules

* Use lowercase for all components
* Separate components with underscores (`_`)
* Include arch, os, and abi at minimum
* Add vendor only if necessary to distinguish variants
* Avoid distribution-specific names unless required

## Porting Steps

### 1. Create Platform Header

Create a new header file named `ulog_<platform_name>.h` in the `include/` directory.

Example: `ulog_avr_asx_gnu.h`

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

### 3. (Optional) Optimize ID Computation

For platforms with **fixed addresses** (no PIE/ASLR), you can override `ulog_id_rel` for better performance:

```c
/**
 * Optimized ID computation for fixed-address platforms.
 * This version omits the subtraction, saving one instruction.
 * Only use this if your platform doesn't use position-independent code.
 */
#define ULOG_CUSTOM_ID_REL
static inline uint8_t ulog_id_rel(const void *p) {
   uintptr_t addr = (uintptr_t)p;
   return (uint8_t)((addr >> 8) & 0xFF);
}
```

**Note:** Platforms with PIE/ASLR (like Linux) should use the default implementation in `ulog.h`.

### 4. Add Platform Detection

Edit `ulog_port.h` to add automatic detection of your platform:

```c
#ifdef __AVR__
#  include "ulog_avr_asx_gnu.h"
#elif defined(__linux__)
#  include "ulog_linux_gnu.h"
#elif defined(__YOUR_PLATFORM__)
#  include "ulog_<your_platform_name>.h"
#else
#  error "ULog: Unsupported platform. Please define porting layer."
#endif
```

### 5. Create Linker Script

The linker must create a `.logs` section to store log metadata.

#### Example Linker Script Fragment:

For **AVR** or embedded platforms with fixed memory layout:

```ld
MEMORY
{
  text   (rx)   : ORIGIN = 0x0000, LENGTH = 0x10000
  data   (rw!x) : ORIGIN = 0x800000, LENGTH = 0x2000
  logmeta (r)   : ORIGIN = 0x10000, LENGTH = 0x10000  /* 64KB for logs */
}

SECTIONS
{
  .logs ALIGN(256) :
  {
    __ulog_logs_start = .;
    KEEP(*(.logs))
    __ulog_logs_end = .;
  } > logmeta
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
    KEEP(*(.logs.*))
    __ulog_logs_end = .;
  }
}
INSERT AFTER .rodata
```

**Key Points:**
* Align to 256 bytes (required for ID computation)
* Use `KEEP()` to prevent linker garbage collection
* Define `__ulog_logs_start` and `__ulog_logs_end` symbols
* For `-fdata-sections`, also include `KEEP(*(.logs.*))`

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

## Platform Examples

### AVR Example

See `include/ulog_avr_asx_gnu.h` and `src/ulog_avr_asx_gnu.cpp`

**Key features:**
- Uses optimized `ulog_id_rel` (no subtraction)
- Interrupt-based critical sections
- Integration with reactor framework

### Linux Example

See `include/ulog_linux_gnu.h` and `src/ulog_linux.cpp`

**Key features:**
- Uses default `ulog_id_rel` (with subtraction for PIE)
- pthread mutex for thread safety
- Background transmitter thread

## Testing Your Port

1. Compile with your platform toolchain
2. Verify `.logs` section exists: `objdump -h yourapp.elf | grep .logs`
3. Check symbol addresses: `nm yourapp.elf | grep __ulog`
4. Test with simple log: `ULOG_INFO("Hello from %s!", "yourplatform");`
5. Verify output on your transport (UART, USB, etc.)

## Troubleshooting

### Log IDs are not continuous
- Ensure `-fdata-sections` is enabled
- Check linker script includes `KEEP(*(.logs))` and `KEEP(*(.logs.*))`
- Verify alignment is 256 bytes

### Logs not appearing
- Check `_ULOG_PORT_NOTIFY()` is called
- Verify `_ULOG_UART_TX_READY()` returns true
- Ensure `_ulog_transmit()` is being invoked

### Compilation errors about `__ulog_logs_start`
- Verify linker script defines these symbols
- Check that linker script is being used (`-T` or `-Wl,-T,script.ld`)

## Reference Implementation

For a complete example, refer to:
- **AVR**: `include/ulog_avr_asx_gnu.h`
- **Linux**: `include/ulog_linux_gnu.h`, `src/ulog_linux.cpp`

