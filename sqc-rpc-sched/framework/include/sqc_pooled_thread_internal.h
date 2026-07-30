#pragma once





typedef struct sqc_task_runner_thread_record {
  sqc_thread_record m_thd;

  sqc_poolable_t m_pobj;

  sqc_mutex_t m_lck;
  sqc_cond_t m_cnd;
  volatile sqc_task_runner_state_t m_state;
  volatile bool m_is_shutdown_requested;
  volatile bool m_is_got_a_task;
  shutdown_grace_level_t m_shutdown_lvl;
  bool m_is_finished;
  bool m_is_cancelled;

  sqc_task_t m_tsk;
  bool m_is_auto_delete;
} sqc_task_runner_thread_record;


typedef struct sqc_pooled_thread_record {
  sqc_poolable_record m_poolable;
  sqc_task_runner_thread_record m_trthd;
} sqc_pooled_thread_record;


/*
 * wraping a sqc_pool_record into a sqc_thread_pool_record to get a
 * warning on purpose when implicit type conversion.
 */
typedef struct sqc_thread_pool_record {
  sqc_pool_record m_pool;
  void *m_dummy;
} sqc_thread_pool_record;
