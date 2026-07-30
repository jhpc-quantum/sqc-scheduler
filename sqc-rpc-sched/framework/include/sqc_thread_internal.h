#ifndef __SQC_THREAD_INTERNAL_H__
#define __SQC_THREAD_INTERNAL_H__





typedef struct sqc_thread_record {
  volatile pthread_t m_pthd;
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
  /*
   * For pthread_setaffinity_np(3)
   */
  cpu_set_t *m_cpusetptr;
#else
  /*
   * Dummy.
   */
  void *m_cpusetptr;
#endif /* HAVE_PTHREAD_SETAFFINITY_NP */
  void *m_arg;

  bool m_is_allocd;

  char m_name[16];	/* max 16 characters. */

  pid_t m_creator_pid;

  sqc_thread_main_proc_t m_main_proc;
  sqc_thread_finalize_proc_t m_final_proc;
  sqc_thread_freeup_proc_t m_freeup_proc;

  sqc_mutex_t m_op_lock;
  sqc_mutex_t m_wait_lock;
  sqc_mutex_t m_cancel_lock;
  sqc_mutex_t m_finalize_lock;

  sqc_cond_t m_wait_cond;
  sqc_cond_t m_startup_cond;
  sqc_cond_t m_finalize_cond;

  sqc_result_t m_result_code;

  volatile bool m_is_started;
  volatile bool m_is_activated;
  volatile bool m_is_canceled;
  volatile bool m_is_finalized;
  volatile bool m_is_destroying;

  volatile bool m_do_autodelete;
  volatile bool m_startup_sync_done;
  volatile size_t m_n_finalized_count;

} sqc_thread_record;





#endif /* ! __SQC_THREAD_INTERNAL_H__ */
