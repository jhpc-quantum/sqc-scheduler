#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <mutex>
#include <string>

#include "job_broker_logger_internal.h"

static job_broker_log_emitter_t s_log_emitter = nullptr;
static log_level_t s_log_level = LOG_LEVEL_DEBUG;
static uint64_t s_debug_level = 0;

static const char *const s_log_level_strs[] = {
  "",
  "[DEBUG]",
  "[INFO ]",
  "[NOTE ]",
  "[WARN ]",
  "[ERROR]",
  "[FATAL]",
  nullptr
};

static std::mutex s_mutex;

void
job_broker_log_initialize() {
  char *level_str = getenv("SQC_LOG_LEVEL");
  if (level_str != nullptr) {
    s_log_level = log_level_t(atoi(level_str));
  }

  char *debug_level_str = getenv("SQC_LOG_DEBUGLEVEL");
  if (debug_level_str != nullptr) {
    s_debug_level = uint64_t(atoi(debug_level_str));
  }
}

void
job_broker_set_log_emitter(job_broker_log_emitter_t emitter) {
  std::lock_guard<std::mutex> lock(s_mutex);
  s_log_emitter = emitter;
}

void
job_broker_log_emit(log_level_t log_level,
                    uint64_t debug_level,
                    const char *file,
                    int line,
                    const char *func,
                    const char *fmt, ...) {
  char msg[8192];

  if (log_level < s_log_level || (log_level == LOG_LEVEL_DEBUG && log_level > s_debug_level)) {
    return;
  }

  if (s_log_emitter != nullptr) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);
    s_log_emitter(static_cast<int>(log_level), debug_level, file, line, func, "%s", msg);
  }
}

static std::string
s_get_datetime_string() {
  struct timespec spec;
  clock_gettime(CLOCK_REALTIME, &spec);
  struct tm tm;
  localtime_r(&spec.tv_sec, &tm);
  char ftime_str[32];
  strftime(ftime_str, sizeof(ftime_str), "%FT%T", &tm);  // yyyy-mm-dd 'T' HH-MM-SS
  long abs_gmtoff;
  char gmtoff_sign;
  if (tm.tm_gmtoff >= 0) {
    abs_gmtoff = long(tm.tm_gmtoff);
    gmtoff_sign = '+';
  } else {
    abs_gmtoff = long(-tm.tm_gmtoff);
    gmtoff_sign = '-';
  }
  char datetime_str[128];
  snprintf(datetime_str, sizeof(datetime_str), "%s.%06ld%c%02ld:%02ld",
           ftime_str, static_cast<long>(spec.tv_nsec / 1'000) % 1'000'000,
           gmtoff_sign, abs_gmtoff / 3600 % 24, abs_gmtoff % 3600);
  return std::string(datetime_str);
}

void
job_broker_log_emit_to_stderr(int log_level,
                              uint64_t debug_level,
                              const char *file,
                              int line,
                              const char *func,
                              const char *fmt, ...) {
  static_cast<void>(debug_level);
  auto datetime = s_get_datetime_string();

  std::lock_guard<std::mutex> lock(s_mutex);
  if (log_level >= LOG_LEVEL_UNKNOWN && log_level <= LOG_LEVEL_FATAL) {
    fprintf(stderr, "%s %s %s:%d:%s: ",
            datetime.c_str(), s_log_level_strs[log_level], file, line, func);
  }

  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
}
