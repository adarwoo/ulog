#pragma once
/**
 * @file ulog.h
 * @brief C/C++ Ultra-lightweight logging framework for embedded systems.
 *
 * This header defines a compile-time evaluated logging system designed for
 * memory-constrained embedded environments. It embeds log metadata into custom
 * ELF sections (.logs) while emitting runtime log payloads via register-based
 * emission routines. Format strings and type signatures are preserved at
 * compile time and can be indexed offline using the binary metadata.
 *
 * Features:
 * - Zero runtime format overhead (all data are sent as raw binary packets)
 * - Compile-time argument encoding and dispatch
 * - Interrupt compatible
 * - Supports up to 8 arguments per log call
 * - Inline assembly generates .logs metadata per callsite
 * - Messages are sent as binary packets using COBS encoding
 * - Up to 32760 unique messages per application
 *
 * Usage:
 * ```c/c++
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
 * @note The C implementation relies on C11 _Generic for type dispatching. The C++ version uses templates.
 * @note The `.logs` section can be parsed from the ELF to map runtime packets back to messages.
 * @note This header is C and C++ compatible, each language use an optimized version - more type safe for C++
 * @note This header requires C11 or later for _Generic support and C++17 or later for template features.
 * @note For zero-argument logging (e.g., ULOG_INFO("text")), use -std=gnu11 or compiler default, not -std=c11
 *       (requires ##__VA_ARGS__ GNU extension)
 *
 * @author software@arreckx.com
 */
#include <stdint.h>
#include "ulog_port.h"

