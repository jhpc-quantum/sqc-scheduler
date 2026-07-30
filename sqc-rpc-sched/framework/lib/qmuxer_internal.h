#ifndef __QMUXER_INTERNAL_H__
#define __QMUXER_INTERNAL_H__





#define NEED_WAIT_READABLE(type) \
  (IS_BIT_SET(((int)(type)), ((int)SQC_QMUXER_POLL_READABLE)))

#define NEED_WAIT_WRITABLE(type) \
  (IS_BIT_SET(((int)(type)), ((int)SQC_QMUXER_POLL_WRITABLE)))

#define IS_VALID_POLL_TYPE(t)                                   \
  (((int)t > (int)SQC_QMUXER_POLL_UNKNOWN &&                \
    (int)t <= (int)SQC_QMUXER_POLL_BOTH) ? true : false)





void
qmuxer_notify(sqc_qmuxer_t qmx);


sqc_result_t
cbuffer_setup_for_qmuxer(sqc_cbuffer_t cb,
                         sqc_qmuxer_t qmx,
                         ssize_t *szptr,
                         ssize_t *remptr,
                         sqc_qmuxer_poll_event_t type,
                         bool is_pre);





#endif /* ! __QMUXER_INTERNAL_H__ */
