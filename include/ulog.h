#pragma once
/**
 * @file ulog.h
 * @brief C-compatible Ultra-lightweight logging framework for embedded systems.
 *
 * This header defines a compile-time evaluated logging system designed for
 * memory-constrained embedded environments. It embeds log metadata into custom
 * ELF sections (.logs) while emitting runtime log payloads via register-based
 * emission routines. Format strings and type signatures are preserved at
 * compile time and can be indexed offline using the binary metadata.
 *
 * Features:
 * - Zero runtime format string overhead
 * - Compile-time argument encoding and dispatch
 * - Interrupt compatible
 * - Packed binary logs with 0/1/2/4-byte payloads
 * - Auto-tagged log level and type signature
 * - Inline assembly generates .logs metadata per callsite
 * - Messages are sent over a UART using COBS encoding
 *
 * Usage:
 * ```c
 * #include "ulog.h"
 *
 * // Simple log, no args
 * ULOG(ULOG_LEVEL_INFO, "Starting up...");
 * // or
 * ULOG_INFO("Starting up...");
 * // 2 bytes, packed from two uint8_t values. Use Python f-string style {} for formatting
 * uint8_t x = 10, y = 20;
 * ULOG_WARN("Pos:", x, y); // No formatting required
 * ULOG_WARN("Pos: ({.2%},{})", x, y); // Formatting supported
 *
 * // 4 bytes, float packed as IEEE 754
 * float temperature = 36.7f;
 * ULOG_INFO("Temp: {.<4f}", temperature);
 * ```
 *
 * @note Only types up to 4 bytes total are supported per log. Format strings must be literals.
 * @note The `.logs` section can be parsed from the ELF to map runtime packets back to messages.
 * @note You are limited to 255 messages per application
 * @note This header is C and C++ compatible, each language use an optimized version - more type safe for C++
 * @note This header requires C11 or later for _Generic support and C++17 or later for template features.
 *
 * @author software@arreckx.com
 */
#include <stdint.h>
#include "ulog_port.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Record structure for log metadata.
 * Fixed-size arrays ensure all data is embedded in .logs section.
 * Each record is exactly 256 bytes:
 * - 1 byte: level
 * - 3 bytes: padding (compiler alignment)
 * - 4 bytes: line
 * - 4 bytes: typecode  
 * - 116 bytes: file path
 * - 128 bytes: format string
 * Total: 256 bytes
 */
struct ulog_record {
   uint8_t  level;
   uint32_t line;
   uint32_t typecode;
   char file[116];
   char fmt[128];
} __attribute__((packed, aligned(256)));

// ============================================================================
// Forwarding prototypes
// ============================================================================
void ulog_detail_enqueue(uint8_t id);
void ulog_detail_enqueue_1(uint8_t id, uint8_t v0);
void ulog_detail_enqueue_2(uint8_t id, uint8_t v0, uint8_t v1);
void ulog_detail_enqueue_3(uint8_t id, uint8_t v0, uint8_t v1, uint8_t v2);
void ulog_detail_enqueue_4(uint8_t id, uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3);
void ulog_flush(void);

#ifdef __cplusplus
}
#endif

// ============================================================================
// MACRO Compatible levels
// ============================================================================
#define ULOG_LEVEL_ERROR    0
#define ULOG_LEVEL_WARN     1
#define ULOG_LEVEL_MILE     2
#define ULOG_LEVEL_INFO     3
#define ULOG_LEVEL_TRACE    4
#define ULOG_LEVEL_DEBUG0   5
#define ULOG_LEVEL_DEBUG1   6
#define ULOG_LEVEL_DEBUG2   7
#define ULOG_LEVEL_DEBUG3   8

