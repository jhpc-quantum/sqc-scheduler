#pragma once

#include "sqc_rpc_sched_enums.h"

__BEGIN_DECLS

bool
sqc_rpc_sched_circuit_fmt_from_string(const char *str, sqc_rpc_sched_circuit_fmt_t *circuit_fmt);

const char *
sqc_rpc_sched_circuit_fmt_to_string(sqc_rpc_sched_circuit_fmt_t circuit_fmt);

bool
sqc_rpc_sched_qc_type_from_string(const char *str, sqc_rpc_sched_qc_type_t *qc_type);

const char *
sqc_rpc_sched_qc_type_to_string(sqc_rpc_sched_qc_type_t qc_type);

bool
sqc_rpc_sched_transpiler_from_string(const char *str, sqc_rpc_sched_transpiler_t *transpiler);

const char *
sqc_rpc_sched_transpiler_to_string(sqc_rpc_sched_transpiler_t transpiler);

const char *
sqc_rpc_sched_job_status_to_string(sqc_rpc_sched_job_status_t status);

__END_DECLS
