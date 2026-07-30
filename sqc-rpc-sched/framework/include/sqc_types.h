#ifndef __SQC_TYPES_H__
#define __SQC_TYPES_H__





#ifdef __cplusplus
#define restrict /**/
#endif /* __cplusplus */





/**
 *	@file	sqc_types.h
 */





/**
 * @details The result type.
 */
typedef int64_t sqc_result_t;


/**
 * @details The flat nano second expression of the time, mainly
 * acquired by \b clock_gettime(). For arithmetic operations, the sign
 * extension is needed.
 */
typedef	int64_t	sqc_chrono_t;





#endif /* ! __SQC_TYPES_H__ */
