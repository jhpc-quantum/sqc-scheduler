#ifndef __SQC_QMUXER_H__
#define __SQC_QMUXER_H__





/**
 * @file sqc_qmuxer.h
 */





__BEGIN_DECLS





typedef struct sqc_cbuffer_record 	*sqc_cbuffer_t;
#define __SQC_CBUFFER_T_DEFINED__
typedef struct sqc_qmuxer_record *sqc_qmuxer_t;
#define __SQC_QMUXER_T_DEFINED__
typedef sqc_cbuffer_t sqc_bbq_t;
#define __SQC_BBQ_T_DEFINED__


typedef struct sqc_qmuxer_poll_record *sqc_qmuxer_poll_t;
#define __SQC_QMUXER_POLL_T_DEFINED__


typedef enum {
  SQC_QMUXER_POLL_UNKNOWN = 0,
  SQC_QMUXER_POLL_READABLE = 0x1,
  SQC_QMUXER_POLL_WRITABLE = 0x2,
  SQC_QMUXER_POLL_BOTH = 0x3
} sqc_qmuxer_poll_event_t;





/**
 * Create a queue muxer.
 *
 *	@param[in,out]	qmxptr	A pointer to a queue muxer to be created.
 *
 *	@retval SQC_RESULT_OK		Succeeded.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval SQC_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 */
sqc_result_t
sqc_qmuxer_create(sqc_qmuxer_t *qmxptr);


/**
 * Destroy a queue muxer.
 *
 *	@param[in]	qmxptr	A pointer to a queue muxer to be destroyed.
 */
void
sqc_qmuxer_destroy(sqc_qmuxer_t *qmxptr);





/**
 * Create a polling object.
 *
 *	@param[in,out]	mpptr	A pointer to a polling onject to be created.
 *	@param[in]	bbq	A queue to be polled.
 *	@param[in]	type	A type of event to poll;
 *	\b SQC_QMUXER_POLL_READABLE ) poll the queue readable;
 *	\b SQC_QMUXER_POLL_WRITABLE ) poll the queue writable;
 *	\b SQC_QMUXER_POLL_BOTH ) poll the queue readable and/or writable.
 *
 *	@retval SQC_RESULT_OK		Succeeded.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval SQC_RESULT_NO_MEMORY	Failed, no memory.
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 */
sqc_result_t
sqc_qmuxer_poll_create(sqc_qmuxer_poll_t *mpptr,
                          sqc_bbq_t bbq,
                          sqc_qmuxer_poll_event_t type);


/**
 * Destroy a polling object.
 *
 *	@param[in]	mpptr	A pointer to a polling object to be destroyed.
 */
void
sqc_qmuxer_poll_destroy(sqc_qmuxer_poll_t *mpptr);


/**
 * Reset internal status of a polling object.
 *
 *	@param[in]	mpptr	A pointer to a polling object to be reset.
 *
 *	@retval SQC_RESULT_OK		Succeeded.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 */
sqc_result_t
sqc_qmuxer_poll_reset(sqc_qmuxer_poll_t *mpptr);


/**
 * Set a queue to a polling object.
 *
 *	@param[in]	mpptr	A pointer to a polling object.
 *	@param[in]	bbq	A queue to be polled (\b NULL allowed).
 *
 *	@retval SQC_RESULT_OK		Succeeded.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval SQC_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 *
 *	@details If the \b bbq is \b NULL, the polling object is not
 *	used as an polling object and just avoided to be polled even
 *	the polling object is on the first parameter for the \b
 *	sqc_qmuxer_poll()
 */
sqc_result_t
sqc_qmuxer_poll_set_queue(sqc_qmuxer_poll_t *mpptr,
                             sqc_bbq_t bbq);


/**
 * Get a queue from a polling object.
 *
 *	@param[in]	mpptr	A pointer to a polling object.
 *	@param[in]	bbqptr	A pointer to a queue to be returned.
 *
 *	@retval SQC_RESULT_OK		Succeeded.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 *
 *	@details I have no guts to unify all the setter/getter APIs'
 *	parameter at this moment. Don't get me wrong.
 */
sqc_result_t
sqc_qmuxer_poll_get_queue(sqc_qmuxer_poll_t *mpptr,
                             sqc_bbq_t *bbqptr);


/**
 * Set a polling event type of a polling onject.
 *
 *	@param[in]	mpptr	A pointer to a polling object.
 *	@param[in]	type	A type of event to poll;
 *	\b SQC_QMUXER_POLL_READABLE ) poll the queue readable;
 *	\b SQC_QMUXER_POLL_WRITABLE ) poll the queue writable;
 *	\b SQC_QMUXER_POLL_BOTH ) poll the queue readable and/or writable.
 *
 *	@retval SQC_RESULT_OK		Succeeded.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 */
sqc_result_t
sqc_qmuxer_poll_set_type(sqc_qmuxer_poll_t *mpptr,
                            sqc_qmuxer_poll_event_t type);


/**
 * Returns # of the values in the queue of a polling ofject.
 *
 *	@param[in]	mpptr	A pointer to a polling object.
 *
 *	@retval	>=0	A # of values in the queue.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 */
sqc_result_t
sqc_qmuxer_poll_size(sqc_qmuxer_poll_t *mpptr);


/**
 * Returns the remaining capacity of the queue in a polling ofject.
 *
 *	@param[in]	mpptr	A pointer to a polling object.
 *
 *	@retval	>=0	A # of values in the queue.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 */
sqc_result_t
sqc_qmuxer_poll_remaining_capacity(sqc_qmuxer_poll_t *mpptr);


/**
 * Wait for an event on any specified poll objects.
 *
 *	@param[in]	polls	An array of pointer of poll objects.
 *	@param[in]	npolls	A # of the poll objects.
 *	@param[in]	nsec	Time to block (in nsec).
 *
 *	@retval	> 0	A # of poll objects having event.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid argument(s).
 *	@retval SQC_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval SQC_RESULT_TIMEDOUT		Failed, timedout.
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 *
 *	@details Note: for performance, we don't take a giant lock
 *	that blocks all the operations of queues in the poll objects
 *	when checkcing events. Because of this, there is slight
 *	possibility of dropping events. In order to avoid this, you
 *	better set appropriate timeout value in \b nsec even
 *	specifying a negative value to the \b nsec is allowed.
 *
 *	@details This API doesn't return zero/SQC_RESULT_OK.
 */
sqc_result_t
sqc_qmuxer_poll(sqc_qmuxer_t *qmxptr,
                   sqc_qmuxer_poll_t const polls[],
                   size_t npolls,
                   sqc_chrono_t nsec);




/**
 * Cleanup an internal state of a qmuxer after thread
 * cancellation.
 *	@param[in]	qmxptr	A pointer to a qmuxer
 */
void
sqc_qmuxer_cancel_janitor(sqc_qmuxer_t *qmxptr);





__END_DECLS





#endif  /* ! __SQC_QMUXER_H__ */
