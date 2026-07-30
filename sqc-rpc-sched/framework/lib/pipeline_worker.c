#ifndef __PIPLELINE_WORKER_C__
#define __PIPLELINE_WORKER_C__





/*
 * Worker implementaion.
 */


typedef sqc_result_t (*worker_main_proc_t)(sqc_pipeline_worker_t w);


typedef struct sqc_pipeline_worker_record {
  sqc_thread_record m_thd;  /* must be placed at the head. */
  sqc_pipeline_stage_t m_stg;
  /* ref. to the parent container. */
  size_t m_idx;			/* A worker index in the m_stg */
  worker_main_proc_t m_proc;
  bool m_is_started;

  uint8_t *m_buf;		/* A buffer for the batch, must be >=
                                 * m_stg->m_batch_buffer_size (in
                                 * bytes.) */
  sqc_pipeline_stage_event_buffer_freeup_proc_t m_freeup_proc;
} sqc_pipeline_worker_record;





static inline void	s_worker_pause(sqc_pipeline_worker_t w,
                                   sqc_pipeline_stage_t ps);
static inline void	s_worker_maintenance(sqc_pipeline_worker_t w,
    sqc_pipeline_stage_t ps);





#define WORKER_LOOP(OPS)                                                \
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;                   \
  if (w != NULL) {                                                      \
    sqc_pipeline_stage_t *sptr = &(w->m_stg);                       \
    if (sptr != NULL && *sptr != NULL) {                                \
      void *evbuf = (void *)(w->m_buf);                                 \
      size_t max_n_evs = (*sptr)->m_max_batch;                          \
      size_t idx = w->m_idx;                                            \
      sqc_result_t st = 0;                                          \
      while ((*sptr)->m_do_loop == true &&                              \
             ((st > 0) ||                                               \
              (st == 0 && (*sptr)->m_sg_lvl == SHUTDOWN_UNKNOWN))) {    \
        if ((*sptr)->m_pause_requested == false) {                      \
          { OPS }                                                       \
        } else {                                                        \
          ((*sptr)->m_maint_proc != NULL) ?                             \
          s_worker_maintenance(w, *sptr) :                          \
          s_worker_pause(w, *sptr);                                 \
        }                                                               \
      }                                                                 \
      if (((*sptr)->m_sg_lvl == SHUTDOWN_RIGHT_NOW ||                   \
           (*sptr)->m_sg_lvl == SHUTDOWN_GRACEFULLY) &&                 \
          st > 0) {                                                     \
        st = SQC_RESULT_OK;                                         \
      }                                                                 \
      ret = st;                                                         \
    } else {                                                            \
      ret = SQC_RESULT_INVALID_ARGS;                                \
    }                                                                   \
  } else {                                                              \
    ret = SQC_RESULT_INVALID_ARGS;                                  \
  }                                                                     \
  return ret;


/*
 * The st indicates how the main loop iterates:
 *
 *	st <  0:	... stop iteration immediately.
 *	st == 0:	... If the graceful shutdown is requested,
 *			    stop iterateion.
 *	st >  0:	... If the immediate shutdown is requested,
 *			    stop iterateion. Otherwise conrinues the
 *			    iteration.
 */


static sqc_result_t
s_worker_f_m_t(sqc_pipeline_worker_t w) {
  WORKER_LOOP
  (
    if ((st = ((*sptr)->m_fetch_proc)(sptr, idx, evbuf,
  max_n_evs)) > 0) {
  if ((st = ((*sptr)->m_main_proc)(sptr, idx, evbuf,
                                     (size_t)st)) > 0) {
      if ((st = ((*sptr)->m_throw_proc)(sptr, idx, evbuf,
                                        (size_t)st)) > 0) {
        continue;
      } else {
        if (st < 0) {
          break;
        }
      }
    } else {
      if (st < 0) {
        break;
      }
    }
  } else {
    if (st < 0) {
      break;
    }
  }
  )
}


