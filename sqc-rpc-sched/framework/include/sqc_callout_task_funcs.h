#ifndef __SQC_CALLOUT_TASK_FUNCS_H__
#define __SQC_CALLOUT_TASK_FUNCS_H__





/**
 *	@file	sqc_callout_task_funcs.h
 */





#ifndef CALLOUT_TASK_T_DECLARED
typedef struct sqc_callout_task_record 	*sqc_callout_task_t;
#define CALLOUT_TASK_T_DECLARED
#endif /* ! CALLOUT_TASK_T_DECLARED */





/**
 * The signature of callout task functions.
 *
 *	@param[in]	arg	An argument.
 *
 *	@retval SQC_RESULT_OK	Succeeded.
 *	@retval !=SQC_RESULT_OK	Failed.
 *
 *	@returns	SQC_RESULT_OK	Succeeded.
 *	@retuens	<0			Failures.
 *
 * @details If the function returns a value other than \b
 * SQC_RESULT_OK and if the task is a periodic task, the task is
 * supposed to be cancelled and not scheduled for the next execution,
 * and the \b arg is destroyed if the freeup function is specified.
 */
typedef sqc_result_t	(*sqc_callout_task_proc_t)(void *arg);


/**
 * The signature of task argument freeup functions.
 *
 *	@param[in]	arg	An argument.
 */
typedef void	(*sqc_callout_task_arg_freeup_proc_t)(void *arg);





#endif /* ! __SQC_CALLOUT_TASK_FUNCS_H__ */
