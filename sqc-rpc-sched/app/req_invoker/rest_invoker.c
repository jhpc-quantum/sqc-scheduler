#include "rexapis.h"

#include "sqc_apis.h"
#include "sqc_rpc_sched.h"

#include "dbmgr.h"

#define RQC_REST_BASE_URL "rqc-dummy-url"
#define RQC_REST_TOKEN "rqc-dummy-token"
#define RQC_REST_POLLING_INTERVAL 1000 // msec
#define RQC_REST_MAX_POLLING_COUNT 60

#define IBM_REST_BASE_URL "ibm-dummy-url"
#define IBM_REST_TOKEN "ibm-dummy-token"
#define IBM_REST_POLLING_INTERVAL 1000 // msec
#define IBM_REST_MAX_POLLING_COUNT 60

#define SLURM_REST_BASE_URL "slurm-dummy-url"
#define SLURM_REST_TOKEN "slurm-dummy-token"
#define SLURM_REST_POLLING_INTERVAL 1000 // msec
#define SLURM_REST_MAX_POLLING_COUNT 60

#define OQTOPUS_REST_BASE_URL "oqtopus-dummy-url"
#define OQTOPUS_REST_TOKEN "oqtopus-dummy-token"
#define OQTOPUS_REST_POLLING_INTERVAL 1000 // msec
#define OQTOPUS_REST_MAX_POLLING_COUNT 60

typedef enum {
  PARSED_JOB_STATUS_UNKNOWN = 0,
  PARSED_JOB_STATUS_RUNNING = 1,
  PARSED_JOB_STATUS_DONE = 2,
} parsed_job_status_t;

static const uint32_t s_rexapis_qc_type_map[SQC_RPC_SCHED_QC_TYPE_NUM] = {
  [SQC_RPC_SCHED_QC_TYPE_UNKNOWN]                    = REXAPIS_QC_TYPE_UNKNOW,
  [SQC_RPC_SCHED_QC_TYPE_RQC_REST]                   = REXAPIS_QC_TYPE_RQC_REST,
  [SQC_RPC_SCHED_QC_TYPE_IBM_REST]                   = REXAPIS_QC_TYPE_IBM_REST,
  [SQC_RPC_SCHED_QC_TYPE_SLURM_REST]                 = REXAPIS_QC_TYPE_SLURM_REST,
  [SQC_RPC_SCHED_QC_TYPE_QTM_GRPC]                   = REXAPIS_QC_TYPE_QTM_GRPC,
  [SQC_RPC_SCHED_QC_TYPE_QTM_SIM_GRPC]               = REXAPIS_QC_TYPE_QTM_SIM_GRPC,
  [SQC_RPC_SCHED_QC_TYPE_IBM_DACC]                   = REXAPIS_QC_TYPE_IBM_DACC,
  [SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_SYSTEM_TOKEN] = REXAPIS_QC_TYPE_A_OQTOPUSREST_SYSTEM_TOKEN,
  [SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_USER_TOKEN]   = REXAPIS_QC_TYPE_A_OQTOPUSREST_USER_TOKEN,
  [SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_BOTH_TOKEN]   = REXAPIS_QC_TYPE_A_OQTOPUSREST_BOTH_TOKEN
};

static const uint32_t rexapis_circuit_fmt_map[SQC_RPC_SCHED_CIRCUIT_FMT_JSON_NUM] = {
  [SQC_RPC_SCHED_CIRCUIT_FMT_UNKNOWN] = REXAPIS_CIRCUIT_FMT_UNKNOW,
  [SQC_RPC_SCHED_CIRCUIT_FMT_QASM]    = REXAPIS_CIRCUIT_FMT_QASM,
  [SQC_RPC_SCHED_CIRCUIT_FMT_QIR]     = REXAPIS_CIRCUIT_FMT_QIR,
  [SQC_RPC_SCHED_CIRCUIT_FMT_QPY]     = REXAPIS_CIRCUIT_FMT_QPY,
  [SQC_RPC_SCHED_CIRCUIT_FMT_JSON]    = REXAPIS_CIRCUIT_FMT_JSON
};