// ============================================================================
// C type codes
// ============================================================================
#define ULOG_TRAIT_ID_NONE  0x00L
#define ULOG_TRAIT_ID_U8    0x10L
#define ULOG_TRAIT_ID_S8    0x11L
#define ULOG_TRAIT_ID_BOOL  0x12L
#define ULOG_TRAIT_ID_U16   0x20L
#define ULOG_TRAIT_ID_S16   0x21L
#define ULOG_TRAIT_ID_PTR   0x22L
#define ULOG_TRAIT_ID_U32   0x40L
#define ULOG_TRAIT_ID_S32   0x41L
#define ULOG_TRAIT_ID_FLOAT 0x42L
#define ULOG_TRAIT_ID_STR4  0x43L

// ============================================================================
// Helpers to generate .logs section and get ID
// ============================================================================

/**
 * Emit a log record directly into the .logs section using inline assembly.
 * 
 * This generic implementation creates .logs section entries but DOES NOT
 * provide a way to get the record address portably. Each platform MUST
 * override this macro with architecture-specific address computation.
 * 
 * Platform implementations:
 * - x86-64: Uses RIP-relative LEA (see ulog_linux_gnu.h)
 * - AVR: Uses LPM/LDS with program memory addressing (see ulog_avr_asx_gnu.h)  
 * - ARM: Uses PC-relative addressing
 * 
 * Layout (256 bytes total):
 * - Byte 0: level (1 byte)
 * - Bytes 1-4: line number (4 bytes)
 * - Bytes 5-8: typecode (4 bytes)
 * - Bytes 9+: file path (null-terminated, packed)
 * - Next: format string (null-terminated, packed immediately after file)
 * 
 * IMPORTANT: Format strings containing % must escape them as %% since
 * they are embedded in inline assembly.
 */
#ifndef _ULOG_EMIT_RECORD
#  error "Platform must define _ULOG_EMIT_RECORD with architecture-specific address computation"
#  define _ULOG_EMIT_RECORD(level, fmt, typecode)                     \
   __asm__ volatile(                                                   \
      ".pushsection .logs,\"a\",@progbits\n\t"                         \
      ".balign 256\n\t"                                                \
      "1:\n\t"                                                         \
      ".byte %c0\n\t"                                                  \
      ".long %c1\n\t"                                                  \
      ".long %c2\n\t"                                                  \
      ".asciz \"" __FILE__ "\"\n\t"                                    \
      ".asciz \"" fmt "\"\n\t"                                         \
      ".popsection"                                                    \
      : /* no outputs */                                               \
      : "i" ((uint8_t)(level)),                                        \
        "i" ((uint32_t)(__LINE__)),                                    \
        "i" ((uint32_t)(typecode))                                     \
   );                                                                  \
   const uint8_t id = 0; /* ERROR: No portable way to get address! */
#endif


// This section creates the macros for C usage
// C++ users should use the templated version in ulog.hpp which is more type-safe
#ifndef __cplusplus

// ============================================================================
// Static inline runtime dispatch functions
// ============================================================================

// ----------------------------------------------------------------------------
// inline helpers to enqueue all possible combinations based on the number and types
// ----------------------------------------------------------------------------
static inline void _ulog_dispatch_0(uint8_t id) {
    ulog_detail_enqueue(id);
}

static inline void _ulog_dispatch_1_u8(uint8_t id, uint8_t a) {
    ulog_detail_enqueue_1(id, a);
}

static inline void _ulog_dispatch_1_u16(uint8_t id, uint16_t a) {
    ulog_detail_enqueue_2(id, (uint8_t)(a), (uint8_t)(a >> 8));
}

static inline void _ulog_dispatch_1_u32(uint8_t id, uint32_t a) {
    ulog_detail_enqueue_4(id,
        (uint8_t)(a),
        (uint8_t)(a >> 8),
        (uint8_t)(a >> 16),
        (uint8_t)(a >> 24));
}

