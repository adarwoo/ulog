/**
 * Configuration file for ULOG
 */

// Define the project preferred log level
#ifndef ULOG_LEVEL // Allow definition from the command line as -DULOG_LEVEL=...
#  ifdef NDEBUG
#     define ULOG_LEVEL ULOG_LEVEL_INFO
#  else
#     define ULOG_LEVEL ULOG_LEVEL_DEBUG3
#  endif
#endif   // ULOG_LEVEL