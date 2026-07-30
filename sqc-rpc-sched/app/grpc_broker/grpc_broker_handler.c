#include "sqc_apis.h"

#include "dbmgr.h"
#include "job_broker_server.h"
#include "req_sched.h"
#include "rpc_jwt_server.h"
#include "rpc_session_server.h"
#include "sqc_rpc_sched_conf.h"
#include "sqc_rpc_sched_conv_enums.h"
#include "srv_session.h"

static const int num_cqs = 1;              // the default is 1.
static const int min_pollers = 1;          // the default is 1.
static const int max_pollers = 5;          // the default is 2.
static const int cq_timeout_msec = 10000;  // the default is 10000.

//
// Create a text string for a reply message.
//
void
s_grpc_create_message_text(char **text, const char *format, ...) __attr_format_printf__(2, 3);

void
s_grpc_create_message_text(char **text, const char *format, ...) {
  va_list args;
  va_start(args, format);
  if (text != NULL) {
    if (vasprintf(text, format, args) == -1) {
      *text = NULL;
    }
  }
  va_end(args);
}

#define s_grpc_log_debug_msg(level, prefix, msg) { \
  if (msg != NULL) { \
    sqc_msg_debug((level), "%s: %s\n", (prefix), (msg)); \
  } else { \
    sqc_msg_debug((level), "%s: (failed to create a log message)\n", (prefix)); \
  } \
}

#define s_grpc_log_info_msg(prefix, msg) { \
  if (msg != NULL) { \
    sqc_msg_info("%s: %s\n", (prefix), (msg));  \
  } else { \
    sqc_msg_info("%s: (failed to create a log message)\n", (prefix)); \
  } \
}

#define s_grpc_log_error_msg(prefix, msg) { \
  if (msg != NULL) { \
    sqc_msg_error("%s: %s\n", (prefix), (msg));  \
  } else { \
    sqc_msg_error("%s: (failed to create a log message)\n", (prefix)); \
  } \
}

//
// Special user name that means "all users".
//
static const char* all_users = "ALL";

static int64_t
s_grpc_broker_get_user_status(const char *subject, bool *enabled, bool *admin) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  dbmgr_user_info_t user_info = NULL;

  if (subject != NULL) {
    rc = dbmgr_ui_user_find(subject, &user_info);
    if (rc == SQC_RESULT_OK) {
      if (enabled != NULL) {
        *enabled = dbmgr_ui_is_user_enabled(user_info);
      }
      if (admin != NULL) {
        *admin = dbmgr_ui_is_user_admin(user_info);
      }
    } else {
      sqc_msg_debug(5, "Failed to get user status, %s: user=%s\n", sqc_error_get_string(rc), subject);
      rc = SQC_RESULT_AUTHENTICATION_ERROR;
    }
  } else {
    rc = SQC_RESULT_INVALID_ARGS;
    sqc_msg_debug(5, "Failed to get user status, %s: user=%s\n", sqc_error_get_string(rc), subject);
  }

  return rc;
}


