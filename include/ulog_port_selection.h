#pragma once
/**
 * @file ulog_port_selection.h
 * @brief Porting layer selection for the ulog logging framework.
 * This header selects the appropriate porting layer based on the target platform.
 * If no platform is detected, a compilation error is raised.
 * Otherwise, a suffix macro is defined for inclusion in other porting headers.
 */
#undef _ULOG_PORT_SUFFIX

// Helper macros for expansion
#define _ULOG_STRINGIFY(x) #x
#define _ULOG_TO_HEADER_FILENAME(prefix) _ULOG_STRINGIFY(prefix##_##_ULOG_PORT_SUFFIX) ".h"

// ---------------------------------------------------------------------------
// Start of platform detection and suffix definition
// ---------------------------------------------------------------------------

#ifdef __AVR__
#  define _ULOG_PORT_SUFFIX AVR_ASX
#endif

// ---------------------------------------------------------------------------
// End of platform detection and suffix definition
// ---------------------------------------------------------------------------

// Build paths
#if !defined(_ULOG_PORT_SUFFIX)
#  error "Please define the porting macros for your platform in ulog_port.h"
#else
#  define _ULOG_PORT_HEADER_PATH _ULOG_TO_HEADER_FILENAME(ulog_port_header_, _ULOG_PORT_SUFFIX)
#  define _ULOG_PORT_IMPL_PATH   _ULOG_TO_HEADER_FILENAME(ulog_port_impl_, _ULOG_PORT_SUFFIX)
#endif