static inline void _ulog_dispatch_1_float(uint8_t id, float a) {
    union { float f; uint32_t u; } conv = { .f = a };
    ulog_detail_enqueue_4(id,
        (uint8_t)(conv.u),
        (uint8_t)(conv.u >> 8),
        (uint8_t)(conv.u >> 16),
        (uint8_t)(conv.u >> 24));
}

static inline void _ulog_dispatch_2_u8_u8(uint8_t id, uint8_t a, uint8_t b) {
    ulog_detail_enqueue_2(id, a, b);
}

static inline void _ulog_dispatch_2_u8_u16(uint8_t id, uint8_t a, uint16_t b) {
    ulog_detail_enqueue_3(id, a, (uint8_t)(b&0xFF), (uint8_t)(b >> 8));
}

static inline void _ulog_dispatch_2_u16_u8(uint8_t id, uint16_t a, uint8_t b) {
    ulog_detail_enqueue_3(id, (uint8_t)(a&0xFF), (uint8_t)(a >> 8), b);
}

static inline void _ulog_dispatch_2_u16_u16(uint8_t id, uint16_t a, uint16_t b) {
    ulog_detail_enqueue_4(id,
        (uint8_t)(a&0xFF), (uint8_t)(a >> 8),
        (uint8_t)(b&0xFF), (uint8_t)(b >> 8));
}

static inline void _ulog_dispatch_3_u8_u8_u8(uint8_t id, uint8_t a, uint8_t b, uint8_t c) {
    ulog_detail_enqueue_3(id, a, b, c);
}

static inline void _ulog_dispatch_3_u8_u16(uint8_t id, uint8_t a, uint16_t b) {
    ulog_detail_enqueue_3(id, a, (uint8_t)(b&0xFF), (uint8_t)(b >> 8));
}

static inline void _ulog_dispatch_3_u16_u8(uint8_t id, uint16_t a, uint8_t b) {
    ulog_detail_enqueue_3(id, (uint8_t)(a&0xFF), (uint8_t)(a >> 8), b);
}

static inline void _ulog_dispatch_4_u8_u8_u8_u8(uint8_t id, uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    ulog_detail_enqueue_4(id, a, b, c, d);
}

// ============================================================================
// Compile time type code extraction helper
// Given a type x, return the corresponding ULOG_TRAIT_ID_XXX value
// ============================================================================
#define _ULOG_TC(x) _Generic((x), \
   uint8_t:  ULOG_TRAIT_ID_U8, \
   int8_t:   ULOG_TRAIT_ID_S8, \
   _Bool:    ULOG_TRAIT_ID_BOOL, \
   uint16_t: ULOG_TRAIT_ID_U16, \
   int16_t:  ULOG_TRAIT_ID_S16, \
   uint32_t: ULOG_TRAIT_ID_U32, \
   int32_t:  ULOG_TRAIT_ID_S32, \
   float:    ULOG_TRAIT_ID_FLOAT, \
   double:   ULOG_TRAIT_ID_FLOAT, \
   default:  ULOG_TRAIT_ID_NONE)

// Helper macros that generate ID first, then call the dispatch function
#define _ULOG_SELECT_0(level, fmt) \
   do { \
      _ULOG_GENERATE_LOG_ID(N, level, fmt, 0) \
      _ulog_dispatch_0(id); \
   } while(0)

// ============================================================================
// Simplified dispatch macros based only on argument count
// The expansion yields a compile-time evaluated typecode and calls the appropriate
// dispatcher function.
// This relies on compiler support for C11 _Generic. The final resolution happens
// at compile time, so there is no runtime overhead.
// ============================================================================

#define _ULOG_DISPATCH_0(level, fmt) \
   do { \
      _ULOG_EMIT_RECORD(level, fmt, 0) \
      _ulog_dispatch_0(id); \
   } while(0)

