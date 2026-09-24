#include "sqc_apis.h"
#include "sqc_rpc_sched.h"
#include "sqc_rpc_sched_util.h"

/*
 * export
 */

uint64_t
sqc_rpc_sched_util_calc_weighted_execution_time(uint64_t exec_time_msec, uint64_t weight) {
  return ((exec_time_msec* weight) / SQC_RPC_SCHED_WEIGHT_SCALE);
}

void
sqc_rpc_sched_util_create_message_text(char **text, const char *format, ...) {
  va_list args;
  va_start(args, format);
  if (text != NULL) {
    if (vasprintf(text, format, args) == -1) {
      *text = NULL;
    }
  }
  va_end(args);
}

