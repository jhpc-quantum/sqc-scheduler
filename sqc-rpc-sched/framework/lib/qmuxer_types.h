#ifndef __QMUXER_TYPES_H__
#define __QMUXER_TYPES_H__





typedef struct sqc_qmuxer_poll_record {
  sqc_bbq_t m_bbq;
  sqc_qmuxer_poll_event_t m_type;
  ssize_t m_q_size;
  ssize_t m_q_rem_capacity;
} sqc_qmuxer_poll_record;


typedef struct sqc_qmuxer_record {
  sqc_mutex_t m_lock;
  sqc_cond_t m_cond;
} sqc_qmuxer_record;





#endif /* __QMUXER_TYPES_H__ */
