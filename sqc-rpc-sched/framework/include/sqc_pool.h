#pragma once





/**
 *	@file	sqc_pool.h
 */





__BEGIN_DECLS





typedef struct sqc_pool_record	*sqc_pool_t;


typedef enum {
  SQC_POOL_TYPE_UNKNOWN = 0,
  SQC_POOL_TYPE_QUEUE = 1,
  SQC_POOL_TYPE_INDEX = 2,
} sqc_pool_type_t;





sqc_result_t
sqc_pool_get_pool(const char *name, sqc_pool_t *pptr);


sqc_result_t
sqc_pool_create(sqc_pool_t *pptr, size_t sz, const char *name,
                   sqc_pool_type_t type,
                   bool is_executor,
                   size_t n_max_objs,
                   size_t pobj_size,
                   sqc_poolable_methods_t m);


sqc_result_t
sqc_pool_add_poolable_by_index(sqc_pool_t *pptr, uint64_t index,
                                  void *args);


sqc_result_t
sqc_pool_acquire_poolable(sqc_pool_t *pptr, sqc_chrono_t to,
                             sqc_poolable_t *pobjptr);


sqc_result_t
sqc_pool_acquire_poolable_by_index(sqc_pool_t *pptr, uint64_t index,
                                      sqc_chrono_t to,
                                      sqc_poolable_t *pobjptr);


sqc_result_t
sqc_pool_release_poolable(sqc_poolable_t *pobjptr);


sqc_result_t
sqc_pool_get_outstanding_obj_num(sqc_pool_t *pptr);


sqc_result_t
sqc_pool_wakeup(sqc_pool_t *pptr, sqc_chrono_t to);


sqc_result_t
sqc_pool_shutdown(sqc_pool_t *pptr,  shutdown_grace_level_t lvl,
                     sqc_chrono_t to);


sqc_result_t
sqc_pool_cancel(sqc_pool_t *pptr);


sqc_result_t
sqc_pool_wait(sqc_pool_t *pptr, sqc_chrono_t to);


void
sqc_pool_destroy(sqc_pool_t *pptr);


__END_DECLS

