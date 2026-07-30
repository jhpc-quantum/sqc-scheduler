#include "sqc_apis.h"
#include "sqc_callout_internal.h"
#include "sqc_pipeline_stage_internal.h"





#define DEFAULT_TASK_ALLOC_SZ	(sizeof(sqc_callout_task_record))

#define CALLOUT_TASK_MAX	1000

#define CALLOUT_TASK_SCHED_JITTER	1000LL	/* 1 usec. */

#define CALLOUT_TASK_SCHED_DELAY_COMPENSATION	50LL * 1000LL	/* 50 usec. */

#define CALLOUT_TASK_MIN_INTERVAL	10LL * 1000LL /* 10 usec. */

#define CALLOUT_IDLE_PROC_MIN_INTERVAL	1000LL * 1000LL /* 1 msec. */

#define CALLOUT_STAGE_SHUTDOWN_TIMEOUT	5LL * 1000LL * 1000LL * 1000LL
/* 5 sec. */

#define sqc_msg_error_with_task(t, str, ...) {                      \
    do {                                                                \
      if (IS_VALID_STRING((t)->m_name) == true) {                       \
        sqc_msg_error("%s: " str, (t)->m_name, ##__VA_ARGS__);      \
      } else {                                                          \
        sqc_msg_error("%p: " str, (void *)(t), ##__VA_ARGS__);      \
      }                                                                 \
    } while (0);                                                        \
  }





TAILQ_HEAD(chrono_task_queue_t, sqc_callout_task_record);
typedef struct chrono_task_queue_t chrono_task_queue_t;


typedef sqc_result_t
(*final_task_schedule_proc_t)(const sqc_callout_task_t *const tasks,
                              sqc_chrono_t start_time,
                              size_t n);





static pthread_once_t s_once = PTHREAD_ONCE_INIT;
static bool s_is_inited = false;

static sqc_mutex_t s_sched_lck = NULL;	/* The scheduler main lock. */
static sqc_cond_t s_sched_cnd = NULL;		/* The scheduler cond. */
static sqc_mutex_t s_lck = NULL;		/* The global lock. */

static sqc_hashmap_t s_tsk_tbl = NULL;	/* The valid tasks table. */

static sqc_bbq_t s_urgent_tsk_q = NULL;	/* The urgent tasks Q. */
static chrono_task_queue_t s_chrono_tsk_q;	/* The timed tasks Q. */
static sqc_mutex_t s_q_lck;			/* The timed task Q lock. */
static sqc_bbq_t s_idle_tsk_q = NULL;	/* The Idle taskss Q. */

static sqc_callout_idle_proc_t s_idle_proc = NULL;
static sqc_callout_idle_arg_freeup_proc_t s_free_proc = NULL;
static void *s_idle_proc_arg = NULL;
static sqc_chrono_t s_idle_interval = -1LL;
static sqc_chrono_t s_next_idle_abstime = -1LL;

static size_t s_n_workers;
static volatile bool s_do_loop = false;		/* The main loop on/off */
static volatile bool s_is_stopped = false;	/* The main loop is
                                                 * stopped or not. */

static volatile bool s_is_handler_inited = false;


static final_task_schedule_proc_t s_final_task_sched_proc = NULL;





static void s_ctors(void) __attr_constructor__(107);
static void s_dtors(void) __attr_destructor__(107);


/**
 * General lock order:
 *
 *	The global lock (s_lock_global)
 *	The timed task queue lock (s_lock_task_q)
 *	A task lock (s_lock_task)
 *
 * Note taht the task locks are recursive.
 */


static void s_lock_global(void);
static void s_unlock_global(void);

static void s_lock_task_q(void);
static void s_unlock_task_q(void);

static void s_lock_task(sqc_callout_task_t t);
static void s_unlock_task(sqc_callout_task_t t);

static void s_wait_task(sqc_callout_task_t t);
static void s_wakeup_task(sqc_callout_task_t t);

static callout_task_state_t s_set_task_state_in_table(
  const sqc_callout_task_t t,
  callout_task_state_t s);
static callout_task_state_t s_get_task_state_in_table(
  const sqc_callout_task_t t);
static inline void s_delete_task_in_table(const sqc_callout_task_t t);

/**
 * _no_lock() version of the APIs need to be between
 * s_lock_global()/s_unlock_global()
 */
static sqc_result_t
s_create_task(sqc_callout_task_t *tptr,
              size_t sz,
              const char *name,
              sqc_callout_task_proc_t proc,
              void *arg,
              sqc_callout_task_arg_freeup_proc_t freeproc);

static void s_destroy_task_no_lock(sqc_callout_task_t);
static void s_destroy_task(sqc_callout_task_t);

static void s_set_cancel_and_destroy_task_no_lock(sqc_callout_task_t t);
static void s_set_cancel_and_destroy_task(sqc_callout_task_t t);

static inline void s_cancel_task_no_lock(const sqc_callout_task_t t);
static inline void s_cancel_task(const sqc_callout_task_t t);

static sqc_result_t s_schedule_timed_task_no_lock(
  sqc_callout_task_t t);
static sqc_result_t s_schedule_timed_task(sqc_callout_task_t t);
static void s_unschedule_timed_task_no_lock(sqc_callout_task_t t);
static void s_unschedule_timed_task(sqc_callout_task_t t);

static sqc_result_t s_exec_task(sqc_callout_task_t t);

static sqc_result_t s_submit_callout_stage(
  const sqc_callout_task_t *const tasks,
  sqc_chrono_t start_time,
  size_t n);

static void s_task_freeup(void **valptr);





#include "callout_task.c"
#include "callout_stage.c"
#include "callout_queue.c"





static void
s_task_freeup(void **valptr) {
  if (likely(valptr != NULL && *valptr != NULL)) {

    s_lock_global();
    {
      s_set_cancel_and_destroy_task_no_lock((sqc_callout_task_t)*valptr);
    }
    s_unlock_global();

  }
}


static void
s_once_proc(void) {
  sqc_result_t r;

  if ((r = sqc_mutex_create(&s_sched_lck)) != SQC_RESULT_OK) {
    sqc_perror(r);
    sqc_exit_fatal("can't initialize the callout scheduler main mutex.\n");
  }

  if ((r = sqc_mutex_create(&s_lck)) != SQC_RESULT_OK) {
    sqc_perror(r);
    sqc_exit_fatal("can't initialize the callout global mutex.\n");
  }

  if ((r = sqc_cond_create(&s_sched_cnd)) != SQC_RESULT_OK) {
    sqc_perror(r);
    sqc_exit_fatal("can't initialize the callout cond.\n");
  }

  if ((r = sqc_hashmap_create(&s_tsk_tbl,
                                 SQC_HASHMAP_TYPE_ONE_WORD,
                                 NULL)) != SQC_RESULT_OK) {
    sqc_perror(r);
    sqc_exit_fatal("can't initialize the callout table.\n");
  }

  if ((r = sqc_bbq_create(&s_urgent_tsk_q, sqc_callout_task_t,
                             CALLOUT_TASK_MAX, s_task_freeup)) !=
      SQC_RESULT_OK) {
    sqc_perror(r);
    sqc_exit_fatal("can't initialize the callout urgent tasks queue.\n");
  }

  if ((r = sqc_bbq_create(&s_idle_tsk_q, sqc_callout_task_t,
                             CALLOUT_TASK_MAX, s_task_freeup)) !=
      SQC_RESULT_OK) {
    sqc_perror(r);
    sqc_exit_fatal("can't initialize the callout idle tasks queue.\n");
  }

  if ((r = sqc_mutex_create_recursive(&s_q_lck)) != SQC_RESULT_OK) {
    sqc_perror(r);
    sqc_exit_fatal("can't initialize the callout task queue mutex.\n");
  }

  TAILQ_INIT(&s_chrono_tsk_q);

  s_is_inited = true;
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();

  sqc_msg_debug(10, "The callout module is initialized.\n");
}


static inline void
s_final(void) {
  sqc_bbq_destroy(&s_idle_tsk_q, true);
  sqc_bbq_destroy(&s_urgent_tsk_q, true);
  sqc_hashmap_destroy(&s_tsk_tbl, true);
  sqc_cond_destroy(&s_sched_cnd);
  sqc_mutex_destroy(&s_lck);
  sqc_mutex_destroy(&s_sched_lck);
}


static void
s_dtors(void) {
  if (s_is_inited == true) {
    if (sqc_module_is_unloading() &&
        sqc_module_is_finalized_cleanly()) {

      s_final();

      sqc_msg_debug(10, "The callout module is finalized.\n");
    } else {
      sqc_msg_debug(10, "The callout module is not finalized because of "
                       "module finalization problem.\n");
    }
  }
}





static inline void
s_lock_global(void) {
  if (likely(s_lck != NULL)) {
    (void)sqc_mutex_lock(&s_lck);
  }
}


static inline void
s_unlock_global(void) {
  if (likely(s_lck != NULL)) {
    (void)sqc_mutex_unlock(&s_lck);
  }
}


static inline void
s_lock_sched(void) {
  if (likely(s_sched_lck != NULL)) {
    (void)sqc_mutex_lock(&s_sched_lck);
  }
}


static inline void
s_unlock_sched(void) {
  if (likely(s_sched_lck != NULL)) {
    (void)sqc_mutex_unlock(&s_sched_lck);
  }
}


static inline void
s_wait_sched(sqc_chrono_t to) {
  if (likely(s_sched_lck != NULL && s_sched_cnd != NULL)) {
    (void)sqc_cond_wait(&s_sched_cnd, &s_sched_lck, to);
  }
}


static inline void
s_wakeup_sched(void) {
  if (likely(s_sched_cnd != NULL)) {
    (void)sqc_cond_notify(&s_sched_cnd, true);
  }
}





static inline sqc_result_t
s_run_tasks_by_self(const sqc_callout_task_t *const tasks,
                    sqc_chrono_t start_time,
                    size_t n) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(tasks != NULL && n > 0)) {
    sqc_callout_task_t t;
    size_t i;

    for (i = 0; i < n; i++) {
      /*
       * Change the state of tasks to TASK_STATE_DEQUEUED and set
       * the last execution (start) time as now.
       */
      t = tasks[i];

      s_lock_task(t);
      {
        if (t->m_status == TASK_STATE_ENQUEUED ||
            t->m_status == TASK_STATE_CREATED) {
          (void)s_set_task_state_in_table(t, TASK_STATE_DEQUEUED);
          t->m_status = TASK_STATE_DEQUEUED;
        }
        t->m_last_abstime = start_time;
      }
      s_unlock_task(t);

      (void)s_exec_task(t);
    }

    ret = (sqc_result_t)n;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline sqc_result_t
s_start_callout_main_loop(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  global_state_t s;
  shutdown_grace_level_t l;

  ret = global_state_wait_for(GLOBAL_STATE_STARTED, &s, &l, -1LL);
  if (likely(ret == SQC_RESULT_OK)) {
    if (likely(s == GLOBAL_STATE_STARTED)) {
#ifdef CO_MSG_DEBUG
      sqc_chrono_t timeout = s_idle_interval;
#else
      sqc_chrono_t timeout;
#endif /* CO_MSG_DEBUG */

      sqc_callout_task_t out_tasks[CALLOUT_TASK_MAX * 3];
      size_t n_out_tasks;

      sqc_callout_task_t urgent_tasks[CALLOUT_TASK_MAX];
      sqc_result_t sn_urgent_tasks;

      sqc_callout_task_t idle_tasks[CALLOUT_TASK_MAX];
      sqc_result_t sn_idle_tasks;

      sqc_callout_task_t timed_tasks[CALLOUT_TASK_MAX];
      sqc_result_t sn_timed_tasks;

      sqc_result_t r;

      sqc_chrono_t now;
      sqc_chrono_t next_wakeup;
#if 0
      sqc_chrono_t prev_wakeup;
#endif

      int cstate = 0;

#if 0
      WHAT_TIME_IS_IT_NOW_IN_NSEC(prev_wakeup);
#endif
      (void)sqc_mutex_enter_critical(&s_sched_lck, &cstate);
      {
        s_is_stopped = false;
        mbar();

        while (s_do_loop == true) {

          n_out_tasks = 0;

          /*
           * Get the current time.
           */
          WHAT_TIME_IS_IT_NOW_IN_NSEC(now);

#ifdef CO_MSG_DEBUG
          sqc_msg_debug(3, "now:  " PF64(d) "\n", now);
#if 0
          sqc_msg_debug(3, "prv:  " PF64(d) "\n", prev_wakeup);
#endif
          sqc_msg_debug(3, "to:   " PF64(d) "\n", timeout);
#endif /* CO_MSG_DEBUG */

          s_lock_global();
          {

            /*
             * Acquire the global lock to make the task
             * submisson/fetch atomic.
             */

            sn_urgent_tasks =
              sqc_bbq_get_n(&s_urgent_tsk_q, (void **)urgent_tasks,
                               CALLOUT_TASK_MAX, 1LL,
                               sqc_callout_task_t,
                               0LL, NULL);
            sn_idle_tasks =
              sqc_bbq_get_n(&s_idle_tsk_q, (void **)idle_tasks,
                               CALLOUT_TASK_MAX, 1LL,
                               sqc_callout_task_t,
                               0LL, NULL);

          }
          s_unlock_global();

          /*
           * Pack the tasks into a buffer.
           */

          sn_timed_tasks = s_get_runnable_timed_task(now, timed_tasks,
                           CALLOUT_TASK_MAX,
                           &next_wakeup);
          if (sn_timed_tasks > 0) {
            /*
             * Pack the timed tasks.
             */
            (void)memcpy((void *)(out_tasks + n_out_tasks),
                         timed_tasks,
                         (size_t)(sn_timed_tasks) *
                         sizeof(sqc_callout_task_t));
            n_out_tasks += (size_t)sn_timed_tasks;

#ifdef CO_MSG_DEBUG
            sqc_msg_debug(3, "timed task " PF64(u) ".\n",
                             sn_timed_tasks);
            sqc_msg_debug(3, "nw:   " PF64(d) ".\n",
                             next_wakeup);
#endif /* CO_MSG_DEBUG */

          } else if (sn_timed_tasks < 0) {
            /*
             * We can't be treat this as a fatal error. Carry on.
             */
            sqc_perror(sn_timed_tasks);
            sqc_msg_error("timed tasks fetch failed.\n");
          }

          if (sn_urgent_tasks > 0) {
            /*
             * Pack the urgent tasks.
             */
            (void)memcpy((void *)(out_tasks + n_out_tasks),
                         urgent_tasks,
                         (size_t)(sn_urgent_tasks) *
                         sizeof(sqc_callout_task_t));
            n_out_tasks += (size_t)sn_urgent_tasks;
          } else if (sn_urgent_tasks < 0) {
            /*
             * We can't be treat this as a fatal error. Carry on.
             */
            sqc_perror(sn_urgent_tasks);
            sqc_msg_error("urgent tasks fetch failed.\n");
          }

          if (sn_idle_tasks > 0) {
            /*
             * Pack the idle tasks.
             */
            (void)memcpy((void *)(out_tasks + n_out_tasks),
                         idle_tasks,
                         (size_t)(sn_idle_tasks) *
                         sizeof(sqc_callout_task_t));
            n_out_tasks += (size_t)sn_idle_tasks;
          } else if (sn_idle_tasks < 0) {
            /*
             * We can't be treat this as a fatal error. Carry on.
             */
            sqc_perror(sn_idle_tasks);
            sqc_msg_error("idle tasks fetch failed.\n");
          }

          if (n_out_tasks > 0) {
            /*
             * Run/Submit the tasks.
             */
            r = (s_final_task_sched_proc)(out_tasks, now, n_out_tasks);
            if (unlikely(r <= 0)) {
              /*
               * We can't be treat this as a fatal error. Carry on.
               */
              sqc_perror(r);
              sqc_msg_error("failed to submit " PFSZ(u)
                               " urgent/timed tasks.\n", n_out_tasks);
            }
          }

          if (s_idle_proc != NULL &&
              s_next_idle_abstime < (now + CALLOUT_TASK_SCHED_JITTER)) {
            if (likely(s_idle_proc(s_idle_proc_arg) ==
                       SQC_RESULT_OK)) {
              s_next_idle_abstime = now + s_idle_interval;
            } else {
              /*
               * Stop the main loop and return (clean finish.)
               */
              s_do_loop = false;
              goto critical_end;
            }
          }

          /*
           * fetch the start time of the timed task in the queue head.
           */
          next_wakeup = s_peek_current_wakeup_time();
          if (next_wakeup <= 0LL) {
            /*
             * Nothing in the timed Q.
             */
            if (s_next_idle_abstime <= 0LL) {
              s_next_idle_abstime = now + s_idle_interval;
            }
            next_wakeup = s_next_idle_abstime;
          }

          /*
           * TODO
           *
           *	Re-optimize forcible waje up by timed task submission
           *	timing and times. See also
           *	callout_queue.c:s_do_sched().
           */

          /*
           * calculate the timeout and sleep.
           */
          timeout = next_wakeup - now;
          if (likely(timeout > 0LL)) {
            if (timeout > s_idle_interval) {
              timeout = s_idle_interval;
              next_wakeup = now + timeout;
            }

#ifdef CO_MSG_DEBUG
            sqc_msg_debug(4,
                             "about to sleep, timeout " PF64(d) " nsec.\n",
                             timeout);
#endif /* CO_MSG_DEBUG */

#if 0
            prev_wakeup = next_wakeup;
#endif

            r = sqc_bbq_wait_gettable(&s_urgent_tsk_q, timeout);
            if (unlikely(r <= 0 &&
                         r != SQC_RESULT_TIMEDOUT &&
                         r != SQC_RESULT_WAKEUP_REQUESTED)) {
              sqc_perror(r);
              sqc_msg_error("Event wait failure.\n");
              ret = r;
              goto critical_end;
            } else {
              if (r == SQC_RESULT_WAKEUP_REQUESTED) {

#ifdef CO_MSG_DEBUG
                sqc_msg_debug(4, "woke up.\n");
#endif /* CO_MSG_DEBUG */

              }
            }
          } else {
            WHAT_TIME_IS_IT_NOW_IN_NSEC(next_wakeup);

#if 0
            prev_wakeup = next_wakeup;
#endif

#ifdef CO_MSG_DEBUG
            sqc_msg_debug(4, "timeout zero. contiune.\n");
#endif /* CO_MSG_DEBUG */

          }

          /*
           * The end of the desired potion of the loop.
           */

        } /* while (s_do_loop == true) */

      }
    critical_end:
      s_is_stopped = true;
      s_wakeup_sched();
      (void)sqc_mutex_leave_critical(&s_sched_lck, cstate);

      if (s_do_loop == false) {
        /*
         * The clean finish.
         */
        ret = SQC_RESULT_OK;
      }

    } else { /* s == GLOBAL_STATE_STARTED */
      s_is_stopped = true;
      ret = SQC_RESULT_INVALID_STATE_TRANSITION;
    }
  } else {
    s_is_stopped = true;
  }

  return ret;
}


static inline sqc_result_t
s_stop_callout_main_loop(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (s_is_stopped == false) {
    if (s_do_loop == true) {
      /*
       * Stop the main loop first.
       */
      s_do_loop = false;
      mbar();
      (void)sqc_bbq_wakeup(&s_urgent_tsk_q, -1LL);

      s_lock_sched();
      {
        while (s_is_stopped == false) {
          s_wait_sched(-1LL);
        }
      }
      s_unlock_sched();
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_OK;
    }
  } else {
    ret = SQC_RESULT_OK;
  }

  return ret;
}





/*
 * Exported APIs
 */


sqc_result_t
sqc_callout_initialize_handler(size_t n_workers,
                                  sqc_callout_idle_proc_t proc,
                                  void *arg,
                                  sqc_chrono_t interval,
                                  sqc_callout_idle_arg_freeup_proc_t
                                  freeup) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(((proc == NULL) ? true :
              (interval > CALLOUT_IDLE_PROC_MIN_INTERVAL)))) {

    if (likely(n_workers > 0)) {
      if (unlikely((ret = s_create_callout_stage(n_workers)) !=
                   SQC_RESULT_OK)) {
        goto done;
      }
    }

    s_n_workers = n_workers;
    s_final_task_sched_proc =
      (s_n_workers == 0) ? s_run_tasks_by_self : s_submit_callout_stage;
    s_idle_proc = proc;
    s_idle_proc_arg = arg;
    s_idle_interval = interval;
    s_free_proc = freeup;
    s_do_loop = false;
    s_is_stopped = false;

    s_is_handler_inited = true;

    ret = SQC_RESULT_OK;

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}


