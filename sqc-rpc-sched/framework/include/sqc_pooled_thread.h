#pragma once





/**
 *	@file	sqc_pooled_thread.h
 */





__BEGIN_DECLS





typedef enum {
  SQC_TASK_RUNNER_STATE_UNKNOWN = 0,
  SQC_TASK_RUNNER_STATE_CREATED = 1,
  SQC_TASK_RUNNER_STATE_NOT_ASSIGNED = 2,
  SQC_TASK_RUNNER_STATE_ASSIGNED = 3,
  SQC_TASK_RUNNER_STATE_RUNNING = 4,
  SQC_TASK_RUNNER_STATE_FINISH = 5,
} sqc_task_runner_state_t;


typedef struct sqc_task_runner_thread_record *sqc_task_runner_thread_t;
typedef struct sqc_pooled_thread_record *sqc_pooled_thread_t;
typedef struct sqc_thread_pool_record *sqc_thread_pool_t;





sqc_result_t
sqc_thread_pool_create(sqc_thread_pool_t *pptr, const char *name,
                          size_t n);


sqc_result_t
sqc_thread_pool_acquire_thread(sqc_thread_pool_t *pptr,
                                  sqc_pooled_thread_t *ptptr,
                                  sqc_chrono_t to);


sqc_result_t
sqc_thread_pool_release_thread(sqc_pooled_thread_t *ptptr);


sqc_result_t
sqc_thread_pool_get_outstanding_thread_num(sqc_thread_pool_t *pptr);


sqc_result_t
sqc_thread_pool_wakeup(sqc_thread_pool_t *pptr, sqc_chrono_t to);


sqc_result_t
sqc_thread_pool_shutdown_all(sqc_thread_pool_t *pptr,
                                shutdown_grace_level_t lvl,
                                sqc_chrono_t to);


sqc_result_t
sqc_thread_pool_cancel_all(sqc_thread_pool_t *pptr);


sqc_result_t
sqc_thread_pool_wait_all(sqc_thread_pool_t *pptr, sqc_chrono_t to);


void
sqc_thread_pool_destroy(sqc_thread_pool_t *pptr);


sqc_result_t
sqc_thread_pool_get(const char *name, sqc_thread_pool_t *pptr);





__END_DECLS
