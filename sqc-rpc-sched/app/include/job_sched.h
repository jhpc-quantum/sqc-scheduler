#pragma once

#include "sqc_apis.h"
#include "sqc_rpc_sched.h"

__BEGIN_DECLS

sqc_result_t
job_sched_register(void);

sqc_result_t
job_sched_submit_job(const char *user_id, const char *group_id, uint8_t priority,
                     const char *qprogram, sqc_rpc_sched_circuit_fmt_t circuit_fmt, size_t shots,
                     sqc_rpc_sched_qc_type_t qc_type, sqc_rpc_sched_transpiler_t transpiler,
                     const char *remark, const char *user_token, char **job_id, char **reply_msg);

__END_DECLS

