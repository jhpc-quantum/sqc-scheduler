#ifndef __SQC_BBQ_H__
#define __SQC_BBQ_H__





/**
 * @file sqc_bbq.h
 */





#include "sqc_cbuffer.h"





#ifndef __SQC_BBQ_T_DEFINED__
typedef sqc_cbuffer_t sqc_bbq_t;
#endif /* ! __SQC_BBQ_T_DEFINED__ */


/**
 * @deprecated Existing just for a backward compatibility.
 */
#define SQC_BOUND_BLOCK_Q_DECL(name, type) sqc_bbq_t





/**
 * Create a bounded blocking queue.
 *
 *     @param[out] bbqptr         A pointer to a queue to be created.
 *     @param[in]  type           A type of a value of the queue.
 *     @param[in]  maxelem        A maximum # of the value the queue holds.
 *     @param[in]  proc           A value free up function (\b NULL allowed).
 *
 *     @retval SQC_RESULT_OK               Succeeded.
 *     @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
 *     @retval SQC_RESULT_ANY_FAILURES     Failed.
 */
#define sqc_bbq_create(bbqptr, type, length, proc)        \
  sqc_cbuffer_create((bbqptr), type, (length), (proc))


/**
 * Shutdown a bounded blocking queue.
 *
 *    @param[in]  bbqptr    A pointer to a queue to be shutdown.
 *    @param[in]  free_values  If \b true, all the values
 *    remaining in the queue are freed if the value free up
 *    function given by the calling of the sqc_cbuffer_create()
 *    is not \b NULL.
 */
#define sqc_bbq_shutdown(bbqptr, free_values)       \
  sqc_cbuffer_shutdown((bbqptr), (free_values))


/**
 * Destroy a bounded blocking queue.
 *
 *    @param[in]  bbqptr    A pointer to a queue to be destroyed.
 *    @param[in]  free_values  If \b true, all the values
 *    remaining in the queue are freed if the value free up
 *    function given by the calling of the sqc_cbuffer_create()
 *    is not \b NULL.
 *
 *    @details if \b bbq is operational, shutdown it.
 */
#define sqc_bbq_destroy(bbqptr, free_values)       \
  sqc_cbuffer_destroy((bbqptr), (free_values))


/**
 * Clear a bounded blocking queue.
 *
 *     @param[in]  bbqptr       A pointer to a queue
 *     @param[in]  free_values  If \b true, all the values
 *     remaining in the queue are freed if the value free up
 *     function given by the calling of the sqc_cbuffer_create()
 *     is not \b NULL.
 *
 *     @retval SQC_RESULT_OK                Succeeded.
 *     @retval SQC_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval SQC_RESULT_ANY_FAILURES      Failed.
 */
#define sqc_bbq_clear(bbqptr, free_values)  \
  sqc_cbuffer_clear((bbqptr), (free_values))


/**
 * Wake up all the waiters in a bounded blocking queue.
 *
 *     @param[in]  bbqptr	A pointer to a queue
 *     @param[in]  nsec		Wait time (nanosec).
 *
 *     @retval SQC_RESULT_OK                Succeeded.
 *     @retval SQC_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval SQC_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval SQC_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval SQC_RESULT_ANY_FAILURES      Failed.
 */
#define sqc_bbq_wakeup(bbqptr, nsec) \
  sqc_cbuffer_wakeup((bbqptr), (nsec))


/**
 * Wait for gettable.
 *
 *     @param[in]  bbqptr	A pointer to a queue.
 *     @param[in]  nsec		Wait time (nanosec).
 *
 *     @retval >0				# of the gettable elements.
 *     @retval SQC_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval SQC_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval SQC_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval SQC_RESULT_ANY_FAILURES      Failed.
 */
#define sqc_bbq_wait_gettable(bbqptr, nsec) \
  sqc_cbuffer_wait_gettable((bbqptr), (nsec))


