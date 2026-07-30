#include "sqc_apis.h"
#include "sqc_poolable_internal.h"





sqc_result_t
sqc_poolable_create_with_size(sqc_poolable_t *pptr,
                                 bool is_executor,
                                 sqc_poolable_methods_t m,
                                 void *carg, size_t sz) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  size_t alloc_sz = sz;
  sqc_poolable_t p = NULL;

  if (likely(pptr != NULL)) {
    if (*pptr == NULL) {
      if (alloc_sz < sizeof(sqc_poolable_record)) {
        alloc_sz = sizeof(sqc_poolable_record);
      }
      if (likely((p = (sqc_poolable_t)malloc(alloc_sz)) != NULL)) {
        *pptr = p;
      } else {
        ret = SQC_RESULT_NO_MEMORY;
        sqc_perror(ret);
        sqc_msg_error("can't allocate a poolable object.\n");
        goto done;
      }
    } else {
      p = *pptr;
    }

    if (unlikely((ret = sqc_mutex_create(&p->m_lck)) != SQC_RESULT_OK)) {
      sqc_perror(ret);
      sqc_msg_error("can't initialize a mutex for a poolable object.\n");
      goto done;
    }
    if (unlikely((ret = sqc_cond_create(&p->m_cnd)) != SQC_RESULT_OK)) {
      sqc_perror(ret);
      sqc_msg_error("can't initialize a cond for a poolable object.\n");
      goto done;
    }
    p->m_pool = NULL;
    p->m_is_executor = is_executor;
    p->m_obj_idx = 0;
    p->m_is_used = false;
    p->m_is_cancelled = false;
    p->m_is_torndown = false;
    p->m_is_wait_done = false;
    p->m_total_obj_size = alloc_sz;
    p->m_st = SQC_POOLABLE_STATE_UNKNOWN;
    p->m_last_result = SQC_RESULT_ANY_FAILURES;
    p->m_shutdown_lvl = SHUTDOWN_UNKNOWN;

#define set_method(mem)                                         \
  p->mem = ((m != NULL && m->mem != NULL) ? m->mem : NULL)

    set_method(m_construct);
    set_method(m_setup);
    set_method(m_shutdown);
    set_method(m_cancel);
    set_method(m_teardown);
    set_method(m_wait);
    set_method(m_destruct);
#undef set_method

    if (is_executor == true) {
      if (unlikely((p->m_construct == NULL && p->m_setup == NULL) ||
                   p->m_shutdown == NULL ||
                   p->m_cancel == NULL ||
                   p->m_teardown == NULL ||
                   p->m_wait == NULL ||
                   p->m_destruct == NULL)) {
        ret = SQC_RESULT_INVALID_ARGS;
        sqc_perror(ret);
        sqc_msg_error("insufficient methods to creating an executor type "
                         "poolable object.\n");
        goto done;
      }
    } else {
      if (unlikely((p->m_construct == NULL && p->m_setup == NULL) ||
                   p->m_destruct == NULL)) {
        ret = SQC_RESULT_INVALID_ARGS;
        sqc_perror(ret);
        sqc_msg_error("insufficient methods to creating a "
                         "poolable object.\n");
        goto done;
      }
    }

    if (p->m_construct != NULL) {
      ret = p->m_construct(pptr, carg);
    } else {
      ret = SQC_RESULT_OK;
    }

    if (likely(ret == SQC_RESULT_OK)) {
      p->m_st = SQC_POOLABLE_STATE_CONSTRUCTED;
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

done:
  if (unlikely(ret != SQC_RESULT_OK)) {
    if (*pptr != NULL && alloc_sz != 0) {
      free(*pptr);
      *pptr = NULL;
    }
  }

  return ret;
}


sqc_result_t
sqc_poolable_setup(sqc_poolable_t *pptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(pptr != NULL && *pptr != NULL)) {

    (void)sqc_mutex_lock(&(*pptr)->m_lck);
    {

      if (likely((*pptr)->m_st == SQC_POOLABLE_STATE_CONSTRUCTED)) {
        if ((*pptr)->m_setup != NULL) {
          ret = (*pptr)->m_setup(pptr);
        } else {
          ret = SQC_RESULT_OK;
        }
        if (likely(ret == SQC_RESULT_OK)) {
          (*pptr)->m_st = SQC_POOLABLE_STATE_OPERATIONAL;
        }
      } else {
        ret = SQC_RESULT_INVALID_STATE_TRANSITION;
      }
      (*pptr)->m_last_result = ret;

    }
    (void)sqc_mutex_unlock(&(*pptr)->m_lck);

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_poolable_shutdown(sqc_poolable_t *pptr,
                         shutdown_grace_level_t lvl) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(pptr != NULL && *pptr != NULL)) {

    sqc_msg_debug(5, "called.\n");

    (void)sqc_mutex_lock(&(*pptr)->m_lck);
    {

      if ((*pptr)->m_st == SQC_POOLABLE_STATE_NOT_OPERATIONAL &&
          (*pptr)->m_is_torndown == true) {
        ret = SQC_RESULT_OK;
      } else if ((*pptr)->m_st == SQC_POOLABLE_STATE_OPERATIONAL) {
        (*pptr)->m_shutdown_lvl = lvl;
        if ((*pptr)->m_shutdown != NULL) {
          ret = (*pptr)->m_shutdown(pptr, lvl);
        } else {
          ret = SQC_RESULT_OK;
        }

        if (likely(ret == SQC_RESULT_OK)) {
          (*pptr)->m_st = SQC_POOLABLE_STATE_SHUTDOWNING;
          if ((*pptr)->m_is_executor == false) {
            if ((*pptr)->m_teardown != NULL) {
              ret = (*pptr)->m_teardown(pptr, false);
            } else {
              ret = SQC_RESULT_OK;
            }
            if (likely(ret == SQC_RESULT_OK)) {
              (*pptr)->m_st = SQC_POOLABLE_STATE_NOT_OPERATIONAL;
            }
          }
        }

      } else {
        ret = SQC_RESULT_INVALID_STATE_TRANSITION;
      }
      (*pptr)->m_last_result = ret;

    }
    (void)sqc_mutex_unlock(&(*pptr)->m_lck);

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_poolable_cancel(sqc_poolable_t *pptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(pptr != NULL && *pptr != NULL)) {

    (void)sqc_mutex_lock(&(*pptr)->m_lck);
    {

      if ((*pptr)->m_st == SQC_POOLABLE_STATE_NOT_OPERATIONAL &&
          (*pptr)->m_is_torndown == true) {
        ret = SQC_RESULT_OK;
      } else if ((*pptr)->m_st == SQC_POOLABLE_STATE_OPERATIONAL ||
                 ((*pptr)->m_st == SQC_POOLABLE_STATE_SHUTDOWNING &&
                  (*pptr)->m_shutdown_lvl == SHUTDOWN_RIGHT_NOW)) {
        if ((*pptr)->m_cancel != NULL) {
          ret = (*pptr)->m_cancel(pptr);
        } else {
          ret = SQC_RESULT_OK;
        }
        if (likely(ret == SQC_RESULT_OK)) {
          (*pptr)->m_st = SQC_POOLABLE_STATE_CANCELLING;
          if ((*pptr)->m_is_executor == false) {
            (*pptr)->m_is_cancelled = true;
            if ((*pptr)->m_teardown != NULL) {
              ret = (*pptr)->m_teardown(pptr, true);
            } else {
              ret = SQC_RESULT_OK;
            }
            if (likely(ret == SQC_RESULT_OK)) {
              (*pptr)->m_st = SQC_POOLABLE_STATE_NOT_OPERATIONAL;
            }
          }
        }
      } else {
        ret = SQC_RESULT_INVALID_STATE_TRANSITION;
      }
      (*pptr)->m_last_result = ret;

    }
    (void)sqc_mutex_unlock(&(*pptr)->m_lck);

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


/*
 * NOTE:
 *
 * When executor poolables are finalized by thread finalized
 * procedures, this poolable teardown proc must be called from
 * sqc_threads' final proc. In order to do this, every executor
 * poolable must have a reverse reference to its parent poolable
 * instance.
 */
sqc_result_t
sqc_poolable_teardown(sqc_poolable_t *pptr, bool is_cancelled) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  sqc_msg_debug(5, "called, cancelled %s.\n",
                   (is_cancelled == true) ? "yes" : "no");

  if (likely(pptr != NULL && *pptr != NULL)) {

    bool do_call_teardown = false;
    bool is_torndown = false;

    if (is_cancelled == true) {
      (void)sqc_mutex_unlock(&(*pptr)->m_lck);
    }

    (void)sqc_mutex_lock(&(*pptr)->m_lck);
    {

      if ((*pptr)->m_is_torndown == true) {
        is_torndown = true;
        ret = SQC_RESULT_OK;
      } else {
        if ((*pptr)->m_teardown != NULL) {
          do_call_teardown = true;
        } else {
          is_torndown = true;
          ret = SQC_RESULT_OK;
        }
      }

    }
    (void)sqc_mutex_unlock(&(*pptr)->m_lck);

    if (is_torndown != true) {
      if (do_call_teardown == true) {
        ret = (*pptr)->m_teardown(pptr, is_cancelled);
      }

      (void)sqc_mutex_lock(&(*pptr)->m_lck);
      {

        (*pptr)->m_last_result = ret;
        (*pptr)->m_is_cancelled = is_cancelled;
        (*pptr)->m_st = SQC_POOLABLE_STATE_NOT_OPERATIONAL;
        (*pptr)->m_is_torndown = true;
        (*pptr)->m_is_wait_done = true;

        ret = sqc_cond_notify(&(*pptr)->m_cnd, true);

      }
      (void)sqc_mutex_unlock(&(*pptr)->m_lck);

    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_poolable_wait(sqc_poolable_t *pptr, sqc_chrono_t nsec) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  sqc_msg_debug(5, "called.\n");

  if (likely(pptr != NULL && *pptr != NULL)) {

    bool do_call_wait = false;

    (void)sqc_mutex_lock(&(*pptr)->m_lck);
    {

      do {

        if (likely((*pptr)->m_is_torndown == true &&
                   (*pptr)->m_is_wait_done == true)) {
          ret = SQC_RESULT_OK;
          break;
        } else {
          ret = sqc_cond_wait(&(*pptr)->m_cnd, &(*pptr)->m_lck, nsec);
          if (likely(ret == SQC_RESULT_OK)) {
            if ((*pptr)->m_wait != NULL) {
              do_call_wait = true;
            }
          } else {
            break;
          }
        }

      } while (true);

    }
    (void)sqc_mutex_unlock(&(*pptr)->m_lck);

    if (do_call_wait == true) {
      ret = (*pptr)->m_wait(pptr, nsec);
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
sqc_poolable_destroy(sqc_poolable_t *pptr) {
  if (likely(pptr != NULL && *pptr != NULL)) {
    sqc_result_t r1 = SQC_RESULT_ANY_FAILURES;
    sqc_result_t r2 = SQC_RESULT_ANY_FAILURES;

    sqc_msg_debug(5, "called.\n");

    r1 = sqc_poolable_shutdown(pptr, SHUTDOWN_RIGHT_NOW);
    r2 = sqc_poolable_wait(pptr, -1LL);

    if (r1 == SQC_RESULT_OK && r2 == SQC_RESULT_OK &&
        (*pptr)->m_is_cancelled == false) {
      if ((*pptr)->m_destruct != NULL) {
        (*pptr)->m_destruct(pptr);
      }
      (void)sqc_mutex_destroy(&(*pptr)->m_lck);
      (void)sqc_cond_destroy(&(*pptr)->m_cnd);
      if ((*pptr)->m_total_obj_size != 0) {
        free(*pptr);
        *pptr = NULL;
      }
    }
  }
}


sqc_result_t
sqc_poolable_is_operational(sqc_poolable_t *pptr, bool *bptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(pptr != NULL && *pptr != NULL && bptr != NULL)) {

    (void)sqc_mutex_lock(&(*pptr)->m_lck);
    {
      *bptr = ((*pptr)->m_st == SQC_POOLABLE_STATE_OPERATIONAL) ? true : false;
    }
    (void)sqc_mutex_unlock(&(*pptr)->m_lck);

    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_poolable_get_status(sqc_poolable_t *pptr,
                           sqc_poolable_state_t *st) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(pptr != NULL && *pptr != NULL && st != NULL)) {

    (void)sqc_mutex_lock(&(*pptr)->m_lck);
    {
      *st = (*pptr)->m_st;
    }
    (void)sqc_mutex_unlock(&(*pptr)->m_lck);

    ret = SQC_RESULT_OK;

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}

