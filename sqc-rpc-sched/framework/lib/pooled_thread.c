#include "sqc_apis.h"

#include "sqc_poolable_internal.h"
#include "sqc_thread_internal.h"
#include "sqc_pool_internal.h"
#include "sqc_pooled_thread_internal.h"
#include "sqc_task_internal.h"





/*
 * Thread methods
 */


static sqc_result_t
s_task_runner_main(const sqc_thread_t *tptr, void *arg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_task_runner_thread_t tr = NULL;
  sqc_poolable_t pobj = NULL;
  char org_thd_name[16];
  char *tmp_name = NULL;

  (void)arg;

  if (likely(tptr != NULL && *tptr != NULL &&
             (tr = (sqc_task_runner_thread_t)*tptr) != NULL &&
             (pobj = tr->m_pobj) != NULL)) {

    bool do_autodelete = false;
    bool do_autorelease = false;
    bool do_shutdown = false;
    bool do_name = false;
    sqc_task_t t = NULL;
    global_state_t gst = GLOBAL_STATE_UNKNOWN;
    shutdown_grace_level_t lvl = SHUTDOWN_UNKNOWN;

    /*
     * Wait for the gala opening.
     */
    ret = global_state_wait_for(GLOBAL_STATE_STARTED, &gst, &lvl, -1LL);
    if (ret != SQC_RESULT_OK || gst != GLOBAL_STATE_STARTED) {
      sqc_msg_error("failed to waiting the gala opening.\n");
      sqc_perror(ret);
      goto done;
    }

    while (true) {

      sqc_msg_debug(5, "task loop start.\n");

      do_autodelete = false;
      do_autorelease = false;
      do_shutdown = false;
      do_name = false;
      t = NULL;

      (void)sqc_mutex_lock(&tr->m_lck);
      {

        /*
         * Wait for a task assignment.
         */
        do {

          do_shutdown = tr->m_is_shutdown_requested;

          if (likely(tr->m_is_got_a_task == true &&
                     (t = tr->m_tsk) != NULL)) {
            ret = SQC_RESULT_OK;
            tr->m_state = SQC_TASK_RUNNER_STATE_ASSIGNED;
            break;
          } else if (do_shutdown == true) {
            break;
          } else {
            ret = sqc_cond_wait(&tr->m_cnd, &tr->m_lck, -1LL);
            if (likely(ret == SQC_RESULT_OK)) {
              continue;
            } else {
              break;
            }
          }

        } while (true);

      }
      (void)sqc_mutex_unlock(&tr->m_lck);

      if (likely(ret == SQC_RESULT_OK && t != NULL &&
                 tr->m_state == SQC_TASK_RUNNER_STATE_ASSIGNED &&
                 do_shutdown == false &&
                 pobj->m_is_used == true)) {

        (void)sqc_mutex_lock(&tr->m_lck);
        {

          tr->m_state = SQC_TASK_RUNNER_STATE_RUNNING;

          (void)sqc_mutex_lock(&t->m_lck);
          {

            t->m_tr = tr;
            t->m_is_started = true;
            t->m_state = SQC_TASK_STATE_RUNNING;
            do_autodelete =
              ((t->m_flag & SQC_TASK_DELETE_CONTEXT_AFTER_EXEC) != 0) ?
              true : false;
            do_autorelease =
              ((t->m_flag & SQC_TASK_RELEASE_THREAD_AFTER_EXEC) != 0) ?
              true : false;
            do_name = (IS_VALID_STRING(t->m_name) == true) ? true : false;

          }
          (void)sqc_mutex_unlock(&t->m_lck);

        }
        (void)sqc_mutex_unlock(&tr->m_lck);

        /*
         * Set core/numa node affinity
         */
        if (t->m_numa_node_num != -1) {
          (void)sqc_thread_set_numa_node_affinity(tptr, t->m_numa_node_num);
        } else {
          (void)sqc_thread_set_cpu_affinity(tptr, -1);
          if (t->m_core_num != -1) {
            (void)sqc_thread_set_cpu_affinity(tptr, t->m_core_num);
          }
        }

        /*
         * Execute the task.
         */
        if (do_name == true) {
          (void)sqc_thread_get_name(tptr, org_thd_name, sizeof(org_thd_name));
          (void)sqc_thread_set_name(tptr, t->m_name);
          tmp_name = t->m_name;
        } else {
          tmp_name = (char *)"???";
        }

        sqc_msg_debug(5, "task \"%s\" start...\n", tmp_name);
        ret = t->m_main(&t);
        sqc_msg_debug(5, "task \"%s\" done.\n", tmp_name);

        if (do_name == true) {
          (void)sqc_thread_set_name(tptr, org_thd_name);
        }

        /*
         * Execution done.
         */
        sqc_msg_debug(5, "update \"%s\" status, return code %ld.\n",
                         tmp_name, ret);
        (void)sqc_mutex_lock(&tr->m_lck);
        {

          tr->m_is_got_a_task = false;
          tr->m_tsk = NULL;
          tr->m_state = SQC_TASK_RUNNER_STATE_NOT_ASSIGNED;

          (void)sqc_mutex_lock(&t->m_lck);
          {

            t->m_tr = NULL;
            t->m_exit_code = ret;
            t->m_is_clean_finished = true;
            t->m_state = SQC_TASK_STATE_CLEAN_FINISHED;

          }
          (void)sqc_mutex_unlock(&t->m_lck);

        }
        (void)sqc_mutex_unlock(&tr->m_lck);

        sqc_task_finalize(&t, false);

        /*
         * Reset core affinity
         */
        (void)sqc_thread_set_cpu_affinity_any(tptr);

        if (do_autodelete == true) {
          sqc_task_destroy(&t);
        }
        if (do_autorelease == true) {
          sqc_pool_release_poolable(&pobj);
        }

      } else {
        if (do_shutdown == true) {

          if (t != NULL) {

            (void)sqc_mutex_lock(&t->m_lck);
            {

              t->m_exit_code = SQC_RESULT_NOT_STARTED;
              t->m_is_runner_shutdown = true;
              t->m_state = SQC_TASK_STATE_RUNNER_SHUTDOWN;

            }
            (void)sqc_mutex_unlock(&t->m_lck);

          }

          ret = SQC_RESULT_OK;

          break;
        } else if (pobj->m_is_used == false) {
          sqc_msg_fatal("poolable obj is used without acquisition??\n");
          ret = SQC_RESULT_INVALID_STATE_TRANSITION;
          break;
        }

      }

      /*
       * end of main loop.
       */
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

done:

  sqc_msg_debug(5, "task runner thread main loop exited.\n");

  return ret;
}


static void
s_task_runner_finalize(const sqc_thread_t *tptr, bool is_cancelled,
                       void *arg) {
  sqc_poolable_t pobj = NULL;
  sqc_task_runner_thread_t tr = NULL;
  sqc_task_t t = NULL;

  (void)arg;

  sqc_msg_debug(5, "called, cancelled %s.\n",
                   (is_cancelled == true) ? "yes" : "no");

  if (likely(tptr != NULL && *tptr != NULL &&
             (tr = (sqc_task_runner_thread_t)*tptr) != NULL &&
             (pobj = tr->m_pobj) != NULL)) {

    if (is_cancelled == true) {
      (void)sqc_mutex_unlock(&tr->m_lck);
    }

    if ((t = tr->m_tsk) != NULL) {

      if (is_cancelled == true) {
        (void)sqc_mutex_unlock(&t->m_lck);
      }

      (void)sqc_mutex_lock(&t->m_lck);
      {

        t->m_is_cancelled = is_cancelled;
        if (is_cancelled == true) {
          t->m_state = SQC_TASK_STATE_CANCELLED;
        } else {
          t->m_state = SQC_TASK_STATE_HALTED;
        }

      }
      (void)sqc_mutex_unlock(&t->m_lck);

      sqc_task_finalize(&t, is_cancelled);
    }

    /*
     * finally call the poolable finalizer. This triggers invokation
     * of this poolable obj's teardown method
     * s_pooled_thread_teardown().
     */
    pobj->m_last_result = sqc_poolable_teardown(&pobj, is_cancelled);

  }

}


static void
s_task_runner_freeup(const sqc_thread_t *tptr, void *arg) {
  sqc_task_runner_thread_t tr = NULL;

  (void)arg;

  sqc_msg_debug(5, "called.\n");

  if (tptr != NULL && (tr = (sqc_task_runner_thread_t)*tptr) != NULL) {
    if (tr->m_lck != NULL) {
      sqc_mutex_destroy(&tr->m_lck);
      tr->m_lck = NULL;
    }
    if (tr->m_cnd != NULL) {
      sqc_cond_destroy(&tr->m_cnd);
      tr->m_cnd = NULL;
    }
  }
}





/*
 * poolable methods
 */


static sqc_result_t
s_pooled_thread_construct(sqc_poolable_t *pptr, void *args) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_pooled_thread_t ptptr = NULL;
  sqc_task_runner_thread_t tr = NULL;
  bool is_thread_created = false;

  (void)args;

  if (likely(pptr != NULL && (ptptr = (sqc_pooled_thread_t)*pptr) != NULL &&
             (tr = &ptptr->m_trthd) != NULL)) {
    /* thread creation */
    ret = sqc_thread_create_with_size((sqc_thread_t *)&tr,
                                         0,
                                         s_task_runner_main,
                                         s_task_runner_finalize,
                                         s_task_runner_freeup,
                                         "pooled_thread",
                                         NULL);
    if (likely(ret == SQC_RESULT_OK)) {

      is_thread_created = true;

      tr->m_pobj = *pptr;
      if (unlikely((ret = sqc_mutex_create(&tr->m_lck)) != SQC_RESULT_OK)) {
        sqc_perror(ret);
        sqc_msg_error("can't initialize a mutex for a pooled thread.\n");
        goto done;
      }
      if (unlikely((ret = sqc_cond_create(&tr->m_cnd)) != SQC_RESULT_OK)) {
        sqc_perror(ret);
        sqc_msg_error("can't initialize a cond for a pooled thread.\n");
        goto done;
      }
      tr->m_state = SQC_TASK_RUNNER_STATE_UNKNOWN;
      tr->m_is_shutdown_requested = false;
      tr->m_is_got_a_task = false;
      tr->m_shutdown_lvl = SHUTDOWN_UNKNOWN;
      tr->m_is_finished = false;
      tr->m_is_cancelled = false;
      tr->m_tsk = NULL;
      tr->m_is_auto_delete = false;

      tr->m_state = SQC_TASK_RUNNER_STATE_CREATED;

    } else {
      sqc_perror(ret);
      sqc_msg_error("can't create a pooled thread.\n");
      goto done;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

done:
  if (unlikely(tr != NULL && ret != SQC_RESULT_OK &&
               is_thread_created == true)) {
    sqc_thread_destroy((sqc_thread_t *)&tr);
  }

  return ret;
}


static sqc_result_t
s_pooled_thread_setup(sqc_poolable_t *pptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_pooled_thread_t ptptr = NULL;
  sqc_task_runner_thread_t tr = NULL;

  if (likely(pptr != NULL && (ptptr = (sqc_pooled_thread_t)*pptr) != NULL &&
             (tr = &ptptr->m_trthd) != NULL &&
             tr->m_state == SQC_TASK_RUNNER_STATE_CREATED)) {
    ret = sqc_thread_start((sqc_thread_t *)&tr, false);
    if (likely(ret == SQC_RESULT_OK)) {
      tr->m_state = SQC_TASK_RUNNER_STATE_NOT_ASSIGNED;
    }
  } else {
    if (tr != NULL && tr->m_state != SQC_TASK_RUNNER_STATE_CREATED) {
      ret = SQC_RESULT_INVALID_OBJECT;
    } else {
      ret = SQC_RESULT_INVALID_ARGS;
    }
  }

  return ret;
}


static sqc_result_t
s_pooled_thread_shutdown(sqc_poolable_t *pptr,
                         shutdown_grace_level_t lvl) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_pooled_thread_t ptptr = NULL;
  sqc_task_runner_thread_t tr = NULL;

  sqc_msg_debug(5, "called.\n");

  if (likely(pptr != NULL && (ptptr = (sqc_pooled_thread_t)*pptr) != NULL &&
             (tr = &ptptr->m_trthd) != NULL)) {

    (void)sqc_mutex_lock(&tr->m_lck);
    {

      if (tr->m_state != SQC_TASK_RUNNER_STATE_CREATED &&
          tr->m_state != SQC_TASK_RUNNER_STATE_FINISH) {
        tr->m_is_shutdown_requested = true;
        tr->m_shutdown_lvl = lvl;

        ret = sqc_cond_notify(&tr->m_cnd, &tr->m_lck);
      } else {
        ret = SQC_RESULT_OK;
      }

    }
    (void)sqc_mutex_unlock(&tr->m_lck);

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static sqc_result_t
s_pooled_thread_cancel(sqc_poolable_t *pptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_pooled_thread_t ptptr = NULL;
  sqc_task_runner_thread_t tr = NULL;

  if (likely(pptr != NULL && (ptptr = (sqc_pooled_thread_t)*pptr) != NULL &&
             (tr = &ptptr->m_trthd) != NULL)) {

    (void)sqc_mutex_lock(&tr->m_lck);
    {

      if (tr->m_state != SQC_TASK_RUNNER_STATE_CREATED &&
          tr->m_state != SQC_TASK_RUNNER_STATE_FINISH) {
        ret = sqc_thread_cancel((sqc_thread_t *)&tr);
      } else {
        ret = SQC_RESULT_OK;
      }

    }
    (void)sqc_mutex_unlock(&tr->m_lck);

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static sqc_result_t
s_pooled_thread_teardown(sqc_poolable_t *pptr, bool is_cancelled) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_pooled_thread_t ptptr = NULL;
  sqc_task_runner_thread_t tr = NULL;

  sqc_msg_debug(5, "called, cancelled %s.\n",
                   (is_cancelled == true) ? "yes" : "no");

  if (likely(pptr != NULL && (ptptr = (sqc_pooled_thread_t)*pptr) != NULL &&
             (tr = &ptptr->m_trthd) != NULL)) {
    if (is_cancelled == true) {
      sqc_mutex_unlock(&tr->m_lck);
    }

    ret = SQC_RESULT_OK;

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static sqc_result_t
s_pooled_thread_wait(sqc_poolable_t *pptr, sqc_chrono_t nsec) {
  (void)pptr;
  (void)nsec;

  sqc_msg_debug(5, "called.\n");

  return SQC_RESULT_OK;
}


static void
s_pooled_thread_destruct(sqc_poolable_t *pptr) {
  sqc_pooled_thread_t ptptr = NULL;
  sqc_task_runner_thread_t tr = NULL;

  if (likely(pptr != NULL && (ptptr = (sqc_pooled_thread_t)*pptr) != NULL &&
             (tr = &ptptr->m_trthd) != NULL)) {

    if (tr->m_lck != NULL) {
      sqc_mutex_destroy(&tr->m_lck);
      tr->m_lck = NULL;
    }
    if (tr->m_cnd != NULL) {
      sqc_cond_destroy(&tr->m_cnd);
      tr->m_cnd = NULL;
    }

    (void)sqc_thread_destroy((sqc_thread_t *)&tr);

  }
}





/*
 * pool wrapper
 */


static sqc_poolable_methods_record s_methods = {
  s_pooled_thread_construct,
  s_pooled_thread_setup,
  s_pooled_thread_shutdown,
  s_pooled_thread_cancel,
  s_pooled_thread_teardown,
  s_pooled_thread_wait,
  s_pooled_thread_destruct,
};


sqc_result_t
sqc_thread_pool_create(sqc_thread_pool_t *pptr, const char *name,
                          size_t n) {
  return sqc_pool_create((sqc_pool_t *)pptr,
                            sizeof(sqc_thread_pool_record),
                            name,
                            SQC_POOL_TYPE_QUEUE,	/* queue type */
                            true,			/* executor */
                            n,
                            sizeof(sqc_pooled_thread_record),
                            &s_methods);
}


sqc_result_t
sqc_thread_pool_acquire_thread(sqc_thread_pool_t *pptr,
                                  sqc_pooled_thread_t *ptptr,
                                  sqc_chrono_t to) {
  return sqc_pool_acquire_poolable((sqc_pool_t *)pptr, to,
                                      (sqc_poolable_t *)ptptr);
}


sqc_result_t
sqc_thread_pool_release_thread(sqc_pooled_thread_t *ptptr) {
  return sqc_pool_release_poolable((sqc_poolable_t *)ptptr);
}


sqc_result_t
sqc_thread_pool_get_outstanding_thread_num(sqc_thread_pool_t *pptr) {
  return sqc_pool_get_outstanding_obj_num((sqc_pool_t *)pptr);
}


sqc_result_t
sqc_thread_pool_wakeup(sqc_thread_pool_t *pptr, sqc_chrono_t to) {
  return sqc_pool_wakeup((sqc_pool_t *)pptr, to);
}


sqc_result_t
sqc_thread_pool_shutdown_all(sqc_thread_pool_t *pptr,
                                shutdown_grace_level_t lvl,
                                sqc_chrono_t to) {
  return sqc_pool_shutdown((sqc_pool_t *)pptr, lvl, to);
}


sqc_result_t
sqc_thread_pool_cancel_all(sqc_thread_pool_t *pptr) {
  return sqc_pool_cancel((sqc_pool_t *)pptr);
}


sqc_result_t
sqc_thread_pool_wait_all(sqc_thread_pool_t *pptr, sqc_chrono_t to) {
  return sqc_pool_wait((sqc_pool_t *)pptr, to);
}


void
sqc_thread_pool_destroy(sqc_thread_pool_t *pptr) {
  sqc_pool_destroy((sqc_pool_t *)pptr);
}


sqc_result_t
sqc_thread_pool_get(const char *name, sqc_thread_pool_t *pptr) {
  return sqc_pool_get_pool(name, (sqc_pool_t *)pptr);
}