/**
 * Wait for puttable.
 *
 *     @param[in]  bbqptr	A pointer to a queue.
 *     @param[in]  nsec		Wait time (nanosec).
 *
 *     @retval >0				# of the puttable elements.
 *     @retval SQC_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval SQC_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval SQC_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval SQC_RESULT_ANY_FAILURES      Failed.
 */
#define sqc_bbq_wait_puttable(bbqptr, nsec) \
  sqc_cbuffer_wait_puttable((bbqptr), (nsec))





/**
 * Put a value into a bounded blocking queue.
 *
 *     @param[in]  bbqptr     A pointer to a queue.
 *     @param[in]  valptr     A pointer to a value.
 *     @param[in]  type       A type of the value.
 *     @param[in]  nsec       A wait time (in nsec).
 *
 *     @retval SQC_RESULT_OK                Succeeded.
 *     @retval SQC_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval SQC_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval SQC_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval SQC_RESULT_ANY_FAILURES      Failed.
 */
#define sqc_bbq_put(bbqptr, valptr, type, nsec)        \
  sqc_cbuffer_put((bbqptr), (valptr), type, (nsec))


/**
 * Put elements at the tail of a bounded blocking queue.
 *
 *     @param[in]  bbqptr       A pointer to a queue.
 *     @param[in]  valptr       A pointer to elements.
 *     @param[in]  n_vals       A # of elements to put.
 *     @param[in]  type         A type of the element.
 *     @param[in]  nsec         A Wait time (in nsec).
 *     @param[out] n_actual_put A pointer to a # of elements successfully put (\b NULL allowed.)
 *
 *     @retval >=0 A # of elemets to put successfully.
 *     @retval SQC_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval SQC_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval SQC_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval SQC_RESULT_WAKEUP_REQUESTED  Failed, timedout.
 *     @retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval SQC_RESULT_ANY_FAILURES      Failed.
 *
 *     @details If the \b nsec is less than zero, it blocks until all the
 *     elements specified by n_vals are put.
 *
 *     @details If the \b nsec is zero, it puts elements limited to a number
 *     of available rooms in the queue (it could be zero.)
 *
 *     @details If the \b nsec is greater than zero, it puts elements limited
 *     to a number of available rooms in the queue until the time specified
 *     by the \b nsec is expired. In this case if the actual number of
 *     elements put is less than the \b n_vals, it returns \b
 *     SQC_RESULT_TIMEDOUT.
 *
 *     @details If the \b n_actual_put is not a \b NULL, a number of elements
 *     actually put is stored.
 *
 *     @details If any errors occur while putting, it always returns an error
 *     result even if at least an element is successfully put, so check
 *     *n_actual_put if needed.
 *
 *     @details It is allowed that more then one thread simultaneously
 *     invokes this function, the atomicity of the operation is not
 *     guaranteed.
 */
#define sqc_bbq_put_n(bbqptr, valptr, n_vals, type, nsec,           \
                         n_actual_put)                                 \
sqc_cbuffer_put_n((bbqptr), (void **)(valptr),                    \
                     (n_vals), type, (nsec),                         \
                     (n_actual_put))


/**
 * Get a value from a bounded blocking queue.
 *
 *     @param[in]  bbqptr     A pointer to a queue.
 *     @param[out] valptr     A pointer to a value.
 *     @param[in]  type       A type of the value.
 *     @param[in]  nsec       A wait time (in nsec).
 *
 *     @retval SQC_RESULT_OK                Succeeded.
 *     @retval SQC_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval SQC_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval SQC_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval SQC_RESULT_ANY_FAILURES      Failed.
 */
#define sqc_bbq_get(bbqptr, valptr, type, nsec)   \
  sqc_cbuffer_get((bbqptr), (valptr), type, (nsec))


