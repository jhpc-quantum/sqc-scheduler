#include "sqc_apis.h"
#include "sqc_thread_internal.h"
#include "sqc_pipeline_stage_internal.h"





#define DEFAULT_STAGE_ALLOC_SZ	(sizeof(sqc_pipeline_stage_record))





static pthread_once_t s_once = PTHREAD_ONCE_INIT;
static bool s_is_inited = false;
static sqc_hashmap_t s_ps_name_tbl;
static sqc_hashmap_t s_ps_obj_tbl;
static void	s_ctors(void) __attr_constructor__(110);
static void	s_dtors(void) __attr_destructor__(110);

static inline void	s_lock_stage(sqc_pipeline_stage_t ps);
static inline void	s_unlock_stage(sqc_pipeline_stage_t ps);
static inline void	s_final_lock_stage(sqc_pipeline_stage_t ps);
static inline void	s_final_unlock_stage(sqc_pipeline_stage_t ps);
static inline void	s_pause_lock_stage(sqc_pipeline_stage_t ps);
static inline void	s_pause_unlock_stage(sqc_pipeline_stage_t ps);
static inline void	s_pause_notify_stage(sqc_pipeline_stage_t ps);
static inline sqc_result_t
s_pause_cond_wait_stage(sqc_pipeline_stage_t ps,
                        sqc_chrono_t nsec);
static inline void	s_resume_notify_stage(sqc_pipeline_stage_t ps);
static inline sqc_result_t
s_resume_cond_wait_stage(sqc_pipeline_stage_t ps,
                         sqc_chrono_t nsec);





#include "pipeline_worker.c"





static void
s_child_at_fork(void) {
  sqc_hashmap_atfork_child(&s_ps_name_tbl);
  sqc_hashmap_atfork_child(&s_ps_obj_tbl);
}


