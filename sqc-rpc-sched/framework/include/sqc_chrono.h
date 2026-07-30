#ifndef __SQC_CHRONO_H__
#define __SQC_CHRONO_H__





/**
 *	@file	sqc_chrono.h
 */





__BEGIN_DECLS





sqc_chrono_t
sqc_chrono_now(void);


sqc_result_t
sqc_chrono_to_timespec(struct timespec *dstptr,
                          sqc_chrono_t nsec);


sqc_result_t
sqc_chrono_to_timeval(struct timeval *dstptr,
                         sqc_chrono_t nsec);


sqc_result_t
sqc_chrono_from_timespec(sqc_chrono_t *dstptr,
                            const struct timespec *specptr);


sqc_result_t
sqc_chrono_from_timeval(sqc_chrono_t *dstptr,
                           const struct timeval *valptr);


sqc_result_t
sqc_chrono_nanosleep(sqc_chrono_t nsec,
                        sqc_chrono_t *remptr);


#ifdef __GNUC__
#if defined(SQC_CPU_X86_64) || defined(SQC_CPU_I386)
static inline uint64_t
sqc_rdtsc(void) {
  uint32_t eax, edx;
  __asm__ volatile ("rdtsc" : "=a" (eax), "=d" (edx));
  return (((uint64_t)edx) << 32) | ((uint64_t)eax);
}
#else
#warning reading TSC thingies is not supported on this platform.
static inline uint64_t
sqc_rdtsc(void) {
  sqc_msg_warning("reading TSC thingies is not supported on this "
                     "platform.\n");
  return 0LL;
}
#endif /* SQC_CPU_X86_64 || SQC_CPU_I386 */
#else
#warning reading TSC thingies is not supported with this compiler.
static inline uint64_t
sqc_rdtsc(void) {
  sqc_msg_warning("reading TSC thingies is not supported with "
                     "this compiler.\n");
  return 0LL;
}
#endif /* __GNUC__ */





__END_DECLS





#endif /* ! __SQC_CHRONO_H__ */
