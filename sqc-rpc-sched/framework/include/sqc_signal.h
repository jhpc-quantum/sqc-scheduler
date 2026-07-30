#ifndef __SQC_SIGNAL_H__
#define __SQC_SIGNAL_H__





/**
 *	@file sqc_signal.h
 */





__BEGIN_DECLS





#ifdef __FreeBSD__
#define sighandler_t sig_t
#endif /* __FreeBSD__ */
#ifdef __NetBSD__
typedef void (*sighandler_t)(int);
#endif /* __NetBSD__ */


#define SIG_CUR	__s_I_g_C_u_R__





void	__s_I_g_C_u_R__(int sig);





/**
 * A MT-Safe signal(2).
 *
 *	@param[in]	signum	A signal.
 *	@param[in]	newhdlr	A new signal handler.
 *	@param[out]	oldhdlrptr	A pointer to the current/old handler.
 *
 *	@retval	SQC_RSULT_OK		Succeeded.
 *	@retval SQC_RESULT_INVALID_ARGS	Failed, invalid args.
 *	@retval SQC_RESULT_NOT_OPERATIONAL	Failed, not operational.
 *	@retval SQC_RESULT_POSIX_API_ERROR	Failed, POSIX error.
 *	@retval SQC_RESULT_ANY_FAILURES	Failed.
 *
 *	@details if the \b newhdlr is \b SIG_CUR, the API just returns the
 *	old signal handler to \b *oldptr.
 */
sqc_result_t
sqc_signal(int signum, sighandler_t newhdlr, sighandler_t *oldhdlrptr);


/**
 * Fallback the signal handling mechanism to the good-old-school
 * semantics.
 */
void	sqc_signal_old_school_semantics(void);





__END_DECLS





#endif /* ! __SQC_SIGNAL_H__ */
