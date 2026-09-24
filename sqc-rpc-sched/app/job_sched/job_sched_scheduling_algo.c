#include "sqc_apis.h"

#include "job_sched_scheduling_algo.h"
#include "job_sched_fair_share.h"

static const job_sched_scheduling_algo_t *s_algo = NULL;


/*
 * export
 */


sqc_result_t
job_sched_scheduling_algo_initialize(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  // Fixed to fair_share for now; revisit selection once a second
  // scheduling algorithm actually exists.
  s_algo = job_sched_fair_share_get_scheduling_algo();

  rc = s_algo->initialize();
  if (rc == SQC_RESULT_OK) {
    sqc_msg_info("Scheduling algorithm selected: %s\n", s_algo->name);
  }

  return rc;
}


void
job_sched_scheduling_algo_finalize(void) {
  if (s_algo != NULL) {
    s_algo->finalize();
    s_algo = NULL;
  }
}


sqc_result_t
job_sched_scheduling_algo_submit(const dbmgr_job_info_t ji_ptr) {
  return s_algo->submit(ji_ptr);
}


sqc_result_t
job_sched_scheduling_algo_dispatch(void) {
  return s_algo->dispatch();
}