static sqc_result_t
s_worker_f_m(sqc_pipeline_worker_t w) {
  WORKER_LOOP
  (
    if ((st = ((*sptr)->m_fetch_proc)(sptr, idx, evbuf,
  max_n_evs)) > 0) {
  if ((st = ((*sptr)->m_main_proc)(sptr, idx, evbuf,
                                     (size_t)st)) > 0) {
      continue;
    } else {
      if (st < 0) {
        break;
      }
    }
  } else {
    if (st < 0) {
      break;
    }
  }
  )
}


static sqc_result_t
s_worker_m_t(sqc_pipeline_worker_t w) {
  WORKER_LOOP
  (
    if ((st = ((*sptr)->m_main_proc)(sptr, idx, evbuf,
  max_n_evs)) > 0) {
  if ((st = ((*sptr)->m_throw_proc)(sptr, idx, evbuf,
                                      (size_t)st)) > 0) {
      continue;
    } else {
      if (st < 0) {
        break;
      }
    }
  } else {
    if (st < 0) {
      break;
    }
  }
  )
}


static sqc_result_t
s_worker_m(sqc_pipeline_worker_t w) {
  WORKER_LOOP
  (
    if ((st = ((*sptr)->m_main_proc)(sptr, idx, evbuf,
  max_n_evs)) > 0) {
  continue;
} else {
  if (st < 0) {
      break;
    }
  }
  )
}


static inline worker_main_proc_t
s_find_worker_proc(sqc_pipeline_stage_fetch_proc_t fetch_proc,
                   sqc_pipeline_stage_main_proc_t main_proc,
                   sqc_pipeline_stage_throw_proc_t throw_proc) {
  worker_main_proc_t ret = NULL;

  if (fetch_proc != NULL && main_proc != NULL && throw_proc != NULL) {
    ret = s_worker_f_m_t;
  } else if (fetch_proc != NULL && main_proc != NULL && throw_proc == NULL) {
    ret = s_worker_f_m;
  } else if (fetch_proc == NULL && main_proc != NULL && throw_proc != NULL) {
    ret = s_worker_m_t;
  } else if (fetch_proc == NULL && main_proc != NULL && throw_proc == NULL) {
    ret = s_worker_m;
  }

  return ret;
}


