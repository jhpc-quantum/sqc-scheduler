#pragma once

///
/// @file	logger.h
///

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifdef __GNUC__
#define __attr_format_printf__(x, y) \
  __attribute__ ((format(printf, x, y)))
#else
#define __attr_format_printf__(x, y)
#endif // __GNUC__

typedef void (*job_broker_log_emitter_t)(int log_level,
                                         uint64_t debug_level,
                                         const char *file,
                                         int line,
                                         const char *func,
                                         const char *fmt, ...)
  __attr_format_printf__(6, 7);

void
job_broker_set_log_emitter(job_broker_log_emitter_t emitter);

#ifdef __cplusplus
}
#endif // __cplusplus
