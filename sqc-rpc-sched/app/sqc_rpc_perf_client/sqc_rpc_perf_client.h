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
parse_size_t(const char *arg, size_t *value);

bool
parse_option_with_value(const char *arg, const char *opt, const char **opt_value);

bool
parse_common_option(const char *arg, const char **server, bool *prefer_ipv4,
                    rpc_auth_method_t *auth_method, const char **conf_dir);

void
print_common_options(void);

sqc_result_t
rpc_submit_job(rpc_session_client_t *session, uint8_t priority, const char *qprogram,
               sqc_rpc_sched_circuit_fmt_t circuit_fmt, size_t shots, sqc_rpc_sched_qc_type_t qc_type,
               sqc_rpc_sched_transpiler_t transpiler, const char *remark, const char *user_token,
               char **job_id, uint32_t thread_id, uint32_t loop_count);

sqc_result_t
subcmd_submit_main(int argc, char *argv[], int arg_index);

void*
thread_entry(void *arg);
