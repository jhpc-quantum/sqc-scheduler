#pragma once

#include "job_broker_logger.h"

///
/// @file	logger.h
///

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef enum {
  LOG_LEVEL_UNKNOWN = 0,
  LOG_LEVEL_DEBUG,
  LOG_LEVEL_INFO,
  LOG_LEVEL_NOTICE,
  LOG_LEVEL_WARNING,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_FATAL
} log_level_t;

void
job_broker_log_initialize();

void
job_broker_log_emit(log_level_t log_level,
                    uint64_t debug_level,
                    const char *file,
                    int line,
                    const char *func,
                    const char *fmt, ...)
__attr_format_printf__(6, 7);

void
job_broker_log_emit_to_stderr(int log_level,
                              uint64_t debug_level,
                              const char *file,
                              int line,
                              const char *func,
                              const char *fmt, ...)
__attr_format_printf__(6, 7);

#ifdef __cplusplus
}
#endif // __cplusplus


#ifdef __GNUC__
#define __PROC__	__PRETTY_FUNCTION__
#else
#define	__PROC__	__func__
#endif /* __GNUC__ */


///
/// Emit a debug message to the log.
///
/// @param[in]	level	A debug level (int).
///
#define msg_debug(level, ...) \
  job_broker_log_emit(LOG_LEVEL_DEBUG, (uint64_t)(level), \
                      __FILE__, __LINE__, __PROC__, __VA_ARGS__)


///
/// Emit an informative message to the log.
///
#define msg_info(...) \
  job_broker_log_emit(LOG_LEVEL_INFO, 0LL, __FILE__, __LINE__, \
                      __PROC__, __VA_ARGS__)


///
/// Emit a notice message to the log.
///
#define msg_notice(...) \
  job_broker_log_emit(LOG_LEVEL_NOTICE, 0LL, __FILE__, __LINE__, \
                      __PROC__, __VA_ARGS__)


///
/// Emit a warning message to the log.
///
#define msg_warning(...) \
  job_broker_log_emit(LOG_LEVEL_WARNING, 0LL, __FILE__, __LINE__, \
                       __PROC__, __VA_ARGS__)


///
/// Emit an error message to the log.
///
#define msg_error(...) \
  job_broker_log_emit(LOG_LEVEL_ERROR, 0LL, __FILE__, __LINE__, \
                      __PROC__, __VA_ARGS__)


///
/// Emit a fatal message to the log.
///
#define msg_fatal(...) \
  job_broker_log_emit(LOG_LEVEL_FATAL, 0LL, __FILE__, __LINE__, \
                      __PROC__, __VA_ARGS__)


///
/// Emit an arbitarary message to the log.
///
#define msg(...) \
  job_broker_log_emit(LOG_LEVEL_UNKNOWN, 0LL, __FILE__, __LINE__, \
                      __PROC__, __VA_ARGS__)


///
/// The minimum level debug emitter.
///
#define dprint(...) \
  msg_debug(1LL, __VA_ARGS__)


///
/// Emit an error message and exit.
///
/// @param[in]	ecode	An exit code (int)
///
#define exit_error(ecode, ...) { \
    msg_error(__VA_ARGS__); \
    exit(ecode); \
  }


///
/// Emit a fatal message and abort.
///
#define exit_fatal(...) { \
    msg_fatal(__VA_ARGS__); \
    abort(); \
  }


///
/// Emit a debug message with a prefix to the log.
///
/// @param[in]	level	A debug level (int).
/// @param[in]	prefix	A prefix (string).
/// @param[in]	msg	    A log message (string).
///
#define msg_debug_with_prefix(level, prefix, msg) { \
  if (msg != NULL) { \
    msg_debug((level), "%s: %s\n", (prefix), (msg));    \
  } else { \
    msg_debug((level), "%s: (failed to create a log message)\n", (prefix)); \
  } \
}

///
/// Emit an informative message with a prefix to the log.
///
/// @param[in]	prefix	A prefix (string).
/// @param[in]	msg	    A log message (string).
///
#define msg_info_with_prefix(prefix, msg) { \
  if (msg != NULL) { \
    msg_info("%s: %s\n", (prefix), (msg));  \
  } else { \
    msg_info("%s: (failed to create a log message)\n", (prefix)); \
  } \
}

///
/// Emit a notice message with a prefix to the log.
///
/// @param[in]	prefix	A prefix (string).
/// @param[in]	msg	    A log message (string).
///
#define msg_notice_with_prefix(prefix, msg) { \
  if (msg != NULL) { \
    msg_notice("%s: %s\n", (prefix), (msg));  \
  } else { \
    msg_notice("%s: (failed to create a log message)\n", (prefix)); \
  } \
}

///
/// Emit a warning message with a prefix to the log.
///
/// @param[in]	prefix	A prefix (string).
/// @param[in]	msg	    A log message (string).
///
#define msg_warning_with_prefix(prefix, msg) { \
  if (msg != NULL) { \
    msg_warning("%s: %s\n", (prefix), (msg));  \
  } else { \
    msg_warning("%s: (failed to create a log message)\n", (prefix)); \
  } \
}

///
/// Emit an error message with a prefix to the log.
///
/// @param[in]	prefix	A prefix (string).
/// @param[in]	msg	    A log message (string).
///
#define msg_error_with_prefix(prefix, msg) { \
  if (msg != NULL) { \
    msg_error("%s: %s\n", (prefix), (msg));  \
  } else { \
    msg_error("%s: (failed to create a log message)\n", (prefix)); \
  } \
}

///
/// Emit a fatal message with a prefix to the log.
///
/// @param[in]	prefix	A prefix (string).
/// @param[in]	msg	    A log message (string).
///
#define msg_fatal_with_prefix(prefix, msg) { \
  if (msg != NULL) { \
    msg_fatal("%s: %s\n", (prefix), (msg));  \
  } else { \
    msg_fatal("%s: (failed to create a log message)\n", (prefix)); \
  } \
}