static sqc_result_t
s_worker_main(const sqc_thread_t *tptr, void *arg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  (void)arg;

  if (tptr != NULL) {
    sqc_pipeline_worker_t w = (sqc_pipeline_worker_t)*tptr;
    if (w != NULL) {
      if (w->m_proc != NULL) {
        global_state_t s;
        shutdown_grace_level_t l;

        /*
         * Wait for the gala opening.
         */
        sqc_msg_debug(10, "waiting for the gala opening...\n");
        ret = global_state_wait_for(GLOBAL_STATE_STARTED, &s, &l, -1LL);
        if (ret == SQC_RESULT_OK) {
          if (s == GLOBAL_STATE_STARTED) {
            sqc_pipeline_stage_t *sptr = &(w->m_stg);

            w->m_is_started = true;
            sqc_msg_debug(10, "gala opening.\n");

            if (sptr != NULL && (*sptr) != NULL &&
                (*sptr)->m_post_start_proc != NULL) {
              sqc_msg_debug(10, "call the post-start for " PFSZ(d) ".\n",
                               w->m_idx);
              ((*sptr)->m_post_start_proc)(sptr, w->m_idx,
                                           (*sptr)->m_post_start_arg);
            }

            /*
             * Do the main loop.
             */
            ret = (w->m_proc)(w);
          } else {
            if (IS_GLOBAL_STATE_KINDA_SHUTDOWN(s) == true) {
              ret = SQC_RESULT_NOT_OPERATIONAL;
            } else {
              sqc_exit_fatal("must not happen.\n");
            }
          }
        } else {
          /*
           * Means we are just about to be shutted down before the gala
           * opening.
           */
          sqc_perror(ret);
          if (ret == SQC_RESULT_NOT_OPERATIONAL) {
            if (IS_GLOBAL_STATE_KINDA_SHUTDOWN(s) == false) {
              sqc_exit_fatal("must not happen.\n");
            }
          }
        }
      } else {
        ret = SQC_RESULT_INVALID_ARGS;
      }
    } else {
      ret = SQC_RESULT_INVALID_ARGS;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static void
s_worker_finalize(const sqc_thread_t *tptr, bool is_canceled, void *arg) {
  (void)arg;

  if (tptr != NULL) {
    sqc_pipeline_worker_t w = (sqc_pipeline_worker_t)*tptr;
    if (w != NULL) {
      sqc_pipeline_stage_t *sptr = &(w->m_stg);

      if (sptr != NULL && *sptr != NULL) {

        s_final_lock_stage(*sptr);
        {
          if (is_canceled == true) {
            if (w->m_is_started == false) {
              /*
               * Means it could be canceled while waiting for the gala
               * opening.
               */
              global_state_cancel_janitor();
            }
            (*sptr)->m_n_canceled_workers++;

            s_pause_unlock_stage(*sptr);
          }
          (*sptr)->m_n_shutdown_workers++;
        }
        s_final_unlock_stage(*sptr);

      }
    }
  }
}


static void
s_worker_freeup(const sqc_thread_t *tptr, void *arg) {
  (void)arg;

  if (tptr != NULL) {
    sqc_pipeline_worker_t w = (sqc_pipeline_worker_t)*tptr;
    if (w != NULL) {
      /*
       * Note: there is nothing to do here for now. This exists only
       * for the future use.
       */
    }
  }
}


static inline sqc_result_t
s_worker_create(sqc_pipeline_worker_t *wptr,
                sqc_pipeline_stage_t *sptr,
                size_t idx,
                worker_main_proc_t proc) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (wptr != NULL &&
      sptr != NULL && *sptr != NULL &&
      IS_VALID_STRING((*sptr)->m_name) == true) {
    sqc_pipeline_worker_t w;
    uint8_t *evbuf = NULL;

    *wptr = NULL;
    /*
     * Allocate a worker.
     */
    w = (sqc_pipeline_worker_t)malloc(sizeof(*w));
    evbuf = (uint8_t *)malloc((*sptr)->m_batch_buffer_size);
    if (w != NULL && evbuf != NULL) {
      char buf[16];
      snprintf(buf, sizeof(buf), "%s:%d", (*sptr)->m_name, (int)idx);
      if ((ret = sqc_thread_create((sqc_thread_t *)&w,
                                      s_worker_main,
                                      s_worker_finalize,
                                      s_worker_freeup,
                                      buf,
                                      NULL)) == SQC_RESULT_OK) {
        w->m_stg = *sptr;
        w->m_idx = idx;
        w->m_proc = proc;
        w->m_is_started = false;
        w->m_buf = evbuf;
        w->m_freeup_proc = free;
        (void)memset((void *)(w->m_buf), 0, (*sptr)->m_batch_buffer_size);
        /*
         * Make the object destroyable via sqc_thread_destroy() so
         * we don't have to worry about not to provide our own worker
         * destructor.
         */
        (void)sqc_thread_free_when_destroy((sqc_thread_t *)&w);
        *wptr = w;
      }
    } else {
      ret = SQC_RESULT_NO_MEMORY;
    }

    if (unlikely(ret != SQC_RESULT_OK)) {
      free((void *)w);
      free((void *)evbuf);
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_worker_start(sqc_pipeline_worker_t *wptr) {
  return sqc_thread_start((sqc_thread_t *)wptr, false);
}


static inline sqc_result_t
s_worker_cancel(sqc_pipeline_worker_t *wptr) {
  return sqc_thread_cancel((sqc_thread_t *)wptr);
}


static inline sqc_result_t
s_worker_wait(sqc_pipeline_worker_t *wptr, sqc_chrono_t nsec) {
  return sqc_thread_wait((sqc_thread_t *)wptr, nsec);
}


static inline void
s_worker_destroy(sqc_pipeline_worker_t *wptr) {
  if (wptr != NULL &&
      (*wptr)->m_buf != NULL && (*wptr)->m_freeup_proc != NULL) {
    ((*wptr)->m_freeup_proc)((*wptr)->m_buf);
  }
  sqc_thread_destroy((sqc_thread_t *)wptr);
}


#ifdef SQC_OS_LINUX
static inline sqc_result_t
s_worker_set_cpu_affinity(sqc_pipeline_worker_t *wptr, int cpu) {
  return sqc_thread_set_cpu_affinity((sqc_thread_t *)wptr, cpu);
}
#endif /* SQC_OS_LINUX */


static inline void *
s_worker_get_buffer(sqc_pipeline_worker_t *wptr) {
  void *ret = NULL;

  if (wptr != NULL && *wptr != NULL) {
    ret = (void *)((*wptr)->m_buf);
  }

  return ret;
}


static inline void *
s_worker_set_buffer(sqc_pipeline_worker_t *wptr,
                    void *buf,
                    sqc_pipeline_stage_event_buffer_freeup_proc_t
                    freeup_proc) {
  void *ret = NULL;

  if (wptr != NULL && *wptr != NULL && buf != NULL) {
    ret = (*wptr)->m_buf;
    if ((*wptr)->m_buf != NULL && (*wptr)->m_freeup_proc != NULL) {
      ((*wptr)->m_freeup_proc)((*wptr)->m_buf);
    }
    (*wptr)->m_buf = buf;
    (*wptr)->m_freeup_proc = freeup_proc;
  }

  return ret;
}





static inline void
s_worker_pause(sqc_pipeline_worker_t w,
               sqc_pipeline_stage_t ps) {
  if (w != NULL && ps != NULL) {
    sqc_result_t st;
    bool is_master = false;

    /*
     * Note that the master stage lock (ps->m_lock) is locked by the
     * pauser at this moment.
     */
    if ((st = sqc_barrier_wait(&(ps->m_pause_barrier), &is_master)) ==
        SQC_RESULT_OK) {

      /*
       * The barrier synchronization done. All the workers are very
       * here now.
       */

      s_pause_lock_stage(ps);
      {
        if (is_master == true) {
          ps->m_status = STAGE_STATE_PAUSED;
          (void)s_pause_notify_stage(ps);
        }

      recheck:
        if (ps->m_pause_requested == true) {
          st = s_resume_cond_wait_stage(ps, -1LL);
          if (st == SQC_RESULT_OK) {
            goto recheck;
          } else {
            sqc_perror(st);
            sqc_msg_error("a worker resume error.\n");
          }
        }
      }
      s_pause_unlock_stage(ps);

    } else {
      sqc_perror(st);
      sqc_msg_error("a worker barrier error.\n");
    }
  }
}


static inline void
s_worker_maintenance(sqc_pipeline_worker_t w,
                     sqc_pipeline_stage_t ps) {
  if (w != NULL && ps != NULL && ps->m_maint_proc != NULL) {
    sqc_result_t st;
    bool is_master = false;

    /*
     * Note that the master stage lock (ps->m_lock) is locked by the
     * pauser at this moment.
     */
    if ((st = sqc_barrier_wait(&(ps->m_pause_barrier), &is_master)) ==
        SQC_RESULT_OK) {

      /*
       * The barrier synchronization done. All the workers are very
       * here now.
       */

      s_pause_lock_stage(ps);
      {

        /*
         * Note that this lock must be done since all the threads
         * re-enter the barrier if not.
         */

        if (is_master == true) {
          sqc_pipeline_stage_t ps2 = ps;

          (ps->m_maint_proc)(&ps2, ps->m_maint_arg);
          ps->m_maint_proc = NULL;
          ps->m_maint_arg = NULL;

          ps->m_pause_requested = false;
          ps->m_status = STAGE_STATE_STARTED;

          (void)s_pause_notify_stage(ps);
        } else {
        recheck:
          if (ps->m_status != STAGE_STATE_STARTED) {
            (void)s_pause_cond_wait_stage(ps, -1LL);
            goto recheck;
          }
        }

      }
      s_pause_unlock_stage(ps);

    }  else {
      sqc_perror(st);
      sqc_msg_error("a worker barrier error.\n");
    }
  }
}





#endif /* __PIPLELINE_WORKER_C__ */
