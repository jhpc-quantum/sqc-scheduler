#ifndef __SQC_DSTRING_H__
#define __SQC_DSTRING_H__





__BEGIN_DECLS





/**
 * @brief	sqc_dstring_t
 */
typedef struct dstring *sqc_dstring_t;

/**
 * Create a dynamic string.
 *
 *     @param[out]	ds	A pointer to a dynamic string to be created.
 *
 *     @retval	SQC_RESULT_OK	Succeeded.
 *     @retval	SQC_RESULT_ANY_FAILURES	Failed.
 *     @retval	SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	SQC_RESULT_NO_MEMORY	Failed, no memory.
 */
sqc_result_t
sqc_dstring_create(sqc_dstring_t *ds);

/**
 * Destroy a dynamic string.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *
 *     @retval	void
 */
void
sqc_dstring_destroy(sqc_dstring_t *ds);

/**
 * Clear a dynamic string.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *
 *     @retval	SQC_RESULT_OK	Succeeded.
 *     @retval	SQC_RESULT_ANY_FAILURES	Failed.
 *     @retval	SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 */
sqc_result_t
sqc_dstring_clear(sqc_dstring_t *ds);

/**
 * Append a dynamic string according to a format for \e va_list.
 *
 *     @param[in,out]	ds	A pointer to a dynamic string.
 *     @param[in]	format	A pointer to a format string.
 *     @param[in]	args	A pointer to a \e va_list.
 *
 *     @retval	SQC_RESULT_OK	Succeeded.
 *     @retval	SQC_RESULT_ANY_FAILURES	Failed.
 *     @retval	SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	SQC_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	SQC_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
sqc_result_t
sqc_dstring_vappendf(sqc_dstring_t *ds, const char *format,
                        va_list *args) __attr_format_printf__(2, 0);

/**
 * Append a dynamic string according to a format.
 *
 *     @param[in,out]	ds	A pointer to a dynamic string.
 *     @param[in]	format	A pointer to a format string.
 *
 *     @retval	SQC_RESULT_OK	Succeeded.
 *     @retval	SQC_RESULT_ANY_FAILURES	Failed.
 *     @retval	SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	SQC_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	SQC_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
sqc_result_t
sqc_dstring_appendf(sqc_dstring_t *ds, const char *format, ...)
__attr_format_printf__(2, 3);

/**
 * Prepend a dynamic string according to a format for \e va_list.
 *
 *     @param[in,out]	ds	A pointer to a dynamic string.
 *     @param[in]	format	A pointer to a format string.
 *     @param[in]	args	A pointer to a \e va_list.
 *
 *     @retval	SQC_RESULT_OK	Succeeded.
 *     @retval	SQC_RESULT_ANY_FAILURES	Failed.
 *     @retval	SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	SQC_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	SQC_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
sqc_result_t
sqc_dstring_vprependf(sqc_dstring_t *ds, const char *format,
                         va_list *args) __attr_format_printf__(2, 0);

/**
 * repend a dynamic string according to a format.
 *
 *     @param[in,out]	ds	A pointer to a dynamic string.
 *     @param[in]	format	A pointer to a format string.
 *
 *     @retval	SQC_RESULT_OK	Succeeded.
 *     @retval	SQC_RESULT_ANY_FAILURES	Failed.
 *     @retval	SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	SQC_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	SQC_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
sqc_result_t
sqc_dstring_prependf(sqc_dstring_t *ds, const char *format, ...)
__attr_format_printf__(2, 3);

/**
 * Insert a dynamic string according to a format for \e va_list.
 *
 *     @param[in,out]	ds	A pointer to a dynamic string.
 *     @param[in]	offset	offset.
 *     @param[in]	format	A pointer to a format string.
 *     @param[in]	args	A pointer to a \e va_list.
 *
 *     @retval	SQC_RESULT_OK	Succeeded.
 *     @retval	SQC_RESULT_ANY_FAILURES	Failed.
 *     @retval	SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	SQC_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	SQC_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
sqc_result_t
sqc_dstring_vinsertf(sqc_dstring_t *ds,
                        size_t offset,
                        const char *format,
                        va_list *args) __attr_format_printf__(3, 0);

/**
 * Prepend a dynamic string according to a format.
 *
 *     @param[in,out]	ds	A pointer to a dynamic string.
 *     @param[in]	offset	offset.
 *     @param[in]	format	A pointer to a format string.
 *
 *     @retval	SQC_RESULT_OK	Succeeded.
 *     @retval	SQC_RESULT_ANY_FAILURES	Failed.
 *     @retval	SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	SQC_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	SQC_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
sqc_result_t
sqc_dstring_insertf(sqc_dstring_t *ds,
                       size_t offset,
                       const char *format, ...)
__attr_format_printf__(3, 4);


/**
 * Concatenate dynamic strings.
 *
 *     @param[in,out]	dst_ds	A pointer to a dynamic string(dst).
 *     @param[in,out]	src_ds	A pointer to a dynamic string(src).
 *
 *     @retval	SQC_RESULT_OK	Succeeded.
 *     @retval	SQC_RESULT_ANY_FAILURES	Failed.
 *     @retval	SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval	SQC_RESULT_NO_MEMORY	Failed, no memory.
 *     @retval	SQC_RESULT_OUT_OF_RANGE	Failed, out of range.
 */
sqc_result_t
sqc_dstring_concat(sqc_dstring_t *dst_ds,
                      const sqc_dstring_t *src_ds);

/**
 * Get string from a dynamic string.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *     @param[out]	ds	A pointer to a string.
 *
 *     @retval	SQC_RESULT_OK	Succeeded.
 *     @retval	SQC_RESULT_ANY_FAILURES	Failed.
 *     @retval	SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 */
sqc_result_t
sqc_dstring_str_get(sqc_dstring_t *ds, char **str);

/**
 * Get size of a string.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *
 *     @retval	>=0	Size of a string
 *     @retval	SQC_RESULT_ANY_FAILURES	Failed.
 *     @retval	SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 */
sqc_result_t
sqc_dstring_len_get(sqc_dstring_t *ds);

/**
 * A dynamic string is empty.
 *
 *     @param[in]	ds	A pointer to a dynamic string.
 *
 *     @retval	true/false
 */
bool
sqc_dstring_empty(sqc_dstring_t *ds);





__END_DECLS





#endif /* __SQC_DSTRING_H__ */
