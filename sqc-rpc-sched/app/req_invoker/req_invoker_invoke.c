#include "sqc_apis.h"
#include "sqc_thread_internal.h"
#include "dbmgr.h"
#include "req_invoker_invoke.h"

#include "dummy_invoker.c"
#include "rest_invoker.c"

#define INVOKE_FUNC_NUM (SQC_RPC_SCHED_QC_TYPE_DUMMY + 1)

typedef void (*invoke_func_t)(const dbmgr_job_info_t);

static sqc_bbq_t req_invoker_priority_qs[SQC_RPC_SCHED_PQ_NUM];
static invoke_func_t invoke_funcs[INVOKE_FUNC_NUM] = { NULL };


static inline void
s_req_invoker_invoke_job(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  sqc_chrono_t wait_time;
  dbmgr_job_info_t ji_ptr = NULL;
  char *job_id = NULL;
  sqc_rpc_sched_job_status_t db_job_status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;
  sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_UNKNOWN;

  for (int i = SQC_RPC_SCHED_MAX_PRIORITY; i >= 0; i--) {
    // wait in the highest priority queue
    if (i == SQC_RPC_SCHED_MAX_PRIORITY) {
      wait_time = SQC_RPC_SCHED_PQ_GET_TIMEOUT;
    } else {
      wait_time = 0LL;
    }

    rc = sqc_bbq_get(&req_invoker_priority_qs[i], &ji_ptr, dbmgr_job_info_t, wait_time);
    if (likely(rc == SQC_RESULT_OK && ji_ptr != NULL)) {
      if (likely(dbmgr_ji_get_job_id(ji_ptr, &job_id)  == SQC_RESULT_OK)) {
        sqc_msg_info("Job dequeued: job_id=%s\n", job_id);

        if (likely(dbmgr_ji_get_status(ji_ptr, &db_job_status) == SQC_RESULT_OK)) {
          if (likely(db_job_status == SQC_RPC_SCHED_JOB_STATUS_QUEUED)) {
            if (likely(dbmgr_ji_get_qc_type(ji_ptr, &qc_type) == SQC_RESULT_OK)) {
              if (invoke_funcs[qc_type] != NULL) {
                // run job
                (*invoke_funcs[qc_type])(ji_ptr);
              } else {
                sqc_msg_error("Unknown qc_type: job_id=%s, qc_type=%d\n", job_id, qc_type);
                if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
                  sqc_msg_error("Failed to transition status to error: job_id=%s, err_msg=%s\n", job_id, sqc_error_get_string(rc));
                }
              }
            } else {
              sqc_msg_error("Failed to get qc_type: job_id=%s, err_msg=%s\n", job_id, sqc_error_get_string(rc));
              if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
                sqc_msg_error("Failed to transition status to error: job_id=%s\n", job_id);
              }
            }
          } else if (db_job_status == SQC_RPC_SCHED_JOB_STATUS_CANCELLED) {
            // Do nothing.
            sqc_msg_info("Job was canceled and did not run: job_id=%s\n", job_id);
          } else if (db_job_status == SQC_RPC_SCHED_JOB_STATUS_DELETED) {
            // Do nothing.
            sqc_msg_info("Job was deleted and did not run: job_id=%s\n", job_id);
          } else {
            // Do nothing.
            sqc_msg_error("Job did not run due to an invalid status: job_id=%s, db_job_status=%d\n", job_id, db_job_status);
          }
        } else {
          sqc_msg_error("Failed to get db job status: job_id=%s\n", job_id);
          if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
            sqc_msg_error("Failed to transition status to error: job_id=%s\n", job_id);
          }
        }
      } else {
        sqc_msg_error("Failed to get db job id\n");
        if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
          sqc_msg_error("Failed to transition status to error.\n");
        }
      }

      free(job_id);
      job_id = NULL;

      break;
    } else {
      // Do nothing.
    }
  }
}


static inline sqc_result_t
s_req_invoker_enqueue(const dbmgr_job_info_t ji_ptr, const bool change_status) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  char *job_id = NULL;
  uint8_t priority = 0;

  // TODO: check qc_type

  if (likely(ji_ptr != NULL &&
             dbmgr_ji_get_job_id(ji_ptr, &job_id) == SQC_RESULT_OK &&
             dbmgr_ji_get_priority(ji_ptr, &priority) == SQC_RESULT_OK)) {
    rc = sqc_bbq_put(&req_invoker_priority_qs[priority], &ji_ptr,
                     dbmgr_job_info_t, SQC_RPC_SCHED_PQ_PUT_TIMEOUT);
    if (likely(rc == SQC_RESULT_OK)) {
      sqc_msg_info("Job enqueued: job_id=%s\n", job_id);

      if (change_status == true) {
        rc = dbmgr_set_job_status_queued(ji_ptr);
        if (rc != SQC_RESULT_OK) {
          sqc_msg_error("Failed to transition status to queued: job_id=%s, err_msg=%s\n", job_id, sqc_error_get_string(rc));
        }
      }
    } else {
      sqc_msg_error("Failed to put job: job_id=%s, err_msg=%s\n", job_id, sqc_error_get_string(rc));
      if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
        sqc_msg_error("Failed to transition status to error: job_id=%s\n", job_id);
      }
    }
  } else {
    rc = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("Failed to put job: %s\n", sqc_error_get_string(rc));
  }

  free(job_id);

  return rc;
}