#ifdef __cplusplus
extern "C" {
#endif


// ============================================================================
// Forwarding prototypes
// ============================================================================
void ulog_detail_enqueue(uint16_t id);
void ulog_detail_enqueue_1(uint16_t id, uint8_t v0);
void ulog_detail_enqueue_2(uint16_t id, uint8_t v0, uint8_t v1);
void ulog_detail_enqueue_3(uint16_t id, uint8_t v0, uint8_t v1, uint8_t v2);
void ulog_detail_enqueue_4(uint16_t id, uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3);
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
// C type codes - 4-bit encoding (0x0 to 0xF)
// Allows packing 8 argument types into a single 32-bit value
// ============================================================================
#define ULOG_TRAIT_ID_NONE  0x0L
#define ULOG_TRAIT_ID_U8    0x1L
#define ULOG_TRAIT_ID_S8    0x2L
#define ULOG_TRAIT_ID_BOOL  0x3L
#define ULOG_TRAIT_ID_U16   0x4L
#define ULOG_TRAIT_ID_S16   0x5L
#define ULOG_TRAIT_ID_PTR   0x6L
#define ULOG_TRAIT_ID_U32   0x7L
#define ULOG_TRAIT_ID_S32   0x8L
#define ULOG_TRAIT_ID_FLOAT 0x9L
#define ULOG_TRAIT_ID_STR   0xAL

// ============================================================================
// Continuation flag for multi-argument logs
// Set the MSB of the ID to indicate this packet is a continuation
// ============================================================================
#define ULOG_ID_CONTINUATION 0x8000

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
 * IMPORTANT:
 * - Format strings containing % must escape them as %% (inline assembly)
 * - .logs section has NO "a" flag (not allocatable) - it's metadata only
 */
#ifndef _ULOG_EMIT_RECORD
#  error "Platform must define _ULOG_EMIT_RECORD with architecture-specific address computation"
#endif


#ifndef _ULOG_MAX_STR_LENGTH
#  define _ULOG_MAX_STR_LENGTH 16
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
static inline void _ulog_dispatch(uint16_t id) {
    ulog_detail_enqueue(id);
}

static inline void _ulog_dispatch_u8(uint16_t id, uint8_t a) {
    ulog_detail_enqueue_1(id, a);
}

static inline void _ulog_dispatch_u16(uint16_t id, uint16_t a) {
    ulog_detail_enqueue_2(id, (uint8_t)(a), (uint8_t)(a >> 8));
}

static inline void _ulog_dispatch_u32(uint16_t id, uint32_t a) {
    ulog_detail_enqueue_4(id, (uint8_t)(a), (uint8_t)(a >> 8), (uint8_t)(a >> 16), (uint8_t)(a >> 24));
}

static inline void _ulog_dispatch_float(uint16_t id, float a) {
    union { float f; uint32_t u; } conv = { .f = a };
    ulog_detail_enqueue_4(id, (uint8_t)(conv.u), (uint8_t)(conv.u >> 8), (uint8_t)(conv.u >> 16), (uint8_t)(conv.u >> 24));
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
      _ulog_dispatch(id); \
   } while(0)

// ============================================================================
// Simplified dispatch macros based only on argument count
// The expansion yields a compile-time evaluated typecode and calls the appropriate
// dispatcher function.
// This relies on compiler support for C11 _Generic. The final resolution happens
// at compile time, so there is no runtime overhead.
// ============================================================================

// Helper macros for GCC-only pragma directives (for portability)
#ifdef __GNUC__
#  define _ULOG_DIAG_PUSH() \
      _Pragma("GCC diagnostic push") \
      _Pragma("GCC diagnostic ignored \"-Wconversion\"") \
      _Pragma("GCC diagnostic ignored \"-Wsign-conversion\"") \
      _Pragma("GCC diagnostic ignored \"-Wfloat-conversion\"")
#  define _ULOG_DIAG_POP() \
      _Pragma("GCC diagnostic pop")
#else
#  define _ULOG_DIAG_PUSH()
#  define _ULOG_DIAG_POP()
#endif

#define _ULOG_DISPATCH_GENERIC(a, flag) \
   _Generic((a), \
      uint8_t:  _ulog_dispatch_u8(id | flag, a), \
      int8_t:   _ulog_dispatch_u8(id | flag, (uint8_t)a), \
      _Bool:    _ulog_dispatch_u8(id | flag, (uint8_t)a), \
      uint16_t: _ulog_dispatch_u16(id | flag, a), \
      int16_t:  _ulog_dispatch_u16(id | flag, (uint16_t)a), \
      uint32_t: _ulog_dispatch_u32(id | flag, a), \
      int32_t:  _ulog_dispatch_u32(id | flag, (uint32_t)a), \
      float:    _ulog_dispatch_float(id | flag, a), \
      double:   _ulog_dispatch_float(id | flag, (float)a)); \

#define _ULOG_DISPATCH_0(level, fmt) \
   do { \
      _ULOG_EMIT_RECORD(level, fmt, 0); \
      _ulog_dispatch(id); \
   } while(0)

#define _ULOG_DISPATCH_1(level, fmt, a) \
   do { \
      _ULOG_DIAG_PUSH() \
      _ULOG_EMIT_RECORD(level, fmt, _ULOG_TC(a)); \
      _ULOG_DISPATCH_GENERIC(a, 0) \
      _ULOG_DIAG_POP() \
   } while(0)


#define _ULOG_DISPATCH_2(level, fmt, a, b) \
   do { \
      _ULOG_DIAG_PUSH() \
      _ULOG_EMIT_RECORD(level, fmt, ((_ULOG_TC(b) << 4) | _ULOG_TC(a))); \
      _ULOG_DISPATCH_GENERIC(a, 0) \
      _ULOG_DISPATCH_GENERIC(b, ULOG_ID_CONTINUATION) \
      _ULOG_DIAG_POP() \
   } while(0)

#define _ULOG_DISPATCH_3(level, fmt, a, b, c) \
   do { \
      _ULOG_DIAG_PUSH() \
      _ULOG_EMIT_RECORD(level, fmt, ((_ULOG_TC(c) << 8) | (_ULOG_TC(b) << 4) | _ULOG_TC(a))); \
      _ULOG_DISPATCH_GENERIC(a, 0) \
      _ULOG_DISPATCH_GENERIC(b, ULOG_ID_CONTINUATION) \
      _ULOG_DISPATCH_GENERIC(c, ULOG_ID_CONTINUATION) \
      _ULOG_DIAG_POP() \
   } while(0)

#define _ULOG_DISPATCH_4(level, fmt, a, b, c, d) \
   do { \
      _ULOG_DIAG_PUSH() \
      _ULOG_EMIT_RECORD(level, fmt, ((_ULOG_TC(d) << 12) | (_ULOG_TC(c) << 8) | (_ULOG_TC(b) << 4) | _ULOG_TC(a))); \
      _ULOG_DISPATCH_GENERIC(a, 0) \
      _ULOG_DISPATCH_GENERIC(b, ULOG_ID_CONTINUATION) \
      _ULOG_DISPATCH_GENERIC(c, ULOG_ID_CONTINUATION) \
      _ULOG_DISPATCH_GENERIC(d, ULOG_ID_CONTINUATION) \
      _ULOG_DIAG_POP() \
   } while(0)

#define _ULOG_DISPATCH_5(level, fmt, a, b, c, d, e) \
   do { \
      _ULOG_DIAG_PUSH() \
      _ULOG_EMIT_RECORD(level, fmt, ((_ULOG_TC(e) << 16) | (_ULOG_TC(d) << 12) | (_ULOG_TC(c) << 8) | (_ULOG_TC(b) << 4) | _ULOG_TC(a))); \
      _ULOG_DISPATCH_GENERIC(a, 0) \
      _ULOG_DISPATCH_GENERIC(b, ULOG_ID_CONTINUATION) \
      _ULOG_DISPATCH_GENERIC(c, ULOG_ID_CONTINUATION) \
      _ULOG_DISPATCH_GENERIC(d, ULOG_ID_CONTINUATION) \
      _ULOG_DISPATCH_GENERIC(e, ULOG_ID_CONTINUATION) \
      _ULOG_DIAG_POP() \
   } while(0)

#define _ULOG_DISPATCH_6(level, fmt, a, b, c, d, e, f) \
   do { \
      _ULOG_DIAG_PUSH() \
      _ULOG_EMIT_RECORD(level, fmt, ((_ULOG_TC(f) << 20) | (_ULOG_TC(e) << 16) | (_ULOG_TC(d) << 12) | (_ULOG_TC(c) << 8) | (_ULOG_TC(b) << 4) | _ULOG_TC(a))); \
      _ULOG_DISPATCH_GENERIC(a, 0) \
      _ULOG_DISPATCH_GENERIC(b, ULOG_ID_CONTINUATION) \
      _ULOG_DISPATCH_GENERIC(c, ULOG_ID_CONTINUATION) \
      _ULOG_DISPATCH_GENERIC(d, ULOG_ID_CONTINUATION) \
      _ULOG_DISPATCH_GENERIC(e, ULOG_ID_CONTINUATION) \
      _ULOG_DISPATCH_GENERIC(f, ULOG_ID_CONTINUATION) \
      _ULOG_DIAG_POP() \
   } while(0)

#define _ULOG_DISPATCH_7(level, fmt, a, b, c, d, e, f, g) \
   do { \
      _ULOG_DIAG_PUSH() \
      _ULOG_EMIT_RECORD(level, fmt, ((_ULOG_TC(g) << 24) | (_ULOG_TC(f) << 20) | (_ULOG_TC(e) << 16) | (_ULOG_TC(d) << 12) | (_ULOG_TC(c) << 8) | (_ULOG_TC(b) << 4) | _ULOG_TC(a))); \
      _ULOG_DISPATCH_GENERIC(a, 0) \
      _ULOG_DISPATCH_GENERIC(b, ULOG_ID_CONTINUATION) \
      _ULOG_DISPATCH_GENERIC(c, ULOG_ID_CONTINUATION) \
      _ULOG_DISPATCH_GENERIC(d, ULOG_ID_CONTINUATION) \
      _ULOG_DISPATCH_GENERIC(e, ULOG_ID_CONTINUATION) \
      _ULOG_DISPATCH_GENERIC(f, ULOG_ID_CONTINUATION) \
      _ULOG_DISPATCH_GENERIC(g, ULOG_ID_CONTINUATION) \
      _ULOG_DIAG_POP() \
   } while(0)

#define _ULOG_DISPATCH_8(level, fmt, a, b, c, d, e, f, g, h) \
   do { \
      _ULOG_DIAG_PUSH() \
      _ULOG_EMIT_RECORD(level, fmt, ((_ULOG_TC(h) << 28) | (_ULOG_TC(g) << 24) | (_ULOG_TC(f) << 20) | (_ULOG_TC(e) << 16) | (_ULOG_TC(d) << 12) | (_ULOG_TC(c) << 8) | (_ULOG_TC(b) << 4) | _ULOG_TC(a))); \
      _ULOG_DISPATCH_GENERIC(a, 0) \
      _ULOG_DISPATCH_GENERIC(b, ULOG_ID_CONTINUATION) \
      _ULOG_DISPATCH_GENERIC(c, ULOG_ID_CONTINUATION) \
      _ULOG_DISPATCH_GENERIC(d, ULOG_ID_CONTINUATION) \
      _ULOG_DISPATCH_GENERIC(e, ULOG_ID_CONTINUATION) \
      _ULOG_DISPATCH_GENERIC(f, ULOG_ID_CONTINUATION) \
      _ULOG_DISPATCH_GENERIC(g, ULOG_ID_CONTINUATION) \
      _ULOG_DISPATCH_GENERIC(h, ULOG_ID_CONTINUATION) \
      _ULOG_DIAG_POP() \
   } while(0)

// Support macros to expand ... into actual dispatcher based on argument count
// NOTE: Requires ##__VA_ARGS__ GNU extension for zero-argument support.
// Use -std=gnu11 or compiler default, not -std=c11
#define _ULOG_GET_ARG_COUNT(...) _ULOG_GET_ARG_COUNT_IMPL(0, ##__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define _ULOG_GET_ARG_COUNT_IMPL(dummy, _1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
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
         none      = ULOG_TRAIT_ID_NONE,
         u8        = ULOG_TRAIT_ID_U8,
         s8        = ULOG_TRAIT_ID_S8,
         b8        = ULOG_TRAIT_ID_BOOL,
         u16       = ULOG_TRAIT_ID_U16,
         s16       = ULOG_TRAIT_ID_S16,
         ptr16     = ULOG_TRAIT_ID_PTR,
         u32       = ULOG_TRAIT_ID_U32,
         s32       = ULOG_TRAIT_ID_S32,
         float32   = ULOG_TRAIT_ID_FLOAT,
         str       = ULOG_TRAIT_ID_STR,
      };

      template <typename T>
      constexpr ArgTrait arg_trait() {
         using U = std::remove_cv_t<std::remove_reference_t<T>>;

         if constexpr (std::is_same_v<U, bool>)
            return ArgTrait::b8;
         else if constexpr (std::is_same_v<U, const char*> || std::is_same_v<U, char*>)
            return ArgTrait::str;
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
         static_assert(sizeof...(Ts) <= 8, "ULOG supports up to 8 arguments");
         uint32_t result = 0;
         uint8_t i = 0;
         ((result |= static_cast<uint32_t>(arg_trait<Ts>()) << (i++ * 4)), ...);
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
               static_assert(sizeof(T) == 0, "Unsupported integer size");
            }
         } else if constexpr (std::is_same_v<U, float>) {
            static_assert(sizeof(float) == 4, "Unexpected float size");
            union {
               float f;
               uint8_t bytes[4];
            } conv = { value };

            return std::make_tuple(conv.bytes[0], conv.bytes[1], conv.bytes[2], conv.bytes[3]);
         } else {
            static_assert(sizeof(U) == 0, "Unsupported type for packing");
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

      // Helper to send a string in 4-byte chunks
      template <typename IdType>
      inline void send_string_chunks(IdType id, const char* str) {
         size_t pos = 0;

         while (true) {
            uint8_t b0 = str[pos];
            if ( b0 == 0 ) {
               ulog_detail_enqueue_1(id, uint8_t{0});
               break;
            }

            uint8_t b1 = str[pos+1];
            if ( b1 == 0 ) {
               ulog_detail_enqueue_2(id, b0, uint8_t{0});
               break;
            }

            uint8_t b2 = str[pos+2];
            if ( b2 == 0 ) {
               ulog_detail_enqueue_3(id, b0, b1, uint8_t{0});
               break;
            }

            uint8_t b3 = str[pos+3];
            if ( b3 == 0 ) {
               ulog_detail_enqueue_4(id, b0, b1, b2, uint8_t{0});
               break;
            } else {
               // If there is less than 4 chars left, we don't need to truncate
               if ( pos > (_ULOG_MAX_STR_LENGTH-3) and (std::strlen(str + pos) > 3) ) {
                  // Safety break to avoid infinite loops on malformed strings or long strings
                  ulog_detail_enqueue_4(id, uint8_t{'.'}, uint8_t{'.'}, uint8_t{'.'}, uint8_t{0});
                  break;
               }

               ulog_detail_enqueue_4(id, b0, b1, b2, b3);

               pos += 4;
               id |= ULOG_ID_CONTINUATION;
            }
         }
      }

      // Helper to emit a single argument as a dedicated packet
      template <typename IdType, typename T>
      inline void emit_single_arg(IdType id, T&& arg) {
         using U = std::remove_cv_t<std::remove_reference_t<T>>;

         // Check for string types: const char*, char*, or char arrays
         if constexpr (std::is_same_v<U, const char*> || std::is_same_v<U, char*> ||
                       (std::is_array_v<U> && std::is_same_v<std::remove_extent_t<U>, char>) ||
                       (std::is_array_v<U> && std::is_same_v<std::remove_extent_t<U>, const char>)) {
            // String handling
            const char* str = arg;
            send_string_chunks(id, str);
         } else {
            // Non-string handling - pack to bytes and send
            auto values = split_to_u8_tuple(std::forward<T>(arg));
            constexpr size_t nbytes = std::tuple_size<decltype(values)>::value;

            if constexpr (nbytes == 1) {
               ulog_detail_enqueue_1(id, std::get<0>(values));
            } else if constexpr (nbytes == 2) {
               ulog_detail_enqueue_2(id, std::get<0>(values), std::get<1>(values));
            } else if constexpr (nbytes == 4) {
               ulog_detail_enqueue_4(id, std::get<0>(values), std::get<1>(values),
                                     std::get<2>(values), std::get<3>(values));
            } else {
               static_assert(nbytes == 0, "Unsupported packed size");
            }
         }
      }
   } // namespace detail
} // namespace ulog

