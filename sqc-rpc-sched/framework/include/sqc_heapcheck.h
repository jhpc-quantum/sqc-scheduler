#ifndef __SQC_HEAPCHECK_H__
#define __SQC_HEAPCHECK_H__





__BEGIN_DECLS





void
sqc_heapcheck_module_initialize(void);

bool
sqc_heapcheck_is_in_heap(const void *addr);

#if 0
bool
sqc_heapcheck_is_mallocd(const void *addr);
#endif





__END_DECLS





#endif /* __SQC_HEAPCHECK_H__ */