static int64_t
s_grpc_broker_handler_submit_job(const char *token, uint32_t priority, const char *qprogram,
                                 int circuit_fmt, size_t shots, int qc_type, int transpiler,
                                 const char *remark, const char *user_token,
                                 char **job_id, char **reply_msg) {
  static const char* log_prefix = "gRPC-SUBMIT_JOB";
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  dbmgr_job_info_t ji_ptr = NULL;
  char *subject = NULL;
  bool is_user_enabled = false;

  if ((sqc_rpc_sched_qc_type_t) qc_type == sqc_rpc_sched_conf_get_qc_type()) {
    rpc_jwt_server_ctx_t *jwt_ctx = srvsession_get_jwt_ctx();
    if (jwt_ctx != NULL) {
      rc = rpc_jwt_server_validate_token(jwt_ctx, token, rpc_session_server_issue_connection_id(),
                                         &subject);
      if (rc == SQC_RESULT_OK) {
        rc = s_grpc_broker_get_user_status(subject, &is_user_enabled, NULL);
        if (rc == SQC_RESULT_OK) {
          if (is_user_enabled) {
            rc = dbmgr_ji_create_job(subject, (uint8_t) priority, qprogram, circuit_fmt,
                                     shots, qc_type, transpiler, remark, user_token,
                                     &ji_ptr);
            if (rc == SQC_RESULT_OK) {
              rc = dbmgr_ji_get_job_id(ji_ptr, job_id);
              if (rc == SQC_RESULT_OK) {
                sqc_msg_debug(5, "%s: Created the job: job_id=%s\n", log_prefix, *job_id);
                rc = req_sched_enqueue(ji_ptr);
                if (rc == SQC_RESULT_OK) {
                  sqc_msg_info("%s: Submitted: job_id=%s\n", log_prefix, *job_id);
                } else  {
                  s_grpc_create_message_text(reply_msg, "Failed to enqueue the job, %s: job_id=%s",
                                             sqc_error_get_string(rc), *job_id);
                  s_grpc_log_error_msg(log_prefix, *reply_msg);
                }
              } else {
                s_grpc_create_message_text(reply_msg, "Failed to get a created job ID, %s",
                                           sqc_error_get_string(rc));
                s_grpc_log_error_msg(log_prefix, *reply_msg);
              }
            } else {
              s_grpc_create_message_text(reply_msg, "Failed to create a job, %s",
                                         sqc_error_get_string(rc));
              s_grpc_log_error_msg(log_prefix, *reply_msg);
            }
          } else {
            rc = SQC_RESULT_DISABLED_USER;
            s_grpc_create_message_text(reply_msg, "Requested by the disabled user: user=%s",
                                       subject);
            s_grpc_log_error_msg(log_prefix, *reply_msg);
          }
        } else {
          s_grpc_create_message_text(reply_msg, "Failed to get status of the user, %s: user=%s",
                                     sqc_error_get_string(rc), subject);
          s_grpc_log_error_msg(log_prefix, *reply_msg);
        }
      } else {
        s_grpc_create_message_text(reply_msg, "Invalid JWT token, %s", sqc_error_get_string(rc));
        s_grpc_log_error_msg(log_prefix, *reply_msg);
      }
    } else {
      rc = SQC_RESULT_AUTHENTICATION_ERROR;
      s_grpc_create_message_text(reply_msg, "Failed to get JWT context");
      s_grpc_log_error_msg(log_prefix, *reply_msg);
    }
  } else {
    rc = SQC_RESULT_INVALID_ARGS;
    s_grpc_create_message_text(reply_msg, "Unexpected qc-type: qc_type=%d(%s)",
                               (int)qc_type, sqc_rpc_sched_qc_type_to_string(qc_type));
    s_grpc_log_error_msg(log_prefix, *reply_msg);
  }

  free(subject);
  return rc;
}


static int64_t
s_grpc_check_job_owner(const char *job_id, const dbmgr_job_info_t ji_ptr, const char *subject) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  char *user_id = NULL;

  if (job_id != NULL && ji_ptr != NULL && subject != NULL) {
    rc = dbmgr_ji_get_user_id(ji_ptr, &user_id);
    if (rc == SQC_RESULT_OK) {
      if (strcmp(subject, user_id) == 0) {
        rc = SQC_RESULT_OK;
        sqc_msg_debug(5, "Checked owner of the job: job_id=%s, job_owner=%s\n", job_id, user_id);
      } else {
        rc = SQC_RESULT_NOT_OWNER;
        sqc_msg_debug(5, "Denied to access the job: job_id=%s, job_owner=%s, requested_user=%s\n",
                      job_id, user_id, subject);
      }
    } else {
      sqc_msg_debug(5, "Failed to get user_id of the job, %s: job_id=%s\n",
                    sqc_error_get_string(rc), job_id);
    }
  }
  free(user_id);

  return rc;
}