#define ULOG(level, fmt, ...)                                                 \
do {                                                                          \
   [&]<typename... Args>(Args&&... args) {                                    \
      constexpr size_t nargs = sizeof...(Args);                               \
      static_assert(nargs <= 8, "ULOG supports up to 8 arguments");           \
      /* Encode all argument traits into a single 32-bit value (4 bits each) */ \
      constexpr uint32_t _typecode = ::ulog::detail::encode_traits<Args...>();\
      (void)_typecode; /* suppress unused warning when inlined as constant */ \
      /* Emit one record with the typecode */                                 \
      _ULOG_EMIT_RECORD(level, fmt, _typecode);                               \
      if constexpr (nargs == 0) {                                             \
         /* No arguments - emit empty record */                               \
         ulog_detail_enqueue(id);                                             \
      } else if constexpr (nargs == 1) {                                      \
         /* Single argument - no continuation flag */                         \
         ::ulog::detail::emit_single_arg(id, std::forward<Args>(args)...);    \
      } else {                                                                \
         /* Multiple arguments - set continuation flag on all but first */     \
         [&]<size_t... Is>(std::index_sequence<Is...>) {                      \
            ((::ulog::detail::emit_single_arg(                                \
               static_cast<uint16_t>(Is == 0 ? id : (id | ULOG_ID_CONTINUATION)), \
               std::get<Is>(std::forward_as_tuple(std::forward<Args>(args)...)))), ...); \
         }(std::make_index_sequence<nargs>{});                                \
      }                                                                       \
   }(__VA_ARGS__);                                                            \
} while(0)

#endif // End of the C-only section

// Include the project trace config (unless passed on the command line)

#ifndef ULOG_LEVEL
   // Convention-based fallback - __has_include is supported by clang and gcc
#  if defined __has_include
#     if __has_include("conf_ulog.h")
#        include "conf_ulog.h"
#     endif
#  elif defined ULOG_CONFIG_FILE
#    include ULOG_CONFIG_FILE
#  endif
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