static const uint32_t rexapis_transpiler_map[SQC_RPC_SCHED_TRANSPILER_NUM] = {
  [SQC_RPC_SCHED_TRANSPILER_UNKNOWN] = REXAPIS_TRANSPILER_UNKNOW,
  [SQC_RPC_SCHED_TRANSPILER_NONE]    = REXAPIS_TRANSPILER_NONE,
  [SQC_RPC_SCHED_TRANSPILER_PASS]    = REXAPIS_TRANSPILER_PASS,
  [SQC_RPC_SCHED_TRANSPILER_NORMAL]  = REXAPIS_TRANSPILER_NORMAL
};

static inline uint32_t
to_rexapis_qc_type(sqc_rpc_sched_qc_type_t qc_type) {
    if ((uint32_t)qc_type >= SQC_RPC_SCHED_QC_TYPE_NUM) {
        sqc_msg_warning("Failed to qc_type convert: %d\n", qc_type);
        return REXAPIS_QC_TYPE_UNKNOW;
    }

    return s_rexapis_qc_type_map[qc_type];
}

static inline uint32_t
to_rexapis_circuit_fmt(sqc_rpc_sched_circuit_fmt_t circuit_fmt) {
    if ((uint32_t)circuit_fmt >= SQC_RPC_SCHED_CIRCUIT_FMT_JSON_NUM) {
        sqc_msg_warning("Failed to circuit_fmt convert: %d\n", circuit_fmt);
        return REXAPIS_CIRCUIT_FMT_UNKNOW;
    }

    return rexapis_circuit_fmt_map[circuit_fmt];
}

static inline uint32_t
to_rexapis_transpiler(sqc_rpc_sched_transpiler_t transpiler) {
    if ((uint32_t)transpiler >= SQC_RPC_SCHED_TRANSPILER_NUM) {
        sqc_msg_warning("Failed to transpiler convert: %d\n", transpiler);
        return REXAPIS_TRANSPILER_UNKNOW;
    }

    return rexapis_transpiler_map[transpiler];
}

// deprecated
static inline void
s_calculate_invoke(const dbmgr_job_info_t ji_ptr,
                   const char *base_url, const char *token,
                   const uint32_t polling_interval, const uint32_t max_polling_count) {
  char *qprogram = NULL;
  sqc_rpc_sched_circuit_fmt_t circuit_fmt;
  size_t shots = 0;
  sqc_rpc_sched_qc_type_t qc_type;
  sqc_rpc_sched_transpiler_t transpiler;
  char *remark = NULL;
  char *output = NULL;
  size_t output_len = 0;
  int ret = 0;

  if (dbmgr_set_job_status_running(ji_ptr) == SQC_RESULT_OK) {
    if (dbmgr_ji_get_qprogram(ji_ptr, &qprogram) == SQC_RESULT_OK &&
        dbmgr_ji_get_circuit_fmt(ji_ptr, &circuit_fmt) == SQC_RESULT_OK &&
        dbmgr_ji_get_qc_type(ji_ptr, &qc_type) == SQC_RESULT_OK &&
        dbmgr_ji_get_shots(ji_ptr, &shots) == SQC_RESULT_OK &&
        dbmgr_ji_get_transpiler(ji_ptr, &transpiler) == SQC_RESULT_OK &&
        dbmgr_ji_get_remark(ji_ptr, &remark) == SQC_RESULT_OK) {
      // Submit a job to QC
      ret = calculate(to_rexapis_qc_type(qc_type),
                      base_url, token, qprogram,
                      to_rexapis_circuit_fmt(circuit_fmt),
                      (uint32_t) shots,
                      to_rexapis_transpiler(transpiler),
                      remark, polling_interval, max_polling_count,
                      &output, &output_len);
      if (ret == 0) {
        sqc_msg_info("Successfully calculated.\n");

        // TODO: Add processing for large result sizes
        if (dbmgr_ji_set_result(ji_ptr, output, output_len) == SQC_RESULT_OK) {
          sqc_msg_info("Successfully saved the results.\n");

          if (dbmgr_set_job_status_done(ji_ptr) != SQC_RESULT_OK) {
            sqc_msg_error("Failed to transition status to done.\n");
          }
        } else {
          sqc_msg_error("Failed to save results.\n");

          if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
            sqc_msg_error("Failed to transition status to error.\n");
          }
        }
      } else {
        sqc_msg_error("Failed to calculate: %d\n", ret);

        if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
          sqc_msg_error("Failed to transition status to error.\n");
        }
      }
    } else {
      sqc_msg_error("Failed to retrieve information to calculate.\n");

      if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
        sqc_msg_error("Failed to transition status to error.\n");
      }
    }
  } else {
    sqc_msg_error("Failed to transition status to running.\n");
  }

  free((void *) qprogram);
  free((void *) remark);
  free((void *) output);
}

