#pragma once

#include "sqc_rpc_sched_enums.h"

__BEGIN_DECLS

sqc_rpc_sched_qc_type_t
sqc_rpc_sched_conf_get_qc_type(void);

void
sqc_rpc_sched_conf_set_qc_type(sqc_rpc_sched_qc_type_t qc_type);

sqc_result_t
sqc_rpc_sched_conf_set_qc_type_from_string(const char *str);

const char *
sqc_rpc_sched_conf_get_conf_dir(void);

sqc_result_t
sqc_rpc_sched_conf_set_conf_dir(const char *dir);

const char *
sqc_rpc_sched_conf_get_rpc_server_address(void);

sqc_result_t
sqc_rpc_sched_conf_set_rpc_server_address(const char *addr);

const char *
sqc_rpc_sched_conf_get_grpc_server_address(void);

sqc_result_t
sqc_rpc_sched_conf_set_grpc_server_address(const char *addr);
