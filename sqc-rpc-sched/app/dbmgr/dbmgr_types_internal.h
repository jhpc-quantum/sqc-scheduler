#pragma once

#include "sqc_apis.h"
#include "sqc_rpc_sched.h"

#include "dbmgr_types.h"

__BEGIN_DECLS

typedef struct dbmgr_user_info_record {
  sqc_rwlock_t rwlck_;

  char user_id[SQC_RPC_SCHED_USER_ID_MAX_SIZE + 1];
  size_t user_id_len;

  sqc_rpc_sched_user_role_type_t role_type;

  sqc_rpc_sched_user_status_t status;

  sqc_chrono_t created_time;
  sqc_chrono_t update_time;
} dbmgr_user_info_record;

typedef struct dbmgr_job_info_record {
  sqc_rwlock_t rwlck_;

  char job_id[SQC_RPC_SCHED_JOB_ID_MAX_SIZE + 1];
  size_t job_id_len;

  char user_id[SQC_RPC_SCHED_USER_ID_MAX_SIZE + 1];
  size_t user_id_len;

  uint8_t priority;

  sqc_rpc_sched_job_status_t status;

  char qc_job_id[SQC_RPC_SCHED_QC_JOB_ID_MAX_SIZE + 1];
  size_t qc_job_id_len;

  char qprogram[SQC_RPC_SCHED_QPROGRAM_MAX_SIZE + 1];
  size_t qprogram_len;

  sqc_rpc_sched_circuit_fmt_t circuit_fmt;

  size_t shots;

  sqc_rpc_sched_qc_type_t qc_type;

  sqc_rpc_sched_transpiler_t transpiler;

  char remark[SQC_RPC_SCHED_REMARK_MAX_SIZE + 1];
  size_t remark_len;

  char result[SQC_RPC_SCHED_RESULT_MAX_SIZE + 1];
  size_t result_len;

  char user_token[SQC_RPC_SCHED_USER_TOKEN_MAX_SIZE + 1];
  size_t user_token_len;

  sqc_chrono_t created_time;
  sqc_chrono_t queued_time;
  sqc_chrono_t running_time;
  sqc_chrono_t done_time;
  sqc_chrono_t cancelled_time;
  sqc_chrono_t error_time;
  sqc_chrono_t deleted_time;
  sqc_chrono_t update_time;
} dbmgr_job_info_record;

__END_DECLS

