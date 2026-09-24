#include "sqc_apis.h"
#include "sqc_thread_internal.h"

#include "modtmpl.h"





typedef struct modtmpl_thread_record {
  struct sqc_thread_record thd_;

  sqc_chrono_t interval_;

  sqc_rwlock_t rwlck_;

  volatile bool do_loop_;
  volatile bool is_started_;
  volatile shutdown_grace_level_t shutdown_level_;
} modtmpl_thread_record;
typedef modtmpl_thread_record *modtmpl_thread_t;





static modtmpl_thread_t s_mthd = NULL;
static bool s_mod_inited = false;





/*
 * module thread methods
 */


static inline void
s_modtmpl_rlock(modtmpl_thread_t mt) {
  if (likely(mt != NULL)) {
    (void)sqc_rwlock_reader_lock(&(mt->rwlck_));
  }
}


static inline void
s_modtmpl_wlock(modtmpl_thread_t mt) {
  if (likely(mt != NULL)) {
    (void)sqc_rwlock_writer_lock(&(mt->rwlck_));
  }
}


static inline void
s_modtmpl_unlock(modtmpl_thread_t mt) {
  if (likely(mt != NULL)) {
    (void)sqc_rwlock_unlock(&(mt->rwlck_));
  }
}


static void
s_modtmpl_thd_finalize(const sqc_thread_t *tptr, bool is_canceled,
                       void *arg) {
  modtmpl_thread_t mt = (modtmpl_thread_t)*tptr;
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
        s_modtmpl_unlock(mt);
      }
    }
  }

  sqc_msg_debug(5, "called with %s self and the thread is %s.\n",
                ((mt != NULL) ? "valid" : "invalid (NULL)"),
                ((is_canceled == true) ? "canceled" : "exited"));
}


static void
s_modtmpl_thd_freeup(const sqc_thread_t *tptr, void *arg) {
  modtmpl_thread_t mt = (modtmpl_thread_t)*tptr;

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
s_modtmpl_thd_main(const sqc_thread_t *tptr, void *arg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  global_state_t s;
  shutdown_grace_level_t l;
  modtmpl_thread_t mt = (modtmpl_thread_t)*tptr;

  (void)arg;

  sqc_msg_debug(5, "waiting for the gala opening...\n");

  ret = global_state_wait_for(GLOBAL_STATE_STARTED, &s, &l, -1LL);
  if (ret == SQC_RESULT_OK && s == GLOBAL_STATE_STARTED) {
    s_modtmpl_wlock(mt);
    {
      mt->is_started_ = true;
    }
    s_modtmpl_unlock(mt);

    sqc_msg_debug(5, "gala opening.\n");

    /*
     * The main loop.
     */
    do {
      sqc_msg_debug(100, "looping...\n");

      (void)sqc_chrono_nanosleep(mt->interval_, NULL);

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
s_modtmpl_thd_create(modtmpl_thread_t *tptr, sqc_chrono_t interval) {
  sqc_result_t ret = SQC_RESULT_INVALID_ARGS;

  sqc_msg_debug(5, "called.\n");

  if (likely(interval >= 1000 * 1000)) {
    if ((ret = sqc_thread_create_with_size((sqc_thread_t *)tptr,
                                           sizeof(modtmpl_thread_record),
                                           s_modtmpl_thd_main,
                                           s_modtmpl_thd_finalize,
                                           s_modtmpl_thd_freeup,
                                           "modtmpl thread",
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
s_modtmpl_thd_destroy(modtmpl_thread_t tptr) {
  sqc_msg_debug(5, "called.\n");
    
  sqc_thread_destroy((sqc_thread_t *)tptr);
}





/*
 * module methods
 */


static sqc_result_t
s_modtmpl_initialize(int argc, const char *const argv[], void *extarg,
		     sqc_thread_t **thdptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  (void)argc;
  (void)argv;
  (void)extarg;

  sqc_msg_debug(5, "called.\n");
  
  if (thdptr != NULL) {
    *thdptr = NULL;
  }

  if (likely((ret = s_modtmpl_thd_create(&s_mthd,
                                         1000LL * 1000LL * 1000LL * 5LL)) ==
             SQC_RESULT_OK)) {
    *thdptr = (sqc_thread_t *)&s_mthd;
    s_mod_inited = true;
  }

  return ret;
}


static sqc_result_t
s_modtmpl_start(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  sqc_msg_debug(5, "called.\n");

  if (likely(s_mod_inited == true)) {
    if (likely(s_mthd != NULL)) {
      if (likely((ret = sqc_thread_start((sqc_thread_t *)&s_mthd, false)) ==
                 SQC_RESULT_OK)) {

        s_modtmpl_wlock(s_mthd);
        {
          s_mthd->do_loop_ = true;
        }
        s_modtmpl_unlock(s_mthd);

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
s_modtmpl_shutdown(shutdown_grace_level_t l) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  sqc_msg_debug(5, "called.\n");

  if (likely(s_mod_inited == true)) {
    if (likely(s_mthd != NULL)) {

      s_modtmpl_wlock(s_mthd);
      {
        if (s_mthd->is_started_ == true) {
          s_mthd->shutdown_level_ = l;
          s_mthd->do_loop_ = false;
          ret = SQC_RESULT_OK;
        } else {
          ret = SQC_RESULT_INVALID_STATE_TRANSITION;
        }
      }
      s_modtmpl_unlock(s_mthd);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_STATE_TRANSITION;
  }

  return ret;
}


static sqc_result_t
s_modtmpl_stop(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  sqc_msg_debug(5, "called.\n");

  if (likely(s_mod_inited == true)) {
    if (likely(s_mthd != NULL)) {

      s_modtmpl_wlock(s_mthd);
      {
        if (s_mthd->is_started_ == true) {
          ret = sqc_thread_cancel((sqc_thread_t *)&s_mthd);
        } else {
          ret = SQC_RESULT_INVALID_STATE_TRANSITION;
        }
      }
      s_modtmpl_unlock(s_mthd);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_STATE_TRANSITION;
  }

  return ret;
}


static void
s_modtmpl_finalize(void) {
  sqc_msg_debug(5, "called.\n");

  if (likely(s_mthd != NULL)) {
    s_modtmpl_thd_destroy(s_mthd);
  }
}





/*
 * export
 */


sqc_result_t
modtmpl_register(void) {
  return sqc_module_register("module template",
                             s_modtmpl_initialize,
                             NULL,
                             s_modtmpl_start,
                             s_modtmpl_shutdown,
                             s_modtmpl_stop,
                             s_modtmpl_finalize,
                             NULL);
}

