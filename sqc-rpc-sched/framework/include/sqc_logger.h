#ifndef __SQC_LOGGER_H__
#define __SQC_LOGGER_H__





/**
 * @file	sqc_logger.h
 */





typedef enum {
  SQC_LOG_LEVEL_MIN = 0,
  SQC_LOG_LEVEL_UNKNOWN = SQC_LOG_LEVEL_MIN,
  SQC_LOG_LEVEL_DEBUG,
  SQC_LOG_LEVEL_INFO,
  SQC_LOG_LEVEL_NOTICE,
  SQC_LOG_LEVEL_WARNING,
  SQC_LOG_LEVEL_ERROR,
  SQC_LOG_LEVEL_FATAL,
  SQC_LOG_LEVEL_MAX
} sqc_log_level_t;


typedef enum {
  SQC_LOG_EMIT_TO_UNKNOWN = 0,
  SQC_LOG_EMIT_TO_FILE,
  SQC_LOG_EMIT_TO_SYSLOG
} sqc_log_destination_t;





__BEGIN_DECLS


/**
 * Initialize the logger.
 *
 *	@param[in]	dst	Where to log;
 *	\b SQC_LOG_EMIT_TO_UNKNOWN: stderr,
 *	\b SQC_LOG_EMIT_TO_FILE: Any regular file,
 *	\b SQC_LOG_EMIT_TO_SYSLOG: syslog
 *	@param[in]	arg	For \b SQC_LOG_EMIT_TO_FILE: a file name,
 *	for \b SQC_LOG_EMIT_TO_SYSLOG: An identifier for syslog.
 *	@param[in]	multi_process	If the \b dst is
 *	\b SQC_LOG_EMIT_TO_FILE, use \b true if the application shares
 *	the log file between child processes.
 *	@param[in]	emit_date	Use \b true if date is needed in each
 *	line header.
 *	@param[in]	debug_level	A debug level.
 *
 *	@retval	SQC_RESULT_OK		Succeeded.
 *	@retval	SQC_RESULT_ANY_FAILURES	Failed.
 */
sqc_result_t
sqc_log_initialize(sqc_log_destination_t dst,
                      const char *arg,
                      bool multi_process,
                      bool emit_date,
                      uint16_t log_level,
                      uint16_t debug_level,
                      size_t  rotate_size);


/**
 * Re-initialize the logger.
 *
 *	@details Calling this function implies 1) close opened log
 *	file. 2) re-open the log file, convenient for the log rotation.
 *
 *	@retval	SQC_RESULT_OK		Succeeded.
 *	@retval	SQC_RESULT_ANY_FAILURES	Failed.
 */
sqc_result_t	sqc_log_reinitialize(void);


/**
 * Finalize the logger.
 */
void	sqc_log_finalize(void);


/**
 * Synchronize the logger for fork(2).
 */
void	sqc_log_sync_for_fork(void);

/**
 * Set the log level for output.
 *
 *	@param[in]	lvl	A log level for output.
 */
void	sqc_log_set_log_level(uint16_t lvl);


/**
 * Get the log level for output.
 *
 *	@returns	The log level for output.
 */
uint16_t	sqc_log_get_log_level(void);


/**
 * Set the log rotate size.
 *
 *	@param[in]	sz	A log rotate size.
 */
void	sqc_log_set_rotate_size(size_t sz);


/**
 * Get the log level rotate.
 *
 *	@returns	The log rotate size.
 */
size_t	sqc_log_get_rotate_size(void);


/**
 * Set the log filename.
 *
 *	@param[in]	n	A log filename.
 */
void	sqc_log_set_filename(const char *n);


/**
 * Get the log filename.
 *
 *	@returns	The log log filename.
 */
const char *sqc_log_get_filename(void);


/**
 * Set the debug level.
 *
 *	@param[in]	lvl	A debug level.
 */
void	sqc_log_set_debug_level(uint16_t lvl);


/**
 * Get the debug level.
 *
 *	@returns	The debug level.
 */
uint16_t	sqc_log_get_debug_level(void);


