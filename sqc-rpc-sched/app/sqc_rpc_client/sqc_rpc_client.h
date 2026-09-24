#pragma once

#include "sqc_apis.h"
#include "dbmgr.h"
#include "rpc_session_client.h"

#define SQC_RPC_CLIENT_MAX_JWT_SIZE  32767

extern const char *program_name;
extern const char *default_conf_dir;
extern const char *default_remark;


sqc_result_t
read_text_file(const char* filename, size_t max_len, char **data);

bool
parse_uint(const char *arg, unsigned long long max_value, unsigned long long *value);

bool
parse_uint8(const char *arg, uint8_t *value);

bool
parse_uint32(const char *arg, uint32_t *value);

bool
parse_uint64(const char *arg, uint64_t *value);

bool
parse_size_t(const char *arg, size_t *value);

bool
parse_option_with_value(const char *arg, const char *opt, const char **opt_value);

bool
parse_common_option(const char *arg, const char **server, bool *prefer_ipv4,
                    rpc_auth_method_t *auth_method, const char **conf_dir);

void
print_rpc_common_result(sqc_result_t ret, sqc_result_t code, const char *msg);

void
print_common_options(void);

sqc_result_t
rpc_submit_job(rpc_session_client_t *session, uint8_t priority, const char *qprogram,
               sqc_rpc_sched_circuit_fmt_t circuit_fmt, size_t shots, sqc_rpc_sched_qc_type_t qc_type,
               sqc_rpc_sched_transpiler_t transpiler, const char *remark,
               const char *user_token, const char *group_id,
               char **job_id);

sqc_result_t
rpc_job_status(rpc_session_client_t *session, const char *job_id,
               sqc_rpc_sched_job_status_t *status);

sqc_result_t
rpc_cancel_job(rpc_session_client_t *session, const char *job_id);

sqc_result_t
rpc_delete_job(rpc_session_client_t *session, const char *job_id);

sqc_result_t
rpc_job_list(rpc_session_client_t *session,   rpc_job_info_t ***jobs, size_t *n_jobs);

sqc_result_t
subcmd_submit_main(int argc, char *argv[], int arg_index);

sqc_result_t
subcmd_status_main(int argc, char *argv[], int arg_index);

sqc_result_t
subcmd_cancel_main(int argc, char *argv[], int arg_index);

sqc_result_t
subcmd_delete_main(int argc, char *argv[], int arg_index);

sqc_result_t
subcmd_list_main(int argc, char *argv[], int arg_index);

sqc_result_t
subcmd_adm_del_jobs_main(int argc, char *argv[], int arg_index);

sqc_result_t
subcmd_adm_add_user_main(int argc, char *argv[], int arg_index);

sqc_result_t
subcmd_adm_set_user_status_main(int argc, char *argv[], int arg_index);

sqc_result_t
subcmd_adm_set_group_exec_time_limit_main(int argc, char *argv[], int arg_index);

sqc_result_t
subcmd_adm_set_user_group_status_main(int argc, char *argv[], int arg_index);
