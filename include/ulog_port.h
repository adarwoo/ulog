#pragma once
/**
 * @file ulog_port.h
 * @brief Porting layer for the ulog logging framework.
 */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Private prototypes used by the ULOG framework
// ---------------------------------------------------------------------------

void _ulog_transmit();
void _ulog_init();

#ifdef __cplusplus
}
#endif

// ---------------------------------------------------------------------------
// Start of platform detection
// ---------------------------------------------------------------------------
#undef _ULOG_LOAD_ID

#ifdef __AVR__
   #define _ULOG_LOAD_ID "ldi %A0, hi8(1b)\n\tldi %B0, lo8(1b)"
   #define _ULOG_EMIT_RECORD_PROLOGUE register uint16_t _ulog_index __asm__ ("r24")
#elif defined(__x86_64__) || defined(__amd64__)
   // x86-64 optimization: Use RIP-relative addressing for direct pointer computation
   // This avoids the extern reference and is more efficient on x86-64 PIE/ASLR systems
   // NOTE: .logs has NO "a" flag - it's metadata only, not loaded at runtime
   #define _ULOG_EMIT_RECORD_PROLOGUE const void *_ulog_index
   #define _ULOG_LOAD_ID "leaq 1b(%%rip), %0"
   #define _ULOG_EMIT_RECORD_EPILOGUE uint16_t id = ulog_id_rel(_ulog_index)

   // x86-64 uses subtraction-based ID computation (supports PIE/ASLR)
   static inline uint16_t ulog_id_rel(const void *p) {
      extern const unsigned char __ulog_logs_start[];
      uintptr_t base = (uintptr_t)__ulog_logs_start;
      uintptr_t addr = (uintptr_t)p;
      return (uint16_t)((addr - base) >> 8);
   }
#elif defined(__arm__) && !defined(__aarch64__)
   // ARM32 optimization: Use PC-relative addressing for direct pointer computation
   // Similar to x86-64 but uses ARM's PC-relative addressing
   #define _ULOG_LOAD_ID "adr %0, 1b"
#elif defined(__mips__)
   // MIPS optimization: Use PC-relative addressing via label reference
   #define _ULOG_LOAD_ID "la %0, 1b"
#elif defined(__powerpc__) || defined(__ppc__) || defined(__PPC__)
   // PowerPC optimization: Use PC-relative addressing
   #define _ULOG_LOAD_ID "lis %0, 1b@ha; addi %0, %0, 1b@l"
#elif defined(CONFIG_IDF_TARGET_ESP32) || defined(ESP_PLATFORM)
   // ESP32 (Xtensa) optimization: Use L32R instruction for PC-relative load
   #define _ULOG_LOAD_ID "movi %0, 1b"
#else
#  error "ULog: Unsupported CPU. Please define porting layer."
#endif

#ifndef _ULOG_EMIT_RECORD_PROLOGUE
   #define _ULOG_EMIT_RECORD_PROLOGUE uint16_t _ulog_index
#endif

#ifndef _ULOG_EMIT_RECORD_EPILOGUE
   #define _ULOG_EMIT_RECORD_EPILOGUE uint16_t id = _ulog_index
#endif

// Check for the framework/scheduler being used
#if defined(USE_ASX) || defined(__ASX__)
   #include "ulog_asx_gnu.h"
#elif defined(__linux__)
#  include "ulog_linux_gnu.h"
#elif defined(FREERTOS)
#  include "ulog_freertos_gnu.h"
#else
#    include "ulog_none_gnu.h"
#endif

// ---------------------------------------------------------------------------
// End of platform detection
// ---------------------------------------------------------------------------

/**
 * Log record emission using direct register loading.
 * Only the high byte (bits 8-15) of the label address is needed for the ID.
 * Each invocation gets a unique local label (1:) which is guaranteed unique
 * per inline asm block by the assembler.
 *
 * NOTE: .logs section is metadata-only (parsed from ELF by host tools),
 * so it uses NO "a" flag to prevent it from being counted in device memory.
 */
#define _ULOG_EMIT_RECORD(level, fmt, typecode)                       \
   _ULOG_EMIT_RECORD_PROLOGUE;                                        \
   __asm__ volatile(                                                  \
      ".pushsection .logs,\"\",@progbits\n\t"                         \
      ".balign 256\n\t"                                               \
      "1:\n\t"                                                        \
      ".long %c1\n\t"                                                 \
      ".long %c2\n\t"                                                 \
      ".long %c3\n\t"                                                 \
      ".asciz \"" __FILE__ "\"\n\t"                                   \
      ".asciz \"" fmt "\"\n\t"                                        \
      ".popsection\n\t"                                               \
      _ULOG_LOAD_ID"\n\t"                                             \
      : "=r" (_ulog_index)                                            \
      : "i" ((uint32_t)(level)),                                      \
        "i" ((uint32_t)(__LINE__)),                                   \
        "i" ((uint32_t)(typecode))                                    \
   );                                                                 \
   _ULOG_EMIT_RECORD_EPILOGUE;                                        \


#include "ulog.h"