static inline parsed_job_status_t
s_ibm_status_checker(const char *qc_job_status) {
  // for debug
  sqc_msg_info("IBM job status: %s\n", qc_job_status);
  if (strncmp(qc_job_status, "Completed", 9) == 0 ||
      strncmp(qc_job_status, "Failed", 6) == 0 ||
      strncmp(qc_job_status, "Cancelled", 9) == 0) {
    return PARSED_JOB_STATUS_DONE;
  } else if (strncmp(qc_job_status, "Queued", 6) == 0 ||
             strncmp(qc_job_status, "Running", 7) == 0) {
    return PARSED_JOB_STATUS_RUNNING;
  }

  return PARSED_JOB_STATUS_UNKNOWN;
}

static inline parsed_job_status_t
s_slurm_status_checker(const char *qc_job_status) {
  // for debug
//  sqc_msg_info("Slurm job status: %s\n", qc_job_status);
  if (strncmp(qc_job_status, "COMPLETED", 9) == 0) {
    return PARSED_JOB_STATUS_DONE;
  } else if (strncmp(qc_job_status, "PENDING", 7) == 0 ||
             strncmp(qc_job_status, "PREEMPTED", 9) == 0 ||
             strncmp(qc_job_status, "RUNNING", 7) == 0 ||
             strncmp(qc_job_status, "SUSPENDED", 9) == 0) {
    return PARSED_JOB_STATUS_RUNNING;
  }

  return PARSED_JOB_STATUS_UNKNOWN;
}

static inline parsed_job_status_t
s_oqtopus_status_checker(const char *qc_job_status) {
  // for debug
//  sqc_msg_info("OQTOPUS job status: %s\n", qc_job_status);
  if (strncmp(qc_job_status, "succeeded", 9) == 0 ||
      strncmp(qc_job_status, "failed", 6) == 0 ||
      strncmp(qc_job_status, "cancelled", 9) == 0) {
    return PARSED_JOB_STATUS_DONE;
  } else if (strncmp(qc_job_status, "submitted", 9) == 0 ||
             strncmp(qc_job_status, "ready", 5) == 0 ||
             strncmp(qc_job_status, "running", 7) == 0) {
    return PARSED_JOB_STATUS_RUNNING;
  }

  return PARSED_JOB_STATUS_UNKNOWN;
}

