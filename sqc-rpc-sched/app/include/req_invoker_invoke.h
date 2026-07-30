#pragma once

#include "sqc_rpc_sched.h"
#include "dbmgr.h"

__BEGIN_DECLS

sqc_result_t
req_invoker_enqueue(const dbmgr_job_info_t job_ptr);

__END_DECLS

