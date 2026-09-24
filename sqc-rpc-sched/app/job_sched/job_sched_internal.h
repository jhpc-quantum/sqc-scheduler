#pragma once

#include "sqc_apis.h"
#include "sqc_rpc_sched.h"

#include "dbmgr.h"

__BEGIN_DECLS

sqc_result_t
job_sched_internal_initialize(void);

void
job_sched_internal_finalize(void);

sqc_result_t
job_sched_internal_enqueue(const dbmgr_job_info_t ji_ptr);

sqc_result_t
job_sched_internal_invoke_job(void);

__END_DECLS

