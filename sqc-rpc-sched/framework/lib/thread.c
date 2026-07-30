#include "sqc_apis.h"
#include "sqc_thread_internal.h"





#define DEFAULT_THREAD_ALLOC_SZ	(sizeof(sqc_thread_record))





static pthread_once_t s_once = PTHREAD_ONCE_INIT;
static bool s_is_inited = false;
static pthread_attr_t s_attr;
static sqc_hashmap_t s_thd_tbl;
static sqc_hashmap_t s_alloc_tbl;

static cpu_set_t **s_numa_cpu_set = NULL;
static cpu_set_t *s_full_cpu_set = NULL;

static bool s_is_numa_available = false;
static size_t s_n_cpus = 0;
static size_t s_n_numa_nodes = 0;
static size_t s_cpu_set_sz = 0;
static bool s_numa_inited = false;

static void s_ctors(void) __attr_constructor__(106);
static void s_dtors(void) __attr_destructor__(106);





static void
s_child_at_fork(void) {
  sqc_hashmap_atfork_child(&s_thd_tbl);
  sqc_hashmap_atfork_child(&s_alloc_tbl);
}


static inline void
s_numa_probe(void) {
  if (s_numa_inited == false) {
    long n;
#ifdef HAVE_NUMA
    if (numa_available() != -1 &&
        (s_n_numa_nodes = (size_t)numa_num_task_nodes()) > 1) {
      s_n_cpus = (size_t)numa_num_task_cpus();
      s_is_numa_available = true;
    } else {
      n = sysconf(_SC_NPROCESSORS_ONLN);
      if (n > 0) {
        s_n_cpus = (size_t)n;
      } else {
        sqc_exit_fatal("can't acquire a # of cpus to use.\n");
      }
      s_n_numa_nodes = 1;
    }
#else
    n = sysconf(_SC_NPROCESSORS_ONLN);
    if (n > 0) {
      s_n_cpus = (size_t)n;
    } else {
      sqc_exit_fatal("can't acquire a # of cpus to use.\n");
    }
    s_n_numa_nodes = 1;
#endif /* HAVE_NUMA */
    s_cpu_set_sz = CPU_ALLOC_SIZE(s_n_cpus);

    s_numa_inited = true;
  }
}


static inline sqc_result_t
s_cpu_set_alloc(cpu_set_t **setp, bool do_clear) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(setp != NULL)) {
    if (*setp != NULL) {
      ret = SQC_RESULT_OK;
    } else {
      *setp = CPU_ALLOC(s_n_cpus);
      if (likely(*setp != NULL)) {
        ret = SQC_RESULT_OK;
      } else {
        ret = SQC_RESULT_NO_MEMORY;
      }
    }

    if (likely(ret == SQC_RESULT_OK && do_clear == true)) {
      CPU_ZERO_S(s_cpu_set_sz, *setp);
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


#ifndef HAVE_NUMA
static inline int
numa_node_of_cpu(int n) {
  (void)n;
  return 0;
}
#endif /* ! HAVE_NUMA */


static void
s_once_proc(void) {
  int st;
  sqc_result_t r;

  s_numa_probe();

  if ((st = pthread_attr_init(&s_attr)) == 0) {
    if ((st = pthread_attr_setdetachstate(&s_attr,
                                          PTHREAD_CREATE_DETACHED)) != 0) {
      errno = st;
      perror("pthread_attr_setdetachstate");
      sqc_exit_fatal("can't initialize detached thread attr.\n");
    }
  } else {
    errno = st;
    perror("pthread_attr_init");
    sqc_exit_fatal("can't initialize thread attr.\n");
  }

  if (s_cpu_set_sz > 0 && s_n_cpus > 0 && s_n_numa_nodes > 0) {
    size_t i;
    size_t j;

    s_full_cpu_set = NULL;
    r = s_cpu_set_alloc(&s_full_cpu_set, true);
    if (likely(r == SQC_RESULT_OK)) {
      for (i = 0; i < s_n_cpus; i++) {
        CPU_SET_S(i, s_cpu_set_sz, s_full_cpu_set);
      }
    } else {
      sqc_exit_fatal("can't allocate a cpu_set_t: %s.\n",
                        sqc_error_get_string(r));
    }

    s_numa_cpu_set = (cpu_set_t **)malloc(sizeof(cpu_set_t *) *
                                          s_n_numa_nodes);
    if (likely(s_numa_cpu_set != NULL)) {
      for (j = 0; j < s_n_numa_nodes; j++) {
        s_numa_cpu_set[j] = NULL;
        r = s_cpu_set_alloc(&s_numa_cpu_set[j], true);
        if (likely(r == SQC_RESULT_OK)) {
          if (s_is_numa_available == true) {
            for (i = 0; i < s_n_cpus; i++) {
              if (numa_node_of_cpu((int)i) == (int)j) {
                CPU_SET_S(i, s_cpu_set_sz, s_numa_cpu_set[j]);
              }
            }
          } else {
            (void)memcpy(s_numa_cpu_set[j], s_full_cpu_set, s_cpu_set_sz);
          }
        } else {
          sqc_exit_fatal("can't allocate a cpu_set_t: %s.\n",
                            sqc_error_get_string(r));
        }
      }
    } else {
      r = SQC_RESULT_NO_MEMORY;
      sqc_exit_fatal("can't allocate a cpu_set_t array: %s.\n",
                        sqc_error_get_string(r));
    }
  }

  if ((r = sqc_hashmap_create(&s_thd_tbl, SQC_HASHMAP_TYPE_ONE_WORD,
                                 NULL)) != SQC_RESULT_OK) {
    sqc_perror(r);
    sqc_exit_fatal("can't initialize the thread table.\n");
  }
  if ((r = sqc_hashmap_create(&s_alloc_tbl, SQC_HASHMAP_TYPE_ONE_WORD,
                                 NULL)) != SQC_RESULT_OK) {
    sqc_perror(r);
    sqc_exit_fatal("can't initialize the thread allocation table.\n");
  }

  (void)pthread_atfork(NULL, NULL, s_child_at_fork);

  s_is_inited = true;
}


static inline void
s_init(void) {
  sqc_heapcheck_module_initialize();
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();

  sqc_msg_debug(10, "The thread module is initialized.\n");
}


static void
s_final(void) {
  sqc_hashmap_destroy(&s_thd_tbl, true);
  sqc_hashmap_destroy(&s_alloc_tbl, true);

  if (s_n_numa_nodes > 0 && s_numa_cpu_set != NULL) {
    size_t j;
    for (j = 0; j < s_n_numa_nodes; j++) {
      if (s_numa_cpu_set[j] != NULL) {
        CPU_FREE(s_numa_cpu_set[j]);
        s_numa_cpu_set[j] = NULL;
      }
    }
    free(s_numa_cpu_set);
  }
  if (s_full_cpu_set != NULL) {
    CPU_FREE(s_full_cpu_set);
    s_full_cpu_set = NULL;
  }
}


static void
s_dtors(void) {
  if (s_is_inited == true) {
    if (sqc_module_is_unloading() &&
        sqc_module_is_finalized_cleanly()) {
      s_final();

      sqc_msg_debug(10, "The thread module is finalized.\n");
    } else {
      sqc_msg_debug(10, "The thread module is not finalized "
                       "because of module finalization problem.\n");
    }
  }
}





static inline sqc_result_t
s_cpu_set_any(cpu_set_t **setp) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely((ret = s_cpu_set_alloc(setp, false)) == SQC_RESULT_OK)) {
    (void)memcpy(*setp, s_full_cpu_set, s_cpu_set_sz);
  }

  return ret;
}


