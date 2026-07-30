#pragma once





/**
 *	@file	sqc_poolable.h
 */





__BEGIN_DECLS





typedef struct sqc_poolable_record	*sqc_poolable_t;


typedef enum {
  SQC_POOLABLE_STATE_UNKNOWN = 0,
  SQC_POOLABLE_STATE_CONSTRUCTED = 1,
  SQC_POOLABLE_STATE_OPERATIONAL = 2,
  SQC_POOLABLE_STATE_SHUTDOWNING = 3,
  SQC_POOLABLE_STATE_CANCELLING = 4,
  SQC_POOLABLE_STATE_NOT_OPERATIONAL = 5,
} sqc_poolable_state_t;


typedef sqc_result_t
(*sqc_poolable_construct_proc_t)(sqc_poolable_t *self,
                                    void *carg);

typedef sqc_result_t
(*sqc_poolable_setup_proc_t)(sqc_poolable_t *self);

typedef sqc_result_t
(*sqc_poolable_shutdown_proc_t)(sqc_poolable_t *self,
                                   shutdown_grace_level_t lvl);

typedef sqc_result_t
(*sqc_poolable_cancel_proc_t)(sqc_poolable_t *self);

typedef sqc_result_t
(*sqc_poolable_teardown_proc_t)(sqc_poolable_t *self,
                                   bool is_cancelled);

typedef sqc_result_t
(*sqc_poolable_wait_proc_t)(sqc_poolable_t *self, sqc_chrono_t nsec);

typedef void
(*sqc_poolable_destruct_proc_t)(sqc_poolable_t *self);


typedef struct sqc_poolable_methods_record {
  sqc_poolable_construct_proc_t m_construct;
  sqc_poolable_setup_proc_t m_setup;
  sqc_poolable_shutdown_proc_t m_shutdown;
  sqc_poolable_cancel_proc_t m_cancel;
  sqc_poolable_teardown_proc_t m_teardown;
  sqc_poolable_wait_proc_t m_wait;
  sqc_poolable_destruct_proc_t m_destruct;
} sqc_poolable_methods_record;
typedef sqc_poolable_methods_record *sqc_poolable_methods_t;





sqc_result_t
sqc_poolable_create_with_size(sqc_poolable_t *pptr,
                                 bool is_executor,
                                 sqc_poolable_methods_t m,
                                 void *carg, size_t sz);


sqc_result_t
sqc_poolable_setup(sqc_poolable_t *pptr);


sqc_result_t
sqc_poolable_shutdown(sqc_poolable_t *pptr, shutdown_grace_level_t lvl);


sqc_result_t
sqc_poolable_cancel(sqc_poolable_t *pptr);


sqc_result_t
sqc_poolable_teardown(sqc_poolable_t *pptr, bool is_cancelled);


sqc_result_t
sqc_poolable_wait(sqc_poolable_t *pptr, sqc_chrono_t nsec);


void
sqc_poolable_destroy(sqc_poolable_t *pptr);


sqc_result_t
sqc_poolable_is_operational(sqc_poolable_t *pptr, bool *bptr);


sqc_result_t
sqc_poolable_get_status(sqc_poolable_t *pptr,
                           sqc_poolable_state_t *st);





__END_DECLS