static int64_t
s_grpc_broker_handler_job_status(const char *token, const char *job_id, int32_t *status,
                                 char** qc_job_id, char** result, char **reply_msg) {
  static const char* log_prefix = "gRPC-JOB_STATUS";
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  dbmgr_job_info_t ji_ptr = NULL;
  sqc_rpc_sched_job_status_t job_status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;
  char *subject = NULL;
  bool is_user_enabled = false;

  rpc_jwt_server_ctx_t *jwt_ctx = srvsession_get_jwt_ctx();
  if (jwt_ctx != NULL) {
    rc = rpc_jwt_server_validate_token(jwt_ctx, token, rpc_session_server_issue_connection_id(),
                                       &subject);
    if (rc == SQC_RESULT_OK) {
      rc = s_grpc_broker_get_user_status(subject, &is_user_enabled, NULL);
      if (rc == SQC_RESULT_OK) {
        if (is_user_enabled) {
          rc = dbmgr_ji_job_find(job_id, &ji_ptr);
          if (rc == SQC_RESULT_OK) {
            rc = s_grpc_check_job_owner(job_id, ji_ptr, subject);
            if (rc == SQC_RESULT_OK) {
              rc = dbmgr_ji_get_status(ji_ptr, &job_status);
              if (rc == SQC_RESULT_OK) {
                *status = job_status;
                sqc_msg_info("%s: Got job status: job_id=%s, status=%d(%s)\n",
                             log_prefix, job_id, job_status,
                             sqc_rpc_sched_job_status_to_string(job_status));

                // get QC Job ID
                if (job_status == SQC_RPC_SCHED_JOB_STATUS_RUNNING ||
                    job_status == SQC_RPC_SCHED_JOB_STATUS_DONE ||
                    job_status == SQC_RPC_SCHED_JOB_STATUS_CANCELLED ||
                    job_status == SQC_RPC_SCHED_JOB_STATUS_ERROR) {
                  rc = dbmgr_ji_get_qc_job_id(ji_ptr, qc_job_id);
                  if (rc != SQC_RESULT_OK) {
                    s_grpc_create_message_text(reply_msg, "Failed to get qc_job_id, %s: job_id=%s",
                                               sqc_error_get_string(rc), job_id);
                    s_grpc_log_error_msg(log_prefix, *reply_msg);
                  }
                }

                // get result
                if (job_status == SQC_RPC_SCHED_JOB_STATUS_DONE) {
                  rc = dbmgr_ji_get_result(ji_ptr, result);
                  if (rc != SQC_RESULT_OK) {
                    s_grpc_create_message_text(reply_msg,
                                               "Failed to get result of the job, %s: job_id=%s",
                                               sqc_error_get_string(rc), job_id);
                    s_grpc_log_error_msg(log_prefix, *reply_msg);
                  }
                }

              } else {
                s_grpc_create_message_text(reply_msg, "Failed to get job status, %s: job_id=%s",
                                           sqc_error_get_string(rc), job_id);
                s_grpc_log_error_msg(log_prefix, *reply_msg);
              }
            } else {
              s_grpc_create_message_text(reply_msg,
                                         "Denied to access the job: job_id=%s, requested_user=%s",
                                         job_id, subject);
              s_grpc_log_error_msg(log_prefix, *reply_msg);
            }
          } else {
            rc = SQC_RESULT_NOT_FOUND;
            s_grpc_create_message_text(reply_msg, "Job not found: job_id=%s", job_id);
            s_grpc_log_error_msg(log_prefix, *reply_msg);
          }
        } else {
          rc = SQC_RESULT_DISABLED_USER;
          s_grpc_create_message_text(reply_msg, "Requested by the disabled user: user=%s", subject);
          s_grpc_log_error_msg(log_prefix, *reply_msg);
        }
      } else {
        s_grpc_create_message_text(reply_msg, "Failed to get status of the user, %s: user=%s",
                                   sqc_error_get_string(rc), subject);
        s_grpc_log_error_msg(log_prefix, *reply_msg);
      }
    } else {
      rc = SQC_RESULT_AUTHENTICATION_ERROR;
      s_grpc_create_message_text(reply_msg, "Failed to verify the JWT token, %s",
                                 sqc_error_get_string(rc));
      s_grpc_log_error_msg(log_prefix, *reply_msg);
    }
  } else {
    rc = SQC_RESULT_AUTHENTICATION_ERROR;
    s_grpc_create_message_text(reply_msg, "Failed to get JWT context");
    s_grpc_log_error_msg(log_prefix, *reply_msg);
  }

  free(subject);
  return rc;
}


