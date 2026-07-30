/**
 * General lock order:
 *
 *	The global lock (s_lock_global)
 *	The timed task queue lock (s_lock_task_q)
 *	A task lock (s_lock_task)
 *
 * Note taht the task locks are recursive.
 */





static inline void
s_lock_task(sqc_callout_task_t t) {
  if (likely(t != NULL)) {
    (void)sqc_mutex_lock(&(t->m_lock));
  }
}


static inline void
s_unlock_task(sqc_callout_task_t t) {
  if (likely(t != NULL)) {
    (void)sqc_mutex_unlock(&(t->m_lock));
  }
}


static inline void
s_wait_task(sqc_callout_task_t t) {
  if (likely(t != NULL)) {
    (void)sqc_cond_wait(&(t->m_cond), &(t->m_lock), -1LL);
  }
}


static inline void
s_wakeup_task(sqc_callout_task_t t) {
  if (likely(t != NULL)) {
    (void)sqc_cond_notify(&(t->m_cond), true);
  }
}





static inline callout_task_state_t
s_get_task_state_in_table(const sqc_callout_task_t t) {
  callout_task_state_t ret = TASK_STATE_UNKNOWN;

  if (likely(t != NULL)) {
    void *val;
    sqc_result_t r;

    r = sqc_hashmap_find(&s_tsk_tbl, (void *)t, &val);
    if (likely(r == SQC_RESULT_OK)) {
      ret = (callout_task_state_t)val;
    }
  }

  return ret;
}


static inline callout_task_state_t
s_set_task_state_in_table(const sqc_callout_task_t t,
                          callout_task_state_t s) {
  callout_task_state_t ret = TASK_STATE_UNKNOWN;

  if (likely(t != NULL)) {
    void *val;
    sqc_result_t r;

    if (likely((r = sqc_hashmap_find(&s_tsk_tbl, (void *)t, &val)) ==
               SQC_RESULT_OK)) {
      ret = (callout_task_state_t)val;
      val = (void *)s;

      if (unlikely((r = sqc_hashmap_add(&s_tsk_tbl, (void *)t,
                                           &val, true)) !=
                   SQC_RESULT_OK)) {
        ret = TASK_STATE_UNKNOWN;
      }
    } else if (likely(r == SQC_RESULT_NOT_FOUND)) {
      val = (void *)s;
      if (likely((r = sqc_hashmap_add(&s_tsk_tbl, (void *)t,
                                         &val, true)) ==
                 SQC_RESULT_OK)) {
        ret = s;
      }
    }
  }

  return ret;
}


static inline void
s_delete_task_in_table(const sqc_callout_task_t t) {
  if (likely(t != NULL)) {
    (void)sqc_hashmap_delete(&s_tsk_tbl, (void *)t, NULL, true);
  }
}





