#include "sqc_apis.h"





sqc_chrono_t
sqc_chrono_now(void) {
  sqc_chrono_t ret = 0;
  WHAT_TIME_IS_IT_NOW_IN_NSEC(ret);
  return ret;
}


sqc_result_t
sqc_chrono_to_timespec(struct timespec *dstptr,
                          sqc_chrono_t nsec) {
  if (dstptr != NULL) {
    NSEC_TO_TS(nsec, *dstptr);
    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
sqc_chrono_to_timeval(struct timeval *dstptr,
                         sqc_chrono_t nsec) {
  if (dstptr != NULL) {
    NSEC_TO_TV(nsec, *dstptr);
    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
sqc_chrono_from_timespec(sqc_chrono_t *dstptr,
                            const struct timespec *specptr) {
  if (dstptr != NULL &&
      specptr != NULL) {
    *dstptr = TS_TO_NSEC(*specptr);
    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
sqc_chrono_from_timeval(sqc_chrono_t *dstptr,
                           const struct timeval *specptr) {
  if (dstptr != NULL &&
      specptr != NULL) {
    *dstptr = TV_TO_NSEC(*specptr);
    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
sqc_chrono_nanosleep(sqc_chrono_t nsec,
                        sqc_chrono_t *remptr) {
  struct timespec t;
  struct timespec r;

  if (nsec < 0) {
    return SQC_RESULT_INVALID_ARGS;
  }

  NSEC_TO_TS(nsec, t);

  if (remptr == NULL) {
  retry:
    errno = 0;
    if (nanosleep(&t, &r) == 0) {
      return SQC_RESULT_OK;
    } else {
      if (errno == EINTR) {
        t = r;
        goto retry;
      } else {
        return SQC_RESULT_POSIX_API_ERROR;
      }
    }
  } else {
    errno = 0;
    if (nanosleep(&t, &r) == 0) {
      return SQC_RESULT_OK;
    } else {
      *remptr = TS_TO_NSEC(r);
      return SQC_RESULT_POSIX_API_ERROR;
    }
  }
}