static int64_t
s_grpc_broker_handler_cancel_job(const char *token, const char *job_id, char **reply_msg) {
  static const char* log_prefix = "gRPC-CANCEL_JOB";
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  dbmgr_job_info_t ji_ptr = NULL;
  char *subject = NULL;
  bool is_user_enabled = false;
  rpc_jwt_server_ctx_t *jwt_ctx = srvsession_get_jwt_ctx();

  if (jwt_ctx != NULL) {
    rc = rpc_jwt_server_validate_token(jwt_ctx, token, rpc_session_server_issue_connection_id(),
                                       &subject);
    if (rc == SQC_RESULT_OK) {
      rc = s_grpc_broker_get_user_status(subject, &is_user_enabled, NULL);
      if (rc == SQC_RESULT_OK) {
        if (is_user_enabled) {
          rc = dbmgr_ji_job_find(job_id, &ji_ptr);
          if (rc == SQC_RESULT_OK) {
            rc = s_grpc_check_job_owner(job_id, ji_ptr, subject);
            if (rc == SQC_RESULT_OK) {
              rc = dbmgr_set_job_status_cancelled(ji_ptr);
              if (rc == SQC_RESULT_OK) {
                sqc_msg_info("%s: Cancelled: job_id=%s\n", log_prefix, job_id);
              } else {
                s_grpc_create_message_text(reply_msg, "Failed to cancel the job, %s: job_id=%s",
                                           sqc_error_get_string(rc), job_id);
                s_grpc_log_error_msg(log_prefix, *reply_msg);
              }
            } else {
              s_grpc_create_message_text(reply_msg, "Denied to access the job: job_id=%s, requested_user=%s",
                                         job_id, subject);
              s_grpc_log_error_msg(log_prefix, *reply_msg);
            }
          } else {
            rc = SQC_RESULT_NOT_FOUND;
            s_grpc_create_message_text(reply_msg, "Job not found: job_id=%s", job_id);
            s_grpc_log_error_msg(log_prefix, *reply_msg);
          }
        } else {
          rc = SQC_RESULT_DISABLED_USER;
          s_grpc_create_message_text(reply_msg, "Requested by the disabled user: user=%s", subject);
          s_grpc_log_error_msg(log_prefix, *reply_msg);
        }
      } else {
        s_grpc_create_message_text(reply_msg, "Failed to get status of the user, %s: %s",
                                   sqc_error_get_string(rc), subject);
        s_grpc_log_error_msg(log_prefix, *reply_msg);
      }
    } else {
      rc = SQC_RESULT_AUTHENTICATION_ERROR;
      s_grpc_create_message_text(reply_msg, "Failed to verify the JWT token, %s",
                                 sqc_error_get_string(rc));
      s_grpc_log_error_msg(log_prefix, *reply_msg);
    }
  } else {
    rc = SQC_RESULT_AUTHENTICATION_ERROR;
    s_grpc_create_message_text(reply_msg, "Failed to get JWT context");
    s_grpc_log_error_msg(log_prefix, *reply_msg);
  }

  free(subject);
  return rc;
}


static int64_t
s_grpc_broker_handler_delete_job(const char *token, const char *job_id, char **reply_msg) {
  static const char* log_prefix = "gRPC-DELETE_JOB";
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  dbmgr_job_info_t ji_ptr = NULL;
  char *subject = NULL;
  bool is_user_enabled = false;
  sqc_rpc_sched_job_status_t status;

  do {
    rpc_jwt_server_ctx_t *jwt_ctx = srvsession_get_jwt_ctx();
    if (jwt_ctx == NULL) {
      rc = SQC_RESULT_AUTHENTICATION_ERROR;
      s_grpc_create_message_text(reply_msg, "Failed to get JWT context");
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    }
    rc = rpc_jwt_server_validate_token(jwt_ctx, token, rpc_session_server_issue_connection_id(),
                                       &subject);
    if (rc != SQC_RESULT_OK) {
      rc = SQC_RESULT_AUTHENTICATION_ERROR;
      s_grpc_create_message_text(reply_msg, "Failed to verify the JWT token: %s",
                                 sqc_error_get_string(rc));
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    }
    rc = s_grpc_broker_get_user_status(subject, &is_user_enabled, NULL);
    if (rc != SQC_RESULT_OK) {
      s_grpc_create_message_text(reply_msg, "Failed to get status of the user, %s: user=%s",
                                 sqc_error_get_string(rc), subject);
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    }
    if (!is_user_enabled) {
      rc = SQC_RESULT_DISABLED_USER;
      s_grpc_create_message_text(reply_msg, "Requested by the disabled user: user=%s", subject);
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    }
    rc = dbmgr_ji_job_find(job_id, &ji_ptr);
    if (rc != SQC_RESULT_OK) {
      rc = SQC_RESULT_NOT_FOUND;
      s_grpc_create_message_text(reply_msg, "Job not found: job_id=%s", job_id);
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    }
    rc = s_grpc_check_job_owner(job_id, ji_ptr, subject);
    if (rc != SQC_RESULT_OK) {
      s_grpc_create_message_text(reply_msg, "Denied to access the job: job_id=%s, user=%s",
                                 job_id, subject);
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    }
    rc = dbmgr_ji_get_status(ji_ptr, &status);
    if (rc != SQC_RESULT_OK) {
      s_grpc_create_message_text(reply_msg, "Failed to get status of the job, %s: job_id=%s",
                                 sqc_error_get_string(rc), job_id);
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    }
    if (status == SQC_RPC_SCHED_JOB_STATUS_DELETED) {
      // Do nothing since the jobs has already been deleted.
      sqc_msg_debug(5, "%s: Already deleted: job_id=%s\n", log_prefix, job_id);
      rc = SQC_RESULT_OK;
      break;
    }
    if (!dbmgr_ji_is_deletable(ji_ptr)) {
      rc = SQC_RESULT_INVALID_STATE;
      s_grpc_create_message_text(reply_msg, "The job is not deletable: %s", job_id);
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    }
    rc = dbmgr_ji_clear_job_result(ji_ptr);
    if (rc != SQC_RESULT_OK) {
      s_grpc_create_message_text(reply_msg, "Failed to delete the job, %s: job_id=%s",
                                 sqc_error_get_string(rc), job_id);
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    }

    sqc_msg_info("%s: Deleted: job_id=%s\n", log_prefix, job_id);
    rc = SQC_RESULT_OK;
  } while (0);

  free(subject);
  return rc;
}


