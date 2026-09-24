#include "sqc_apis.h"
#include "sqc_thread_internal.h"
#include "sqc_rpc_sched_util.h"
#include "sqc_rpc_sched_conv_enums.h"
#include "dbmgr.h"

#include "req_invoker.h"

#include "dummy_invoker.c"
#include "rest_invoker.c"

#define INVOKE_FUNC_NUM (SQC_RPC_SCHED_QC_TYPE_DUMMY + 1)

typedef void (*invoke_func_t)(const dbmgr_job_info_t);

static invoke_func_t invoke_funcs[INVOKE_FUNC_NUM] = {
  [SQC_RPC_SCHED_QC_TYPE_RQC_REST] = s_rqc_invoke,
  [SQC_RPC_SCHED_QC_TYPE_IBM_REST] = s_ibm_invoke,
  [SQC_RPC_SCHED_QC_TYPE_SLURM_REST] = s_slurm_invoke,
  [SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_SYSTEM_TOKEN] = s_oqtopus_invoke,
  [SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_USER_TOKEN] = s_oqtopus_invoke,
  [SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_BOTH_TOKEN] = s_oqtopus_invoke,
  [SQC_RPC_SCHED_QC_TYPE_DUMMY] = s_dummy_invoke,
};

static inline void
s_req_invoker_update_group_exec_time_total_msec(dbmgr_job_info_t ji_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  char *job_id = NULL;
  char *group_id = NULL;
  dbmgr_group_info_t gi_ptr = NULL;
  uint8_t priority;
  uint64_t exec_time_estimate_msec;
  uint64_t exec_time_msec;
  uint64_t weight;
  uint64_t exec_time_diff_msec;

  rc = dbmgr_ji_get_job_id(ji_ptr, &job_id);
  if (rc != SQC_RESULT_OK) {
    sqc_msg_error("Failed to update group exec_time_total_msec: err_msg=%s\n",
                  sqc_error_get_string(rc));
    goto error;
  }

  rc = dbmgr_ji_get_group_id(ji_ptr, &group_id);
  if (rc != SQC_RESULT_OK) {
    sqc_msg_error("Failed to update group exec_time_total_msec: job_id=%s, err_msg=%s\n",
                  job_id, sqc_error_get_string(rc));
    goto error;
  }

  rc = dbmgr_gi_group_find(group_id, &gi_ptr);
  if (rc != SQC_RESULT_OK) {
    sqc_msg_error("Failed to update group exec_time_total_msec: job_id=%s, group_id=%s, err_msg=%s\n",
                  job_id, group_id, sqc_error_get_string(rc));
    goto error;
  }

  rc = dbmgr_ji_get_priority(ji_ptr, &priority);
  if (rc != SQC_RESULT_OK) {
    sqc_msg_error("Failed to update group exec_time_total_msec: job_id=%s, group_id=%s, err_msg=%s\n",
                  job_id, group_id, sqc_error_get_string(rc));
    goto error;
  }

  rc = dbmgr_ji_get_exec_time_estimate_msec(ji_ptr, &exec_time_estimate_msec);
  if (rc != SQC_RESULT_OK) {
    sqc_msg_error("Failed to update group exec_time_total_msec: job_id=%s, group_id=%s, err_msg=%s\n",
                  job_id, group_id, sqc_error_get_string(rc));
    goto error;
  }

  rc = dbmgr_ji_get_exec_time_msec(ji_ptr, &exec_time_msec);
  if (rc != SQC_RESULT_OK) {
    sqc_msg_error("Failed to update group exec_time_total_msec: job_id=%s, group_id=%s, err_msg=%s\n",
                  job_id, group_id, sqc_error_get_string(rc));
    goto error;
  }

  rc = dbmgr_wi_get_weight(priority, &weight);
  if (rc != SQC_RESULT_OK) {
    sqc_msg_error("Failed to update group exec_time_total_msec: job_id=%s, group_id=%s, err_msg=%s\n",
                  job_id, group_id, sqc_error_get_string(rc));
    goto error;
  }

  exec_time_msec = sqc_rpc_sched_util_calc_weighted_execution_time(exec_time_msec, weight);

  if (exec_time_msec > exec_time_estimate_msec) {
    exec_time_diff_msec = exec_time_msec - exec_time_estimate_msec;
    rc = dbmgr_gi_add_exec_time_total_msec(gi_ptr, exec_time_diff_msec);
    if (rc != SQC_RESULT_OK) {
      sqc_msg_error("Failed to update group exec_time_total_msec: job_id=%s, group_id=%s, err_msg=%s\n",
                    job_id, group_id, sqc_error_get_string(rc));
      goto error;
    }

    sqc_msg_info("Update(add) group exec_time_total_msec: job_id=%s, group_id=%s, exec_time_diff_msec=%ld\n",
                 job_id, group_id, exec_time_diff_msec);
  } else if (exec_time_msec < exec_time_estimate_msec) {
    exec_time_diff_msec = exec_time_estimate_msec - exec_time_msec;
    rc = dbmgr_gi_subtract_exec_time_total_msec(gi_ptr, exec_time_diff_msec);
    if (rc != SQC_RESULT_OK) {
      sqc_msg_error("Failed to update group exec_time_total_msec: job_id=%s, group_id=%s, err_msg=%s\n",
                    job_id, group_id, sqc_error_get_string(rc));
      goto error;
    }

    sqc_msg_info("Update(subtract) group exec_time_total_msec: job_id=%s, group_id=%s, exec_time_diff_msec=%ld\n",
                 job_id, group_id, exec_time_diff_msec);
  } else {
    // Do nothing.
    sqc_msg_info("No Update group exec_time_total_msec: job_id=%s, group_id=%s\n",
                 job_id, group_id);
  }

error:
  free(job_id);
  job_id = NULL;
  free(group_id);
  group_id = NULL;
}


