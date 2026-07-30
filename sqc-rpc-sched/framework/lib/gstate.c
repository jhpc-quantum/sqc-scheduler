#include "sqc_apis.h"





static pthread_once_t s_once = PTHREAD_ONCE_INIT;
static bool s_is_inited = false;

static sqc_mutex_t s_lck = NULL;
static sqc_cond_t s_cond = NULL;
static global_state_t s_gs = GLOBAL_STATE_UNKNOWN;
static shutdown_grace_level_t s_gl = SHUTDOWN_UNKNOWN;

static void s_ctors(void) __attr_constructor__(109);
static void s_dtors(void) __attr_destructor__(109);





static void
s_child_at_fork(void) {
  (void)sqc_mutex_reinitialize(&s_lck);
}


static void
s_once_proc(void) {
  sqc_result_t r;

  if ((r = sqc_mutex_create(&s_lck)) != SQC_RESULT_OK) {
    sqc_perror(r);
    sqc_exit_fatal("Can't initilize a mutex.\n");
  }
  if ((r = sqc_cond_create(&s_cond)) !=  SQC_RESULT_OK) {
    sqc_perror(r);
    sqc_exit_fatal("Can't initilize a cond.\n");
  }

  (void)pthread_atfork(NULL, NULL, s_child_at_fork);

  s_is_inited = true;
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();

  sqc_msg_debug(10, "The global status tracker initialized.\n");
}


static inline void
s_final(void) {
  if (s_cond != NULL) {
    sqc_cond_destroy(&s_cond);
  }
  if (s_lck != NULL) {
    (void)sqc_mutex_lock(&s_lck);
    (void)sqc_mutex_unlock(&s_lck);
    sqc_mutex_destroy(&s_lck);
  }
}


static void
s_dtors(void) {
  if (s_is_inited == true) {
    if (sqc_module_is_unloading() &&
        sqc_module_is_finalized_cleanly()) {

      s_final();

      sqc_msg_debug(10, "The global status tracker finalized.\n");
    } else {
      sqc_msg_debug(10, "The global status tracker is not finalized "
                       "because of module finalization problem.\n");
    }
  }
}





static inline void
s_lock(void) {
  (void)sqc_mutex_lock(&s_lck);
}


static inline void
s_unlock(void) {
  (void)sqc_mutex_unlock(&s_lck);
}


static inline bool
s_is_valid_state(global_state_t s) {
  /*
   * Make sure the state transition one way only.
   */
  return ((int)s >= (int)s_gs) ? true : false;
}





sqc_result_t
global_state_set(global_state_t s) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (IS_VALID_GLOBAL_STATE(s) == true) {

    s_lock();
    {
      if (s_is_valid_state(s) == true) {
        s_gs = s;
        (void)sqc_cond_notify(&s_cond, true);
        ret = SQC_RESULT_OK;
      } else {
        ret = SQC_RESULT_INVALID_STATE_TRANSITION;
      }
    }
    s_unlock();

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
global_state_get(global_state_t *sptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (sptr != NULL) {

    s_lock();
    {
      *sptr = s_gs;
    }
    s_unlock();

    ret = SQC_RESULT_OK;

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
global_state_wait_for(global_state_t s_wait_for,
                      global_state_t *cur_sptr,
                      shutdown_grace_level_t *cur_gptr,
                      sqc_chrono_t nsec) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (IS_VALID_GLOBAL_STATE(s_wait_for) == true) {

    s_lock();
    {
    recheck:
      if ((int)s_gs < (int)s_wait_for &&
          IS_GLOBAL_STATE_SHUTDOWN(s_gs) == false) {
        ret = sqc_cond_wait(&s_cond, &s_lck, nsec);
        if (ret == SQC_RESULT_OK) {
          goto recheck;
        }
      } else {
        if (cur_sptr != NULL) {
          *cur_sptr = s_gs;
        }
        if (cur_gptr != NULL) {
          *cur_gptr = s_gl;
        }
        if ((int)s_gs >= (int)s_wait_for) {
          ret = SQC_RESULT_OK;
        } else if (IS_GLOBAL_STATE_SHUTDOWN(s_gs) == true &&
                   IS_GLOBAL_STATE_SHUTDOWN(s_wait_for) == false) {
          ret = SQC_RESULT_NOT_OPERATIONAL;
        } else {
          ret = SQC_RESULT_OK;
        }
      }
    }
    s_unlock();

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
global_state_wait_for_shutdown_request(shutdown_grace_level_t *cur_gptr,
                                       sqc_chrono_t nsec) {
  return global_state_wait_for(GLOBAL_STATE_REQUEST_SHUTDOWN,
                               NULL, cur_gptr, nsec);
}


sqc_result_t
global_state_request_shutdown(shutdown_grace_level_t l) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (IS_VALID_SHUTDOWN(l) == true) {

    s_lock();
    {
      if (IS_GLOBAL_STATE_SHUTDOWN(s_gs) == false &&
          s_gs != GLOBAL_STATE_REQUEST_SHUTDOWN) {
        if (s_is_valid_state(GLOBAL_STATE_REQUEST_SHUTDOWN) == true) {
          s_gs = GLOBAL_STATE_REQUEST_SHUTDOWN;
          s_gl = l;
          (void)sqc_cond_notify(&s_cond, true);

          /*
           * Wait until someone changes the state to
           * GLOBAL_STATE_ACCEPT_SHUTDOWN
           */
        recheck:
          if (IS_GLOBAL_STATE_SHUTDOWN(s_gs) == false) {
            if ((ret = sqc_cond_wait(&s_cond, &s_lck, -1LL)) ==
                SQC_RESULT_OK) {
              goto recheck;
            }
          }
        } else {
          ret = SQC_RESULT_INVALID_STATE_TRANSITION;
        }
      }
    }
    s_unlock();

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
global_state_cancel_janitor(void) {
  s_unlock();
}


void
global_state_reset(void) {
  s_lock();
  {
    s_gs = GLOBAL_STATE_UNKNOWN;
  }
  s_unlock();
}