static inline void
s_rest_invoke(const dbmgr_job_info_t ji_ptr,
              const char *base_url, const char *token,
              const uint32_t polling_interval, const uint32_t max_polling_count) {
  char *job_id = NULL;
  char *qprogram = NULL;
  sqc_rpc_sched_circuit_fmt_t circuit_fmt;
  size_t shots = 0;
  sqc_rpc_sched_qc_type_t qc_type;
  sqc_rpc_sched_transpiler_t transpiler;
  char *remark = NULL;
  int ret = 0;
  char *qc_job_id = NULL;
  size_t qc_job_id_len = 0;
  char *qc_job_status = NULL;
  size_t qc_job_status_len = 0;
  char *result = NULL;
  size_t result_len = 0;
  char *err_msg = NULL;
  size_t err_msg_len = 0;
  parsed_job_status_t parsed_status = PARSED_JOB_STATUS_UNKNOWN;
  sqc_chrono_t job_begin_time_nsec = 0;
  sqc_chrono_t job_end_time_nsec = 0;
  uint64_t exec_time_msec;

  if (likely(dbmgr_set_job_status_running(ji_ptr) == SQC_RESULT_OK)) {
    if (dbmgr_ji_get_job_id(ji_ptr, &job_id) == SQC_RESULT_OK &&
        dbmgr_ji_get_qprogram(ji_ptr, &qprogram) == SQC_RESULT_OK &&
        dbmgr_ji_get_circuit_fmt(ji_ptr, &circuit_fmt) == SQC_RESULT_OK &&
        dbmgr_ji_get_qc_type(ji_ptr, &qc_type) == SQC_RESULT_OK &&
        dbmgr_ji_get_shots(ji_ptr, &shots) == SQC_RESULT_OK &&
        dbmgr_ji_get_transpiler(ji_ptr, &transpiler) == SQC_RESULT_OK &&
        dbmgr_ji_get_remark(ji_ptr, &remark) == SQC_RESULT_OK) {

      // Start measuring the job execution time
      job_begin_time_nsec = sqc_chrono_now();

      // submit job
      {
        ret = submit_job(to_rexapis_qc_type(qc_type),
                         base_url, token, qprogram,
                         to_rexapis_circuit_fmt(circuit_fmt),
                         (uint32_t) shots,
                         to_rexapis_transpiler(transpiler),
                         remark, &qc_job_id, &qc_job_id_len,
                         &err_msg, &err_msg_len);
        if (ret == 0) {
          sqc_msg_info("submit_job succeeded: job_id=%s, qc_job_id=%s\n",
                       job_id, qc_job_id);

          // Save QC job ID
          if (dbmgr_ji_set_qc_job_id(ji_ptr, qc_job_id) == SQC_RESULT_OK) {
            sqc_msg_info("Save qc job id succeeded: job_id=%s, qc_job_id=%s\n",
                         job_id, qc_job_id);
          } else {
            sqc_msg_error("Failed to save qc job id: job_id=%s, qc_job_id=%s\n",
                          job_id, qc_job_id);
          }
        } else {
          sqc_msg_error("Failed to submit_job: job_id=%s, err_msg=%s\n", job_id, err_msg);
          if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
            sqc_msg_error("Failed to transition status to error: job_id=%s\n", job_id);
          }
          goto done;
        }
      }

      // get job status
      {
        for (uint32_t i = 0; i < max_polling_count; i++) {
          // check job cancel
          sqc_rpc_sched_job_status_t db_job_status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;
          if (likely(dbmgr_ji_get_status(ji_ptr, &db_job_status) == SQC_RESULT_OK)) {
            if (db_job_status == SQC_RPC_SCHED_JOB_STATUS_CANCELLED) {
              // execute cancel
              ret = cancel_job(to_rexapis_qc_type(qc_type),
                               base_url, token, qc_job_id, &err_msg, &err_msg_len);
              if (ret == 0) {
                  sqc_msg_info("cancel_job succeeded: job_id=%s, qc_job_id=%s\n",
                               job_id, qc_job_id);
                  goto done;
              } else {
                  sqc_msg_error("Failed to cancel_job: job_id=%s, qc_job_id=%s, err_msg=%s\n",
                                job_id, qc_job_id, err_msg);
                  if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
                    sqc_msg_error("Failed to transition status to error: job_id=%s, qc_job_id=%s\n",
                                  job_id, qc_job_id);
                  }
                  goto done;
              }
            }
          } else {
            sqc_msg_error("Failed to get db job status: job_id=%s, qc_job_id=%s\n",
                          job_id, qc_job_id);
            if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
              sqc_msg_error("Failed to transition status to error: job_id=%s, qc_job_id=%s\n",
                            job_id, qc_job_id);
            }
            goto done;
          }

          ret = get_job_status(to_rexapis_qc_type(qc_type),
                               base_url, token, qc_job_id,
                               &qc_job_status, &qc_job_status_len, &err_msg, &err_msg_len);
          if (ret == 0) {
            sqc_msg_debug(5, "get_job_status succeeded: job_id=%s, qc_job_id=%s, qc_job_status=%s\n",
                          job_id, qc_job_id, qc_job_status);

            // parse status of each QC
            if (qc_type == SQC_RPC_SCHED_QC_TYPE_IBM_REST) {
              parsed_status = s_ibm_status_checker(qc_job_status);
            } else if (qc_type == SQC_RPC_SCHED_QC_TYPE_SLURM_REST) {
              parsed_status = s_slurm_status_checker(qc_job_status);
            } else if (qc_type == SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_SYSTEM_TOKEN ||
                       qc_type == SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_USER_TOKEN ||
                       qc_type == SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_BOTH_TOKEN) {
              parsed_status = s_oqtopus_status_checker(qc_job_status);
            } else {
              sqc_msg_error("Unsupported qc: job_id=%s, qc_job_id=%s, qc_type=%d\n",
                            job_id, qc_job_id, qc_type);
              if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
                sqc_msg_error("Failed to transition status to error: job_id=%s, qc_job_id=%s\n",
                              job_id, qc_job_id);
              }
              goto done;
            }

            // determine processing from the results of parse status
            if (parsed_status == PARSED_JOB_STATUS_DONE) {
              break;
            } else if (parsed_status == PARSED_JOB_STATUS_RUNNING) {
              if (qc_job_status != NULL) {
                free((void *) qc_job_status);
                qc_job_status = NULL;
              }
              usleep(polling_interval * 1000);
              continue;
            } else {
              sqc_msg_error("Unknown parsed status: job_id=%s, qc_job_id=%s, parsed_status=%d\n",
                            job_id, qc_job_id, parsed_status);
              if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
                sqc_msg_error("Failed to transition status to error: job_id=%s, qc_job_id=%s\n",
                              job_id, qc_job_id);
              }
              goto done;
            }
          } else {
            sqc_msg_error("Failed to get_job_status: job_id=%s, qc_job_id=%s, err_msg=%s\n",
                          job_id, qc_job_id, err_msg);
            if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
              sqc_msg_error("Failed to transition status to error: job_id=%s, qc_job_id=%s\n",
                            job_id, qc_job_id);
            }
            goto done;
          }
        }

        // check job result
        if (qc_type == SQC_RPC_SCHED_QC_TYPE_IBM_REST) {
          if (qc_job_status != NULL && strcmp(qc_job_status, "Completed") == 0) {
            sqc_msg_info("Job completed successfully: job_id=%s, qc_job_id=%s\n",
                         job_id, qc_job_id);
          } else {
            sqc_msg_error("Job execution failed: job_id=%s, qc_job_id=%s\n",
                          job_id, qc_job_id);
            if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
              sqc_msg_error("Failed to transition status to error: job_id=%s, qc_job_id=%s\n",
                            job_id, qc_job_id);
            }
            goto done;
          }
        } else if (qc_type == SQC_RPC_SCHED_QC_TYPE_SLURM_REST) {
          if (qc_job_status != NULL && strcmp(qc_job_status, "COMPLETED") == 0) {
            sqc_msg_info("Job completed successfully: job_id=%s, qc_job_id=%s\n",
                         job_id, qc_job_id);
          } else {
            sqc_msg_error("Job execution failed: job_id=%s, qc_job_id=%s\n", job_id, qc_job_id);
            if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
              sqc_msg_error("Failed to transition status to error: job_id=%s, qc_job_id=%s\n",
                            job_id, qc_job_id);
            }
            goto done;
          }
        } else if (qc_type == SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_SYSTEM_TOKEN ||
                   qc_type == SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_USER_TOKEN ||
                   qc_type == SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_BOTH_TOKEN) {
          if (qc_job_status != NULL && strcmp(qc_job_status, "succeeded") == 0) {
            sqc_msg_info("Job completed successfully: job_id=%s, qc_job_id=%s\n", job_id, qc_job_id);
          } else {
            sqc_msg_error("Job execution failed: job_id=%s, qc_job_id=%s\n", job_id, qc_job_id);
            if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
              sqc_msg_error("Failed to transition status to error: job_id=%s, qc_job_id=%s\n",
                            job_id, qc_job_id);
            }
            goto done;
          }
        } else {
          // Unsupported qc_type is error in get_job_status
        }
      }

      // Stop measuring the job execution time
      job_end_time_nsec = sqc_chrono_now();

      // Calculate the job execution time(nano sec -> milli sec)
      exec_time_msec = (uint64_t)((job_end_time_nsec - job_begin_time_nsec) / 1000000LL);
      if (likely(dbmgr_ji_set_exec_time_msec(ji_ptr, exec_time_msec) == SQC_RESULT_OK)) {
        sqc_msg_info("Save exec_time_msec succeeded: job_id=%s, qc_job_id=%s, exec_time_msec=%ld\n",
                     job_id, qc_job_id, exec_time_msec);
      } else {
        sqc_msg_warning("Save exec_time_msec failed: job_id=%s, qc_job_id=%s, exec_time_msec=%ld\n",
                        job_id, qc_job_id, exec_time_msec);
      }

      // get job result
      {
        ret = get_job_result(to_rexapis_qc_type(qc_type),
                             base_url, token, qc_job_id,
                             &result, &result_len, &err_msg, &err_msg_len);
        if (ret == 0) {
          sqc_msg_info("get_job_result succeeded: job_id=%s, qc_job_id=%s, result_len=%ld\n",
                       job_id, qc_job_id, result_len);

          // TODO: Add processing for large result sizes
          if (likely(dbmgr_ji_set_result(ji_ptr, result, result_len) == SQC_RESULT_OK)) {
            sqc_msg_info("Save results succeeded: job_id=%s, qc_job_id=%s\n", job_id, qc_job_id);

            if (dbmgr_set_job_status_done(ji_ptr) != SQC_RESULT_OK) {
              sqc_msg_error("Failed to transition status to done: job_id=%s, qc_job_id=%s\n",
                            job_id, qc_job_id);
            }
          } else {
            sqc_msg_error("Failed to save results: job_id=%s, qc_job_id=%s\n", job_id, qc_job_id);

            if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
              sqc_msg_error("Failed to transition status to error: job_id=%s, qc_job_id=%s\n",
                            job_id, qc_job_id);
            }
          }
        } else {
          sqc_msg_error("Failed to get_job_result: job_id=%s, qc_job_id=%s, err_msg=%s\n",
                        job_id, qc_job_id, err_msg);
          if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
            sqc_msg_error("Failed to transition status to error: job_id=%s, qc_job_id=%s\n",
                          job_id, qc_job_id);
          }
          goto done;
        }
      }
    } else {
      sqc_msg_error("Failed to get job_info for invocation: job_id=%s\n",
                    job_id != NULL ? job_id : "(unknown)");
      if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
        sqc_msg_error("Failed to transition status to error: job_id=%s\n",
                      job_id != NULL ? job_id : "(unknown)");
      }
    }
  } else {
    sqc_msg_error("Failed to transition status to running: job_id=%s\n", job_id);

    if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
      sqc_msg_error("Failed to transition status to error: job_id=%s\n",
                    job_id != NULL ? job_id : "(unknown)");
    }
  }