/*
 * export
 */


sqc_result_t
req_invoker_invoke(const dbmgr_job_info_t ji_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  char *job_id = NULL;
  sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_UNKNOWN;
  sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;

  if (ji_ptr == NULL) {
    rc = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("Job is invalid: err_msg=%s\n", sqc_error_get_string(rc));
    goto error;
  }

  rc = dbmgr_ji_get_job_id(ji_ptr, &job_id);
  if (rc != SQC_RESULT_OK) {
    sqc_msg_error("Failed to get db job id: err_msg=%s\n", sqc_error_get_string(rc));
    if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
      sqc_msg_error("Failed to transition status to error.\n");
    }
    goto error;
  }

  rc = dbmgr_ji_get_qc_type(ji_ptr, &qc_type);
  if (rc != SQC_RESULT_OK) {
    sqc_msg_error("Failed to get qc_type: job_id=%s, err_msg=%s\n",
                  job_id, sqc_error_get_string(rc));
    if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
      sqc_msg_error("Failed to transition status to error: job_id=%s\n", job_id);
    }
    goto error;
  }

  rc = dbmgr_ji_get_status(ji_ptr, &status);
  if (rc != SQC_RESULT_OK) {
    sqc_msg_error("Failed to get db job status: job_id=%s, err_msg=%s\n",
                  job_id, sqc_error_get_string(rc));
    if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
      sqc_msg_error("Failed to transition status to error: job_id=%s\n", job_id);
    }
    goto error;
  }

  sqc_msg_info("Starting job invocation: job_id=%s, qc_type=%d status=%d(%s)\n",
               job_id, qc_type, status, sqc_rpc_sched_job_status_to_string(status));

  if (status == SQC_RPC_SCHED_JOB_STATUS_CANCELLED) {
    rc = SQC_RESULT_OK;
    // Do nothing.
    sqc_msg_info("Job was canceled and did not run: job_id=%s\n", job_id);
    goto error;
  } else if (status == SQC_RPC_SCHED_JOB_STATUS_DELETED) {
    rc = SQC_RESULT_OK;
    // Do nothing.
    sqc_msg_info("Job was deleted and did not run: job_id=%s\n", job_id);
    goto error;
  } else if (status != SQC_RPC_SCHED_JOB_STATUS_QUEUED) {
    rc = SQC_RESULT_OK;
    // Do nothing.
    sqc_msg_error("Job did not run due to an invalid status: job_id=%s, status=%d(%s)\n",
                  job_id, status, sqc_rpc_sched_job_status_to_string(status));
    goto error;
  }

  if (invoke_funcs[qc_type] != NULL) {
    sqc_msg_info("Invoking job: job_id=%s, qc_type=%d status=%d(%s)\n",
                 job_id, qc_type, status, sqc_rpc_sched_job_status_to_string(status));

    // Invoke job
    (*invoke_funcs[qc_type])(ji_ptr);

    // Update group exec_time_total_msec
    s_req_invoker_update_group_exec_time_total_msec(ji_ptr);
  } else {
    rc = SQC_RESULT_ANY_FAILURES;
    sqc_msg_error("Unknown qc_type: job_id=%s, qc_type=%d\n", job_id, qc_type);
    if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
      sqc_msg_error("Failed to transition status to error: job_id=%s\n", job_id);
    }
    goto error;
  }

error:
  free(job_id);
  job_id = NULL;

  return rc;
}