void
sqc_callout_finalize_handler(void) {
  if (s_is_handler_inited == true) {
    sqc_result_t r = SQC_RESULT_ANY_FAILURES;

    /*
     * Stop the main loop first then shutdown/wait/destroy the callout
     * stage/workers.
     */
    if (likely((r = s_stop_callout_main_loop()) == SQC_RESULT_OK)) {
      if (likely(s_n_workers > 0)) {
        r = s_finish_callout_stage(CALLOUT_STAGE_SHUTDOWN_TIMEOUT);
        if (unlikely(r != SQC_RESULT_OK)) {
          sqc_perror(r);
          sqc_msg_warning("failed to stop the callout stage so the "
                             "callout queues still remain.\n");
          goto done;
        }
      }

      s_destroy_all_queued_timed_tasks();
      (void)sqc_bbq_clear(&s_urgent_tsk_q, true);
      (void)sqc_bbq_clear(&s_idle_tsk_q, true);

    } else {
      sqc_perror(r);
      sqc_msg_warning("failed to stop the callout scheduler main loop.\n");
    }

  done:
    s_is_handler_inited = false;
  }
}


sqc_result_t
sqc_callout_start_main_loop(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(s_is_handler_inited == true)) {
    if (s_n_workers > 0) {
      ret = s_start_callout_stage();
      if (likely(ret == SQC_RESULT_OK)) {
        s_do_loop = true;
        mbar();

        ret = s_start_callout_main_loop();
      }
    } else {
      s_do_loop = true;
      mbar();

      ret = s_start_callout_main_loop();
    }
  } else {
    ret = SQC_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


sqc_result_t
sqc_callout_stop_main_loop(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(s_is_handler_inited == true)) {
    ret = s_stop_callout_main_loop();
  } else {
    ret = SQC_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}





/*
 * Task APIs
 */


sqc_result_t
sqc_callout_create_task(sqc_callout_task_t *tptr,
                           size_t sz,
                           const char *name,
                           sqc_callout_task_proc_t proc,
                           void *arg,
                           sqc_callout_task_arg_freeup_proc_t freeproc) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(s_is_handler_inited == true)) {
    if (likely(tptr != NULL)) {
      ret = s_create_task(tptr, sz, name, proc, arg, freeproc);
    } else {
      ret = SQC_RESULT_INVALID_ARGS;
    }
  } else {
    ret = SQC_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


sqc_result_t
sqc_callout_submit_task(const sqc_callout_task_t *tptr,
                           sqc_chrono_t delay,
                           sqc_chrono_t interval) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(s_is_handler_inited == true)) {
    if (likely(tptr != NULL && *tptr != NULL)) {
      callout_task_state_t st;
      if (likely((st = s_get_task_state_in_table(*tptr)) ==
                 TASK_STATE_CREATED)) {
        ret = s_submit_task(*tptr, delay, interval);
      } else {
        if (st == TASK_STATE_UNKNOWN) {
          ret = SQC_RESULT_INVALID_OBJECT;
        } else {
          ret = SQC_RESULT_INVALID_STATE_TRANSITION;
        }
      }
    } else {
      ret = SQC_RESULT_INVALID_ARGS;
    }
  } else {
    ret = SQC_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


void
sqc_callout_cancel_task(const sqc_callout_task_t *tptr) {
  if (likely(s_is_handler_inited == true)) {
    if (likely(tptr != NULL && *tptr != NULL)) {
#if 0
      s_cancel_task(*tptr);
#else
      /*
       * FIXME:
       *
       * I think we need the global lock here, but It causes a deadlock.
       * Use the no lock version of the cancel API.
       */
      s_cancel_task_no_lock(*tptr);
#endif
    }
  }
}


sqc_result_t
sqc_callout_exec_task_forcibly(const sqc_callout_task_t *tptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(s_is_handler_inited == true)) {
    if (likely(tptr != NULL && *tptr != NULL)) {
      callout_task_state_t st;
      if (likely((st = s_get_task_state_in_table(*tptr)) !=
                 TASK_STATE_UNKNOWN)) {
        ret = s_force_schedule_task(*tptr);
      } else {
        ret = SQC_RESULT_INVALID_OBJECT;
      }
    } else {
      ret = SQC_RESULT_INVALID_ARGS;
    }
  } else {
    ret = SQC_RESULT_NOT_OPERATIONAL;
  }

  return ret;
}


sqc_result_t
sqc_callout_task_reset_interval(sqc_callout_task_t *tptr,
                                   sqc_chrono_t interval) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(tptr != NULL && *tptr != NULL)) {
    if (likely(interval >= CALLOUT_TASK_MIN_INTERVAL)) {
      sqc_callout_task_t t = *tptr;

      s_lock_task(t);
      {
        if (likely(t->m_status == TASK_STATE_EXECUTING)) {
          t->m_interval_time = interval;
          ret = SQC_RESULT_OK;
        } else {
          ret = SQC_RESULT_INVALID_STATE_TRANSITION;
        }
      }
      s_unlock_task(t);

    } else {
      ret = SQC_RESULT_TOO_SMALL;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_callout_task_state(sqc_callout_task_t *tptr,
                          sqc_callout_task_state_t *sptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(tptr != NULL && *tptr != NULL &&
             sptr != NULL)) {
    *sptr = s_get_task_state_in_table(*tptr);
    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}
