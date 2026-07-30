#ifndef __SQC_NUMA_H__
#define __SQC_NUMA_H__





/**
 *	@file	sqc_numa.h
 */





/**
 * Allocate dynamic memory on a specified NUMA node
 *
 *	@param[in]	sz	A size to allocate (in bytes.)
 *	@param[in]	node	A NUMA node.
 *
 *	@retval	!=NULL		Succeeded, the allocated memory.
 *	@retval NULL		Failed.
 */
void 	*sqc_malloc_on_numanode(size_t sz, unsigned int node);


/**
 * Allocate dynamic memory on the nearest NUMA node which the
 * specified CPU belongs.
 *
 *	@param[in]	sz	A size to allocate (in bytes.)
 *	@param[in]	cpu	A cpu/core, identical to an index for
 *				the \b cpu_set_t. Passing a negative
 *				value makes the API returns \b
 *				malloc(\b sz )
 *
 *	@retval	!=NULL		Succeeded, the allocated memory.
 *	@retval NULL		Failed.
 */
void 	*sqc_malloc_on_cpu(size_t sz, int cpu);


/**
 * Free memory allocated by the \b sqc_malloc_on_numanode().
 *
 *	@param[in]	p	A pointer to memory.
 */
void	sqc_free_on_numanode(void *p);


/**
 * Free memory allocated by the \b sqc_malloc_on_cpu().
 *
 *	@param[in]	p	A pointer to memory.
 */
void	sqc_free_on_cpu(void *p);


/**
 * Returns the NUMA is enabled or not.
 *
 *	@retval	true	The NUMA is enabled.
 *	@retval	true	The NUMA is disabled.
 */
bool	sqc_is_numa_enabled(void);





#endif /* ! __SQC_NUMA_H__ */
