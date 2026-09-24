#pragma once

#include "sqc_apis.h"
#include "sqc_rpc_sched.h"
#include "dbmgr.h"

__BEGIN_DECLS

sqc_result_t
req_invoker_invoke(const dbmgr_job_info_t ji_ptr);

__END_DECLS

