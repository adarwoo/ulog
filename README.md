# ULog - Ultra-Lightweight Logger for Embedded Systems

## Description

ULog is a minimal, portable logging library designed for microcontrollers, bare-metal systems, and resource-constrained environments. It focuses on tiny footprint, predictable runtime performance, and flexible transport backends for real-time and no-OS projects.

### Key Features
- **Extremely small footprint**: Configurable code and data size
- **Simple integration**: Only requires including a linker script fragment - no external build tools
- **Compile-time optimization**: Log level filtering removes unused messages at build time
- **Zero dynamic allocation**: Suitable for static + stack-only environments
- **Platform-portable**: Works on x86-64, AVR, ARM, and other architectures
- **Real-time safe**: Predictable, bounded execution time
- **Type-safe**: Compile-time type checking in both C11 and C++17
- **Rich formatting**: Python f-string style format specifiers
- **Interrupt compatible**: Can be called from interrupt handlers
- **Multiple transports**: UART, ring buffer, memory buffer, or custom backends

### Performance Characteristics

ULOG provides **real-time safe logging** with:

| Feature | Specification |
|---------|---------------|
| ✅ **Ultra-fast execution** | 6.2µs per call @20MHz AVR |
| ✅ **Predictable latency** | Fixed 124-cycle execution path |
| ✅ **Tiny footprint** | 760 bytes flash, 113 bytes RAM |
| ✅ **High throughput** | 161k calls/second capacity |
| ✅ **Printf alternative** | Similar interface, 65-121× faster |

## Quick Start

### C++ Example (Template-based, Type-safe)
```cpp
#include "ulog.h"

int main(void) {
   uint8_t temp_sensor = 42;
   uint16_t x_pos = 100, y_pos = 200;
   
   ULOG_INFO("System initialized");
   ULOG_DEBUG("Temperature: {}", temp_sensor);
   ULOG_WARN("Position: ({},{})", x_pos, y_pos);
   
   while (1) { /* ... */ }
}
```

### C Example (Generic-based, Type-safe)
```c
#include "ulog.h"

int main(void) {
   uint8_t temp_sensor = 42;
   uint16_t x_pos = 100, y_pos = 200;
   
   ULOG_INFO("System initialized");
   ULOG_DEBUG("Temperature: {}", temp_sensor);
   ULOG_WARN("Position: ({},{})", x_pos, y_pos);
   
   while (1) { /* ... */ }
}
```

### Setup

1. **Include the header**:
   ```c
   #include "ulog.h"
   ```

2. **Add linker script fragment**:
   - For x86-64/Linux: `-Wl,-T,ulog_pie.ld`
   - For MCU: Example code to add

3. **Implement platform-specific functions** (see `ulog_port.h`):
   - Critical section (interrupt disable/enable)
   - Transport (UART transmit)
   - Optional: Notification mechanism

That's it! No build tools, no preprocessing steps.

## How It Works

### Compile-Time Metadata

ULog uses a clever approach where **90% of log information is known at compile time**:
- File name and line number
- Format string
- Number and types of arguments

Instead of transmitting all this data, ULog:
1. **Stores metadata in ELF `.logs` section** (not loaded to device)
2. **Transmits only**: 1-byte log ID + 0-4 bytes of variable data
3. **Host tool decodes**: Reads `.logs` section from ELF file to reconstruct full messages

### No Post-Processing Required

Unlike other minimal loggers, ULog requires **no build tool integration**:
- ✅ Uses inline assembly to generate metadata
- ✅ Linker creates the `.logs` section automatically
- ✅ Ready to use immediately after linking


## Performance

### Real-World Benchmarks (20MHz AVR)

| Metric | Value |
|--------|-------|
| Execution time | 124 cycles = **6.2 µs per call** |
| Maximum throughput | **161,290 calls/second** @ 100% CPU |
| Practical rate | **161 calls/millisecond** |

### Comparison to printf()

| Implementation | Time (µs) | Cycles | Speedup |
|----------------|-----------|--------|---------|
| **ULOG** | 6.2 | 124 | Baseline |
| **printf()** | 400-750 | 8,000-15,000 | **65-121× slower** |

### CPU Utilization Examples

| Usage Scenario | Loop Period | CPU Impact | Recommendation |
|----------------|-------------|------------|----------------|
| 1Hz system status | 1000ms | 0.0006% | ✅ Excellent |
| 1kHz control loop | 1ms | 0.62% | ✅ Good |
| 10kHz fast control | 100µs | 6.2% | ⚠️ Moderate |
| 50kHz ultra-fast | 20µs | 31% | ❌ Too heavy |

