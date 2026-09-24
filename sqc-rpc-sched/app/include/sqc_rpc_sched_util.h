#pragma once

__BEGIN_DECLS

///
/// @brief   Calculates a weighted value based on the job's execution time.
///
/// @param[in]     format       Job's execution time.
/// @param[in]     format       Weight.
///
/// @return The weighted execution time.
///
uint64_t
sqc_rpc_sched_util_calc_weighted_execution_time(uint64_t exec_time_msec, uint64_t weight);

///
/// @brief   Create a text string for a reply message.
///
/// @param[out]    text         A formatted text.
/// @param[in]     format       A format string.
/// @param[in]     ...          Arguments depending on `format`.
///
/// @details The function <tt>rpc_create_message_text()</tt> is equivalent with `vasprintf(3)`,
/// but its return type is `void`.  When `vasprintf(3)` fails to allocates a string, `*text`
/// is set to NULL.
///
void
sqc_rpc_sched_util_create_message_text(char **text, const char *format, ...)
  __attr_format_printf__(2, 3);

__END_DECLS

