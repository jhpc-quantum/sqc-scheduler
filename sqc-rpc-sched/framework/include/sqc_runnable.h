#ifndef __SQC_RUNNABLE_H__
#define __SQC_RUNNABLE_H__





/**
 *	@file	sqc_runnable.h
 */





#include "sqc_runnable_funcs.h"





#ifndef RUNNABLE_T_DECLARED
typedef struct sqc_runnable_record 	*sqc_runnable_t;
#define RUNNABLE_T_DECLARED
#endif /* ! RUNNABLE_T_DECLARED */





__BEGIN_DECLS





/**
 * Create a runnable.
 *
 *	@param[out]	rptr	A pointer to a created runnable.
 *	@param[in]	sz	A memory allocation size for this object
 *	(in bytes.)
 *	@param[in]	func	A runnable procedure.
 *	@param[in]	arg	An argument for the\b func (\b NULL allowed.)
 *	@param[in]	freeup_proc	A freeup prosedure (\b NULL allowed.)
 *
 *	@retval SQC_RESULT_OK		Succeeded.
 *	@retval SQC_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 *
 * @details If the \b rptr is \b NULL, it allocates a memory area and
 * the allocated area is always free'd by calling \b
 * sqc_runner_destroy(). In this case if the \b sz is greater than
 * the original object size, \b sz bytes momory area is
 * allocated. Otherwise if the \b rptr is not NULL the pointer given
 * is used as is.
 */
sqc_result_t
sqc_runnable_create(sqc_runnable_t *rptr,
                       size_t sz,
                       sqc_runnable_proc_t func,
                       void *arg,
                       sqc_runnable_freeup_proc_t freeup_proc);


/**
 * Destroy a runnable.
 *
 *	@param[in]	rptr	A pointer to a runnable.
 *
 * @details If the \b freeup_proc was specified at creation, the \b
 * freeup_proc is called BEFORE free the \b *rptr up.
 */
void
sqc_runnable_destroy(sqc_runnable_t *rptr);


/**
 * Execute a runnable.
 *
 *	@param[in]	rptr	A pointer to a runnable.
 */
sqc_result_t
sqc_runnable_start(const sqc_runnable_t *rptr);





__END_DECLS





#endif /* ! __SQC_RUNNABLE_H__ */