### UART Throughput

Maximum throughput depends on baud rate:

| Baud Rate | Throughput | Packets/sec |
|-----------|------------|-------------|
| 115,200 | 11.5 KB/s | ~3800 logs/s |

*Assuming 3-byte average packet with COBS framing*

## Architecture

### Double-Buffer System

ULog uses a double-buffer design for non-blocking operation:

```
Application → Circular Buffer → COBS Encoder → TX Buffer → UART/DMA
     ↓              ↓                                ↓
   Returns    Reactor/Idle                    Async Transmit
immediately  Task Processes
```

#### Circular Buffer (Primary)
Stores raw log packets:
```c
struct LogPacket {
    uint8_t payload_len;     // 1-5 bytes total
    uint8_t id;              // Log message ID
    uint8_t data[4];         // Variable data (0-4 bytes)
};
```

#### TX Buffer (Secondary)
COBS-encoded output ready for UART:
- Adds framing (0xA6 end-of-frame marker)
- Escapes zero bytes
- Worst-case overhead: +2 bytes per packet

### Flow Control

1. **Enqueue Phase** (Fast, O(1))
   - Application calls `ULOG_INFO("msg", args...)`
   - Packet stored in circular buffer
   - Notification triggered
   - Returns immediately (non-blocking)

2. **Dequeue Phase** (Idle time)
   - Reactor/idle task triggered
   - Read packet from circular buffer
   - COBS encode into TX buffer
   - Start UART/DMA transmission

3. **Transmission Phase** (Async)
   - UART sends via DMA/interrupt
   - On completion, process next packet
   - Continues until buffer empty

### Key Benefits

- **Real-time Safe**: Logging never blocks on slow UART
- **Burst Handling**: Circular buffer absorbs traffic spikes
- **Reliable Framing**: COBS encoding prevents corruption
- **Efficient**: Only processes during idle time
- **Overrun Protection**: Tracks and reports buffer overflow
- **Minimal Footprint**: No dynamic allocation, configurable sizes

## Platform Support

### Supported Architectures
- ✅ **x86-64** (Linux, PIE/ASLR)
- ✅ **AVR** (ATmega, ATtiny)
- ✅ **ARM Cortex-M** (planned)
- ✅ **RISC-V** (planned)

### Platform-Specific Optimizations
- **x86-64**: RIP-relative addressing for PIE binaries
- **AVR**: Direct address-to-ID computation (no subtraction)
- **Generic**: Subtraction-based ID computation

## Log Levels

```c
ULOG_LEVEL_ERROR    // 0 - Critical errors
ULOG_LEVEL_WARN     // 1 - Warnings
ULOG_LEVEL_MILE     // 2 - Milestones
ULOG_LEVEL_INFO     // 3 - Informational
ULOG_LEVEL_TRACE    // 4 - Trace execution
ULOG_LEVEL_DEBUG0   // 5 - Debug level 0
ULOG_LEVEL_DEBUG1   // 6 - Debug level 1
ULOG_LEVEL_DEBUG2   // 7 - Debug level 2
ULOG_LEVEL_DEBUG3   // 8 - Debug level 3
```

You can set a compile-time level in a file:
```c
#define ULOG_LEVEL ULOG_LEVEL_INFO
#include "ulog.h"
```

You can also override the default by add the pre-processor define ULOG_LEVEL during the compilation:
```
gcc ... -DULOG_LEVEL=ULOG_LEVEL_INFO ...
```

You can also instruct ulog to include a configuration file conf_ulog.h which must be in the -I paths of the compiler by adding the macro
```
gcc ... -DULOG_HAS_CONFIG_FILE=1 -I<Path to conf_ulog.h> ...
```

## Format Specifiers

ULog supports Python f-string style formatting:
```c
ULOG_INFO("Value: {}", value);           // Basic
ULOG_INFO("Hex: {:#04x}", value);        // Hex with 0x prefix
ULOG_INFO("Float: {:.2f}", temperature); // 2 decimal places
ULOG_INFO("Percent: {:.1%}", ratio);     // Percentage
```

## Configuration

In `conf_ulog.h` or build flags:

```c
// Set log level (removes lower-level logs at compile time)
#define ULOG_LEVEL ULOG_LEVEL_INFO

// Circular buffer depth
#define ULOG_QUEUE_DEPTH 32

// Maximum log payload size
#define ULOG_MAX_PAYLOAD 5
```

## License

MIT

## Contributing

Welcome

## See Also

- [Porting Guide](porting_guide.md) - Detailed platform porting instructions
- Examples in `test/` directory