static int64_t
s_grpc_broker_handler_job_list(const char *token, grpc_job_info_t ***jobs, size_t *n_jobs, char **reply_msg) {
  static const char* log_prefix = "gRPC-JOB_LIST";
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  dbmgr_job_info_t *ji_ptr_arr = NULL;
  char *subject = NULL;
  bool is_user_enabled = false;
  sqc_rpc_sched_job_status_t status;

  rpc_jwt_server_ctx_t *jwt_ctx = srvsession_get_jwt_ctx();
  if (likely(jwt_ctx != NULL)) {
    rc = rpc_jwt_server_validate_token(jwt_ctx, token, rpc_session_server_issue_connection_id(),
                                       &subject);
    if (likely(rc == SQC_RESULT_OK)) {
      rc = s_grpc_broker_get_user_status(subject, &is_user_enabled, NULL);
      if (rc == SQC_RESULT_OK) {
        if (is_user_enabled) {
          rc = dbmgr_ji_job_find_by_user_id(subject, &ji_ptr_arr, n_jobs);
          if (likely(rc == SQC_RESULT_OK)) {
            rc = (sqc_result_t) grpc_job_info_create_array(jobs, *n_jobs);
            if (likely(rc == SQC_RESULT_OK)) {
              for (size_t i = 0u; i < *n_jobs; i++) {
                const dbmgr_job_info_t ji_ptr = ji_ptr_arr[i];
                grpc_job_info_t *job = (*jobs)[i];
                rc = dbmgr_ji_get_job_id(ji_ptr, &job->job_id);
                if (unlikely(rc != SQC_RESULT_OK)) {
                  goto error;
                }
                rc = dbmgr_ji_get_status(ji_ptr, &status);
                if (unlikely(rc != SQC_RESULT_OK)) {
                  goto error;
                }
                job->status = (int32_t) status;
                rc = dbmgr_ji_get_qc_job_id(ji_ptr, &job->qc_job_id);
                if (unlikely(rc != SQC_RESULT_OK)) {
                  goto error;
                }
#ifdef JOB_LIST_WITH_RESULT
                rc = dbmgr_ji_get_result(ji_ptr, &job->result);
                if (unlikely(rc != SQC_RESULT_OK)) {
                  goto error;
                }
#endif
              }

              sqc_msg_info("%s: Got status of submitted %zu job(s)\n", log_prefix, *n_jobs);
              rc = SQC_RESULT_OK;
            }
          error:
            if (unlikely(rc != SQC_RESULT_OK)) {
              grpc_job_info_destroy_array(*jobs, *n_jobs);
            }
          } else {
            s_grpc_create_message_text(reply_msg, "Failed to get a list of user jobs, %s: user=%s",
                                       sqc_error_get_string(rc), subject);
            s_grpc_log_error_msg(log_prefix, *reply_msg);
            rc = SQC_RESULT_ANY_RUNTIME_ERROR;
          }
        } else {
          rc = SQC_RESULT_DISABLED_USER;
          s_grpc_create_message_text(reply_msg, "Requested by the disabled user: user=%s", subject);
          s_grpc_log_error_msg(log_prefix, *reply_msg);
        }
      } else {
        s_grpc_create_message_text(reply_msg, "Failed to get status of the user, %s: user=%s",
                                   sqc_error_get_string(rc), subject);
        s_grpc_log_error_msg(log_prefix, *reply_msg);
        rc = SQC_RESULT_ANY_RUNTIME_ERROR;
      }
    } else {
      rc = SQC_RESULT_AUTHENTICATION_ERROR;
      s_grpc_create_message_text(reply_msg, "Failed to verify the JWT token, %s",
                                 sqc_error_get_string(rc));
      s_grpc_log_error_msg(log_prefix, *reply_msg);
    }
  } else {
    rc = SQC_RESULT_AUTHENTICATION_ERROR;
    s_grpc_create_message_text(reply_msg, "Failed to get JWT context");
    s_grpc_log_error_msg(log_prefix, *reply_msg);
  }

  free(ji_ptr_arr);
  free(subject);
  return rc;
}


