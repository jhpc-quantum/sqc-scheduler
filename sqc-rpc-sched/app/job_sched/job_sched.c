#include "sqc_apis.h"
#include "sqc_thread_internal.h"
#include "sqc_rpc_sched_conf.h"
#include "sqc_rpc_sched_conv_enums.h"
#include "sqc_rpc_sched_util.h"
#include "dbmgr.h"
#include "req_invoker.h"

#include "job_sched.h"
#include "job_sched_scheduling_algo.h"

#define DEFAULT_EXEX_TIME_ESTIMATE_MSEC 1000

typedef struct job_sched_thread_record {
  struct sqc_thread_record thd_;

  sqc_chrono_t interval_;

  sqc_rwlock_t rwlck_;

  volatile bool do_loop_;
  volatile bool is_started_;
  volatile shutdown_grace_level_t shutdown_level_;
} job_sched_thread_record;
typedef job_sched_thread_record *job_sched_thread_t;

static job_sched_thread_t s_mthd = NULL;
static bool s_mod_inited = false;

/*
 * module thread methods
 */


static inline void
s_job_sched_rlock(job_sched_thread_t mt) {
  if (likely(mt != NULL)) {
    (void)sqc_rwlock_reader_lock(&(mt->rwlck_));
  }
}


static inline void
s_job_sched_wlock(job_sched_thread_t mt) {
  if (likely(mt != NULL)) {
    (void)sqc_rwlock_writer_lock(&(mt->rwlck_));
  }
}


static inline void
s_job_sched_unlock(job_sched_thread_t mt) {
  if (likely(mt != NULL)) {
    (void)sqc_rwlock_unlock(&(mt->rwlck_));
  }
}


static void
s_job_sched_thd_finalize(const sqc_thread_t *tptr, bool is_canceled,
                         void *arg) {
  job_sched_thread_t mt = (job_sched_thread_t)*tptr;
  (void)arg;

  sqc_msg_debug(5, "called.\n");

  if (likely(mt != NULL)) {

    if (is_canceled == true) {
      if (mt->is_started_ == false) {
        /*
         * Means this thread is canceled while waiting for the global
         * state change.
         */
        global_state_cancel_janitor();
        s_job_sched_unlock(mt);
      }
    }
  }

  sqc_msg_debug(5, "called with %s self and the thread is %s.\n",
                ((mt != NULL) ? "valid" : "invalid (NULL)"),
                ((is_canceled == true) ? "canceled" : "exited"));
}


static void
s_job_sched_thd_freeup(const sqc_thread_t *tptr, void *arg) {
  job_sched_thread_t mt = (job_sched_thread_t)*tptr;

  (void)arg;

  sqc_msg_debug(5, "called with %s self.\n",
                ((mt != NULL) ? "valid" : "invalid (NULL)"));
  if (mt != NULL) {
    /*
     * Here we can free up all the resource related to this thread.
     */
    if (mt->rwlck_ != NULL) {
      (void)sqc_rwlock_destroy(&(mt->rwlck_));
    }
  }
}


static sqc_result_t
s_job_sched_thd_main(const sqc_thread_t *tptr, void *arg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  global_state_t s;
  shutdown_grace_level_t l;
  job_sched_thread_t mt = (job_sched_thread_t)*tptr;

  (void)arg;

  sqc_msg_debug(5, "waiting for the gala opening...\n");

  ret = global_state_wait_for(GLOBAL_STATE_STARTED, &s, &l, -1LL);
  if (ret == SQC_RESULT_OK && s == GLOBAL_STATE_STARTED) {
    s_job_sched_wlock(mt);
    {
      mt->is_started_ = true;
    }
    s_job_sched_unlock(mt);

    sqc_msg_debug(5, "gala opening.\n");

    /*
     * The main loop.
     */
    do {
      sqc_msg_debug(100, "looping...\n");

      // Error handling is performed by the caller, so no action is taken here.
      (void)job_sched_scheduling_algo_dispatch();

      /*
       * Create an explicit cancalation point since this loop has
       * none of it.
       */
      pthread_testcancel();

    } while (mt->do_loop_ == true);

    /*
     * Reaching here means someone called a shutdown request.
     */
    if (mt->shutdown_level_ == SHUTDOWN_GRACEFULLY) {
      /*
       * This is just emulating/mimicking a graceful shutdown by
       * sleep().  Don't do this on actual modules.
       */
      sqc_msg_debug(5, "mimicking gracefull shutdown...\n");
      sleep(5);
      sqc_msg_debug(5, "mimicking gracefull shutdown done.\n");
      ret = SQC_RESULT_OK;
    } else {
      ret = 1LL;
    }
  }

  return ret;
}