static inline sqc_result_t
s_cpu_set_numa_node(cpu_set_t **setp, int node) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (s_is_numa_available == true) {
    if (likely(node >= 0 && (size_t)node < s_n_numa_nodes)) {
      if (likely((ret = s_cpu_set_alloc(setp, false)) == SQC_RESULT_OK)) {
        (void)memcpy(*setp, s_numa_cpu_set[node], s_cpu_set_sz);
      }
    } else {
      ret = SQC_RESULT_INVALID_ARGS;
    }
  } else {
    if (likely((ret = s_cpu_set_alloc(setp, false)) == SQC_RESULT_OK)) {
      ret = s_cpu_set_any(setp);
    }
  }

  return ret;
}


static inline sqc_result_t
s_cpu_set_cpu(cpu_set_t **setp, int cpu) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(cpu >= 0 && (size_t)cpu < s_n_cpus)) {
    if (likely((ret = s_cpu_set_alloc(setp, false)) == SQC_RESULT_OK)) {
      CPU_SET_S((size_t)cpu, s_cpu_set_sz, *setp);
    }
  } else if (likely(cpu < 0)) {
    ret = s_cpu_set_alloc(setp, true);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline void
s_add_thd(const sqc_thread_t thd) {
  void *val = (void *)true;
  (void)sqc_hashmap_add(&s_thd_tbl, (void *)thd, &val, true);
}


static inline void
s_alloc_mark_thd(const sqc_thread_t thd) {
  void *val = (void *)true;
  (void)sqc_hashmap_add(&s_alloc_tbl, (void *)thd, &val, true);
}


static inline void
s_delete_thd(const sqc_thread_t thd) {
  (void)sqc_hashmap_delete(&s_thd_tbl, (void *)thd, NULL, true);
  (void)sqc_hashmap_delete(&s_alloc_tbl, (void *)thd, NULL, true);
}


static inline bool
s_is_thd(const sqc_thread_t thd) {
  void *val;
  sqc_result_t r = sqc_hashmap_find(&s_thd_tbl, (void *)thd, &val);
  return (r == SQC_RESULT_OK && (bool)val == true) ?
         true : false;
}


static inline bool
s_is_alloc_marked(const sqc_thread_t thd) {
  void *val;
  sqc_result_t r = sqc_hashmap_find(&s_alloc_tbl, (void *)thd, &val);
  return (r == SQC_RESULT_OK && (bool)val == true) ?
         true : false;
}


static inline void
s_op_lock(const sqc_thread_t thd) {
  if (thd != NULL) {
    (void)sqc_mutex_lock(&(thd->m_op_lock));
  }
}


static inline void
s_op_unlock(const sqc_thread_t thd) {
  if (thd != NULL) {
    (void)sqc_mutex_unlock(&(thd->m_op_lock));
  }
}


static inline void
s_wait_lock(const sqc_thread_t thd) {
  if (thd != NULL) {
    (void)sqc_mutex_lock(&(thd->m_wait_lock));
  }
}


static inline void
s_wait_unlock(const sqc_thread_t thd) {
  if (thd != NULL) {
    (void)sqc_mutex_unlock(&(thd->m_wait_lock));
  }
}


static inline void
s_cancel_lock(const sqc_thread_t thd) {
  if (thd != NULL) {
    (void)sqc_mutex_lock(&(thd->m_cancel_lock));
  }
}


static inline void
s_cancel_unlock(const sqc_thread_t thd) {
  if (thd != NULL) {
    (void)sqc_mutex_unlock(&(thd->m_cancel_lock));
  }
}





static inline sqc_result_t
s_initialize(sqc_thread_t thd,
             bool is_allocd,
             sqc_thread_main_proc_t mainproc,
             sqc_thread_finalize_proc_t finalproc,
             sqc_thread_freeup_proc_t freeproc,
             const char *name,
             void *arg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (thd != NULL) {
    cpu_set_t *def_set = NULL;
    (void)memset((void *)thd, 0, sizeof(*thd));
    s_add_thd(thd);
    if (((ret = sqc_mutex_create(&(thd->m_wait_lock))) ==
         SQC_RESULT_OK) &&
        ((ret = sqc_mutex_create(&(thd->m_op_lock))) ==
         SQC_RESULT_OK) &&
        ((ret = sqc_mutex_create(&(thd->m_cancel_lock))) ==
         SQC_RESULT_OK) &&
        ((ret = sqc_mutex_create(&(thd->m_finalize_lock))) ==
         SQC_RESULT_OK) &&
        ((ret = sqc_cond_create(&(thd->m_wait_cond))) ==
         SQC_RESULT_OK) &&
        ((ret = sqc_cond_create(&(thd->m_startup_cond))) ==
         SQC_RESULT_OK) &&
        ((ret = sqc_cond_create(&(thd->m_finalize_cond))) ==
         SQC_RESULT_OK) &&
        ((ret = s_cpu_set_any(&def_set)) == SQC_RESULT_OK)) {
      thd->m_arg = arg;
      thd->m_is_allocd = is_allocd;
      if (IS_VALID_STRING(name) == true) {
        snprintf(thd->m_name, sizeof(thd->m_name), "%s", name);
      }
      thd->m_creator_pid = getpid();
      thd->m_pthd = SQC_INVALID_THREAD;

      thd->m_main_proc = mainproc;
      thd->m_final_proc = finalproc;
      thd->m_freeup_proc = freeproc;
      thd->m_result_code = SQC_RESULT_NOT_STARTED;

      thd->m_cpusetptr = def_set;
      thd->m_is_started = false;
      thd->m_is_activated = false;
      thd->m_is_canceled = false;
      thd->m_is_finalized = false;
      thd->m_is_destroying = false;

      thd->m_do_autodelete = false;
      thd->m_startup_sync_done = false;
      thd->m_n_finalized_count = 0LL;

      ret = SQC_RESULT_OK;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_finalize(sqc_thread_t thd, bool is_canceled,
           sqc_result_t rcode) {
  if (likely(thd != NULL)) {
    sqc_thread_t cthd = thd;
    bool is_valid = false;
    sqc_result_t r = sqc_thread_is_valid(&cthd, &is_valid);

    if (likely(r == SQC_RESULT_OK && is_valid == true)) {
      bool do_autodelete = false;
      int o_cancel_state;

      (void)sqc_mutex_enter_critical(&(thd->m_finalize_lock),
                                        &o_cancel_state);
      {

        sqc_msg_debug(10, "enter: %s\n",
                         (is_canceled == true) ? "canceled" : "exit");

        s_cancel_lock(thd);
        {
          thd->m_is_canceled = is_canceled;
        }
        s_cancel_unlock(thd);

        s_op_lock(thd);
        {
          thd->m_result_code = rcode;
        }
        s_op_unlock(thd);

        s_wait_lock(thd);
        {
          if (thd->m_final_proc != NULL) {
            if (likely(thd->m_n_finalized_count == 0)) {
              thd->m_final_proc(&thd, is_canceled, thd->m_arg);
            } else {
              sqc_msg_warning("the thread is already %s, count "
                                 PFSZ(u) " (now %s)\n",
                                 (thd->m_is_canceled == false) ?
                                 "exit" : "canceled",
                                 thd->m_n_finalized_count,
                                 (is_canceled == false) ?
                                 "exit" : "canceled");
            }
          }
          thd->m_pthd = SQC_INVALID_THREAD;
          do_autodelete = thd->m_do_autodelete;
          thd->m_is_finalized = true;
          thd->m_is_activated = false;
          thd->m_n_finalized_count++;
          (void)sqc_cond_notify(&(thd->m_wait_cond), true);
        }
        s_wait_unlock(thd);

        (void)sqc_cond_notify(&(thd->m_finalize_cond), true);

      }
      (void)sqc_mutex_leave_critical(&(thd->m_finalize_lock),
                                        o_cancel_state);

      if (do_autodelete == true) {
        sqc_thread_destroy(&thd);
      }

      /*
       * NOTE:
       *
       *	Don't call pthread_exit() at here because now it
       *	triggers the cancel handler as of current cancellation
       *	handling.
       */
    }
  }
}


static inline void
s_delete(sqc_thread_t thd) {
  if (thd != NULL) {
    if (s_is_thd(thd) == true) {
      if (thd->m_is_allocd == true ||
          s_is_alloc_marked(thd) == true) {
        sqc_msg_debug(10, "free %p\n", (void *)thd);
        free((void *)thd);
      } else {
        if (sqc_heapcheck_is_in_heap((const void *)thd) == true) {
          sqc_msg_debug(10, "%p is a thread object NOT allocated by the "
                           "sqc_thread_create(). If you want to free(3) "
                           "this by calling the sqc_thread_destroy(), "
                           "call sqc_thread_free_when_destroy(%p).\n",
                           (void *)thd, (void *)thd);
        } else {
          sqc_msg_debug(10, "A thread was created in an address %p and it "
                           "seems not in the heap area.\n", (void *)thd);
        }
      }
      s_delete_thd(thd);
    }
  }
}


static inline void
s_destroy(sqc_thread_t *thdptr, bool is_clean_finish) {
  if (thdptr != NULL && *thdptr != NULL) {

    s_wait_lock(*thdptr);
    {

      if (is_clean_finish == true) {

        if ((*thdptr)->m_is_destroying == false) {
          (*thdptr)->m_is_destroying = true;

          if ((*thdptr)->m_freeup_proc != NULL) {
            (*thdptr)->m_freeup_proc(thdptr, (*thdptr)->m_arg);
          }
        }

      }

      if ((*thdptr)->m_cpusetptr != NULL) {
        CPU_FREE((*thdptr)->m_cpusetptr);
        (*thdptr)->m_cpusetptr = NULL;
      }

      if ((*thdptr)->m_op_lock != NULL) {
        (void)sqc_mutex_destroy(&((*thdptr)->m_op_lock));
        (*thdptr)->m_op_lock = NULL;
      }
      if ((*thdptr)->m_cancel_lock != NULL) {
        (void)sqc_mutex_destroy(&((*thdptr)->m_cancel_lock));
        (*thdptr)->m_cancel_lock = NULL;
      }
      if ((*thdptr)->m_finalize_lock != NULL) {
        (void)sqc_mutex_destroy(&((*thdptr)->m_finalize_lock));
        (*thdptr)->m_finalize_lock = NULL;
      }
      if ((*thdptr)->m_wait_cond != NULL) {
        (void)sqc_cond_destroy(&((*thdptr)->m_wait_cond));
        (*thdptr)->m_wait_cond = NULL;
      }
      if ((*thdptr)->m_startup_cond != NULL) {
        (void)sqc_cond_destroy(&((*thdptr)->m_startup_cond));
        (*thdptr)->m_startup_cond = NULL;
      }
      if ((*thdptr)->m_finalize_cond != NULL) {
        (void)sqc_cond_destroy(&((*thdptr)->m_finalize_cond));
        (*thdptr)->m_finalize_cond = NULL;
      }

    }
    s_wait_unlock(*thdptr);

    if ((*thdptr)->m_wait_lock != NULL) {
      (void)sqc_mutex_destroy(&((*thdptr)->m_wait_lock));
    }

    s_delete(*thdptr);
  }
}





static void
s_pthd_cancel_handler(void *ptr) {
  if (ptr != NULL) {
    sqc_thread_t thd = (sqc_thread_t)ptr;
    s_finalize(thd, true, SQC_RESULT_INTERRUPTED);
  }
}


static void *
s_pthd_entry_point(void *ptr) {
  if (likely(ptr != NULL)) {

    pthread_cleanup_push(s_pthd_cancel_handler, ptr);
    {
      int o_cancel_state;
      sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
      sqc_thread_t thd = (sqc_thread_t)ptr;
      const sqc_thread_t *tptr =
        (const sqc_thread_t *)&thd;

      (void)sqc_mutex_enter_critical(&(thd->m_wait_lock), &o_cancel_state);
      {

        s_cancel_lock(thd);
        {
          thd->m_is_canceled = false;
        }
        s_cancel_unlock(thd);

        thd->m_is_finalized = false;
        thd->m_is_activated = true;
        thd->m_is_started = true;
        (void)sqc_cond_notify(&(thd->m_startup_cond), true);

        /*
         * The notifiction is done and then the parent thread send us
         * "an ACK". Wait for it.
         */
      sync_done_check:
        mbar();
        if (thd->m_startup_sync_done == false) {
          ret = sqc_cond_wait(&(thd->m_startup_cond),
                                 &(thd->m_wait_lock),
                                 -1);

          if (ret == SQC_RESULT_OK) {
            goto sync_done_check;
          } else {
            sqc_perror(ret);
            sqc_msg_error("startup synchronization failed with the"
                             "parent thread.\n");
          }
        }

      }
      (void)sqc_mutex_leave_critical(&(thd->m_wait_lock), o_cancel_state);

      ret = thd->m_main_proc(tptr, thd->m_arg);

      /*
       * NOTE:
       *
       *	This very moment this thread could be cancelled and no
       *	correct result is set. Even it is impossible to
       *	prevent it completely, try to prevent it anyway.
       */
      (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &o_cancel_state);

      /*
       * We got through anyhow.
       */
      s_finalize(thd, false, ret);

      /*
       * NOTE
       *
       *	Don't call pthread_exit() at here or the cancellation
       *	handler will be called.
       */
    }
    pthread_cleanup_pop(0);

  }

  return NULL;
}





sqc_result_t
sqc_thread_create_with_size(sqc_thread_t *thdptr,
                               size_t alloc_size,
                               sqc_thread_main_proc_t mainproc,
                               sqc_thread_finalize_proc_t finalproc,
                               sqc_thread_freeup_proc_t freeproc,
                               const char *name,
                               void *arg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      mainproc != NULL) {
    sqc_thread_t thd;
    size_t sz = alloc_size;
    bool is_allocd = false;

    if (*thdptr == NULL) {
      if (sz < DEFAULT_THREAD_ALLOC_SZ) {
        sz = DEFAULT_THREAD_ALLOC_SZ;
      }
      thd = (sqc_thread_t)malloc(sz);
      if (thd != NULL) {
        (void)memset(thd, 0, sz);
        *thdptr = thd;
        s_alloc_mark_thd(thd);
      } else {
        *thdptr = NULL;
        ret = SQC_RESULT_NO_MEMORY;
        goto done;
      }
      is_allocd = true;
    } else {
      thd = *thdptr;
    }

    ret = s_initialize(thd, is_allocd,
                       mainproc, finalproc, freeproc, name, arg);
    if (ret != SQC_RESULT_OK) {
      s_destroy(&thd, false);
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}


sqc_result_t
sqc_thread_create(sqc_thread_t *thdptr,
                     sqc_thread_main_proc_t mainproc,
                     sqc_thread_finalize_proc_t finalproc,
                     sqc_thread_freeup_proc_t freeproc,
                     const char *name,
                     void *arg) {
  return sqc_thread_create_with_size(thdptr, DEFAULT_THREAD_ALLOC_SZ,
                                        mainproc, finalproc, freeproc,
                                        name, arg);
}


sqc_result_t
sqc_thread_start(const sqc_thread_t *thdptr,
                    bool autodelete) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL) {

    if (s_is_thd(*thdptr) == true) {

      int st;

      s_wait_lock(*thdptr);
      {
        if ((*thdptr)->m_is_activated == false) {
          (*thdptr)->m_do_autodelete = autodelete;
          errno = 0;
          (*thdptr)->m_pthd = SQC_INVALID_THREAD;
          if ((st = pthread_create((pthread_t *)&((*thdptr)->m_pthd),
                                   &s_attr,
                                   s_pthd_entry_point,
                                   (void *)*thdptr)) == 0) {
#ifdef HAVE_PTHREAD_SETNAME_NP
            if (IS_VALID_STRING((*thdptr)->m_name) == true) {
              (void)pthread_setname_np((*thdptr)->m_pthd, (*thdptr)->m_name);
            }
#endif /* HAVE_PTHREAD_SETNAME_NP */
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
            if ((*thdptr)->m_cpusetptr != NULL &&
                (st =
                   pthread_setaffinity_np((*thdptr)->m_pthd,
                                          s_cpu_set_sz,
                                          (*thdptr)->m_cpusetptr)) != 0) {
              int s_errno = errno;
              errno = st;
              sqc_perror(SQC_RESULT_POSIX_API_ERROR);
              sqc_msg_warning("Can't set cpu affinity.\n");
              errno = s_errno;
            }
#endif /* HAVE_PTHREAD_SETAFFINITY_NP */
            (*thdptr)->m_is_activated = false;
            (*thdptr)->m_is_finalized = false;
            (*thdptr)->m_is_destroying = false;
            (*thdptr)->m_is_canceled = false;
            (*thdptr)->m_is_started = false;
            (*thdptr)->m_startup_sync_done = false;
            (*thdptr)->m_n_finalized_count = 0LL;
            mbar();

            /*
             * Wait the spawned thread starts.
             */

          startcheck:
            mbar();
            if ((*thdptr)->m_is_started == false) {

              /*
               * Note that very here, very this moment the spawned
               * thread starts to run since this thread sleeps via
               * calling of the pthread_cond_wait() that implies
               * release of the lock.
               */

              ret = sqc_cond_wait(&((*thdptr)->m_startup_cond),
                                     &((*thdptr)->m_wait_lock),
                                     -1);

              if (ret == SQC_RESULT_OK) {
                goto startcheck;
              } else {
                goto unlock;
              }
            }

            /*
             * The newly created thread notified its start. Then "send
             * an ACK" to the thread to notify that the thread can do
             * anything, including its deletion.
             */
            (*thdptr)->m_startup_sync_done = true;
            (void)sqc_cond_notify(&((*thdptr)->m_startup_cond), true);

            ret = SQC_RESULT_OK;

          } else {
            errno = st;
            ret = SQC_RESULT_POSIX_API_ERROR;
          }
        } else {
          ret = SQC_RESULT_ALREADY_EXISTS;
        }
      }
    unlock:
      s_wait_unlock(*thdptr);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_thread_cancel(const sqc_thread_t *thdptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL) {
    if (getpid() == (*thdptr)->m_creator_pid) {
      if (s_is_thd(*thdptr) == true) {

        s_wait_lock(*thdptr);
        {
          if ((*thdptr)->m_is_activated == true) {

            s_cancel_lock(*thdptr);
            {
              if ((*thdptr)->m_is_canceled == false &&
                  (*thdptr)->m_pthd != SQC_INVALID_THREAD) {
                int st;

                errno = 0;
                if ((st = pthread_cancel((*thdptr)->m_pthd)) == 0) {
                  ret = SQC_RESULT_OK;
                } else {
                  errno = st;
                  ret = SQC_RESULT_POSIX_API_ERROR;
                }
              } else {
                ret = SQC_RESULT_OK;
              }
            }
            s_cancel_unlock(*thdptr);

          } else {
            ret = SQC_RESULT_OK;
          }
        }
        s_wait_unlock(*thdptr);

        /*
         * Very here, very this moment the s_finalize() would start to
         * run.
         */

      } else {
        ret = SQC_RESULT_INVALID_OBJECT;
      }
    } else {
      ret = SQC_RESULT_NOT_OWNER;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_thread_wait(const sqc_thread_t *thdptr,
                   sqc_chrono_t nsec) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL) {
    if (getpid() == (*thdptr)->m_creator_pid) {
      if (s_is_thd(*thdptr) == true) {
        if ((*thdptr)->m_do_autodelete == false) {

          int o_cancel_state;

          (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,
                                       &o_cancel_state);
          {

            s_wait_lock(*thdptr);
            {
            waitcheck:
              mbar();
              if ((*thdptr)->m_is_activated == true) {
                ret = sqc_cond_wait(&((*thdptr)->m_wait_cond),
                                       &((*thdptr)->m_wait_lock),
                                       nsec);
                if (ret == SQC_RESULT_OK) {
                  goto waitcheck;
                }
              } else {
                ret = SQC_RESULT_OK;
              }
            }
            s_wait_unlock(*thdptr);

            if ((*thdptr)->m_is_started == true) {

              (void)sqc_mutex_lock(&((*thdptr)->m_finalize_lock));
              {
                mbar();
              finalcheck:
                if ((*thdptr)->m_n_finalized_count == 0) {
                  ret = sqc_cond_wait(&((*thdptr)->m_finalize_cond),
                                         &((*thdptr)->m_finalize_lock),
                                         nsec);
                  if (ret == SQC_RESULT_OK) {
                    goto finalcheck;
                  }
                }
              }
              (void)sqc_mutex_unlock(&((*thdptr)->m_finalize_lock));

            }

          }
          (void)pthread_setcancelstate(o_cancel_state, NULL);

        } else {
          ret = SQC_RESULT_NOT_OPERATIONAL;
        }
      } else {
        ret = SQC_RESULT_INVALID_OBJECT;
      }
    } else {
      ret = SQC_RESULT_NOT_OWNER;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
sqc_thread_destroy(sqc_thread_t *thdptr) {
  if (thdptr != NULL &&
      *thdptr != NULL &&
      s_is_thd(*thdptr) == true) {
    (void)sqc_thread_cancel(thdptr);
    (void)sqc_thread_wait(thdptr, -1);

    s_destroy(thdptr, true);
    *thdptr = NULL;
  }
}


static inline sqc_result_t
s_get_pthdid(const sqc_thread_t *thdptr,
             pthread_t *tidptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if ((*thdptr)->m_is_started == true) {
    *tidptr = (*thdptr)->m_pthd;
    if ((*thdptr)->m_pthd != SQC_INVALID_THREAD) {
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_ALREADY_HALTED;
    }
  } else {
    ret = SQC_RESULT_NOT_STARTED;
  }

  return ret;
}


sqc_result_t
sqc_thread_get_pthread_id(const sqc_thread_t *thdptr,
                             pthread_t *tidptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL &&
      tidptr != NULL) {
    if (s_is_thd(*thdptr) == true) {
      int o_cancel_state;
      *tidptr = SQC_INVALID_THREAD;

      (void)pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &o_cancel_state);
      {

        s_wait_lock(*thdptr);
        {

          ret = s_get_pthdid(thdptr, tidptr);

        }
        s_wait_unlock(*thdptr);

      }
      (void)pthread_setcancelstate(o_cancel_state, NULL);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_thread_set_cpu_affinity(const sqc_thread_t *thdptr,
                               int cpu) {
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (thdptr != NULL && *thdptr != NULL) {
    if (s_is_thd(*thdptr) == true) {
      pthread_t tid;

      s_wait_lock(*thdptr);
      {
        if (likely((ret = s_cpu_set_cpu(&((*thdptr)->m_cpusetptr), cpu)) ==
                   SQC_RESULT_OK)) {
          ret = s_get_pthdid(thdptr, &tid);
          if (ret == SQC_RESULT_OK) {
            int st;
            if ((st = pthread_setaffinity_np(tid,
                                             s_cpu_set_sz,
                                             (*thdptr)->m_cpusetptr)) == 0) {
              ret = SQC_RESULT_OK;
            } else {
              errno = st;
              ret = SQC_RESULT_POSIX_API_ERROR;
            }
          } else {
            if (ret == SQC_RESULT_NOT_STARTED) {
              ret = SQC_RESULT_OK;
            }
          }
        }
      }
      s_wait_unlock(*thdptr);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
#else
  (void)thdptr;
  (void)cpu;
  return SQC_RESULT_OK;
#endif /* HAVE_PTHREAD_SETAFFINITY_NP */
}


sqc_result_t
sqc_thread_set_numa_node_affinity(const sqc_thread_t *thdptr,
                                     int node) {
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (thdptr != NULL && *thdptr != NULL) {
    if (s_is_thd(*thdptr) == true) {
      pthread_t tid;

      s_wait_lock(*thdptr);
      {
        if (likely((ret = s_cpu_set_numa_node(&((*thdptr)->m_cpusetptr),
                                              node)) == SQC_RESULT_OK)) {
          ret = s_get_pthdid(thdptr, &tid);
          if (ret == SQC_RESULT_OK) {
            int st;
            if ((st = pthread_setaffinity_np(tid,
                                             s_cpu_set_sz,
                                             (*thdptr)->m_cpusetptr)) == 0) {
              ret = SQC_RESULT_OK;
            } else {
              errno = st;
              ret = SQC_RESULT_POSIX_API_ERROR;
            }
          } else {
            if (ret == SQC_RESULT_NOT_STARTED) {
              ret = SQC_RESULT_OK;
            }
          }
        }
      }
      s_wait_unlock(*thdptr);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
#else
  (void)thdptr;
  (void)cpu;
  return SQC_RESULT_OK;
#endif /* HAVE_PTHREAD_SETAFFINITY_NP */
}


sqc_result_t
sqc_thread_set_cpu_affinity_any(const sqc_thread_t *thdptr) {
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (thdptr != NULL && *thdptr != NULL) {
    if (s_is_thd(*thdptr) == true) {
      pthread_t tid;

      s_wait_lock(*thdptr);
      {
        if (likely((ret = s_cpu_set_any(&((*thdptr)->m_cpusetptr))) ==
                   SQC_RESULT_OK)) {
          ret = s_get_pthdid(thdptr, &tid);
          if (ret == SQC_RESULT_OK) {
            int st;
            if ((st = pthread_setaffinity_np(tid,
                                             s_cpu_set_sz,
                                             (*thdptr)->m_cpusetptr)) == 0) {
              ret = SQC_RESULT_OK;
            } else {
              errno = st;
              ret = SQC_RESULT_POSIX_API_ERROR;
            }
          } else {
            if (ret == SQC_RESULT_NOT_STARTED) {
              ret = SQC_RESULT_OK;
            }
          }
        }
      }
      s_wait_unlock(*thdptr);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
#else
  (void)thdptr;
  (void)cpu;
  return SQC_RESULT_OK;
#endif /* HAVE_PTHREAD_SETAFFINITY_NP */
}


sqc_result_t
sqc_thread_get_cpu_affinity(const sqc_thread_t *thdptr) {
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (thdptr != NULL && *thdptr != NULL) {
    if (s_is_thd(*thdptr) == true) {
      pthread_t tid;

      s_wait_lock(*thdptr);
      {

        if ((ret = s_get_pthdid(thdptr, &tid)) == SQC_RESULT_OK) {
          int st;
          cpu_set_t *cur_set = NULL;

          if (likely((ret = s_cpu_set_alloc(&cur_set, true)) ==
                     SQC_RESULT_OK)) {
            if ((st = pthread_getaffinity_np(tid, s_cpu_set_sz, cur_set))
                == 0) {
              size_t i;
              sqc_result_t cpu = -INT_MAX;

              for (i = 0; i < s_n_cpus; i++) {
                if (CPU_ISSET_S(i, s_cpu_set_sz, cur_set)) {
                  cpu = (sqc_result_t)i;
                  break;
                }
              }

              if (cpu != -INT_MAX) {

#if SIZEOF_PTHREAD_T == SIZEOF_INT64_T
#define TIDFMT "0x" SQCIDS(016, x)
#elif SIZEOF_PTHREAD_T == SIZEOF_INT
#define TIDFMT "0x" SQCIDS(08, x)
#endif /* SIZEOF_PTHREAD_T == SIZEOF_INT64_T ... */

                if ((*thdptr)->m_cpusetptr != NULL &&
                    (!(CPU_ISSET_S((size_t)cpu, s_cpu_set_sz,
                                   (*thdptr)->m_cpusetptr)))) {
                  const char *name =
                    (IS_VALID_STRING((*thdptr)->m_name) == true) ?
                    (*thdptr)->m_name : "???";


                  sqc_msg_warning("Thread " TIDFMT " \"%s\" is running on "
                                     "CPU %ld, but is not specified to run "
                                     "on it.\n",
                                     tid, name, cpu);
                }

                ret = (sqc_result_t)cpu;

              } else {
                sqc_msg_error("Thread " TIDFMT " is running on unknown "
                                 "CPU??\n", tid);
                ret = SQC_RESULT_POSIX_API_ERROR;
              }

#undef TIDFMT

            } else {	/* (st = pthread_getaffinity_np(...)) ... */
              errno = st;
              ret = SQC_RESULT_POSIX_API_ERROR;
            }

            CPU_FREE(cur_set);
            cur_set = NULL;
          }		/* (ret = s_cpu_set_alloc(...)) ... */

        } else {	/* (ret = s_get_pthdid(thdptr, &tid)) ... */

          if (ret == SQC_RESULT_NOT_STARTED) {
            if ((*thdptr)->m_cpusetptr != NULL) {
              size_t i;
              sqc_result_t cpu = -INT_MAX;

              for (i = 0; i < s_n_cpus; i++) {
                if (CPU_ISSET_S(i, s_cpu_set_sz,
                                (*thdptr)->m_cpusetptr)) {
                  cpu = (sqc_result_t)i;
                  break;
                }
              }

              if (cpu != -INT_MAX) {
                ret = (sqc_result_t)cpu;
              } else {
                ret = SQC_RESULT_NOT_DEFINED;
              }
            } else {
              ret = SQC_RESULT_NOT_DEFINED;
            }
          }

        }

      }
      s_wait_unlock(*thdptr);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
#else
  (void)thdptr;
  return 0;
#endif /* HAVE_PTHREAD_SETAFFINITY_NP */
}


sqc_result_t
sqc_thread_set_result_code(const sqc_thread_t *thdptr,
                              sqc_result_t code) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL) {

    if (s_is_thd(*thdptr) == true) {

      s_op_lock(*thdptr);
      {
        (*thdptr)->m_result_code = code;
        ret = SQC_RESULT_OK;
      }
      s_op_unlock(*thdptr);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_thread_get_result_code(const sqc_thread_t *thdptr,
                              sqc_result_t *codeptr,
                              sqc_chrono_t nsec) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL &&
      codeptr != NULL) {

    *codeptr = SQC_RESULT_ANY_FAILURES;

    if (s_is_thd(*thdptr) == true) {
      if ((ret = sqc_thread_wait(thdptr, nsec)) == SQC_RESULT_OK) {

        s_op_lock(*thdptr);
        {
          *codeptr = (*thdptr)->m_result_code;
          ret = SQC_RESULT_OK;
        }
        s_op_unlock(*thdptr);

      }
    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_thread_is_canceled(const sqc_thread_t *thdptr,
                          bool *retptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL &&
      retptr != NULL) {

    *retptr = false;

    if (s_is_thd(*thdptr) == true) {

      s_cancel_lock(*thdptr);
      {
        *retptr = (*thdptr)->m_is_canceled;
        ret = SQC_RESULT_OK;
      }
      s_cancel_unlock(*thdptr);

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_thread_free_when_destroy(sqc_thread_t *thdptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL) {

    if (s_is_thd(*thdptr) == true) {
      s_alloc_mark_thd(*thdptr);
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_thread_is_valid(const sqc_thread_t *thdptr,
                       bool *retptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (thdptr != NULL &&
      *thdptr != NULL &&
      retptr != NULL) {

    if (s_is_thd(*thdptr) == true) {
      *retptr = true;
      ret = SQC_RESULT_OK;
    } else {
      *retptr = false;
      ret = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_thread_get_name(const sqc_thread_t *thdptr, char *buf, size_t n) {
  if (likely(thdptr != NULL && *thdptr != NULL && buf != NULL && n > 0 &&
             IS_VALID_STRING((*thdptr)->m_name) == true)) {

    s_op_lock(*thdptr);
    {

      (void)snprintf(buf, n, "%s", (*thdptr)->m_name);

    }
    s_op_unlock(*thdptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
sqc_thread_set_name(const sqc_thread_t *thdptr, const char *name) {
  if (likely(thdptr != NULL && *thdptr != NULL &&
             IS_VALID_STRING(name) == true)) {

    s_op_lock(*thdptr);
    {

      (void)snprintf((*thdptr)->m_name, sizeof((*thdptr)->m_name), "%s", name);
#ifdef HAVE_PTHREAD_SETNAME_NP
      if ((*thdptr)->m_pthd != SQC_INVALID_THREAD) {
        (void)pthread_setname_np((*thdptr)->m_pthd, (*thdptr)->m_name);
      }
#endif /* HAVE_PTHREAD_SETNAME_NP */

    }
    s_op_unlock(*thdptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


void
sqc_thread_atfork_child(const sqc_thread_t *thdptr) {
  if (thdptr != NULL &&
      *thdptr != NULL) {

    if (s_is_thd(*thdptr) == true) {
      (void)sqc_mutex_reinitialize(&((*thdptr)->m_op_lock));
      (void)sqc_mutex_reinitialize(&((*thdptr)->m_wait_lock));
      (void)sqc_mutex_reinitialize(&((*thdptr)->m_cancel_lock));
      (void)sqc_mutex_reinitialize(&((*thdptr)->m_finalize_lock));
    }
  }
}


void
sqc_thread_module_initialize(void) {
  s_init();
}


void
sqc_thread_module_finalize(void) {
  s_final();
}