static sqc_result_t
s_runnable_run(const sqc_runnable_t *rptr, void *arg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  (void)arg;

  if (likely(rptr != NULL && *rptr != NULL)) {
    sqc_callout_task_t t = (sqc_callout_task_t)*rptr;
    if (likely(t->m_proc != NULL)) {
      ret = (t->m_proc)(t->m_arg);
    } else {
      ret = SQC_RESULT_INVALID_ARGS;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline sqc_result_t
s_create_task(sqc_callout_task_t *tptr,
              size_t sz,
              const char *name,
              sqc_callout_task_proc_t proc,
              void *arg,
              sqc_callout_task_arg_freeup_proc_t freeproc) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  const char *nm = NULL;
  sqc_mutex_t rcrmtx = NULL;
  sqc_cond_t cnd = NULL;

  if (IS_VALID_STRING(name) == true) {
    if (unlikely((nm = strdup(name)) == NULL)) {
      ret = SQC_RESULT_NO_MEMORY;
      goto done;
    }
  }

  if (sz == 0) {
    sz = DEFAULT_TASK_ALLOC_SZ;
  }

  if (likely((ret = sqc_mutex_create_recursive(&rcrmtx)) ==
             SQC_RESULT_OK)) {
    if (unlikely((ret = sqc_cond_create(&cnd)) != SQC_RESULT_OK)) {
      sqc_mutex_destroy(&rcrmtx);
      goto done;
    }
  } else {
    goto done;
  }

  s_lock_global();
  {
    if (likely((ret = sqc_runnable_create((sqc_runnable_t *)tptr, sz,
                      s_runnable_run, NULL, NULL)) ==
               SQC_RESULT_OK)) {

      (void)memset((void *)&((*tptr)->m_entry), 0, sizeof((*tptr)->m_entry));

      (*tptr)->m_status = TASK_STATE_CREATED;
      (*tptr)->m_lock = rcrmtx;
      (*tptr)->m_cond = cnd;
      (*tptr)->m_exec_ref_count = 0;
      (*tptr)->m_cancel_ref_count = 0;
      (*tptr)->m_name = nm;
      (*tptr)->m_proc = proc;
      (*tptr)->m_arg = arg;
      (*tptr)->m_freeproc = freeproc;
      (*tptr)->m_do_repeat = false;
      (*tptr)->m_is_first = true;
      (*tptr)->m_is_in_timed_q = false;
      (*tptr)->m_initial_delay_time = -1LL;
      (*tptr)->m_interval_time = -1LL;
      (*tptr)->m_last_abstime = 0;
      (*tptr)->m_next_abstime = 0;

      if (unlikely(s_set_task_state_in_table(*tptr, TASK_STATE_CREATED) !=
                   TASK_STATE_CREATED)) {
        /*
         * Avoid arg be freed by destroyer at this moment.
         */
        (*tptr)->m_arg = NULL;
        (*tptr)->m_freeproc = NULL;
        s_destroy_task_no_lock(*tptr);
        nm = NULL;
        *tptr = NULL;
        ret = SQC_RESULT_NO_MEMORY;
      }
    }
  }
  s_unlock_global();

done:
  if (ret != SQC_RESULT_OK && nm != NULL) {
    free((void *)nm);
  }

  return ret;
}





static inline void
s_delete(sqc_callout_task_t t) {
  sqc_runnable_t r = (sqc_runnable_t)t;
  callout_task_state_t st = s_get_task_state_in_table(t);

  if (likely(st == TASK_STATE_CANCELLED)) {
    bool did_delete = false;

    s_lock_task(t);
    {
      if (likely(t->m_status != TASK_STATE_DELETING)) {
        s_delete_task_in_table(t);
        t->m_status = TASK_STATE_DELETING;
        did_delete = true;

        if (t->m_freeproc != NULL && t->m_arg != NULL) {
          (t->m_freeproc)(t->m_arg);
        }

        if (IS_VALID_STRING(t->m_name) == true) {
          free((void *)t->m_name);
        }
      }
      t->m_cancel_ref_count = 0;
      t->m_exec_ref_count = 0;
      mbar();
      (void)sqc_cond_notify(&(t->m_cond), true);
    }
    s_unlock_task(t);

    if (likely(did_delete == true)) {
      sqc_mutex_destroy(&(t->m_lock));
      sqc_cond_destroy(&(t->m_cond));

      sqc_runnable_destroy(&r);
    }
  }
}


static inline void
s_destroy_task_no_lock(sqc_callout_task_t t) {
  if (likely(t != NULL)) {
    s_delete(t);
  }
}


static inline void
s_destroy_task(sqc_callout_task_t t) {
  if (likely(t != NULL)) {

    s_lock_global();
    {
      s_delete(t);
    }
    s_unlock_global();

  }
}





static inline void
s_set_cancel_and_destroy_task_no_lock(sqc_callout_task_t t) {
  if (likely(t != NULL)) {
    s_unschedule_timed_task_no_lock(t);
    (void)s_set_task_state_in_table(t, TASK_STATE_CANCELLED);
    s_destroy_task_no_lock(t);
  }
}


static inline void
s_set_cancel_and_destroy_task(sqc_callout_task_t t) {
  if (likely(t != NULL)) {

    s_lock_global();
    {
      s_unschedule_timed_task_no_lock(t);
      (void)s_set_task_state_in_table(t, TASK_STATE_CANCELLED);
      s_destroy_task_no_lock(t);
    }
    s_unlock_global();

  }
}





static inline void
s_cancel(const sqc_callout_task_t t) {
  if (likely(t != NULL)) {

    callout_task_state_t st = s_get_task_state_in_table(t);
    if (likely(st != TASK_STATE_UNKNOWN &&
               st != TASK_STATE_CANCELLED)) {

      if (__sync_fetch_and_add(&(t->m_exec_ref_count), 0) != 0) {

        /*
         * The task is executing at this moment.
         */

        /*
         * Increment a # of canceller to notify the task executioner
         * that there is a canceller.
         */
        (void)__sync_fetch_and_add(&(t->m_cancel_ref_count), 1);

        s_lock_task(t);
        {
          /*
           * Then wait for the execution finish of this task. Note
           * that we can wait that by using the CAS, but we prefer to
           * sleep with a cond.
           */
          while (__sync_fetch_and_add(&(t->m_exec_ref_count), 0) != 0) {
            s_wait_task(t);
          }
        }
        s_unlock_task(t);

        if (t->m_status == TASK_STATE_DELETING) {
          return;
        }
        if (__sync_sub_and_fetch(&(t->m_cancel_ref_count), 1) > 0) {
          /*
           * There are other threads which also want to cancel this task.
           * Let the last one do that.
           */
          return;
        } else {
          /*
           * The last one.
           */
          s_set_cancel_and_destroy_task_no_lock(t);
        }

      } else {

        if (st == TASK_STATE_ENQUEUED) {
          bool can_delete = false;

          s_lock_task(t);
          {
            if (t->m_is_in_timed_q == true) {
              /*
               * If it is in the timed task Q, it is deletable safely.
               */
              can_delete = true;
            } else {
              /*
               * Set cancel flag.
               */
              (void)s_set_task_state_in_table(t, TASK_STATE_CANCELLED);
              t->m_status = TASK_STATE_CANCELLED;
            }
          }
          s_unlock_task(t);

          if (can_delete == true) {
            s_set_cancel_and_destroy_task_no_lock(t);
          }

        } else if (st == TASK_STATE_DEQUEUED) {
          /*
           * The task is about to execute. Just set the cancel flag
           * and let the callout task worker/main scheduler delete
           * this.
           */

          s_lock_task(t);
          {
            (void)s_set_task_state_in_table(t, TASK_STATE_CANCELLED);
            t->m_status = TASK_STATE_CANCELLED;
          }
          s_unlock_task(t);

        }

      }
    }
  }
}


static inline void
s_cancel_task_no_lock(const sqc_callout_task_t t) {
  if (likely(t != NULL)) {
    s_cancel(t);
  }
}


static inline void
s_cancel_task(sqc_callout_task_t t) {
  if (likely(t != NULL)) {

    s_lock_global();
    {
      s_cancel(t);
    }
    s_unlock_global();

  }
}





static inline sqc_result_t
s_exec_task(sqc_callout_task_t t) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  sqc_msg_debug(4, "exec enter.\n");

  if (likely(t != NULL)) {
    bool do_delete = false;
    callout_task_state_t st = s_get_task_state_in_table(t);

    if (likely(st == TASK_STATE_DEQUEUED)) {

      /*
       * Check the t->m_exec_ref_count FIRST.
       */
      size_t is_execing = __sync_fetch_and_add(&(t->m_exec_ref_count), 1);

      if (likely(is_execing == 0)) {
        sqc_runnable_t r = (sqc_runnable_t)t;

        s_lock_task(t);
        {
          /*
           * The task could be cancelled while reaching here. Check
           * the outer task state again.
           */
          st = s_get_task_state_in_table(t);

          if (likely(st == TASK_STATE_DEQUEUED &&
                     t->m_status == TASK_STATE_DEQUEUED)) {

            /*
             * Change the state to TASK_STATE_EXECUTING.
             */
            (void)s_set_task_state_in_table(t, TASK_STATE_EXECUTING);
            t->m_status = TASK_STATE_EXECUTING;

            /*
             * Then execute the task.
             */
            t->m_is_first = false;
            ret = sqc_runnable_start(&r);

            if (unlikely(ret != SQC_RESULT_OK)) {
              /*
               * The execution failed.
               */
              s_set_task_state_in_table(t, TASK_STATE_EXEC_FAILED);
              t->m_status = TASK_STATE_EXEC_FAILED;
              sqc_perror(ret);
              sqc_msg_error_with_task(t,
                                         "task execution failure.\n");
              do_delete = true;
            }
          } else {
            if (likely(st == t->m_status)) {
              /*
               * The task state is changed to other than the
               * TASK_STATE_DEQUEUED. Handle this.
               */
              s_unlock_task(t);
              goto not_the_dequeued_state;
            } else {
              /*
               * Must not happens.
               */
              sqc_exit_fatal("Must not be here. A callout task state "
                                "mismatch occured.\n");
              /* not reached. */
            }
          }
        }
        s_unlock_task(t);

        /*
         * Note:
         *
         *	Very here any threads can acquire the task lock to
         *	cancel this task but no need to warry about it since
         *	the threads sleep by a cond wait until the
         *	t->m_exec_ref_count becomes zero.
         */

        if (likely(ret == SQC_RESULT_OK)) {
          /*
           * The execution succeeded.
           */
          if (__sync_fetch_and_add(&(t->m_cancel_ref_count), 0) == 0) {

            if (t->m_do_repeat == true) {
              /*
               * Use the global-locked version of the scheduler to make
               * the task submisson/fetch atomic. So we must not have
               * the task lock here, in order to keep the lock order,
               * which locks the global first then locks the task.
               */
              ret = s_schedule_timed_task(t);
              if (likely(ret >= 0)) {
                ret = SQC_RESULT_OK;
              } else {
                /*
                 * Make this task to be deleted.
                 */
                do_delete = true;
                sqc_perror(ret);
                sqc_msg_error_with_task(t,
                                           "can't re-schedule the task.\n");
              }

            } else {
              do_delete = true;
            }
          } else {
            do_delete = true;
          }
        } else {
          do_delete = true;
        }

      } else {	/* is_execing == 0 */
        /*
         * Do really nothing.
         */
        if (unlikely(is_execing > 10LL)) {
          /*
           * Atomic incr/decr problem??
           */
          sqc_msg_warning("the exec ref. count too large, " PFSZ(u) ".\n",
                             is_execing);
        }
        ret = SQC_RESULT_OK;
      }

      if (likely(do_delete == false)) {
        (void)__sync_fetch_and_sub(&(t->m_exec_ref_count), 1);
      } else {
        if (__sync_fetch_and_add(&(t->m_cancel_ref_count), 0) == 0) {

          (void)__sync_fetch_and_sub(&(t->m_exec_ref_count), 1);

          /*
           * Delete the task. Don't change the return value anymore.
           */

          s_lock_global();
          {
            s_set_cancel_and_destroy_task_no_lock(t);
          }
          s_unlock_global();

        } else {

          /*
           * Here are somebodies who want to cancel this task. Wake
           * them up and let one of them do that and we just leave
           * here now.
           */

          s_lock_task(t);
          {
            (void)__sync_fetch_and_sub(&(t->m_exec_ref_count), 1);
            s_wakeup_task(t);
          }
          s_unlock_task(t);

        }
      }

    } else {	/* st == TASK_STATE_DEQUEUED */
    not_the_dequeued_state:

      /*
       * Illegal state, the most likely already executing.
       */
      switch (st) {
        case TASK_STATE_EXECUTING: {
          /*
           * Do nothing.
           */
          ret = SQC_RESULT_OK;
          break;
        }
        case TASK_STATE_CANCELLED: {
          /*
           * Delayed deletion.
           */

          s_lock_global();
          {
            s_set_cancel_and_destroy_task_no_lock(t);
          }
          s_unlock_global();

          ret = SQC_RESULT_OK;
          break;
        }
        case TASK_STATE_UNKNOWN: {
          ret = SQC_RESULT_INVALID_OBJECT;
          break;
        }
        default: {
          ret = SQC_RESULT_INVALID_STATE_TRANSITION;
          sqc_msg_warning("About to execute a task with invalid state "
                             "%d.\n", (int)st);
          break;
        }
      }

    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  sqc_msg_debug(4, "exec leave.\n");

  return ret;
}





static inline sqc_result_t
s_submit_task(sqc_callout_task_t t,
              sqc_chrono_t initial_delay,
              sqc_chrono_t interval) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(t != NULL)) {

    sqc_callout_task_t tt = t;

    if (likely(interval >= CALLOUT_TASK_MIN_INTERVAL ||
               interval == 0LL)) {

      s_lock_global();
      {

        /*
         * Acquire the global lock to make the task submisson/fetch
         * atomic.
         */

        s_lock_task_q();
        {
          s_lock_task(t);
          {
            t->m_initial_delay_time = initial_delay;
            if (interval > 0LL) {
              t->m_interval_time = interval;
              t->m_do_repeat = true;
            }

            if (likely(initial_delay == 0LL)) {
              /*
               * An urgent task.
               */
              ret = sqc_bbq_put(&s_urgent_tsk_q, (void **)&tt,
                                   sqc_callout_task_t, -1LL);
            } else if (likely(initial_delay > 0LL)) {
              /*
               * A timed task.
               */
              if (likely(s_schedule_timed_task_no_lock(t) > 0)) {
                ret = SQC_RESULT_OK;
              } else {
                ret = SQC_RESULT_ANY_FAILURES;
              }
            } else {
              /*
               * An idle task.
               */
              ret = sqc_bbq_put(&s_idle_tsk_q, (void **)&tt,
                                   sqc_callout_task_t, -1LL);
            }

            if (ret == SQC_RESULT_OK && t->m_is_in_timed_q == false) {
              /*
               * For the tasks except the timed ones, set the task state
               * to TASK_STATE_ENQUEUED.
               */
              (void)s_set_task_state_in_table(t, TASK_STATE_ENQUEUED);
              t->m_status = TASK_STATE_ENQUEUED;
              t->m_is_in_bbq = true;
            }
          }
          s_unlock_task(t);
        }
        s_unlock_task_q();
      }
      s_unlock_global();

    } else {
      ret = SQC_RESULT_TOO_SMALL;
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_force_schedule_task(sqc_callout_task_t t) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  callout_task_state_t st = TASK_STATE_UNKNOWN;
  sqc_callout_task_t ta[1] = { t };
  sqc_chrono_t now;
  bool do_run = false;

  if (likely(t != NULL)) {

    /*
     * Acquire the global lock to make the task submisson/fetch
     * atomic.
     */

    s_lock_global();
    {
      st = s_get_task_state_in_table(t);
      if (likely(st == TASK_STATE_CREATED ||
                 st == TASK_STATE_ENQUEUED)) {
        if (likely(__sync_fetch_and_add(&(t->m_exec_ref_count), 0) == 0)) {

          s_lock_task(t);
          {
            if (t->m_is_in_timed_q == true) {
              s_unschedule_timed_task_no_lock(t);
              /*
               * Set the task state to TASK_STATE_DEQUEUED.
               */
              (void)s_set_task_state_in_table(t, TASK_STATE_DEQUEUED);
              t->m_status = TASK_STATE_DEQUEUED;
              do_run = true;
            } else {
              ret = SQC_RESULT_INVALID_STATE;
            }
          }
          s_unlock_task(t);

        } else {
          ret = SQC_RESULT_OK;
        }
      } else {
        ret = SQC_RESULT_INVALID_STATE;
      }
    }
    s_unlock_global();

    if (do_run == true) {
      WHAT_TIME_IS_IT_NOW_IN_NSEC(now);
      ret = (s_final_task_sched_proc)(ta, now, 1);
      if (likely(ret == 1)) {
        ret = SQC_RESULT_OK;
      }
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}
