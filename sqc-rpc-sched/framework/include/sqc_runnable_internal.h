#ifndef __SQC_RUNNABLE_INTERNAL_H__
#define __SQC_RUNNABLE_INTERNAL_H__





#include "sqc_runnable_funcs.h"





typedef struct sqc_runnable_record {
  sqc_runnable_proc_t m_func;
  void *m_arg;
  sqc_runnable_freeup_proc_t m_freeup_func;
  bool m_is_heap_allocd;
} sqc_runnable_record;





#endif /* ! __SQC_RUNNABLE_INTERNAL_H__ */
