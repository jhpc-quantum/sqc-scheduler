#ifndef __SQC_CALLOUT_H__
#define __SQC_CALLOUT_H__





/**
 *	@file	sqc_callout.h
 */





#include "sqc_callout_task_state.h"
#include "sqc_callout_task_funcs.h"
#include "sqc_runnable.h"





#ifndef CALLOUT_TASK_T_DECLARED
typedef struct sqc_callout_task_record 	*sqc_callout_task_t;
#define CALLOUT_TASK_T_DECLARED
#endif /* ! CALLOUT_TASK_T_DECLARED */





__BEGIN_DECLS





/**
 * The signature of the callout handler idle function.
 *
 *	@param[in]	arg	An argument.
 *
 *	@retval	SQC_RESULT_OK	Succeeded and the callout master
 *					scheduler carries on.
 *	@retval <0			Failed and the callout master
 *					sceduler stops.
 */
typedef sqc_result_t	(*sqc_callout_idle_proc_t)(void *arg);


/**
 * The signature of callout handler idle function argument freeup functions.
 *
 *	@param[in]	arg	An argument.
 *
 */
typedef void	(*sqc_callout_idle_arg_freeup_proc_t)(void *arg);





/**
 * Initialize the callout handler
 *
 *	@param[in]	n_workers	A # of worker threads.
 *	@param[in]	proc	A function for idle time (NULL allowed.)
 *	@param[in]	arg	An argument for the \b proc. (NULL allowed.)
 *	@param[in] interval	An interval time (in nsec) to execute the
 *				\b proc.
 *	@param[in]	freeproc	A function to freeup the \b arg
 *					(NULL allowed.)
 *
 *	@retval	SQC_RESULT_OK		Succeeded.
 *      @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
 *      @retval SQC_RESULT_INVALID_ARGS     Failed, invalid args.
 *      @retval SQC_RESULT_ANY_FAILURES     Failed.
 */
sqc_result_t
sqc_callout_initialize_handler(size_t n_workers,
                                  sqc_callout_idle_proc_t proc,
                                  void *arg,
                                  sqc_chrono_t interval,
                                  sqc_callout_idle_arg_freeup_proc_t
                                  freeup);


/**
 * Finalize (stop) the callout handler.
 *
 * @details If the functionn is called, all the tasks submitted are
 * cancelled and its argument are freed up, then the callout module
 * itself is shutdown.
 */
void
sqc_callout_finalize_handler(void);


/**
 * Start the callout handler main event loop.
 *
 *	@retval	SQC_RESULT_OK		Succeeded.
 *      @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
 *      @retval SQC_RESULT_INVALID_ARGS     Failed, invalid args.
 *      @retval SQC_RESULT_ANY_FAILURES     Failed.
 */
sqc_result_t
sqc_callout_start_main_loop(void);


/**
 * Stop the callout handler main event loop.
 *
 *	@retval	SQC_RESULT_OK		Succeeded.
 *      @retval SQC_RESULT_ANY_FAILURES     Failed.
 *
 * @detils If the main event loop stops cleanly, the \b arg is freed
 * up by calling of the \b freeproc, which are specified by the \b
 * sqc_callout_initialize_handler(), if both the \b arg and \b
 * freeproc are non-NULL.
 */
sqc_result_t
sqc_callout_stop_main_loop(void);





/**
 * Create a callout task.
 *
 *	@param[out]	tptr	A pointer to a created task.
 *	@param[in]	sz	A memory allocation size for this object (in bytes, zero allowed.)
 *	@param[in]	name	A name of the task (NULL allowed.)
 *	@param[in]	proc	A task main function.
 *	@param[in]	arg	An argument for the \b proc (NULL allowed.)
 *	@param[in]	freeproc	A function to freeup the \b arg (NULL allowed.)
 *
 *	@retval	SQC_RESULT_OK		Succeeded.
 *      @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
 *      @retval SQC_RESULT_INVALID_ARGS     Failed, invalid args.
 *      @retval SQC_RESULT_ANY_FAILURES     Failed.
 *
 * @details This function and the \b sqc_callout_submit_task() are
 * not allowed to be called in the \b proc to make the task scheduler
 * simpler.
 */
sqc_result_t
sqc_callout_create_task(sqc_callout_task_t *tptr,
                           size_t sz,
                           const char *name,
                           sqc_callout_task_proc_t proc,
                           void *arg,
                           sqc_callout_task_arg_freeup_proc_t freeproc);

/**
 * Submit a callout task.
 *
 *	@param[in]	tptr	A pointer to a task.
 *	@param[in]	delay	A (relative) time when the task supposed to be initially executed: \b 0 : right now; \b <0 : "idle" time; \b >0 : within after the \b delay (in nsec.)
 *	@param[in]	interval	A (relative) time when the task supposed to be re-executed: \b >0 : within after the \b interval (in nsec.)
 *
 *	@retval	SQC_RESULT_OK		Succeeded.
 *      @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
 *      @retval SQC_RESULT_INVALID_ARGS     Failed, invalid args.
 *      @retval SQC_RESULT_ANY_FAILURES     Failed.
 *
 * @details the \b *tptr (the task itself), and the argument if
 * specified with non-NULL \b freearg for the
 * sqc_callout_create_task(), are destroyed when; 1) the \b proc
 * returned non-SQC_RESULT_OK. 2) when the \b proc finished if the
 * task is submitted as a "single-shot".
 *
 * @details If once a task, which is created with both the \b arg and
 * \b freeproc non-NULL, is submitted, don't freeup/cleanup the \b arg
 * explicitly or it causes a core dump.
 */
sqc_result_t
sqc_callout_submit_task(const sqc_callout_task_t *tptr,
                           sqc_chrono_t delay,
                           sqc_chrono_t interval);


/**
 * Cancel a submitted task.
 *
 *	@param[in]	tptr	A pointer to a task.
 *
 * @details \b If the \b *tptr has created with the non-NULL \b arg
 * and \b freeproc, calling this function implicitly free/clean the \b
 * arg up.
 */
void
sqc_callout_cancel_task(const sqc_callout_task_t *tptr);


/**
 * Execute the sumitted task forcibly.
 *
 *	@param[in]	tptr	A pointer to a task.
 *
 *	@retval	SQC_RESULT_OK		Succeeded.
 *	@retval <0				Failure.
 */
sqc_result_t
sqc_callout_exec_task_forcibly(const sqc_callout_task_t *tptr);


/**
 * Reset task execution interval.
 *
 *	@param[in]	tptr	A pointer to a task.
 *	@paran[in]	interval	An interval (in nsec.)
 *
 *	@retval	SQC_RESULT_OK		Succeeded.
 *	@retval <0				Failure.
 *
 *	@details This API only useable when it is isssued in an
 *	execution task. Otherwise the API returns an error.
 */
sqc_result_t
sqc_callout_task_reset_interval(sqc_callout_task_t *tptr,
                                   sqc_chrono_t interval);


/**
 * Acquire a task state.
 *
 *	@param[in]	tptr	A pointer to a task.
 *	@paran[out]	sptr	A pointer to a task state returns.
 *
 *	@retval	SQC_RESULT_OK		Succeeded.
 *	@retval <0				Failure.
 */
sqc_result_t
sqc_callout_task_state(sqc_callout_task_t *tptr,
                          sqc_callout_task_state_t *sptr);





__END_DECLS





#endif /* ! __SQC_CALLOUT_H__ */
