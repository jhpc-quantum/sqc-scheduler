#ifndef __SQC_ERROR_H__
#define __SQC_ERROR_H__





/**
 *	@file	sqc_error.h
 */





#include "sqc_ecode.h"





__BEGIN_DECLS


/**
 * Get a human readable error message from an API result code.
 *
 *	@param[in]	err	A result code.
 *
 *	@returns	A human readable error message.
 */
const char 	*sqc_error_get_string(sqc_result_t err);


__END_DECLS





#endif /* ! __SQC_ERROR_H__ */