static int64_t
s_grpc_broker_handler_adm_del_jobs(const char *token, const char* user_id, int64_t from_time,
                                   int64_t to_time, char **reply_msg) {
  static const char* log_prefix = "gRPC-ADM_DEL_JOBS";
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  char *subject = NULL;
  bool is_user_enabled = false;
  bool is_user_admin = false;
  dbmgr_user_info_t user_info = NULL;
  dbmgr_job_info_t *ji_ptr_arr = NULL;
  size_t arr_len = 0u;
  const char *target_user_id = NULL;

  do {
    rpc_jwt_server_ctx_t *jwt_ctx = srvsession_get_jwt_ctx();
    if (jwt_ctx == NULL) {
      rc = SQC_RESULT_AUTHENTICATION_ERROR;
      s_grpc_create_message_text(reply_msg, "Failed to get JWT context");
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    }
    rc = rpc_jwt_server_validate_token(jwt_ctx, token, rpc_session_server_issue_connection_id(), &subject);
    if (rc != SQC_RESULT_OK) {
      rc = SQC_RESULT_AUTHENTICATION_ERROR;
      s_grpc_create_message_text(reply_msg, "Failed to verify the JWT token, %s",
                                 sqc_error_get_string(rc));
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    }
    rc = s_grpc_broker_get_user_status(subject, &is_user_enabled, &is_user_admin);
    if (rc != SQC_RESULT_OK) {
      s_grpc_create_message_text(reply_msg, "Failed to get status of the user, %s: user=%s",
                                 sqc_error_get_string(rc), subject);
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
      break;
    }
    if (!is_user_admin) {
      rc = SQC_RESULT_NOT_ADMIN_USER;
      s_grpc_create_message_text(reply_msg, "Requested by the non-administrator user: user=%s", subject);
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    } else if (!is_user_enabled) {
      rc = SQC_RESULT_DISABLED_USER;
      s_grpc_create_message_text(reply_msg, "Requested by the disabled user: user=%s", subject);
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    }
    if (strcmp(user_id, all_users) != 0) {
      rc = dbmgr_ui_user_find(user_id, &user_info);
      if (rc != SQC_RESULT_OK) {
        s_grpc_create_message_text(reply_msg, "Failed to get information of the user, %s: user=%s",
                                   sqc_error_get_string(rc), user_id);
        s_grpc_log_error_msg(log_prefix, *reply_msg);
        rc = SQC_RESULT_ANY_RUNTIME_ERROR;
        break;
      }
      target_user_id = user_id;
    }
    rc = dbmgr_ji_job_find_by_delete_target(target_user_id, (sqc_chrono_t) from_time,
                                            (sqc_chrono_t) to_time, &ji_ptr_arr, &arr_len);
    if (rc != SQC_RESULT_OK) {
      s_grpc_create_message_text(reply_msg, "Failed to get a list of user jobs, %s",
                                 sqc_error_get_string(rc));
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
      break;
    }
    rc = dbmgr_ji_clear_job_arr_result(ji_ptr_arr, arr_len);
    if (rc != SQC_RESULT_OK) {
      s_grpc_create_message_text(reply_msg, "Failed to delete user jobs, %s", sqc_error_get_string(rc));
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
      break;
    }
    s_grpc_create_message_text(reply_msg, "Deleted %zu job(s)", arr_len);
    s_grpc_log_info_msg(log_prefix, *reply_msg);
    rc = SQC_RESULT_OK;
  } while (0);

  free(ji_ptr_arr);
  free(subject);
  return rc;
}


