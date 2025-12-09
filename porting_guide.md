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
#define _ULOG_ID_REL_DEFINED
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

**Key Points:**
* Align to 256 bytes (required for ID computation)
* Use `KEEP()` to prevent linker garbage collection
* Define `__ulog_logs_start` and `__ulog_logs_end` symbols
* For MCUs: `.logs` goes to `LOGMETA` (metadata only, not burned to flash)
* For PIE systems: `.logs` is placed after `.rodata` in the executable

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

### AVR Example (Fixed Addresses)

See `include/ulog_avr_none_gnu.h` and `src/ulog_avr_none_gnu.cpp`

**Key features:**
- Uses optimized `ulog_id_rel` (no subtraction - saves 1 instruction!)
- Direct address bit extraction for log IDs
- Polling UART at 115200 baud
- No OS - minimal bare metal implementation

**Linker setup:**
```bash
LDFLAGS += -Wl,-T,linker/ulog_mcu.ld
```

### Linux Example (PIE/ASLR)

See `include/ulog_linux_gnu.h` and `src/ulog_linux.cpp`

**Key features:**
- Uses default `ulog_id_rel` (with subtraction for PIE compatibility)
- pthread mutex/condition for thread safety
- Background transmitter thread
- Constructor/destructor for automatic init/cleanup

**Linker setup:**
```bash
LDFLAGS += -Wl,-T,linker/ulog_pie.ld
```

### FreeRTOS Example (RTOS)

See `include/ulog_freertos_gnu.h` and `src/ulog_freertos.cpp`

**Key features:**
- Event groups for notification (instead of condition variables)
- Transmits logs during idle task execution
- Critical sections using `taskENTER_CRITICAL/taskEXIT_CRITICAL`
- Requires `configUSE_IDLE_HOOK` enabled in FreeRTOSConfig.h

**Linker setup:**
```bash
LDFLAGS += -Wl,-T,linker/ulog_mcu.ld  # Or ulog_pie.ld for systems with MMU
```

## Testing Your Port

1. **Compile with your platform toolchain**
   ```bash
   # Example for AVR
   avr-gcc -mmcu=atmega328p -fdata-sections -Wl,-T,linker/ulog_mcu.ld ...
   
   # Example for Linux
   gcc -fdata-sections -Wl,-T,linker/ulog_pie.ld ...
   ```

2. **Verify `.logs` section exists**
   ```bash
   objdump -h yourapp.elf | grep .logs
   # Should show: .logs with 256-byte alignment
   ```

3. **Check symbol addresses**
   ```bash
   nm yourapp.elf | grep __ulog
   # Should show: __ulog_logs_start and __ulog_logs_end
   ```

4. **Inspect log records**
   ```bash
   objdump -s -j .logs yourapp.elf
   # Should show 256-byte aligned records with visible strings
   ```

5. **Test with simple log**
   ```c
   ULOG_INFO("Hello from %s!", "yourplatform");
   ULOG_DEBUG("Test value: %d", 42);
   ```

6. **Verify output on your transport**
   - UART: Check serial monitor
   - USB: Check USB terminal
   - Network: Check network logs

7. **Check code generation efficiency** (optional)
   ```bash
   # Disassemble to verify minimal instructions
   objdump -d -M intel yourapp.elf | less
   # Look for ULOG calls - should be very few instructions
   ```

## Troubleshooting

### Log IDs are not continuous
- **Cause**: Records not properly aligned or scattered across memory
- **Fix**: 
  - Ensure `-fdata-sections` is enabled
  - Check linker script includes `KEEP(*(.logs))`
  - Verify alignment is exactly 256 bytes
  - For PIE systems, use `ulog_pie.ld`

### Logs not appearing
- **Cause**: Transmitter not being triggered or transport not ready
- **Fix**:
  - Check `_ULOG_PORT_NOTIFY()` is called (add debug print)
  - Verify `_ULOG_UART_TX_READY()` returns true
  - Ensure `_ulog_transmit()` is being invoked
  - For FreeRTOS: verify `configUSE_IDLE_HOOK` is enabled
  - For Linux: check pthread thread is running