#define _ULOG_DISPATCH_1(level, fmt, a) \
   do { \
      _ULOG_EMIT_RECORD(level, fmt, _ULOG_TC(a)) \
      _Generic((a), \
         uint8_t:  _ulog_dispatch_1_u8(id, a), \
         int8_t:   _ulog_dispatch_1_u8(id, a), \
         _Bool:    _ulog_dispatch_1_u8(id, a), \
         uint16_t: _ulog_dispatch_1_u16(id, a), \
         int16_t:  _ulog_dispatch_1_u16(id, a), \
         uint32_t: _ulog_dispatch_1_u32(id, a), \
         int32_t:  _ulog_dispatch_1_u32(id, a), \
         float:    _ulog_dispatch_1_float(id, a), \
         double:   _ulog_dispatch_1_float(id, a)); \
   } while(0)

#define _ULOG_DISPATCH_2(level, fmt, a, b) \
   do { \
      _ULOG_EMIT_RECORD(level, fmt, ((_ULOG_TC(b) << 8) | _ULOG_TC(a))) \
      _Generic((a), \
         uint8_t: _Generic((b), \
            uint8_t:  _ulog_dispatch_2_u8_u8( id, a, b), \
            int8_t:   _ulog_dispatch_2_u8_u8( id, a, b), \
            _Bool:    _ulog_dispatch_2_u8_u8( id, a, b), \
            uint16_t: _ulog_dispatch_2_u8_u16(id, a, b), \
            int16_t:  _ulog_dispatch_2_u8_u16(id, a, b), \
            default:  _ulog_dispatch_2_u8_u8( id, a, b)), \
         int8_t: _Generic((b), \
            uint8_t:  _ulog_dispatch_2_u8_u8( id, a, b), \
            int8_t:   _ulog_dispatch_2_u8_u8( id, a, b), \
            _Bool:    _ulog_dispatch_2_u8_u8( id, a, b), \
            uint16_t: _ulog_dispatch_2_u8_u16(id, a, b), \
            int16_t:  _ulog_dispatch_2_u8_u16(id, a, b), \
            default:  _ulog_dispatch_2_u8_u8( id, a, b)), \
         _Bool: _Generic((b), \
            uint8_t:  _ulog_dispatch_2_u8_u8( id, a, b), \
            int8_t:   _ulog_dispatch_2_u8_u8( id, a, b), \
            _Bool:    _ulog_dispatch_2_u8_u8( id, a, b), \
            uint16_t: _ulog_dispatch_2_u8_u16(id, a, b), \
            int16_t:  _ulog_dispatch_2_u8_u16(id, a, b), \
            default:  _ulog_dispatch_2_u8_u8( id, a, b)), \
         uint16_t: _Generic((b), \
            uint8_t:  _ulog_dispatch_2_u16_u8( id, a, b), \
            int8_t:   _ulog_dispatch_2_u16_u8( id, a, b), \
            _Bool:    _ulog_dispatch_2_u16_u8( id, a, b), \
            uint16_t: _ulog_dispatch_2_u16_u16(id, a, b), \
            int16_t:  _ulog_dispatch_2_u16_u16(id, a, b), \
            default:  _ulog_dispatch_2_u16_u8( id, a, b)), \
         int16_t: _Generic((b), \
            uint8_t:  _ulog_dispatch_2_u16_u8( id, a, b), \
            int8_t:   _ulog_dispatch_2_u16_u8( id, a, b), \
            _Bool:    _ulog_dispatch_2_u16_u8( id, a, b), \
            uint16_t: _ulog_dispatch_2_u16_u16(id, a, b), \
            int16_t:  _ulog_dispatch_2_u16_u16(id, a, b), \
            default:  _ulog_dispatch_2_u16_u8( id, a, b)), \
         default: _ulog_dispatch_2_u8_u8(id, a, b)); \
   } while(0)

