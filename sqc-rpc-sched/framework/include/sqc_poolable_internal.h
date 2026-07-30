#pragma once





typedef struct sqc_poolable_record {
  sqc_mutex_t m_lck;
  sqc_cond_t m_cnd;

  sqc_pool_t m_pool;
  bool m_is_executor;
  uint64_t m_obj_idx;

  volatile bool m_is_used;
  volatile bool m_is_cancelled;
  volatile bool m_is_torndown;
  volatile bool m_is_wait_done;

  size_t m_total_obj_size;

  sqc_poolable_state_t m_st;

  sqc_result_t m_last_result;

  shutdown_grace_level_t m_shutdown_lvl;

  sqc_poolable_construct_proc_t m_construct;
  sqc_poolable_setup_proc_t m_setup;
  sqc_poolable_shutdown_proc_t m_shutdown;
  sqc_poolable_cancel_proc_t m_cancel;
  sqc_poolable_teardown_proc_t m_teardown;
  sqc_poolable_wait_proc_t m_wait;
  sqc_poolable_destruct_proc_t m_destruct;

} sqc_poolable_record;
