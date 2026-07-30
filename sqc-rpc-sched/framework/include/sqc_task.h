#pragma once





/**
 *	@file	sqc_task.h
 */





#define SQC_TASK_DELETE_CONTEXT_AFTER_EXEC	1
#define SQC_TASK_RELEASE_THREAD_AFTER_EXEC	2





__BEGIN_DECLS





typedef enum {
  SQC_TASK_STATE_UNKNOWN = 0,
  SQC_TASK_STATE_CONSTRUCTED = 1,
  SQC_TASK_STATE_ATTACHED = 2,
  SQC_TASK_STATE_SETUP = 3,
  SQC_TASK_STATE_RUNNING = 4,
  SQC_TASK_STATE_CLEAN_FINISHED = 5,
  SQC_TASK_STATE_HALTED = 6,
  SQC_TASK_STATE_CANCELLED = 7,
  SQC_TASK_STATE_RUNNER_SHUTDOWN = 8,
} sqc_task_state_t;


typedef struct sqc_task_record *sqc_task_t;


typedef sqc_result_t (*sqc_task_main_proc_t)(sqc_task_t *tptr);
typedef void (*sqc_task_finalize_proc_t)(sqc_task_t *tptr,
    bool is_cancelled);
typedef void (*sqc_task_freeup_proc_t)(sqc_task_t *tptr);





sqc_result_t
sqc_task_create(sqc_task_t *tptr, size_t sz, const char *name,
                   sqc_task_main_proc_t main_func,
                   sqc_task_finalize_proc_t finalize_func,
                   sqc_task_freeup_proc_t freeup_func);

sqc_result_t
sqc_task_set_cpu_affinity(const sqc_task_t *tptr, int cpu);


sqc_result_t
sqc_task_set_numa_node_affinity(const sqc_task_t *tptr, int node);


sqc_result_t
sqc_task_run(sqc_task_t *tptr, sqc_pooled_thread_t *ptptr, int flag);


void
sqc_task_finalize(sqc_task_t *tptr, bool is_cancelled);


sqc_result_t
sqc_task_wait(sqc_task_t *tptr, sqc_chrono_t to);


sqc_result_t
sqc_task_get_exit_code(sqc_task_t *tptr);


sqc_result_t
sqc_task_get_state(sqc_task_t *tptr,
                      sqc_task_state_t *stptr);


void
sqc_task_destroy(sqc_task_t *tptr);





__END_DECLS



