#include "sqc_apis.h"

#include "req_sched.h"
#include "req_invoker.h"

/*
 * export
 */

sqc_result_t
req_sched_enqueue(const dbmgr_job_info_t ji_ptr) {
  if (likely(ji_ptr != NULL)) {
    return req_invoker_enqueue(ji_ptr);
  }

  return SQC_RESULT_INVALID_ARGS;
}