#define _ULOG_DISPATCH_3(level, fmt, a, b, c) \
   do { \
      _ULOG_EMIT_RECORD(level, fmt, ((_ULOG_TC(c) << 16) | (_ULOG_TC(b) << 8) | _ULOG_TC(a))) \
      _Generic((a), \
         uint8_t: _Generic((b), \
            uint8_t: _ulog_dispatch_3_u8_u8_u8(id, a, b, c), \
            uint16_t: _ulog_dispatch_3_u8_u16(id, a, b), \
            default: _ulog_dispatch_3_u8_u8_u8(id, a, b, c)), \
         uint16_t: _ulog_dispatch_3_u16_u8(id, a, b), \
         default: _ulog_dispatch_3_u8_u8_u8(id, a, b, c)); \
   } while(0)

#define _ULOG_DISPATCH_4(level, fmt, a, b, c, d) \
   do { \
      _ULOG_EMIT_RECORD(level, fmt, ((_ULOG_TC(d) << 24) | (_ULOG_TC(c) << 16) | (_ULOG_TC(b) << 8) | _ULOG_TC(a))) \
      _ulog_dispatch_4_u8_u8_u8_u8(id, a, b, c, d); \
   } while(0)

// Support macros to expand ... into actual dispatcher based on argument count
#define _ULOG_GET_ARG_COUNT(...) _ULOG_GET_ARG_COUNT_IMPL(0, ##__VA_ARGS__, 4, 3, 2, 1, 0)
#define _ULOG_GET_ARG_COUNT_IMPL(dummy, _1, _2, _3, _4, N, ...) N
#define _ULOG_DISPATCH(n) _ULOG_DISPATCH_IMPL(n)
#define _ULOG_DISPATCH_IMPL(n) _ULOG_DISPATCH_##n