/**
 * Get elements from the head of a bounded blocking queue.
 *
 *     @param[in]  bbqptr       A pointer to a queue.
 *     @param[out] valptr       A pointer to an element.
 *     @param[in]  n_vals_max   A maximum # of elemetns to get.
 *     @param[in]  n_at_least   A minimum # of elements to get until timeout.
 *     @param[in]  type         A type of the element.
 *     @param[in]  nsec         A wait time (in nsec).
 *     @param[out] n_actual_get A pointer to a # of elements successfully get (\b NULL allowed.)
 *
 *     @retval >=0 A # of acuired elements.
 *     @retval SQC_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval SQC_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval SQC_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval SQC_RESULT_WAKEUP_REQUESTED  Failed, timedout.
 *     @retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval SQC_RESULT_ANY_FAILURES      Failed.
 *
 *     @details If the \b nsec is less than zero, it blocks until all the
 *     specified number (\b n_vals_max) of the elements are acquired.
 *
 *     @details If the \b nsec is zero, only the available elements at the
 *     moment (it could be zero) are acuqured.
 *
 *     @details If the \b nsec is greater than zero, only the elements
 *     which become available while in the time specified by the \b nsec
 *     are acquired.
 *
 *     @details If the \b nsec is greater than zero and the \b
 *     n_at_least is zero, it treats that the \b nsec is
 *     zero. Otherwise, if bothe the \b nsec and the \b n_at_least are
 *     greater than zero, it returns when the number of the elements
 *     sepcified by the \b n_at_least are acquired, even before the
 *     time period specified by the \b nsec is expired.
 *
 *     @details The \b valptr must point a sufficient size (\b sizeof(type) *
 *     \b n_vals_max) buffer enough to store elements.
 *
 *     @details If the \b n_actual_get is not a \b NULL, a number of elements
 *     actually get is stored.
 *
 *     @details If any errors occur while getting, it always returns an error
 *     result even if at least an element is successfully got, so check
 *     *n_actual_get if needed.
 */
#define sqc_bbq_get_n(bbqptr, valptr, n_vals_max, n_at_least,       \
                         type, nsec, n_actual_get)                     \
sqc_cbuffer_get_n((bbqptr), (void **)(valptr),                    \
                     (n_vals_max), (n_at_least),                     \
                     type, (nsec), (n_actual_get))


/**
 * Peek the first value from a bounded blocking queue.
 *
 *     @param[in]  bbqptr     A pointer to a queue.
 *     @param[out] valptr     A pointer to a value.
 *     @param[in]  type       A type of the value.
 *     @param[in]  nsec       A wait time (in nsec).
 *
 *     @retval SQC_RESULT_OK                Succeeded.
 *     @retval SQC_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval SQC_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval SQC_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval SQC_RESULT_ANY_FAILURES      Failed.
 */
#define sqc_bbq_peek(bbqptr, valptr, type, nsec)            \
  sqc_cbuffer_peek((bbqptr), (valptr), type, (nsec))


/**
 * Peek elements from the head of a bounded blocking queue.
 *
 *     @param[in]  bbqptr        A pointer to a queue.
 *     @param[out] valptr        A pointer to a element.
 *     @param[in]  n_vals_max    A maximum # of elemetns to get.
 *     @param[in]  n_at_least    A minimum # of elements to get until timeout.
 *     @param[in]  type          A type of the element.
 *     @param[in]  nsec          A wait time (in nsec).
 *     @param[out] n_actual_peek A pointer to a # of elements successfully get (\b NULL allowed.)
 *
 *     @retval >=0 A # of peeked elements.
 *     @retval SQC_RESULT_NOT_OPERATIONAL   Failed, not operational.
 *     @retval SQC_RESULT_POSIX_API_ERROR   Failed, posix API error.
 *     @retval SQC_RESULT_TIMEDOUT          Failed, timedout.
 *     @retval SQC_RESULT_WAKEUP_REQUESTED  Failed, timedout.
 *     @retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *     @retval SQC_RESULT_ANY_FAILURES      Failed.
 *
 *     @details If the \b nsec is less than zero, it blocks until all the
 *     specified number (\b n_vals_max) of the elements are peeked.
 *
 *     @details If the \b nsec is zero, only the available elements at the
 *     moment (it could be zero) are peeked.
 *
 *     @details If the \b nsec is greater than zero, only the elements
 *     which become available while in the time specified by the \b nsec
 *     are peeked.
 *
 *     @details If the \b nsec is greater than zero and the \b
 *     n_at_least is zero, it treats that the \b nsec is
 *     zero. Otherwise, if bothe the \b nsec and the \b n_at_least are
 *     greater than zero, it returns when the number of the elements
 *     sepcified by the \b n_at_least are peeked, even before the
 *     time period specified by the \b nsec is expired.
 *
 *     @details The \b valptr must point a sufficient size (\b sizeof(type) *
 *     \b n_vals_max) buffer enough to store elements.
 *
 *     @details If the \b n_actual_peek is not a \b NULL, a number of elements
 *     actually get is stored.
 *
 *     @details If any errors occur while peeking, it always returns an error
 *     result even if at least an element is successfully got, so check
 *     *n_actual_peek if needed.
 */