### Compilation errors about `__ulog_logs_start`
- **Cause**: Linker script not being used or symbols not defined
- **Fix**:
  - Verify linker script is being used (`-Wl,-T,linker/ulog_mcu.ld`)
  - Check that linker script defines `__ulog_logs_start` and `__ulog_logs_end`
  - Ensure linker script is in the correct path

### Wrong log IDs or crashes
- **Cause**: Using wrong `ulog_id_rel` for your platform
- **Fix**:
  - **PIE/ASLR systems**: Use default (with subtraction)
  - **Fixed address systems**: Define `_ULOG_ID_REL_DEFINED` and override
  - Verify with `objdump -s -j .logs` that records are 256 bytes apart

### `.logs` section too large or not loading
- **Cause**: `.logs` being placed in flash/ROM
- **Fix**:
  - For MCUs: Use `ulog_mcu.ld` which places `.logs` in `LOGMETA`
  - Verify `LOGMETA` memory region is defined correctly
  - `.logs` should NOT be in a loadable segment (AT directive)

### Struct size not exactly 256 bytes
- **Cause**: Compiler adding padding
- **Fix**:
  - Struct already uses `__attribute__((packed, aligned(256)))`
  - Verify with: `sizeof(struct ulog_record)` should be 256
  - Check field sizes: 1 (level) + 4 (line) + 4 (typecode) + 116 (file) + 128 (fmt) + 3 (padding) = 256

## Reference Implementation

For complete working examples, refer to:

### Bare Metal (Fixed Addresses)
- **Header**: `include/ulog_avr_none_gnu.h`
- **Implementation**: `src/ulog_avr_none_gnu.cpp`
- **Linker**: `linker/ulog_mcu.ld`
- **Features**: Optimized ID computation, polling UART, no OS overhead

### Linux (PIE/ASLR)
- **Header**: `include/ulog_linux_gnu.h`
- **Implementation**: `src/ulog_linux.cpp`
- **Linker**: `linker/ulog_pie.ld`
- **Features**: pthread synchronization, background thread, PIE-compatible

### FreeRTOS (RTOS)
- **Header**: `include/ulog_freertos_gnu.h`
- **Implementation**: `src/ulog_freertos.cpp`
- **Linker**: `linker/ulog_mcu.ld` (or `ulog_pie.ld` for MMU systems)
- **Features**: Event groups, idle hook transmission, RTOS integration

## Advanced Topics

### Custom Log Record Structure

The log record is defined as a packed struct with fixed-size arrays:

```c
struct ulog_record {
    uint8_t level;          // 1 byte: log level
    uint32_t line;          // 4 bytes: line number
    uint32_t typecode;      // 4 bytes: format type encoding
    char file[116];         // 116 bytes: source file name
    char fmt[128];          // 128 bytes: format string
    uint8_t _pad[3];        // 3 bytes: padding to 256 bytes
} __attribute__((packed, aligned(256)));
```

Total size: **exactly 256 bytes** for efficient ID computation.

### Performance Characteristics

**Code generation** (per log call):
- **AVR**: ~6 instructions, 1 for ID calculation
- **x86_64**: ~10 instructions, 3 for ID calculation
- **ARM**: Similar to x86_64

**Memory overhead**:
- 256 bytes per unique log statement (in `.logs` metadata section)
- Runtime: Ring buffer in RAM (configurable size)

**Runtime cost**:
- Function call overhead
- Minimal arithmetic (shift, mask, optional subtract)
- No string formatting at log site (deferred to decoder)

### Integration with Build Systems

#### CMake Example
```cmake
target_compile_options(myapp PRIVATE -fdata-sections)
target_link_options(myapp PRIVATE 
    -Wl,-T,${CMAKE_SOURCE_DIR}/linker/ulog_mcu.ld
)
```

#### Makefile Example
```makefile
CFLAGS += -fdata-sections
LDFLAGS += -Wl,-T,linker/ulog_mcu.ld
```

#### Platform.io Example
```ini
[env:myboard]
build_flags = 
    -fdata-sections
    -Wl,-T,linker/ulog_mcu.ld
```

---

**For questions or issues, please refer to the main README.md or open an issue on the project repository.**