// Main ULOG macro (C version)
#define ULOG(level, fmt, ...) \
   _ULOG_DISPATCH(_ULOG_GET_ARG_COUNT(__VA_ARGS__))(level, fmt, ##__VA_ARGS__)

#else
#  include <tuple>
#  include <cstring>
#  include <type_traits>
#  include <utility>

namespace ulog {
   namespace detail {
      /** Value to pack for the argument trait */
      enum class ArgTrait : uint8_t {
         none    = ULOG_TRAIT_ID_NONE,
         u8      = ULOG_TRAIT_ID_U8,
         s8      = ULOG_TRAIT_ID_S8,
         b8      = ULOG_TRAIT_ID_BOOL,
         u16     = ULOG_TRAIT_ID_U16,
         s16     = ULOG_TRAIT_ID_S16,
         ptr16   = ULOG_TRAIT_ID_PTR,
         u32     = ULOG_TRAIT_ID_U32,
         s32     = ULOG_TRAIT_ID_S32,
         float32 = ULOG_TRAIT_ID_FLOAT,
         str4    = ULOG_TRAIT_ID_STR4
      };

      template <typename T>
      constexpr ArgTrait arg_trait() {
         using U = std::remove_cv_t<std::remove_reference_t<T>>;

         if constexpr (std::is_same_v<U, bool>)
            return ArgTrait::b8;
         else if constexpr (std::is_same_v<U, const char*> || std::is_same_v<U, char*>)
            return ArgTrait::str4;
         else if constexpr (std::is_floating_point_v<U>)
            return ArgTrait::float32;
         else if constexpr (std::is_pointer_v<U> && sizeof(U) == 2)
            return ArgTrait::ptr16;
         else if constexpr (std::is_integral_v<U>) {
            if constexpr (std::is_signed_v<U>) {
               if constexpr (sizeof(U) == 1) return ArgTrait::s8;
               if constexpr (sizeof(U) == 2) return ArgTrait::s16;
               if constexpr (sizeof(U) == 4) return ArgTrait::s32;
            } else {
               if constexpr (sizeof(U) == 1) return ArgTrait::u8;
               if constexpr (sizeof(U) == 2) return ArgTrait::u16;
               if constexpr (sizeof(U) == 4) return ArgTrait::u32;
            }
         }

         return ArgTrait::none;
      }

      template<typename... Ts>
      constexpr uint32_t encode_traits() {
         uint32_t result = 0;
         uint8_t i = 0;
         ((result |= static_cast<uint32_t>(arg_trait<Ts>()) << (i++ * 8)), ...);
         return result;
      }

      template<typename... Ts>
      constexpr size_t packed_sizeof() {
         return (sizeof(Ts) + ... + 0);
      }

      template <typename T>
      constexpr auto split_to_u8_tuple(T value) {
         using U = std::remove_cv_t<std::remove_reference_t<T>>;

         if constexpr (std::is_integral_v<U>) {
            if constexpr (sizeof(T) == 1) {
               return std::make_tuple(static_cast<uint8_t>(value));
            } else if constexpr (sizeof(T) == 2) {
               return std::make_tuple(
                     static_cast<uint8_t>(value & 0xFF),
                     static_cast<uint8_t>((value >> 8) & 0xFF)
               );
            } else if constexpr (sizeof(T) == 4) {
               return std::make_tuple(
                     static_cast<uint8_t>(value & 0xFF),
                     static_cast<uint8_t>((value >> 8) & 0xFF),
                     static_cast<uint8_t>((value >> 16) & 0xFF),
                     static_cast<uint8_t>((value >> 24) & 0xFF)
               );
            } else {
               //static_assert(0, "Unsupported integer size");
            }
         } else if constexpr (std::is_same_v<U, float>) {
            static_assert(sizeof(float) == 4, "Unexpected float size");
            union {
               float f;
               uint8_t bytes[4];
            } conv = { value };

            return std::make_tuple(conv.bytes[0], conv.bytes[1], conv.bytes[2], conv.bytes[3]);
         } else if constexpr (std::is_same_v<U, const char*> || std::is_same_v<U, char*>) {
            // We could read beyond the string - but that's OK, the display will fix it for us
            return std::make_tuple(
               static_cast<uint8_t>(value[0]),
               static_cast<uint8_t>(value[1]),
               static_cast<uint8_t>(value[2]),
               static_cast<uint8_t>(value[3])
            );
         } else {
            //static_assert(0, "Unsupported type for packing");
         }
      }

      template <typename... Args>
      constexpr auto pack_bytes_to_tuple(Args&&... args) {
         static_assert((... && (
            std::is_integral_v<std::remove_reference_t<Args>> ||
            std::is_same_v<std::remove_reference_t<Args>, float>
         )), "Only integral or float arguments are supported");

         return std::tuple_cat(split_to_u8_tuple(std::forward<Args>(args))...);
      }
   } // namespace detail
} // namespace ulog

#define ULOG(level, fmt, ...)                                                 \
do {                                                                          \
   [&]<typename... Args>(Args&&... args) {                                    \
      constexpr uint32_t _typecode = ::ulog::detail::encode_traits<Args...>();\
      (void)_typecode; /* suppress unused warning when inlined as constant */ \
      auto values = ::ulog::detail::pack_bytes_to_tuple(args...);             \
      constexpr size_t _nbytes = std::tuple_size<decltype(values)>::value;    \
      static_assert(_nbytes <= 4, "ULOG supports up to 4 bytes of payload");  \
      _ULOG_EMIT_RECORD(level, fmt, _typecode);                       \
      if constexpr (_nbytes == 0) {                                           \
         ulog_detail_enqueue(id);                                             \
      } else if constexpr (_nbytes == 1) {                                    \
         auto&& b0 = std::get<0>(values);                                     \
         ulog_detail_enqueue_1(id, b0);                                       \
      } else if constexpr (_nbytes == 2) {                                    \
         auto&& b0 = std::get<0>(values);                                     \
         auto&& b1 = std::get<1>(values);                                     \
         ulog_detail_enqueue_2(id, b0, b1);                                   \
      } else if constexpr (_nbytes == 3) {                                    \
         auto&& b0 = std::get<0>(values);                                     \
         auto&& b1 = std::get<1>(values);                                     \
         auto&& b2 = std::get<2>(values);                                     \
         ulog_detail_enqueue_3(id, b0, b1, b2);                               \
      } else if constexpr (_nbytes == 4) {                                    \
         auto&& b0 = std::get<0>(values);                                     \
         auto&& b1 = std::get<1>(values);                                     \
         auto&& b2 = std::get<2>(values);                                     \
         auto&& b3 = std::get<3>(values);                                     \
         ulog_detail_enqueue_4(id, b0, b1, b2, b3);                           \
      }                                                                       \
   }(__VA_ARGS__);                                                            \
} while(0)

#endif // End of the C-only section

// Include the project trace config (unless passed on the command line)
#if defined HAS_ULOG_CONFIG_FILE && !defined ULOG_LEVEL
#  include "conf_ulog.h"
#endif

// Default log level if not defined yet
#ifndef ULOG_LEVEL
#  ifdef NDEBUG
#     define ULOG_LEVEL ULOG_LEVEL_INFO
#  else
#     define ULOG_LEVEL ULOG_LEVEL_DEBUG3
#  endif
#endif

#if ULOG_LEVEL >= ULOG_LEVEL_ERROR
  #define ULOG_ERROR(text, ...)       ULOG(ULOG_LEVEL_ERROR, text, ##__VA_ARGS__)
#else
  #define ULOG_ERROR(text, ...)       do {} while (0)
#endif

#if ULOG_LEVEL >= ULOG_LEVEL_WARN
  #define ULOG_WARN(text, ...)       ULOG(ULOG_LEVEL_WARN, text, ##__VA_ARGS__)
#else
  #define ULOG_WARN(text, ...)       do {} while (0)
#endif

#if ULOG_LEVEL >= ULOG_LEVEL_MILE
   #define ULOG_MILE(text, ...)       ULOG(ULOG_LEVEL_MILE, text, ##__VA_ARGS__)
#else
   #define ULOG_MILE(text, ...)       do {} while (0)
#endif

#if ULOG_LEVEL >= ULOG_LEVEL_INFO
  #define ULOG_INFO(text, ...)       ULOG(ULOG_LEVEL_INFO, text, ##__VA_ARGS__)
#else
  #define ULOG_INFO(text, ...)       do {} while (0)
#endif

#if ULOG_LEVEL >= ULOG_LEVEL_TRACE
  #define ULOG_TRACE(text, ...)       ULOG(ULOG_LEVEL_TRACE, text, ##__VA_ARGS__)
#else
  #define ULOG_TRACE(text, ...)       do {} while (0)
#endif

#if ULOG_LEVEL >= ULOG_LEVEL_DEBUG0
  #define ULOG_DEBUG0(text, ...)      ULOG(ULOG_LEVEL_DEBUG0, text, ##__VA_ARGS__)
#else
  #define ULOG_DEBUG0(text, ...)      do {} while (0)
#endif

#if ULOG_LEVEL >= ULOG_LEVEL_DEBUG1
  #define ULOG_DEBUG1(text, ...)      ULOG(ULOG_LEVEL_DEBUG1, text, ##__VA_ARGS__)
#else
  #define ULOG_DEBUG1(text, ...)      do {} while (0)
#endif

#if ULOG_LEVEL >= ULOG_LEVEL_DEBUG2
  #define ULOG_DEBUG2(text, ...)      ULOG(ULOG_LEVEL_DEBUG2, text, ##__VA_ARGS__)
#else
  #define ULOG_DEBUG2(text, ...)      do {} while (0)
#endif

#if ULOG_LEVEL >= ULOG_LEVEL_DEBUG3
  #define ULOG_DEBUG3(text, ...)      ULOG(ULOG_LEVEL_DEBUG3, text, ##__VA_ARGS__)
#else
  #define ULOG_DEBUG3(text, ...)      do {} while (0)
#endif