#define sqc_bbq_peek_n(bbqptr, valptr, n_vals_max, n_at_least,      \
                          type, nsec, n_actual_peek)                   \
sqc_cbuffer_peek_n((cbptr), (void **)(valptr),                    \
                      (n_vals_max), (n_at_least),                    \
                      sizeof(type), (nsec), (n_actual_peek))





/**
 * Get a # of values in a bounded blocking queue.
 *	@param[in]   bbqptr    A pointer to a queue.
 *
 *	@retval	>=0	A # of values in the queue.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval SQC_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 */
#define sqc_bbq_size(bbqptr)                \
  sqc_cbuffer_size((bbqptr))


/**
 * Get the remaining capacity of bounded blocking queue.
 *	@param[in]   bbqptr    A pointer to a queue.
 *
 *	@retval	>=0	The remaining capacity of the queue.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval SQC_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 */
#define sqc_bbq_remaining_capacity(bbqptr)        \
  sqc_cbuffer_remaining_capacity((bbqptr))


/**
 * Get the maximum capacity of bounded blocking queue.
 *	@param[in]   bbqptr    A pointer to a queue.
 *
 *	@retval	>=0	The maximum capacity of the queue.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval SQC_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 */
#define sqc_bbq_max_capacity(bbqptr)        \
  sqc_cbuffer_max_capacity((bbqptr))





/**
 * Returns \b true if the bounded blocking queue is full.
 *
 *    @param[in]   bbqptr   A pointer to a queue.
 *    @param[out]  retptr   A pointer to a result.
 *
 *	@retval	SQC_RESULT_OK		Succeeded.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval SQC_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 */
#define sqc_bbq_is_full(bbqptr, retptr)     \
  sqc_cbuffer_is_full((bbqptr), (retptr))


/**
 * Returns \b true if the bounded blocking queue is empty.
 *
 *    @param[in]   bbqptr   A pointer to a queue.
 *    @param[out]  retptr   A pointer to a result.
 *
 *	@retval	SQC_RESULT_OK		Succeeded.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval SQC_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 */
#define sqc_bbq_is_empty(bbqptr, retptr)    \
  sqc_cbuffer_is_empty((bbqptr), (retptr))


/**
 * Returns \b true if the bounded blocking queue is operational.
 *
 *    @param[in]   cbptr    A pointer to a queue.
 *    @param[out]  retptr   A pointer to a result.
 *
 *	@retval	SQC_RESULT_OK		Succeeded.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 */
#define sqc_bbq_is_operational(bbqptr, retptr)    \
  sqc_cbuffer_is_operational((bbqptr), (retptr))


/**
 * Cleanup an internal state of a circular buffer after thread
 * cancellation.
 *	@param[in]	cbptr	A pointer to a circular buffer
 */
#define sqc_bbq_cancel_janitor(cbptr)       \
  sqc_cbuffer_cancel_janitor((cbptr))






#endif  /* ! __SQC_BBQ_H__ */
