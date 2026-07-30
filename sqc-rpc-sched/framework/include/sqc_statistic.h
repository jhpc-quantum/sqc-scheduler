#ifndef __SQC_STATISTIC_H__
#define __SQC_STATISTIC_H__





/**
 *	@file	sqc_statistic.h
 */





__BEGIN_DECLS





typedef struct sqc_statistic_struct	*sqc_statistic_t;





/**
 * Create a statistic.
 *
 *	@param[in,out]	sptr	A pointer to a statistic.
 *	@param[in]	name	Name of the statistic.
 *
 *	@retval	SQC_RESULT_OK		Suceeded.
 *	@retval SQC_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 */
sqc_result_t
sqc_statistic_create(sqc_statistic_t *sptr, const char *name);


/**
 * Find a statistic by name.
 *
 *	@param[out]	sptr	A pointer to a statistic.
 *	@param[in]	name	Name of the statistic.
 *
 *	@retval	SQC_RESULT_OK		Suceeded.
 *	@retval SQC_RESULT_NOT_FOUND	Failed, not found.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 */
sqc_result_t
sqc_statistic_find(sqc_statistic_t *sptr, const char *name);


/**
 * Destroy a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 */
void
sqc_statistic_destroy(sqc_statistic_t *sptr);


/**
 * Destroy a statistic by name.
 *
 *	@param[in]	name	Name of the statistic.
 */
void
sqc_statistic_destroy_by_name(const char *name);


/**
 * Record a value to a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 *	@param[in]	val	A value.
 *
 *	@retval	SQC_RESULT_OK		Suceeded.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 */
sqc_result_t
sqc_statistic_record(sqc_statistic_t *sptr, int64_t val);


/**
 * Reset a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 *
 *	@retval	SQC_RESULT_OK		Suceeded.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 */
sqc_result_t
sqc_statistic_reset(sqc_statistic_t *sptr);


/**
 * Acquire # of the sample from a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 *
 *	@retval	>=0				# of the sample.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 */
sqc_result_t
sqc_statistic_sample_n(sqc_statistic_t *sptr);


/**
 * Acquire the minimum value from a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 *	@param[in]	valptr	A pointer to a value.
 *
 *	@retval	SQC_RESULT_OK		Suceeded.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 */
sqc_result_t
sqc_statistic_min(sqc_statistic_t *sptr, int64_t *valptr);


/**
 * Acquire the maximum value from a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 *	@param[in]	valptr	A pointer to a value.
 *
 *	@retval	SQC_RESULT_OK		Suceeded.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 */
sqc_result_t
sqc_statistic_max(sqc_statistic_t *sptr, int64_t *valptr);


/**
 * Acquire the average of a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 *	@param[in]	valptr	A pointer to a value.
 *
 *	@retval	SQC_RESULT_OK		Suceeded.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 */
sqc_result_t
sqc_statistic_average(sqc_statistic_t *sptr, double *valptr);


/**
 * Acquire the standard deviation of a statistic.
 *
 *	@param[in]	sptr	A pointer to a statistic.
 *	@param[in]	valptr	A pointer to a value.
 *	@param[in]	is_ssd	\b true) returns the Sample Standard Deviation (a.k.a. Unbiased Standard Deviation) instead of the standard deviation;
 *
 *	@retval	SQC_RESULT_OK		Suceeded.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.

 */
sqc_result_t
sqc_statistic_sd(sqc_statistic_t *sptr, double *valptr, bool is_ssd);





__END_DECLS





#endif /* ! __SQC_STATISTIC_H__ */
