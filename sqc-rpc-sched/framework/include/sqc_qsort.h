#ifndef SRC_INCLUDE_SQC_QSORT_H_
#define SRC_INCLUDE_SQC_QSORT_H_





__BEGIN_DECLS




#ifdef __linux__
#define sqc_qsort_r qsort_r
#else
void
sqc_qsort_r(void *array,
               size_t nelem,
               size_t size,
               int (*cmp)(const void *, const void *, void *),
               void *arg);
#endif /* __linux__ */





__END_DECLS





#endif /* SRC_INCLUDE_SQC_QSORT_H_ */