static void
s_req_invoker_invoke_priority_q_freeup(void **val) {
  (void) val;

  // Do nothing. Free of record is performed with DB Mgr.
}


static sqc_result_t
s_req_invoker_invoke_initialize(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  dbmgr_job_info_t *queued_ji_ptr_arr = NULL;
  size_t queued_arr_len = 0;
  char *job_id = NULL;

  // Create job queues by priority
  for (int i = 0; i <= SQC_RPC_SCHED_MAX_PRIORITY; i++) {
    if ((rc = sqc_bbq_create(&req_invoker_priority_qs[i], sqc_callout_task_t,
                             SQC_RPC_SCHED_PQ_LEN, s_req_invoker_invoke_priority_q_freeup)) != SQC_RESULT_OK) {
      sqc_perror(rc);
      return rc;
    }
  }

  invoke_funcs[SQC_RPC_SCHED_QC_TYPE_RQC_REST] = s_rqc_invoke;
  invoke_funcs[SQC_RPC_SCHED_QC_TYPE_IBM_REST] = s_ibm_invoke;
  invoke_funcs[SQC_RPC_SCHED_QC_TYPE_SLURM_REST] = s_slurm_invoke;
  invoke_funcs[SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_SYSTEM_TOKEN] = s_oqtopus_invoke;
  invoke_funcs[SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_USER_TOKEN] = s_oqtopus_invoke;
  invoke_funcs[SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_BOTH_TOKEN] = s_oqtopus_invoke;
  invoke_funcs[SQC_RPC_SCHED_QC_TYPE_DUMMY] = s_dummy_invoke;

  // Find job in status running
  rc = dbmgr_ji_job_find_by_job_status(SQC_RPC_SCHED_JOB_STATUS_RUNNING,
                                       &queued_ji_ptr_arr, &queued_arr_len);
  if (likely(rc == SQC_RESULT_OK)) {
    sqc_msg_info("Found job(running): %ld\n", queued_arr_len);

    if (queued_arr_len > 0) {
      for (size_t i = 0; i < queued_arr_len; i++) {
        rc = dbmgr_ji_get_job_id(queued_ji_ptr_arr[i], &job_id);
        if (likely(rc == SQC_RESULT_OK)) {
          sqc_msg_warning("Scheduler terminated during job execution. "
                          "Therefore, the job execution cancelled: job_id=%s\n", job_id);

          if (dbmgr_set_job_status_cancelled(queued_ji_ptr_arr[i]) != SQC_RESULT_OK) {
            sqc_msg_error("Failed to transition status to cancelled: "
                          "job_id=%s, err_msg=%s\n", job_id, sqc_error_get_string(rc));
          }

          free(job_id);
          job_id = NULL;
        }
      }
    }
  }
  free(queued_ji_ptr_arr);
  queued_ji_ptr_arr = NULL;
  queued_arr_len = 0;

  // Find job in status queued
  rc = dbmgr_ji_job_find_by_job_status(SQC_RPC_SCHED_JOB_STATUS_QUEUED,
                                       &queued_ji_ptr_arr, &queued_arr_len);
  if (likely(rc == SQC_RESULT_OK)) {
    sqc_msg_info("Found job(queued): %ld\n", queued_arr_len);

    if (queued_arr_len > 0) {
      rc = dbmgr_ji_job_arr_sort_by_created_time(queued_ji_ptr_arr, queued_arr_len);
      if (likely(rc == SQC_RESULT_OK)) {
        for (size_t i = 0; i < queued_arr_len; i++) {
          rc = s_req_invoker_enqueue(queued_ji_ptr_arr[i], false);
          if (rc != SQC_RESULT_OK) {
            break;
          }
        }
      } else {
        sqc_msg_error("Failed to sort job array for status queued: %s\n", sqc_error_get_string(rc));
      }
    }
  } else {
    sqc_msg_error("Failed to find job with status queued: %s\n", sqc_error_get_string(rc));
  }
  free(queued_ji_ptr_arr);
  queued_ji_ptr_arr = NULL;

  return rc;
}


static void
s_req_invoker_invoke_finalize(void) {
  for (int i = 0; i <= SQC_RPC_SCHED_MAX_PRIORITY; i++) {
    if (req_invoker_priority_qs[i] != NULL) {
      sqc_bbq_destroy(&req_invoker_priority_qs[i], true);
      req_invoker_priority_qs[i] = NULL;
    }
  }
}


/*
 * export
 */


sqc_result_t
req_invoker_enqueue(const dbmgr_job_info_t ji_ptr) {
  return s_req_invoker_enqueue(ji_ptr, true);
}