static int64_t
s_grpc_broker_handler_adm_add_user(const char *token, const char* user_id, char **reply_msg) {
  static const char* log_prefix = "gRPC-ADM_ADD_USER";
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  char *subject = NULL;
  bool is_user_enabled = false;
  bool is_user_admin = false;
  dbmgr_user_info_t user_info = NULL;

  do {
    rpc_jwt_server_ctx_t *jwt_ctx = srvsession_get_jwt_ctx();
    if (jwt_ctx == NULL) {
      rc = SQC_RESULT_AUTHENTICATION_ERROR;
      s_grpc_create_message_text(reply_msg, "Failed to get JWT context");
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    }
    rc = rpc_jwt_server_validate_token(jwt_ctx, token, rpc_session_server_issue_connection_id(), &subject);
    if (rc != SQC_RESULT_OK) {
      rc = SQC_RESULT_AUTHENTICATION_ERROR;
      s_grpc_create_message_text(reply_msg, "Failed to verify the JWT token, %s", sqc_error_get_string(rc));
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    }
    rc = s_grpc_broker_get_user_status(subject, &is_user_enabled, &is_user_admin);
    if (rc != SQC_RESULT_OK) {
      s_grpc_create_message_text(reply_msg, "Failed to get status of the user, %s: user=%s",
                                 sqc_error_get_string(rc), subject);
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
      break;
    }
    if (!is_user_admin) {
      rc = SQC_RESULT_NOT_ADMIN_USER;
      s_grpc_create_message_text(reply_msg, "Requested by the non-administrator user: user=%s", subject);
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    } else if (!is_user_enabled) {
      rc = SQC_RESULT_DISABLED_USER;
      s_grpc_create_message_text(reply_msg, "Requested by the disabled user: user=%s", subject);
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    }
    if (*user_id == '\0' || strcmp(user_id, all_users) == 0) {
      rc = SQC_RESULT_INVALID_ARGS;
      s_grpc_create_message_text(reply_msg, "Invalid user name: user=%s", user_id);
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    }
    rc = dbmgr_ui_user_find(user_id, &user_info);
    if (rc == SQC_RESULT_OK) {
      rc = SQC_RESULT_ALREADY_EXISTS;
      s_grpc_create_message_text(reply_msg, "Failed to add the user, %s: user=%s",
                                 sqc_error_get_string(rc), user_id);
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    } else if (rc != SQC_RESULT_NOT_FOUND) {
      s_grpc_create_message_text(reply_msg, "Failed to get user information, %s: user=%s",
                                 sqc_error_get_string(rc), user_id);
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    }
    rc = dbmgr_ui_create_user(user_id, &user_info);
    if (rc != SQC_RESULT_OK) {
      s_grpc_create_message_text(reply_msg, "Failed to add the user, %s: user=%s",
                                 sqc_error_get_string(rc), user_id);
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    }
    rc = SQC_RESULT_OK;
    s_grpc_create_message_text(reply_msg, "Added: user=%s", user_id);
    s_grpc_log_info_msg(log_prefix, *reply_msg);
  } while (0);

  free(subject);
  return rc;
}


