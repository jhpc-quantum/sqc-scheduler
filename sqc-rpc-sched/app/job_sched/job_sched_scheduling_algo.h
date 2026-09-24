#pragma once

#include "sqc_apis.h"
#include "sqc_rpc_sched.h"

#include "dbmgr.h"

__BEGIN_DECLS

/*
 * A scheduling algorithm implementation (e.g. fair-share) plugs itself in by
 * providing one of these. Selected once at startup; never swapped while
 * running.
 */
typedef struct job_sched_scheduling_algo {
  const char *name;
  sqc_result_t (*initialize)(void);
  void (*finalize)(void);
  sqc_result_t (*submit)(const dbmgr_job_info_t ji_ptr);
  sqc_result_t (*dispatch)(void);
} job_sched_scheduling_algo_t;

sqc_result_t
job_sched_scheduling_algo_initialize(void);

void
job_sched_scheduling_algo_finalize(void);

sqc_result_t
job_sched_scheduling_algo_submit(const dbmgr_job_info_t ji_ptr);

sqc_result_t
job_sched_scheduling_algo_dispatch(void);

__END_DECLS