done:
  free((void *) job_id);
  job_id = NULL;
  free((void *) qprogram);
  qprogram = NULL;
  free((void *) remark);
  remark = NULL;
  free((void *) qc_job_id);
  qc_job_id = NULL;
  free((void *) qc_job_status);
  qc_job_status = NULL;
  free((void *) result);
  result = NULL;
  free((void *) err_msg);
  err_msg = NULL;
}

static inline void
s_rqc_invoke(const dbmgr_job_info_t ji_ptr) {
  sqc_msg_info("invoke RQC REST\n");
  s_calculate_invoke(ji_ptr, RQC_REST_BASE_URL, RQC_REST_TOKEN,
                     RQC_REST_POLLING_INTERVAL, RQC_REST_MAX_POLLING_COUNT);
}

static inline void
s_ibm_invoke(const dbmgr_job_info_t ji_ptr) {
  sqc_msg_info("invoke IBM REST\n");
  s_rest_invoke(ji_ptr, IBM_REST_BASE_URL, IBM_REST_TOKEN,
                IBM_REST_POLLING_INTERVAL, IBM_REST_MAX_POLLING_COUNT);
}

static inline void
s_slurm_invoke(const dbmgr_job_info_t ji_ptr) {
  sqc_msg_info("invoke Slurm REST\n");
  s_rest_invoke(ji_ptr, SLURM_REST_BASE_URL, SLURM_REST_TOKEN,
                SLURM_REST_POLLING_INTERVAL, SLURM_REST_MAX_POLLING_COUNT);
}