static inline sqc_result_t
s_job_sched_thd_create(job_sched_thread_t *tptr, sqc_chrono_t interval) {
  sqc_result_t ret = SQC_RESULT_INVALID_ARGS;

  sqc_msg_debug(5, "called.\n");

  if (likely(interval >= 1000 * 1000)) {
    if ((ret = sqc_thread_create_with_size((sqc_thread_t *)tptr,
                                           sizeof(job_sched_thread_record),
                                           s_job_sched_thd_main,
                                           s_job_sched_thd_finalize,
                                           s_job_sched_thd_freeup,
                                           "job_sched_thd",
                                           NULL)) == SQC_RESULT_OK) {
      (*tptr)->rwlck_ = NULL;
      if ((ret = sqc_rwlock_create(&((*tptr)->rwlck_))) == SQC_RESULT_OK) {
        (*tptr)->interval_ = interval;
        (*tptr)->do_loop_ = false;
        (*tptr)->is_started_ = false;
        (*tptr)->shutdown_level_ = SHUTDOWN_UNKNOWN;
        goto done;
      }

      sqc_thread_destroy((sqc_thread_t *)tptr);
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}


static inline void
s_job_sched_thd_destroy(job_sched_thread_t tptr) {
  sqc_msg_debug(5, "called.\n");

  sqc_thread_destroy((sqc_thread_t *)tptr);
}


/*
 * module methods
 */


static sqc_result_t
s_job_sched_initialize(int argc, const char *const argv[], void *extarg,
                       sqc_thread_t **thdptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  (void)argc;
  (void)argv;
  (void)extarg;

  sqc_msg_debug(5, "called.\n");

  if (thdptr != NULL) {
    *thdptr = NULL;
  }

  if (likely((ret = s_job_sched_thd_create(&s_mthd,
                                           1000LL * 1000LL * 1000LL * 5LL)) ==
             SQC_RESULT_OK)) {
    *thdptr = (sqc_thread_t *)&s_mthd;
    s_mod_inited = true;
  }

  if ((ret = job_sched_scheduling_algo_initialize()) != SQC_RESULT_OK) {
    sqc_perror(ret);
  }

  return ret;
}


static sqc_result_t
s_job_sched_start(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  sqc_msg_debug(5, "called.\n");

  if (likely(s_mod_inited == true)) {
    if (likely(s_mthd != NULL)) {
      if (likely((ret = sqc_thread_start((sqc_thread_t *)&s_mthd, false)) ==
                 SQC_RESULT_OK)) {

        s_job_sched_wlock(s_mthd);
        {
          s_mthd->do_loop_ = true;
        }
        s_job_sched_unlock(s_mthd);

      }
    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_STATE_TRANSITION;
  }

  return ret;
}


static sqc_result_t
s_job_sched_shutdown(shutdown_grace_level_t l) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  sqc_msg_debug(5, "called.\n");

  if (likely(s_mod_inited == true)) {
    if (likely(s_mthd != NULL)) {

      s_job_sched_wlock(s_mthd);
      {
        if (s_mthd->is_started_ == true) {
          s_mthd->shutdown_level_ = l;
          s_mthd->do_loop_ = false;
          ret = SQC_RESULT_OK;
        } else {
          ret = SQC_RESULT_INVALID_STATE_TRANSITION;
        }
      }
      s_job_sched_unlock(s_mthd);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_STATE_TRANSITION;
  }

  return ret;
}


static sqc_result_t
s_job_sched_stop(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  sqc_msg_debug(5, "called.\n");

  if (likely(s_mod_inited == true)) {
    if (likely(s_mthd != NULL)) {

      s_job_sched_wlock(s_mthd);
      {
        if (s_mthd->is_started_ == true) {
          ret = sqc_thread_cancel((sqc_thread_t *)&s_mthd);
        } else {
          ret = SQC_RESULT_INVALID_STATE_TRANSITION;
        }
      }
      s_job_sched_unlock(s_mthd);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_STATE_TRANSITION;
  }

  return ret;
}


static void
s_job_sched_finalize(void) {
  sqc_msg_debug(5, "called.\n");

  job_sched_scheduling_algo_finalize();

  if (likely(s_mthd != NULL)) {
    s_job_sched_thd_destroy(s_mthd);
  }
}


static inline bool
s_job_sched_is_quota_exceeded(uint64_t exec_time_limit_msec,
                              uint64_t exec_time_total_msec,
                              uint64_t exec_time_estimate_msec) {
  if ((exec_time_total_msec + exec_time_estimate_msec) > exec_time_limit_msec) {
    return true;
  }

  return false;
}


/*
 * export
 */


sqc_result_t
job_sched_register(void) {
  return sqc_module_register("job sched",
                             s_job_sched_initialize,
                             NULL,
                             s_job_sched_start,
                             s_job_sched_shutdown,
                             s_job_sched_stop,
                             s_job_sched_finalize,
                             NULL);
}


sqc_result_t
job_sched_submit_job(const char *user_id, const char *group_id, uint8_t priority,
                     const char *qprogram, sqc_rpc_sched_circuit_fmt_t circuit_fmt, size_t shots,
                     sqc_rpc_sched_qc_type_t qc_type, sqc_rpc_sched_transpiler_t transpiler,
                     const char *remark, const char *user_token, char **job_id, char **reply_msg) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  uint64_t weight;
  uint64_t exec_time_limit_msec;
  uint64_t exec_time_total_msec;
  uint64_t exec_time_estimate_msec;
  dbmgr_group_info_t gi_ptr = NULL;
  dbmgr_user_group_info_t ugi_ptr = NULL;
  dbmgr_job_info_t ji_ptr = NULL;
  bool skip_job_deletion = false;

  if (IS_VALID_STRING(user_id) == false) {
    rc = SQC_RESULT_INVALID_ARGS;
    sqc_rpc_sched_util_create_message_text(reply_msg, "Invalid parameter(user_id)");
    goto error;
  }

  if (IS_VALID_STRING(group_id) == false) {
    rc = SQC_RESULT_INVALID_ARGS;
    sqc_rpc_sched_util_create_message_text(reply_msg, "Invalid parameter(group_id)");
    goto error;
  }

  if (IS_VALID_STRING(qprogram) == false) {
    rc = SQC_RESULT_INVALID_ARGS;
    sqc_rpc_sched_util_create_message_text(reply_msg, "Invalid parameter(qprogram)");
    goto error;
  }

  if (priority > SQC_RPC_SCHED_MAX_PRIORITY) {
    rc = SQC_RESULT_TOO_LARGE;
    sqc_rpc_sched_util_create_message_text(reply_msg,
                                           "Invalid parameter(priority): %s",
                                           sqc_error_get_string(rc));
    goto error;
  }

  if (qc_type != sqc_rpc_sched_conf_get_qc_type()) {
    rc = SQC_RESULT_INVALID_ARGS;
    sqc_rpc_sched_util_create_message_text(reply_msg,
                                           "Unexpected qc-type: user_id=%s, group_id=%s, qc-type=%d(%s)",
                                           user_id, group_id,
                                           (int)qc_type, sqc_rpc_sched_qc_type_to_string(qc_type));
    goto error;
  }

  rc = dbmgr_wi_get_weight(priority, &weight);
  if (rc != SQC_RESULT_OK) {
    sqc_rpc_sched_util_create_message_text(reply_msg,
                                           "Failed to get weight: user_id=%s, group_id=%s, priority=%d",
                                           user_id, group_id, priority);
    goto error;
  }

  rc = dbmgr_gi_group_find(group_id, &gi_ptr);
  if (rc != SQC_RESULT_OK) {
    sqc_rpc_sched_util_create_message_text(reply_msg,
                                           "Specified group does not exist: user_id=%s, group_id=%s",
                                           user_id, group_id);
    goto error;
  }

  rc = dbmgr_ugi_user_find(user_id, group_id, &ugi_ptr);
  if (rc != SQC_RESULT_OK) {
    sqc_rpc_sched_util_create_message_text(reply_msg,
                                           "User is not in the specified group: user_id=%s, group_id=%s",
                                           user_id, group_id);
    goto error;
  }

  if (dbmgr_ugi_is_user_group_enabled(ugi_ptr) == false) {
    rc = SQC_RESULT_DISABLED_USER;
    sqc_rpc_sched_util_create_message_text(reply_msg,
                                           "User is disabled in the specified group: user_id=%s, group_id=%s",
                                           user_id, group_id);
    goto error;
  }

  rc = dbmgr_gi_get_exec_time_limit_msec(gi_ptr, &exec_time_limit_msec);
  if (rc != SQC_RESULT_OK) {
    sqc_rpc_sched_util_create_message_text(reply_msg,
                                           "Failed to get exec_time_limit_msec: user_id=%s, group_id=%s",
                                           user_id, group_id);
    goto error;
  }

  rc = dbmgr_gi_get_exec_time_total_msec(gi_ptr, &exec_time_total_msec);
  if (rc != SQC_RESULT_OK) {
    sqc_rpc_sched_util_create_message_text(reply_msg,
                                           "Failed to get exec_time_total_msec: user_id=%s, group_id=%s",
                                           user_id, group_id);
    goto error;
  }

  // Calculation of the estimated execution time is approximate
  exec_time_estimate_msec = sqc_rpc_sched_util_calc_weighted_execution_time(DEFAULT_EXEX_TIME_ESTIMATE_MSEC,
                                                                            weight);
  sqc_msg_debug(5, "Calculated exec_time_estimate: %ld\n", exec_time_estimate_msec);

  // Check quota
  if (s_job_sched_is_quota_exceeded(exec_time_limit_msec,
                                    exec_time_total_msec,
                                    exec_time_estimate_msec) == true) {
    rc = SQC_RESULT_QUOTA_EXCEEDED;
    sqc_rpc_sched_util_create_message_text(reply_msg,
                                           "Failed to submit the job(quota exceeded): user_id=%s, group_id=%s",
                                           user_id, group_id);
    goto error;
  }

  // Create job_info
  rc = dbmgr_ji_create_job(user_id, group_id, priority, qprogram, circuit_fmt, shots,
                           qc_type, transpiler, remark, user_token, &ji_ptr);
  if (rc != SQC_RESULT_OK) {
    sqc_rpc_sched_util_create_message_text(reply_msg,
                                           "Failed to submit the job(job creation failed(cause: %s)): user_id=%s, group_id=%s",
                                           sqc_error_get_string(rc), user_id, group_id);
    goto error;
  }

  rc = dbmgr_ji_get_job_id(ji_ptr, job_id);
  if (rc != SQC_RESULT_OK) {
    sqc_rpc_sched_util_create_message_text(reply_msg,
                                           "Failed to submit the job(failed to get a job ID): user_id=%s, group_id=%s",
                                           user_id, group_id);

    // job_id is NULL, so the job cannot be deleted; mark it as error instead.
    if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
      sqc_msg_error("Failed to transition status to error: user_id=%s, group_id=%s\n",
                    user_id, group_id);
    } else {
      skip_job_deletion = true;
    }

    goto error;
  }

  sqc_msg_info("Created the job: user_id=%s, group_id=%s, job_id=%s\n", user_id, group_id, *job_id);

  rc = job_sched_scheduling_algo_submit(ji_ptr);
  if (rc == SQC_RESULT_OK) {
    sqc_msg_info("Enqueued the job: job_id=%s\n", *job_id);

    // Run the job even if the estimated time cannot be set
    rc = dbmgr_ji_set_exec_time_estimate_msec(ji_ptr, exec_time_estimate_msec);
    if (rc != SQC_RESULT_OK) {
      sqc_msg_warning("Failed to update job_info exec_time_estimate_msec: "
                      "user_id=%s, group_id=%s, job_id=%s, msg=%s\n",
                      user_id, group_id, *job_id, sqc_error_get_string(rc));
    }

    rc = dbmgr_gi_add_exec_time_total_msec(gi_ptr, exec_time_estimate_msec);
    if (rc != SQC_RESULT_OK) {
      sqc_msg_warning("Failed to update group_info exec_time_total_msec: "
                      "user_id=%s, group_id=%s, job_id=%s, msg=%s\n",
                      user_id, group_id, *job_id, sqc_error_get_string(rc));
    }

    return SQC_RESULT_OK;
  } else {
    sqc_rpc_sched_util_create_message_text(reply_msg,
                                           "Failed to enqueue the job to scheduler: job_id=%s, msg=%s",
                                           *job_id, sqc_error_get_string(rc));

    if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
      sqc_msg_error("Failed to transition status to error: job_id=%s\n", *job_id);
    } else {
      // Keep the job record so callers can still observe the ERROR status.
      skip_job_deletion = true;
    }

    goto error;
  }

error:
  if (ji_ptr != NULL && skip_job_deletion == false) {
    if (*job_id != NULL &&
        (rc = dbmgr_ji_delete_job(*job_id)) != SQC_RESULT_OK) {
      sqc_msg_warning("Failed to delete the job_info for the job that failed to submit: "
                      "user_id=%s, group_id=%s, msg=%s\n",
                      user_id, group_id, sqc_error_get_string(rc));
    }
  }

  if (*reply_msg != NULL) {
    sqc_msg_error("%s\n", *reply_msg);
  } else {
    sqc_msg_error("(failed to create a log message)\n");
  }

  return rc;
}

