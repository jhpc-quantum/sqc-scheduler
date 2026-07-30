#include "sqc_apis.h"
#include "sqc_thread_internal.h"
#include "sqc_rpc_sched_conf.h"
#include "sqc_rpc_sched_conv_enums.h"

#include "srv_session.h"
#include "srv_session_accept.c"

typedef struct srvsession_thread_record {
  struct sqc_thread_record thd_;

  sqc_chrono_t interval_;

  sqc_rwlock_t rwlck_;

  volatile bool do_loop_;
  volatile bool is_started_;
  volatile shutdown_grace_level_t shutdown_level_;
} srvsession_thread_record;
typedef srvsession_thread_record *srvsession_thread_t;

static srvsession_thread_t s_mthd = NULL;
static bool s_mod_inited = false;

/*
 * Configuration about user authentication.
 */
#include "protobuf-c/protobuf-c.h"

static sqc_tls_conf_t s_tls_conf = NULL;
static rpc_jwt_server_ctx_t s_jwt_ctx = NULL;


/*
 * module thread methods
 */


static inline void
s_srvsession_rlock(srvsession_thread_t mt) {
  if (likely(mt != NULL)) {
    (void)sqc_rwlock_reader_lock(&(mt->rwlck_));
  }
}


static inline void
s_srvsession_wlock(srvsession_thread_t mt) {
  if (likely(mt != NULL)) {
    (void)sqc_rwlock_writer_lock(&(mt->rwlck_));
  }
}


static inline void
s_srvsession_unlock(srvsession_thread_t mt) {
  if (likely(mt != NULL)) {
    (void)sqc_rwlock_unlock(&(mt->rwlck_));
  }
}


static void
s_srvsession_thd_finalize(const sqc_thread_t *tptr, bool is_canceled,
                          void *arg) {
  srvsession_thread_t mt = (srvsession_thread_t)*tptr;
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
        s_srvsession_unlock(mt);
      }
    }
  }

  sqc_msg_debug(5, "called with %s self and the thread is %s.",
                ((mt != NULL) ? "valid" : "invalid (NULL)"),
                ((is_canceled == true) ? "canceled" : "exited"));
}


