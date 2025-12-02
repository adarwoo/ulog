/**
 * Configuration file for ULOG
 * You can override these settings in your build system or
 * define them before including ulog.h by adding defines to the complier command line.
 * Example:
 * -DULOG_LEVEL=ULOG_LEVEL_INFO
 * would remove all logs below INFO level from the build.
 */

// Define the project preferred log level
#ifndef ULOG_LEVEL
#  ifdef NDEBUG
#     define ULOG_LEVEL ULOG_LEVEL_INFO
#  else
#     define ULOG_LEVEL ULOG_LEVEL_DEBUG3
#  endif
#endif   // ULOG_LEVEL