static int64_t
s_grpc_broker_handler_adm_set_user_status(const char *token, const char* user_id, bool enabled,
                                          char **reply_msg) {
  static const char* log_prefix = "gRPC-ADM_SET_USER_STATUS";
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  char *subject = NULL;
  bool is_user_enabled = false;
  bool is_user_admin = false;
  dbmgr_user_info_t user_info = NULL;

  do {
    rpc_jwt_server_ctx_t *jwt_ctx = srvsession_get_jwt_ctx();
    if (jwt_ctx == NULL) {
      rc = SQC_RESULT_AUTHENTICATION_ERROR;
      s_grpc_create_message_text(reply_msg, "Failed to get JWT context");
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    }
    rc = rpc_jwt_server_validate_token(jwt_ctx, token, rpc_session_server_issue_connection_id(),
                                       &subject);
    if (rc != SQC_RESULT_OK) {
      rc = SQC_RESULT_AUTHENTICATION_ERROR;
      s_grpc_create_message_text(reply_msg, "Failed to verify the JWT token, %s",
                                 sqc_error_get_string(rc));
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    }
    rc = s_grpc_broker_get_user_status(subject, &is_user_enabled, &is_user_admin);
    if (rc != SQC_RESULT_OK) {
      s_grpc_create_message_text(reply_msg, "Failed to get user status, %s: user=%s",
                                 sqc_error_get_string(rc), subject);
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
      break;
    }
    if (!is_user_admin) {
      rc = SQC_RESULT_NOT_ADMIN_USER;
      s_grpc_create_message_text(reply_msg, "Requested by the non-administrator user: user=%s", subject);
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    } else if (!is_user_enabled) {
      rc = SQC_RESULT_DISABLED_USER;
      s_grpc_create_message_text(reply_msg, "Requested by the disabled user: user=%s", subject);
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      break;
    }
    rc = dbmgr_ui_user_find(user_id, &user_info);
    if (rc != SQC_RESULT_OK) {
      s_grpc_create_message_text(reply_msg, "Failed to get information of the user, %s: user=%s",
                                 sqc_error_get_string(rc), user_id);
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
      break;
    }
    if (enabled) {
      rc = dbmgr_ui_set_user_enabled(user_info);
    } else {
      rc = dbmgr_ui_set_user_disabled(user_info);
    }
    if (rc != SQC_RESULT_OK) {
      s_grpc_create_message_text(reply_msg, "Failed to set user status, %s: user=%s",
                                 sqc_error_get_string(rc), user_id);
      s_grpc_log_error_msg(log_prefix, *reply_msg);
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
      break;
    }
    s_grpc_create_message_text(reply_msg, "Set status: user=%s, status=%s", user_id,
                               enabled ? "enable" : "disable");
    s_grpc_log_info_msg(log_prefix, *reply_msg);
    rc = SQC_RESULT_OK;
  } while (0);

  free(subject);
  return rc;
}


static inline sqc_result_t
s_grpc_broker_handler_initialize(void) {
  int64_t ret;

  job_broker_handlers_t handlers;
  initialize_job_broker_handlers(&handlers);
  handlers.submit_job = s_grpc_broker_handler_submit_job;
  handlers.job_status = s_grpc_broker_handler_job_status;
  handlers.cancel_job = s_grpc_broker_handler_cancel_job;
  handlers.delete_job = s_grpc_broker_handler_delete_job;
  handlers.job_list = s_grpc_broker_handler_job_list;
  handlers.adm_del_jobs = s_grpc_broker_handler_adm_del_jobs;
  handlers.adm_add_user = s_grpc_broker_handler_adm_add_user;
  handlers.adm_set_user_status = s_grpc_broker_handler_adm_set_user_status;

  ret = sqc_job_broker_initialize(sqc_rpc_sched_conf_get_grpc_server_address(),
                                  sqc_rpc_sched_conf_get_conf_dir(),
                                  &handlers,
                                  num_cqs,
                                  min_pollers,
                                  max_pollers,
                                  cq_timeout_msec);

  if (ret != 0) {
    sqc_msg_error("Failed to initialize: error_code=%ld(%s)\n", ret, sqc_error_get_string(ret));
    return SQC_RESULT_ANY_RUNTIME_ERROR;
  }

  return SQC_RESULT_OK;
}


static inline sqc_result_t
s_grpc_broker_handler_start(void) {
  int64_t ret;

  ret = sqc_job_broker_start();
  if (ret != 0) {
    sqc_msg_error("Failed to start: error_code=%ld(%s)\n", ret, sqc_error_get_string(ret));
    return SQC_RESULT_ANY_RUNTIME_ERROR;
  }

  return SQC_RESULT_OK;
}


static inline sqc_result_t
s_grpc_broker_handler_stop(void) {
  int64_t ret;

  ret = sqc_job_broker_stop();
  if (ret != 0) {
    sqc_msg_error("Failed to stop: error_code=%ld(%s)\n", ret, sqc_error_get_string(ret));
    return SQC_RESULT_ANY_RUNTIME_ERROR;
  }

  return SQC_RESULT_OK;
}


static inline void
s_grpc_broker_handler_finalize(void) {
}


/*
 * export
 */
