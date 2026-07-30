#pragma once





typedef enum {
  SQC_POOL_STATE_UNKNOWN = 0,
  SQC_POOL_STATE_CONSTRUCTED = 1,
  SQC_POOL_STATE_OPERATIONAL = 2,
  SQC_POOL_STATE_SHUTTINGDOWN = 3,
  SQC_POOL_STATE_NOT_OPERATIONAL = 4,
} sqc_pool_state_t;


typedef struct sqc_pool_record {
  sqc_pool_type_t m_type;
  bool m_is_executor;
  sqc_pool_state_t m_state;
  uint64_t m_total_obj_size;

  char *m_name;

  sqc_mutex_t m_lck;
  sqc_cond_t m_cnd;
  sqc_cond_t m_awakened_cnd;
  volatile bool m_is_awakened;
  volatile bool m_is_cancelled;

  size_t m_n_waiters;

  size_t m_obj_idx;

  sqc_poolable_methods_record m_m;
  size_t m_pobj_size;

  size_t m_n_max;
  size_t m_n_cur;
  size_t m_n_free;

  sqc_poolable_t *m_objs;	/* size: m_n_max */

  /* For SQC_POOL_TYPE_QUEUE */
  sqc_bbq_t m_free_q;

} sqc_pool_record;
