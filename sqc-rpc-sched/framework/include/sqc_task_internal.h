#pragma once





typedef struct sqc_task_record {
  sqc_task_state_t m_state;
  char *m_name;

  sqc_mutex_t m_lck;
  sqc_cond_t m_cnd;

  size_t m_total_obj_size;
  sqc_result_t m_exit_code;

  volatile bool m_is_started;
  volatile bool m_is_clean_finished;
  volatile bool m_is_runner_shutdown;
  volatile bool m_is_cancelled;

  volatile bool m_is_wait_done;

  sqc_task_main_proc_t m_main;
  sqc_task_finalize_proc_t m_finalize;
  sqc_task_freeup_proc_t m_freeup;

  int m_core_num;
  int m_numa_node_num;

  int m_flag;

  sqc_task_runner_thread_t m_tr;

  sqc_thread_t m_tmp_thd;
} sqc_task_record;
