#include "sqc_apis.h"
#include "sqc_runnable_internal.h"





#define DEFAULT_RUNNABLE_ALLOC_SZ	(sizeof(sqc_runnable_record))





sqc_result_t
sqc_runnable_create(sqc_runnable_t *rptr,
                       size_t alloc_sz,
                       sqc_runnable_proc_t func,
                       void *arg,
                       sqc_runnable_freeup_proc_t freeup_func) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  bool is_heap_allocd = false;

  if (rptr != NULL &&
      func != NULL) {
    if (*rptr == NULL) {
      size_t sz = (DEFAULT_RUNNABLE_ALLOC_SZ > alloc_sz) ?
                  DEFAULT_RUNNABLE_ALLOC_SZ : alloc_sz;
      *rptr = (sqc_runnable_t)malloc(sz);
      if (*rptr != NULL) {
        is_heap_allocd = true;
      } else {
        ret = SQC_RESULT_NO_MEMORY;
        goto done;
      }
    }

    (*rptr)->m_func = func;
    (*rptr)->m_arg = arg;
    (*rptr)->m_freeup_func = freeup_func;
    (*rptr)->m_is_heap_allocd = is_heap_allocd;

    ret = SQC_RESULT_OK;

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

done:
  if (ret != SQC_RESULT_OK) {
    if (is_heap_allocd == true) {
      free((void *)(*rptr));
    }
    if (rptr != NULL) {
      *rptr = NULL;
    }
  }

  return ret;
}


void
sqc_runnable_destroy(sqc_runnable_t *rptr) {
  if (rptr != NULL) {
    if ((*rptr)->m_freeup_func != NULL) {
      ((*rptr)->m_freeup_func)(rptr);
    }
    if ((*rptr)->m_is_heap_allocd == true) {
      (void)free((void *)(*rptr));
    }
    *rptr = NULL;
  }
}


sqc_result_t
sqc_runnable_start(const sqc_runnable_t *rptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (rptr != NULL && *rptr != NULL) {
    ret = ((*rptr)->m_func)(rptr, (*rptr)->m_arg);
  }

  return ret;
}
