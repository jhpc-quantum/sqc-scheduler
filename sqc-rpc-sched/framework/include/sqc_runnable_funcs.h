#ifndef __SQC_RUNNABLE_FUNCS_H__
#define __SQC_RUNNABLE_FUNCS_H__




/**
 *	@file	sqc_runnable_funcs.h
 */





#ifndef RUNNABLE_T_DECLARED
typedef struct sqc_runnable_record 	*sqc_runnable_t;
#define RUNNABLE_T_DECLARED
#endif /* ! RUNNABLE_T_DECLARED */





/**
 * A procedure for a runnable.
 *
 *	@param[in]	rptr	A runnable.
 *	@param[in]	arg	An argument.
 *
 * @details If any resoures acquired in the function must be released
 * before returning. Doing this must be the functions's responsibility.
 *
 * @details This function must not loop infinitely, but must return in
 * appropriate amount of execution time.
 */
typedef sqc_result_t (*sqc_runnable_proc_t)(
  const sqc_runnable_t *rptr,
  void *arg);


/**
 * Free a runnable up.
 *
 *	@param[in]	rptr	A pointer to a runnable.
 *
 * @details Don't free the \b *rptr itself up.
 */
typedef void (*sqc_runnable_freeup_proc_t)(sqc_runnable_t *rptr);





#endif /* ! __SQC_RUNNABLE_FUNCS_H__ */