static void
s_srvsession_thd_freeup(const sqc_thread_t *tptr, void *arg) {
  srvsession_thread_t mt = (srvsession_thread_t)*tptr;

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
s_srvsession_thd_main(const sqc_thread_t *tptr, void *arg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  global_state_t s;
  shutdown_grace_level_t l;
  srvsession_thread_t mt = (srvsession_thread_t)*tptr;

  (void)arg;

  sqc_msg_debug(5, "waiting for the gala opening...\n");

  ret = global_state_wait_for(GLOBAL_STATE_STARTED, &s, &l, -1LL);
  if (ret == SQC_RESULT_OK && s == GLOBAL_STATE_STARTED) {
    s_srvsession_wlock(mt);
    {
      mt->is_started_ = true;
    }
    s_srvsession_unlock(mt);

    sqc_msg_debug(5, "gala opening.\n");

    /*
     * The main loop.
     */
    do {
      sqc_msg_debug(6, "looping...\n");

      (void)s_srvsession_accept();

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
s_srvsession_thd_create(srvsession_thread_t *tptr, sqc_chrono_t interval) {
  sqc_result_t ret = SQC_RESULT_INVALID_ARGS;

  sqc_msg_debug(5, "called.\n");

  if (likely(interval >= 1000 * 1000)) {
    if ((ret = sqc_thread_create_with_size((sqc_thread_t *)tptr,
                                           sizeof(srvsession_thread_record),
                                           s_srvsession_thd_main,
                                           s_srvsession_thd_finalize,
                                           s_srvsession_thd_freeup,
                                           "srvsession_thread",
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
s_srvsession_thd_destroy(srvsession_thread_t tptr) {
  sqc_msg_debug(5, "called.\n");

  sqc_thread_destroy((sqc_thread_t *)tptr);
}


/*
 * module methods
 */


static sqc_result_t
s_srvsession_initialize(int argc, const char *const argv[], void *extarg,
                        sqc_thread_t **thdptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  (void)argc;
  (void)argv;
  (void)extarg;

  sqc_msg_debug(5, "called.\n");

  (void)sqc_tls_conf_create_from_conf_dir(&s_tls_conf, sqc_rpc_sched_conf_get_conf_dir(),
                                          TLS_ROLE_SERVER);
  sqc_tls_conf_dump(s_tls_conf);
  (void)rpc_jwt_server_create_ctx_from_conf_dir(&s_jwt_ctx, sqc_rpc_sched_conf_get_conf_dir());
  rpc_jwt_server_dump_ctx(&s_jwt_ctx);

  if (thdptr != NULL) {
    *thdptr = NULL;
  }
  if (likely((ret = s_srvsession_thd_create(&s_mthd, 1000LL * 1000LL * 1000LL * 5LL)) == SQC_RESULT_OK)) {
    const char *addr = sqc_rpc_sched_conf_get_rpc_server_address();
    *thdptr = (sqc_thread_t *)&s_mthd;
    s_mod_inited = true;
    if ((ret = s_srvsession_accept_initialize(addr, true)) != SQC_RESULT_OK) {
      sqc_perror(ret);
    }
  }

  return ret;
}


static sqc_result_t
s_srvsession_start(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  sqc_msg_debug(5, "called.\n");

  if (likely(s_mod_inited == true)) {
    if (likely(s_mthd != NULL)) {
      if (likely((ret = sqc_thread_start((sqc_thread_t *)&s_mthd, false)) ==
                 SQC_RESULT_OK)) {

        s_srvsession_wlock(s_mthd);
        {
          s_mthd->do_loop_ = true;
        }
        s_srvsession_unlock(s_mthd);

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
s_srvsession_shutdown(shutdown_grace_level_t l) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  sqc_msg_debug(5, "called.\n");

  s_srvsession_accept_shutdown(l);

  if (likely(s_mod_inited == true)) {
    if (likely(s_mthd != NULL)) {

      s_srvsession_wlock(s_mthd);
      {
        if (s_mthd->is_started_ == true) {
          s_mthd->shutdown_level_ = l;
          s_mthd->do_loop_ = false;
          ret = SQC_RESULT_OK;
        } else {
          ret = SQC_RESULT_INVALID_STATE_TRANSITION;
        }
      }
      s_srvsession_unlock(s_mthd);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_STATE_TRANSITION;
  }

  return ret;
}


static sqc_result_t
s_srvsession_stop(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  sqc_msg_debug(5, "called.\n");

  s_srvsession_accept_stop();

  if (likely(s_mod_inited == true)) {
    if (likely(s_mthd != NULL)) {

      s_srvsession_wlock(s_mthd);
      {
        if (s_mthd->is_started_ == true) {
          ret = sqc_thread_cancel((sqc_thread_t *)&s_mthd);
        } else {
          ret = SQC_RESULT_INVALID_STATE_TRANSITION;
        }
      }
      s_srvsession_unlock(s_mthd);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_STATE_TRANSITION;
  }

  return ret;
}


static void
s_srvsession_finalize(void) {
  sqc_msg_debug(5, "called.\n");

  s_srvsession_accept_finalize();

  if (likely(s_mthd != NULL)) {
    s_srvsession_thd_destroy(s_mthd);
  }
}


/*
 * export
 */

sqc_tls_conf_t
srvsession_get_tls_conf(void) {
  return s_tls_conf;
}


rpc_jwt_server_ctx_t *
srvsession_get_jwt_ctx(void) {
  return &s_jwt_ctx;
}


sqc_result_t
srvsession_register(void) {
  return sqc_module_register("Server Session",
                             s_srvsession_initialize,
                             NULL,
                             s_srvsession_start,
                             s_srvsession_shutdown,
                             s_srvsession_stop,
                             s_srvsession_finalize,
                             NULL);
}
