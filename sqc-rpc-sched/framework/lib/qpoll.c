#include "sqc_apis.h"
#include "qmuxer_types.h"
#include "qmuxer_internal.h"





static inline sqc_result_t
s_poll_initialize(sqc_qmuxer_poll_t mp,
                  sqc_bbq_t bbq,
                  sqc_qmuxer_poll_event_t type) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (mp != NULL &&
      bbq != NULL &&
      IS_VALID_POLL_TYPE(type) == true) {
    (void)memset((void *)mp, 0, sizeof(*mp));
    mp->m_bbq = bbq;
    mp->m_type = type;
    mp->m_q_size = 0;
    mp->m_q_rem_capacity = 0;

    ret = SQC_RESULT_OK;

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_poll_destroy(sqc_qmuxer_poll_t mp) {
  free((void *)mp);
}





sqc_result_t
sqc_qmuxer_poll_create(sqc_qmuxer_poll_t *mpptr,
                          sqc_bbq_t bbq,
                          sqc_qmuxer_poll_event_t type) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (mpptr != NULL) {
    *mpptr = (sqc_qmuxer_poll_t)malloc(sizeof(**mpptr));
    if (*mpptr == NULL) {
      ret = SQC_RESULT_NO_MEMORY;
      goto done;
    }
    ret = s_poll_initialize(*mpptr, bbq, type);
    if (ret != SQC_RESULT_OK) {
      s_poll_destroy(*mpptr);
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}


void
sqc_qmuxer_poll_destroy(sqc_qmuxer_poll_t *mpptr) {
  if (mpptr != NULL) {
    s_poll_destroy(*mpptr);
  }
}


sqc_result_t
sqc_qmuxer_poll_reset(sqc_qmuxer_poll_t *mpptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL) {
    (*mpptr)->m_q_size = 0;
    (*mpptr)->m_q_rem_capacity = 0;

    ret = SQC_RESULT_OK;

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_qmuxer_poll_set_queue(sqc_qmuxer_poll_t *mpptr,
                             sqc_bbq_t bbq) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL) {
    (*mpptr)->m_bbq = bbq;
    if (bbq != NULL) {
      bool tmp = false;
      ret = sqc_bbq_is_operational(&bbq, &tmp);
      if (ret == SQC_RESULT_OK && tmp == true) {
        ret = SQC_RESULT_OK;
      } else {
        /*
         * rather SQC_RESULT_INVALID_ARGS ?
         */
        ret = SQC_RESULT_NOT_OPERATIONAL;
      }
    } else {
      ret = SQC_RESULT_OK;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_qmuxer_poll_get_queue(sqc_qmuxer_poll_t *mpptr,
                             sqc_bbq_t *bbqptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL &&
      bbqptr != NULL) {
    *bbqptr = (*mpptr)->m_bbq;
    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_qmuxer_poll_set_type(sqc_qmuxer_poll_t *mpptr,
                            sqc_qmuxer_poll_event_t type) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL &&
      IS_VALID_POLL_TYPE(type) == true) {

    if ((*mpptr)->m_bbq != NULL) {
      (*mpptr)->m_type = type;
    } else {
      (*mpptr)->m_type = SQC_QMUXER_POLL_UNKNOWN;
    }
    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_qmuxer_poll_size(sqc_qmuxer_poll_t *mpptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL) {
    if ((*mpptr)->m_bbq != NULL) {
      ret = (*mpptr)->m_q_size;
    } else {
      ret = 0;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_qmuxer_poll_remaining_capacity(sqc_qmuxer_poll_t *mpptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (mpptr != NULL &&
      *mpptr != NULL) {
    if ((*mpptr)->m_bbq != NULL) {
      ret = (*mpptr)->m_q_rem_capacity;
    } else {
      ret = 0;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}