static void
s_once_proc(void) {
  sqc_result_t r;

  if ((r = sqc_hashmap_create(&s_ps_name_tbl, SQC_HASHMAP_TYPE_STRING,
                                 NULL)) != SQC_RESULT_OK) {
    sqc_perror(r);
    sqc_exit_fatal("can't initialize the name - pipeline stage table.\n");
  }

  if ((r = sqc_hashmap_create(&s_ps_obj_tbl, SQC_HASHMAP_TYPE_ONE_WORD,
                                 NULL)) != SQC_RESULT_OK) {
    sqc_perror(r);
    sqc_exit_fatal("can't initialize the pipeline stage table.\n");
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

  sqc_msg_debug(10, "The pipeline stage module is initialized.\n");
}


static void
s_final(void) {
  sqc_hashmap_destroy(&s_ps_name_tbl, true);
  sqc_hashmap_destroy(&s_ps_obj_tbl, true);
}


static void
s_dtors(void) {
  if (s_is_inited == true) {
    if (sqc_module_is_unloading() &&
        sqc_module_is_finalized_cleanly()) {
      s_final();

      sqc_msg_debug(10, "The pipeline stage module is finalized.\n");
    } else {
      sqc_msg_debug(10, "The pipeline stage module is not finalized "
                       "because of module finalization problem.\n");
    }
  }
}





static inline sqc_result_t
s_add_stage(sqc_pipeline_stage_t ps) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (ps != NULL && IS_VALID_STRING(ps->m_name) == true) {
    void *val = (void *)ps;
    if ((ret = sqc_hashmap_add(&s_ps_name_tbl,
                                  (void *)(ps->m_name), &val, false)) ==
        SQC_RESULT_OK) {
      val = (void *)true;
      ret = sqc_hashmap_add(&s_ps_obj_tbl, (void *)ps, &val, true);
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_delete_stage(sqc_pipeline_stage_t ps) {
  if (ps != NULL) {
    (void)sqc_hashmap_delete(&s_ps_obj_tbl, (void *)ps, NULL, true);
    if (IS_VALID_STRING(ps->m_name) == true) {
      (void)sqc_hashmap_delete(&s_ps_name_tbl,
                                  (void *)(ps->m_name), NULL, true);
    }
  }
}


static inline bool
s_is_stage(sqc_pipeline_stage_t ps) {
  void *val;
  sqc_result_t r = sqc_hashmap_find(&s_ps_obj_tbl, (void *)ps, &val);
  return (r == SQC_RESULT_OK && (bool)val == true) ?
         true : false;
}


static inline sqc_pipeline_stage_t
s_find_stage(const char *name) {
  sqc_pipeline_stage_t ret = NULL;

  if (IS_VALID_STRING(name) == true) {
    void *val;
    if (sqc_hashmap_find(&s_ps_name_tbl, (void *)name, &val) ==
        SQC_RESULT_OK) {
      ret = (sqc_pipeline_stage_t)val;
    }
  }

  return ret;
}





static inline void
s_lock_stage(sqc_pipeline_stage_t ps) {
  if (ps != NULL && ps->m_lock != NULL) {
    (void)sqc_mutex_lock(&(ps->m_lock));
  }
}


static inline void
s_unlock_stage(sqc_pipeline_stage_t ps) {
  if (ps != NULL && ps->m_lock != NULL) {
    (void)sqc_mutex_unlock(&(ps->m_lock));
  }
}


static inline void
s_final_lock_stage(sqc_pipeline_stage_t ps) {
  if (ps != NULL && ps->m_final_lock != NULL) {
    (void)sqc_mutex_lock(&(ps->m_final_lock));
  }
}


static inline void
s_final_unlock_stage(sqc_pipeline_stage_t ps) {
  if (ps != NULL && ps->m_final_lock != NULL) {
    (void)sqc_mutex_unlock(&(ps->m_final_lock));
  }
}


static inline void
s_notify_stage(sqc_pipeline_stage_t ps) {
  if (ps != NULL && ps->m_cond != NULL) {
    (void)sqc_cond_notify(&(ps->m_cond), true);
  }
}


static inline sqc_result_t
s_cond_wait_stage(sqc_pipeline_stage_t ps, sqc_chrono_t nsec) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (ps != NULL && ps->m_lock != NULL && ps->m_cond != NULL) {
    ret = sqc_cond_wait(&(ps->m_cond), &(ps->m_lock), nsec);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_pause_lock_stage(sqc_pipeline_stage_t ps) {
  if (ps != NULL && ps->m_pause_lock != NULL) {
    (void)sqc_mutex_lock(&(ps->m_pause_lock));
  }
}


static inline void
s_pause_unlock_stage(sqc_pipeline_stage_t ps) {
  if (ps != NULL && ps->m_pause_lock != NULL) {
    (void)sqc_mutex_unlock(&(ps->m_pause_lock));
  }
}


static inline void
s_pause_notify_stage(sqc_pipeline_stage_t ps) {
  if (ps != NULL && ps->m_pause_cond != NULL) {
    (void)sqc_cond_notify(&(ps->m_pause_cond), true);
  }
}


static inline sqc_result_t
s_pause_cond_wait_stage(sqc_pipeline_stage_t ps, sqc_chrono_t nsec) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (ps != NULL && ps->m_pause_lock != NULL && ps->m_pause_cond != NULL) {
    ret = sqc_cond_wait(&(ps->m_pause_cond), &(ps->m_pause_lock), nsec);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_resume_notify_stage(sqc_pipeline_stage_t ps) {
  if (ps != NULL && ps->m_resume_cond != NULL) {
    (void)sqc_cond_notify(&(ps->m_resume_cond), true);
  }
}


static inline sqc_result_t
s_resume_cond_wait_stage(sqc_pipeline_stage_t ps, sqc_chrono_t nsec) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (ps != NULL && ps->m_pause_lock != NULL && ps->m_resume_cond != NULL) {
    ret = sqc_cond_wait(&(ps->m_resume_cond), &(ps->m_pause_lock), nsec);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline void
s_resume_stage(sqc_pipeline_stage_t ps) {
  s_pause_lock_stage(ps);
  {
    ps->m_pause_requested = false;
    s_resume_notify_stage(ps);
  }
  s_pause_unlock_stage(ps);
}


static inline sqc_result_t
s_cancel_stage(sqc_pipeline_stage_t ps, size_t n) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (ps != NULL) {
    size_t i;
    sqc_result_t first_err = SQC_RESULT_OK;

    /*
     * Cancel all the worker anyway.
     */
    for (i = 0; i < n; i++) {
      ret = s_worker_cancel(&(ps->m_workers[i]));
      if (ret != SQC_RESULT_OK &&
          first_err == SQC_RESULT_OK) {
        first_err = ret;
        /*
         * Just carry on cancelling no matter what kind of
         * errors occur.
         */
      }
    }

    /*
     * In case which any workers ignore the cancelation (or
     * don't have any cancelation points at the first place,)
     * mimic immediate shutdown.
     */
    ps->m_sg_lvl = SHUTDOWN_RIGHT_NOW;
    ps->m_do_loop = false;

    ret = first_err;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_wait_stage(sqc_pipeline_stage_t ps, size_t n, sqc_chrono_t nsec,
             bool is_in_destroy) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (ps != NULL) {
    size_t i;

    if (is_in_destroy == false && nsec > 0) {
      sqc_chrono_t w_begin;
      sqc_chrono_t w_end;
      sqc_chrono_t w = nsec;

      for (ret = SQC_RESULT_OK, i = 0;
           i < n && ret == SQC_RESULT_OK && w > 0LL;
           i++) {
        WHAT_TIME_IS_IT_NOW_IN_NSEC(w_begin);
        ret = s_worker_wait(&(ps->m_workers[i]), w);
        WHAT_TIME_IS_IT_NOW_IN_NSEC(w_end);

        if (ret == SQC_RESULT_OK) {
          w = w_end - w_begin;
        }
      }
    } else {
      for (i = 0; i < n; i++) {
        (void)s_worker_wait(&(ps->m_workers[i]), -1LL);
      }
      ret = SQC_RESULT_OK;
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_destroy_stage(sqc_pipeline_stage_t ps, bool is_clean_finish) {
  if (ps != NULL) {

    s_lock_stage(ps);
    {
      if (is_clean_finish == true) {

        ps->m_status = STAGE_STATE_DESTROYING;
        s_notify_stage(ps);

        (void)s_cancel_stage(ps, ps->m_n_workers);
        (void)s_wait_stage(ps, ps->m_n_workers, -1LL, true);

        if (ps->m_n_workers > 0 && ps->m_workers != NULL) {
          size_t i;
          for (i = 0; i < ps->m_n_workers; i++) {
            s_worker_destroy(&(ps->m_workers[i]));
          }
        }

        if (ps->m_freeup_proc != NULL) {
          (ps->m_freeup_proc)(&ps);
        }

      }

      s_delete_stage(ps);
      free((void *)(ps->m_name));
      free((void *)(ps->m_workers));

      if (ps->m_cond != NULL) {
        sqc_cond_destroy(&(ps->m_cond));
        ps->m_cond = NULL;
      }
      if (ps->m_final_lock != NULL) {
        sqc_mutex_destroy(&(ps->m_final_lock));
        ps->m_final_lock = NULL;
      }
      if (ps->m_pause_barrier != NULL) {
        sqc_barrier_destroy(&(ps->m_pause_barrier));
        ps->m_pause_barrier = NULL;
      }
      if (ps->m_pause_lock != NULL) {
        sqc_mutex_destroy(&(ps->m_pause_lock));
        ps->m_pause_lock = NULL;
      }
      if (ps->m_pause_cond != NULL) {
        sqc_cond_destroy(&(ps->m_pause_cond));
        ps->m_pause_cond = NULL;
      }
      if (ps->m_resume_cond != NULL) {
        sqc_cond_destroy(&(ps->m_resume_cond));
        ps->m_resume_cond = NULL;
      }

    }
    s_unlock_stage(ps);

    if (ps->m_lock != NULL) {
      sqc_mutex_destroy(&(ps->m_lock));
      ps->m_lock = NULL;
    }

    if (ps->m_is_heap_allocd == true) {
      free((void *)ps);
    }
  }
}


static inline sqc_result_t
s_init_stage(sqc_pipeline_stage_t *sptr,
             const char *name,
             bool is_heap_allocd,
             size_t n_workers,
             size_t event_size,
             size_t max_batch_size,
             sqc_pipeline_stage_pre_pause_proc_t pre_pause_proc,
             sqc_pipeline_stage_sched_proc_t sched_proc,
             sqc_pipeline_stage_setup_proc_t setup_proc,
             sqc_pipeline_stage_fetch_proc_t fetch_proc,
             sqc_pipeline_stage_main_proc_t main_proc,
             sqc_pipeline_stage_throw_proc_t throw_proc,
             sqc_pipeline_stage_shutdown_proc_t shutdown_proc,
             sqc_pipeline_stage_finalize_proc_t final_proc,
             sqc_pipeline_stage_freeup_proc_t freeup_proc) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  /*
   * Note that receiving a pipeline stage as a reference
   * (sqc_pipeline_stage_t *) IS VERY IMPOETANT in order to
   * create workers by calling the s_worker_create().
   */

  if (sptr != NULL && *sptr != NULL) {
    sqc_pipeline_stage_t ps = *sptr;
    worker_main_proc_t proc = s_find_worker_proc(fetch_proc,
                              main_proc,
                              throw_proc);
    if (proc != NULL) {

      (void)memset((void *)ps, 0, DEFAULT_STAGE_ALLOC_SZ);

      if (((ret = sqc_mutex_create(&(ps->m_lock))) ==
           SQC_RESULT_OK) &&
          ((ret = sqc_mutex_create(&(ps->m_final_lock))) ==
           SQC_RESULT_OK) &&
          ((ret = sqc_cond_create(&(ps->m_cond))) ==
           SQC_RESULT_OK) &&
          ((ret = sqc_barrier_create(&(ps->m_pause_barrier), n_workers)) ==
           SQC_RESULT_OK) &&
          ((ret = sqc_mutex_create(&(ps->m_pause_lock))) ==
           SQC_RESULT_OK) &&
          ((ret = sqc_cond_create(&(ps->m_pause_cond))) ==
           SQC_RESULT_OK) &&
          ((ret = sqc_cond_create(&(ps->m_resume_cond))) ==
           SQC_RESULT_OK)) {
        if ((ps->m_name = strdup(name)) != NULL &&
            (ps->m_workers = (sqc_pipeline_worker_t *)
                             malloc(sizeof(sqc_pipeline_worker_t) * n_workers)) != NULL) {
          size_t i;

          ps->m_event_size = event_size;
          ps->m_max_batch = max_batch_size;
          ps->m_batch_buffer_size = event_size * max_batch_size;

          for (i = 0; i < n_workers && ret == SQC_RESULT_OK; i++) {
            ret = s_worker_create(&(ps->m_workers[i]), sptr, i, proc);
          }
          if (ret == SQC_RESULT_OK) {
            ps->m_pre_pause_proc = pre_pause_proc;
            ps->m_sched_proc = sched_proc;
            ps->m_setup_proc = setup_proc;
            ps->m_fetch_proc = fetch_proc;
            ps->m_main_proc = main_proc;
            ps->m_throw_proc = throw_proc;
            ps->m_shutdown_proc = shutdown_proc;
            ps->m_final_proc = final_proc;
            ps->m_freeup_proc = freeup_proc;

            ps->m_n_workers = n_workers;
            ps->m_is_heap_allocd = is_heap_allocd;

            ps->m_do_loop = false;
            ps->m_sg_lvl = SHUTDOWN_UNKNOWN;
            ps->m_status = STAGE_STATE_INITIALIZED;

            ps->m_n_canceled_workers = 0LL;
            ps->m_n_shutdown_workers = 0LL;

            ps->m_pause_requested = false;

            ps->m_maint_proc = NULL;
            ps->m_maint_arg = NULL;

            ps->m_post_start_proc = NULL;
            ps->m_post_start_arg = NULL;

            /*
             * finally.
             */
            ret = SQC_RESULT_OK;

          } else {
            size_t n_created = i;

            for (i = 0; i < n_created; i++) {
              s_worker_destroy(&(ps->m_workers[i]));
            }
          }
        } else {
          free((void *)(ps->m_name));
          ps->m_name = NULL;
          free((void *)(ps->m_workers));
          ps->m_workers = NULL;
          ret = SQC_RESULT_NO_MEMORY;
        }
      }
    } else {
      ret = SQC_RESULT_INVALID_ARGS;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





sqc_result_t
sqc_pipeline_stage_create(sqc_pipeline_stage_t *sptr,
                             size_t alloc_size,
                             const char *name,
                             size_t n_workers,
                             size_t event_size,
                             size_t max_batch_size,
                             sqc_pipeline_stage_pre_pause_proc_t
                             pre_pause_proc,
                             sqc_pipeline_stage_sched_proc_t sched_proc,
                             sqc_pipeline_stage_setup_proc_t setup_proc,
                             sqc_pipeline_stage_fetch_proc_t fetch_proc,
                             sqc_pipeline_stage_main_proc_t main_proc,
                             sqc_pipeline_stage_throw_proc_t throw_proc,
                             sqc_pipeline_stage_shutdown_proc_t
                             shutdown_proc,
                             sqc_pipeline_stage_finalize_proc_t
                             final_proc,
                             sqc_pipeline_stage_freeup_proc_t
                             freeup_proc) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_pipeline_stage_t ps = NULL;
  sqc_pipeline_stage_t checked_ps = NULL;

  if (sptr != NULL &&
      IS_VALID_STRING(name) == true &&
      (checked_ps = s_find_stage(name)) == NULL &&
      event_size > 0 &&
      max_batch_size > 0 &&
      ((main_proc != NULL) ||
       (fetch_proc != NULL && main_proc != NULL) ||
       (main_proc != NULL && throw_proc != NULL) ||
       (fetch_proc != NULL && main_proc != NULL && throw_proc != NULL))) {

    bool is_heap_allocd = false;

    if (*sptr == NULL) {
      size_t sz = (DEFAULT_STAGE_ALLOC_SZ > alloc_size) ?
                  DEFAULT_STAGE_ALLOC_SZ : alloc_size;
      ps = (sqc_pipeline_stage_t)malloc(sz);
      if (ps != NULL) {
        *sptr = ps;
        is_heap_allocd = true;
      } else {
        ret = SQC_RESULT_NO_MEMORY;
        goto done;
      }
    } else {
      ps = *sptr;
    }

    ret = s_init_stage(sptr, name, is_heap_allocd,
                       n_workers, event_size, max_batch_size,
                       pre_pause_proc,
                       sched_proc,
                       setup_proc,
                       fetch_proc,
                       main_proc,
                       throw_proc,
                       shutdown_proc,
                       final_proc,
                       freeup_proc);
    if (ret == SQC_RESULT_OK) {
      ret = s_add_stage(ps);
    }
  } else {
    if (checked_ps != NULL) {
      ret = SQC_RESULT_ALREADY_EXISTS;
    } else {
      ret = SQC_RESULT_INVALID_ARGS;
    }
  }

done:
  if (ret != SQC_RESULT_OK) {
    s_destroy_stage(ps, false);
    if (sptr != NULL) {
      *sptr = NULL;
    }
  }

  return ret;
}


void
sqc_pipeline_stage_destroy(sqc_pipeline_stage_t *sptr) {
  if (sptr != NULL && *sptr != NULL) {
    sqc_pipeline_stage_t ps = *sptr;

    if (s_is_stage(ps) == true) {
      s_destroy_stage(ps, true);
    }
  }
}





sqc_result_t
sqc_pipeline_stage_setup(const sqc_pipeline_stage_t *sptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (sptr != NULL && *sptr != NULL) {
    sqc_pipeline_stage_t ps = *sptr;

    if (s_is_stage(ps) == true) {

      s_lock_stage(ps);
      {
        if (ps->m_status == STAGE_STATE_INITIALIZED) {
          if (ps->m_setup_proc != NULL) {
            ret = (ps->m_setup_proc)(sptr);
            if (ret == SQC_RESULT_OK) {
              ps->m_status = STAGE_STATE_SETUP;
            }
          } else {
            ret = SQC_RESULT_OK;
          }
        } else {
          if (ps->m_status == STAGE_STATE_SETUP) {
            ret = SQC_RESULT_OK;
          } else {
            ret = SQC_RESULT_INVALID_STATE_TRANSITION;
          }
        }
      }
      s_unlock_stage(ps);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_pipeline_stage_start(const sqc_pipeline_stage_t *sptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (sptr != NULL && *sptr != NULL) {
    sqc_pipeline_stage_t ps = *sptr;

    if (s_is_stage(ps) == true) {

      s_lock_stage(ps);
      {
        if (ps->m_status == STAGE_STATE_INITIALIZED ||
            ps->m_status == STAGE_STATE_SETUP ||
            ps->m_status == STAGE_STATE_FINALIZED) {
          size_t i;

          ps->m_maint_proc = NULL;
          ps->m_maint_arg = NULL;

          ps->m_n_canceled_workers = 0LL;
          ps->m_n_shutdown_workers = 0LL;

          ps->m_do_loop = true;

          for (i = 0, ret = SQC_RESULT_OK;
               i < ps->m_n_workers && ret == SQC_RESULT_OK;
               i++) {
            ret = s_worker_start(&(ps->m_workers[i]));
          }

          if (ret == SQC_RESULT_OK) {
            ps->m_status = STAGE_STATE_STARTED;
          } else {
            /*
             * Don't destroy the workers. Just cancel them instead.
             */
            (void)s_cancel_stage(ps, i);
            (void)s_wait_stage(ps, i, -1LL, true);

            ps->m_n_canceled_workers = 0LL;
            ps->m_n_shutdown_workers = 0LL;
          }

        } else {
          ret = SQC_RESULT_INVALID_STATE_TRANSITION;
        }
      }
      s_unlock_stage(ps);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_pipeline_stage_cancel(const sqc_pipeline_stage_t *sptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (sptr != NULL && *sptr != NULL) {
    sqc_pipeline_stage_t ps = *sptr;

    if (s_is_stage(ps) == true) {

      s_lock_stage(ps);
      {
        if (ps->m_status == STAGE_STATE_STARTED) {
          ret = s_cancel_stage(ps, ps->m_n_workers);
        } else {
          ret = SQC_RESULT_INVALID_STATE_TRANSITION;
        }
      }
      s_unlock_stage(ps);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_pipeline_stage_shutdown(const sqc_pipeline_stage_t *sptr,
                               shutdown_grace_level_t lvl) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (sptr != NULL && *sptr != NULL) {
    sqc_pipeline_stage_t ps = *sptr;

    if (s_is_stage(ps) == true) {

      s_lock_stage(ps);
      {
        if (ps->m_status == STAGE_STATE_STARTED ||
            ps->m_status == STAGE_STATE_PAUSED) {

          if (ps->m_status == STAGE_STATE_PAUSED) {
            s_resume_stage(ps);
            ps->m_status = STAGE_STATE_STARTED;
          }

          ps->m_sg_lvl = lvl;
          if (lvl == SHUTDOWN_RIGHT_NOW) {
            /*
             * No matter what the main worker loop stops for all the
             * workers.
             */
            ret = s_cancel_stage(ps, ps->m_n_workers);
          } else {
            ret = SQC_RESULT_OK;
            /*
             * Note that the ps->m_do_loop is still true in this cases.
             */
          }

        } else {
          ret = SQC_RESULT_INVALID_STATE_TRANSITION;
        }
      }
      s_unlock_stage(ps);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_pipeline_stage_wait(const sqc_pipeline_stage_t *sptr,
                           sqc_chrono_t nsec) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (sptr != NULL && *sptr != NULL) {
    sqc_pipeline_stage_t ps = *sptr;

    if (s_is_stage(ps) == true) {

      s_lock_stage(ps);
      {
        if (ps->m_status == STAGE_STATE_STARTED) {
          size_t n_shutdown;
          size_t n_canceled;

          ret = s_wait_stage(ps, ps->m_n_workers, nsec, false);

          s_final_lock_stage(ps);
          {
            n_shutdown = ps->m_n_shutdown_workers;
            n_canceled = ps->m_n_canceled_workers;
          }
          s_final_unlock_stage(ps);

          if (ret == SQC_RESULT_OK) {
            if (n_shutdown == ps->m_n_workers) {
              if (n_canceled > 0LL) {
                ps->m_status = STAGE_STATE_CANCELED;
              } else {
                ps->m_status = STAGE_STATE_SHUTDOWN;
              }

              if (ps->m_final_proc != NULL) {
                (ps->m_final_proc)(&ps,
                                   (n_canceled > 0) ? true : false);
              }

              if (ps->m_shutdown_proc != NULL) {
                (void)(ps->m_shutdown_proc)(&ps, ps->m_sg_lvl);
              }

            } else {
              sqc_exit_fatal("must not happen, waiting for all the worker "
                                "exit succeeded but the number of the exited "
                                "workers and the number of succeeded API "
                                "calls differ on stage '%s', "
                                "workers " PFSZ(u) ", "
                                "shutdown " PFSZ(u) ", "
                                "cancel " PFSZ(u) "\n",
                                ps->m_name,
                                ps->m_n_workers, n_shutdown, n_canceled);
            }

            ps->m_do_loop = false;
          }

        } else {
          ret = SQC_RESULT_INVALID_STATE_TRANSITION;
        }
      }
      s_unlock_stage(ps);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
sqc_pipeline_stage_cancel_janitor(const sqc_pipeline_stage_t *sptr) {
  if (sptr != NULL && *sptr != NULL &&
      s_is_stage(*sptr) == true) {
    s_pause_unlock_stage(*sptr);
    s_unlock_stage(*sptr);
  }
}


#ifdef SQC_OS_LINUX
sqc_result_t
sqc_pipeline_stage_set_worker_cpu_affinity(
  const sqc_pipeline_stage_t *sptr,
  size_t idx, int cpu) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_pipeline_stage_t ps = NULL;

  if (sptr != NULL && (ps = *sptr) != NULL &&
      idx < ps->m_n_workers) {
    ret = s_worker_set_cpu_affinity(&(ps->m_workers[idx]), cpu);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}
#endif /* SQC_OS_LINUX */





sqc_result_t
sqc_pipeline_stage_submit(const sqc_pipeline_stage_t *sptr,
                             void *evbuf,
                             size_t n_evs,
                             void *hint) {
  if (sptr != NULL && *sptr != NULL) {
    if ((*sptr)->m_sched_proc != NULL) {
      return ((*sptr)->m_sched_proc)(sptr, evbuf, n_evs, hint);
    }
  }
  return SQC_RESULT_INVALID_ARGS;
}





sqc_result_t
sqc_pipeline_stage_pause(const sqc_pipeline_stage_t *sptr,
                            sqc_chrono_t nsec) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (sptr != NULL && *sptr != NULL) {
    sqc_pipeline_stage_t ps = *sptr;
    if (s_is_stage(ps) == true) {

      s_lock_stage(ps);
      {
        if (ps->m_status == STAGE_STATE_STARTED) {

          s_pause_lock_stage(ps);
          {
            /*
             * Firstly, set the pause flag. Note that there is a
             * slight possibility that setting the flag won't work
             * because of the compiler optimization problem. In order
             * to make the flag works fine, maybe we need a
             * compiler/runtime-supported atomic memory read/write
             * mechanism. For now, we just believe the compiler is
             * fine with a volatile declaration for the flag.
             */

            ps->m_pause_requested = true;

            /*
             * Wake sleeping workers up if needed.
             */
            if (ps->m_pre_pause_proc != NULL) {
              (ps->m_pre_pause_proc)(sptr);
            }

            /*
             * Then all the workers enter the barrier
             * (ps->m_pause_barrier.) And after the barrier
             * synchronization, a worker sets the ps->m_status to
             * STAGE_STATE_PAUSED and wakes this thread up. Then all
             * the worker sleep with ps->m_resume_cond with
             * ps->m_pause_lock acquired.
             */

          recheck:
            if (ps->m_status != STAGE_STATE_PAUSED) {
              /*
               * Note that we are about to sleep even having the
               * master stage lock (ps->m_lock) acquired, in order to
               * avoid be disturbed by any other threads trying to
               * cancel/shutdown/pause this stage.
               */
              ret = s_pause_cond_wait_stage(ps, nsec);
              if (ret == SQC_RESULT_OK) {
                goto recheck;
              }
            } else {
              ret = SQC_RESULT_OK;
            }
          }
          s_pause_unlock_stage(ps);

          if (ret != SQC_RESULT_OK &&
              ret != SQC_RESULT_TIMEDOUT) {
            ps->m_pause_requested = false;
          }

        } else {
          if (ps->m_status == STAGE_STATE_PAUSED) {
            ret = SQC_RESULT_OK;
          } else {
            ret = SQC_RESULT_INVALID_STATE_TRANSITION;
          }
        }
      }
      s_unlock_stage(ps);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_pipeline_stage_resume(const sqc_pipeline_stage_t *sptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (sptr != NULL && *sptr != NULL) {
    sqc_pipeline_stage_t ps = *sptr;
    if (s_is_stage(ps) == true) {

      s_lock_stage(ps);
      {
        if (ps->m_status == STAGE_STATE_PAUSED) {

          s_resume_stage(ps);

          ps->m_status = STAGE_STATE_STARTED;
          ret = SQC_RESULT_OK;

        } else {
          if (ps->m_status == STAGE_STATE_STARTED) {
            ret = SQC_RESULT_OK;
          } else {
            ret = SQC_RESULT_INVALID_STATE_TRANSITION;
          }
        }
      }
      s_unlock_stage(ps);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





sqc_result_t
sqc_pipeline_stage_schedule_maintenance(
  const sqc_pipeline_stage_t *sptr,
  sqc_pipeline_stage_maintenance_proc_t func, void *arg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (sptr != NULL && *sptr != NULL) {
    sqc_pipeline_stage_t ps = *sptr;
    if (s_is_stage(ps) == true) {

      s_lock_stage(ps);
      {
        if (ps->m_status == STAGE_STATE_STARTED) {

          s_pause_lock_stage(ps);
          {
            /*
             * Firstly, set the func, the arg and the pause flag. Note
             * that there is a slight possibility that setting the
             * flag won't work because of the compiler optimization
             * problem. In order to make the flag works fine, maybe we
             * need a compiler/runtime-supported atomic memory
             * read/write mechanism. For now, we just believe the
             * compiler is fine with a volatile declaration for the
             * flag.
             */

            ps->m_status = STAGE_STATE_MAINTENANCE_REQUESTED;
            ps->m_maint_proc = func;
            ps->m_maint_arg = arg;
            ps->m_pause_requested = true;

            /*
             * Wake sleeping workers up if needed.
             */
            if (ps->m_pre_pause_proc != NULL) {
              (ps->m_pre_pause_proc)(sptr);
            }

            /*
             * Then all the workers enter the barrier
             * (ps->m_pause_barrier.) And after the barrier
             * synchronization, a worker invokes the func(arg) and
             * reset the ps->m_status to STAGE_STATE_STARTED. Then the
             * worker wakes this thread up.
             */

          recheck:
            if (ps->m_status == STAGE_STATE_MAINTENANCE_REQUESTED) {
              /*
               * Note that we are about to sleep even having the
               * master stage lock (ps->m_lock) acquired, in order to
               * avoid be disturbed by any other threads trying to
               * cancel/shutdown/pause this stage.
               */
              ret = s_pause_cond_wait_stage(ps, -1LL);
              if (ret == SQC_RESULT_OK) {
                goto recheck;
              }
            } else {
              if (ps->m_status == STAGE_STATE_STARTED) {
                ret = SQC_RESULT_OK;
              } else {
                /*
                 * Must not happen, though.
                 */
                ret = SQC_RESULT_INVALID_STATE_TRANSITION;
              }
            }
          }
          s_pause_unlock_stage(ps);

          if (ret != SQC_RESULT_OK) {
            ps->m_pause_requested = false;
            ps->m_maint_proc = NULL;
            ps->m_maint_arg = NULL;
          }

        } else {
          ret = SQC_RESULT_INVALID_STATE_TRANSITION;
        }
      }
      s_unlock_stage(ps);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





sqc_result_t
sqc_pipeline_stage_find(const char *name,
                           sqc_pipeline_stage_t *retptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(name) == true && retptr != NULL) {
    if ((*retptr = s_find_stage(name)) != NULL) {
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_NOT_FOUND;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





sqc_result_t
sqc_pipeline_stage_set_post_start_hook(
  sqc_pipeline_stage_t *sptr,
  sqc_pipeline_stage_post_start_proc_t func,
  void *arg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (sptr != NULL && *sptr != NULL) {
    sqc_pipeline_stage_t ps = *sptr;

    if (s_is_stage(ps) == true) {

      s_lock_stage(ps);
      {
        ps->m_post_start_proc = func;
        ps->m_post_start_arg = arg;
        ret = SQC_RESULT_OK;
      }
      s_unlock_stage(ps);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





sqc_result_t
sqc_pipeline_stage_get_worker_nubmer(sqc_pipeline_stage_t *sptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (sptr != NULL && *sptr != NULL) {
    ret = (sqc_result_t)((*sptr)->m_n_workers);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_pipeline_stage_get_worker_event_buffer(sqc_pipeline_stage_t *sptr,
    size_t index, void **buf) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (sptr != NULL && *sptr != NULL && buf != NULL &&
      index < (*sptr)->m_n_workers) {
    *buf = s_worker_get_buffer(&((*sptr)->m_workers[index]));
    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_pipeline_stage_set_worker_event_buffer(
  sqc_pipeline_stage_t *sptr,
  size_t index, void *buf,
  sqc_pipeline_stage_event_buffer_freeup_proc_t freeup_proc) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (sptr != NULL && *sptr != NULL && buf != NULL &&
      index < (*sptr)->m_n_workers) {
    void *obuf = s_worker_set_buffer(&((*sptr)->m_workers[index]),
                                     buf, freeup_proc);
    if (obuf != NULL) {
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_INVALID_ARGS;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_pipeline_stage_get_name(const sqc_pipeline_stage_t *sptr,
                               const char **name) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (sptr != NULL && *sptr != NULL && name != NULL) {
    *name = (*sptr)->m_name;
    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}
