#ifndef __SQC_CALLOUT_INTERNAL_H__
#define __SQC_CALLOUT_INTERNAL_H__






/**
 *	@file	sqc_callout_internal.h
 */





#include "sqc_callout_task_state.h"
#include "sqc_callout_task_funcs.h"
#include "sqc_runnable_internal.h"





typedef struct sqc_callout_task_record {
  sqc_runnable_record m_runnable;
  TAILQ_ENTRY(sqc_callout_task_record) m_entry;

  sqc_mutex_t m_lock;	/** A recursive lock. */
  sqc_cond_t m_cond;	/** A cond, mainly for cancel sync. */

  volatile callout_task_state_t m_status;

  volatile size_t m_exec_ref_count;	/** 0 ... no one is executing;
                                            >0 ... someone is executing. */
  volatile size_t m_cancel_ref_count;	/** 0 ... no one is cancelling;
                                            >0 ... someone is cancelling. */

  const char *m_name;
  sqc_callout_task_proc_t m_proc;
  void *m_arg;
  sqc_callout_task_arg_freeup_proc_t m_freeproc;
  bool m_do_repeat;
  bool m_is_first;
  bool m_is_in_timed_q;	/** We need this since the TAILQ_* APIs don't
                            check that a specified entry is really in
                            a specified queue for the
                            insertion/removal. */
  bool m_is_in_bbq;	/** \b true ... the task is either in the
                            uegent Q or the idle Q. */
  sqc_chrono_t m_initial_delay_time;
  sqc_chrono_t m_interval_time;
  sqc_chrono_t m_last_abstime;
  sqc_chrono_t m_next_abstime;
} sqc_callout_task_record;





#endif /* ! __SQC_CALLOUT_INTERNAL_H__ */