static inline void
s_oqtopus_invoke(const dbmgr_job_info_t ji_ptr) {
  sqc_rpc_sched_qc_type_t qc_type;
  char *user_token = NULL;

  if (dbmgr_ji_get_qc_type(ji_ptr, &qc_type) == SQC_RESULT_OK) {
    if (qc_type == SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_SYSTEM_TOKEN) {
      sqc_msg_info("invoke OQTOPUS REST(system)\n");
      s_rest_invoke(ji_ptr, OQTOPUS_REST_BASE_URL, OQTOPUS_REST_TOKEN,
                    OQTOPUS_REST_POLLING_INTERVAL, OQTOPUS_REST_MAX_POLLING_COUNT);
    } else if (qc_type == SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_USER_TOKEN) {
      if (dbmgr_ji_get_user_token(ji_ptr, &user_token) == SQC_RESULT_OK &&
          IS_VALID_STRING(user_token) == true) {
        sqc_msg_info("invoke OQTOPUS REST(user)\n");
        s_rest_invoke(ji_ptr, OQTOPUS_REST_BASE_URL, user_token,
                      OQTOPUS_REST_POLLING_INTERVAL, OQTOPUS_REST_MAX_POLLING_COUNT);
      } else {
        sqc_msg_error("Failed to oqtopus_invoke: Failed to get user_token.\n");
        if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
          sqc_msg_error("Failed to transition status to error.\n");
        }
      }
    } else if (qc_type == SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_BOTH_TOKEN) {
      if (dbmgr_ji_get_user_token(ji_ptr, &user_token) == SQC_RESULT_OK &&
          IS_VALID_STRING(user_token) == true) {
        sqc_msg_info("invoke OQTOPUS REST(both-user)\n");
        s_rest_invoke(ji_ptr, OQTOPUS_REST_BASE_URL, user_token,
                      OQTOPUS_REST_POLLING_INTERVAL, OQTOPUS_REST_MAX_POLLING_COUNT);
      } else {
        sqc_msg_info("invoke OQTOPUS REST(both-system)\n");
        s_rest_invoke(ji_ptr, OQTOPUS_REST_BASE_URL, OQTOPUS_REST_TOKEN,
                      OQTOPUS_REST_POLLING_INTERVAL, OQTOPUS_REST_MAX_POLLING_COUNT);
      }
    } else {
      sqc_msg_error("Failed to oqtopus_invoke: Invalid qc-type=%d\n", qc_type);
      if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
        sqc_msg_error("Failed to transition status to error.\n");
      }
    }
  } else {
    sqc_msg_error("Failed to oqtopus_invoke: Failed to get qc_type.\n");
    if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
      sqc_msg_error("Failed to transition status to error.\n");
    }
  }

  free((void *) user_token);
  user_token = NULL;
}