/**
 * Check where the log is emitted to.
 *
 *	@param[out]	arg	A pointer to the argument that was passed via the \b sqc_log_get_destination(). (NULL allowed.)
 *
 *	@returns The log destination.
 *
 *	@details If the \b arg is specified as non-NULL pointer, the
 *	returned \b *arg must not be free()'d nor modified.
 */
sqc_log_destination_t
sqc_log_get_destination(const char **arg);


/**
 * Set/Unset multi-process mode
 *
 *	@param[in]	v	\btrue multi-process, \bfalse single-process.
 */
void
sqc_log_set_multi_process(bool v);


/**
 * Get current multi-process mode
 *
 *	@param[in]	v	a pointer to return value.
 *
 *	@returns Always SQC_REULT_OK
 */
sqc_result_t
sqc_log_get_multi_process(bool *v);


/**
 * The main logging workhorse: not intended for direct use.
 */
void	sqc_log_emit(sqc_log_level_t log_level,
                      uint64_t debug_level,
                      const char *file,
                      int line,
                      const char *func,
                      const char *fmt, ...)
__attr_format_printf__(6, 7);


__END_DECLS





#ifdef __GNUC__
#define __PROC__	__PRETTY_FUNCTION__
#else
#define	__PROC__	__func__
#endif /* __GNUC__ */


/**
 * Emit a debug message to the log.
 *
 *	@param[in]	level	A debug level (int).
 */
#define sqc_msg_debug(level, ...) \
  sqc_log_emit(SQC_LOG_LEVEL_DEBUG, (uint64_t)(level), \
                  __FILE__, __LINE__, __PROC__, __VA_ARGS__)


/**
 * Emit an informative message to the log.
 */
#define sqc_msg_info(...) \
  sqc_log_emit(SQC_LOG_LEVEL_INFO, 0LL, __FILE__, __LINE__, \
                  __PROC__, __VA_ARGS__)


/**
 * Emit a notice message to the log.
 */
#define sqc_msg_notice(...) \
  sqc_log_emit(SQC_LOG_LEVEL_NOTICE, 0LL, __FILE__, __LINE__, \
                  __PROC__, __VA_ARGS__)


/**
 * Emit a warning message to the log.
 */
#define sqc_msg_warning(...) \
  sqc_log_emit(SQC_LOG_LEVEL_WARNING, 0LL, __FILE__, __LINE__, \
                  __PROC__, __VA_ARGS__)


/**
 * Emit an error message to the log.
 */
#define sqc_msg_error(...) \
  sqc_log_emit(SQC_LOG_LEVEL_ERROR, 0LL, __FILE__, __LINE__, \
                  __PROC__, __VA_ARGS__)


/**
 * Emit a fatal message to the log.
 */
#define sqc_msg_fatal(...) \
  sqc_log_emit(SQC_LOG_LEVEL_FATAL, 0LL, __FILE__, __LINE__, \
                  __PROC__, __VA_ARGS__)


/**
 * Emit an arbitarary message to the log.
 */
#define sqc_msg(...) \
  sqc_log_emit(SQC_LOG_LEVEL_UNKNOWN, 0LL, __FILE__, __LINE__, \
                  __PROC__, __VA_ARGS__)


/**
 * The minimum level debug emitter.
 */
#define sqc_dprint(...) \
  sqc_msg_debug(1LL, __VA_ARGS__)


/**
 * Emit a readable error message for the errornous result.
 *
 *	@param[in]	s	A result code (sqc_result_t)
 */
#define sqc_perror(s) \
  do {                                                                  \
    (s == SQC_RESULT_POSIX_API_ERROR) ?                             \
    sqc_msg_error("SQC_RESULT_POSIX_API_ERROR: %s.\n",      \
                     strerror(errno)) :                            \
    sqc_msg_error("%s.\n", sqc_error_get_string((s)));      \
  } while (0)


/**
 * Emit an error message and exit.
 *
 *	@param[in]	ecode	An exit code (int)
 */
#define sqc_exit_error(ecode, ...) {        \
    sqc_msg_error(__VA_ARGS__);             \
    exit(ecode);                                \
  }


/**
 * Emit a fatal message and abort.
 */
#define sqc_exit_fatal(...) {                   \
    sqc_msg_fatal(__VA_ARGS__);                 \
    abort();                                        \
  }





#endif /* ! __SQC_LOGGER_H__ */
