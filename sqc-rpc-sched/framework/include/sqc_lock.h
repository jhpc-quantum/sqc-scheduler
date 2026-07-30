#ifndef __SQC_LOCK_H__
#define __SQC_LOCK_H__





/**
 *	@file	sqc_lock.h
 */





__BEGIN_DECLS





typedef enum {
  SQC_MUTEX_TYPE_UNKNOWN = 0,
  SQC_MUTEX_TYPE_DEFAULT,
  SQC_MUTEX_TYPE_RECURSIVE
} sqc_mutex_type_t;


typedef struct sqc_mutex_record 	*sqc_mutex_t;
typedef struct sqc_rwlock_record 	*sqc_rwlock_t;

typedef struct sqc_cond_record 	*sqc_cond_t;

typedef struct sqc_barrier_record 	*sqc_barrier_t;





sqc_result_t
sqc_mutex_create(sqc_mutex_t *mtxptr);

sqc_result_t
sqc_mutex_create_recursive(sqc_mutex_t *mtxptr);

void
sqc_mutex_destroy(sqc_mutex_t *mtxptr);

sqc_result_t
sqc_mutex_reinitialize(sqc_mutex_t *mtxptr);

sqc_result_t
sqc_mutex_get_type(sqc_mutex_t *mtxptr, sqc_mutex_type_t *tptr);

sqc_result_t
sqc_mutex_lock(sqc_mutex_t *mtxptr);

sqc_result_t
sqc_mutex_trylock(sqc_mutex_t *mtxptr);

sqc_result_t
sqc_mutex_timedlock(sqc_mutex_t *mtxptr, sqc_chrono_t nsec);


sqc_result_t
sqc_mutex_unlock(sqc_mutex_t *mtxptr);


sqc_result_t
sqc_mutex_enter_critical(sqc_mutex_t *mtxptr, int *ostateptr);

sqc_result_t
sqc_mutex_leave_critical(sqc_mutex_t *mtxptr, int ostate);





sqc_result_t
sqc_rwlock_create(sqc_rwlock_t *rwlptr);

void
sqc_rwlock_destroy(sqc_rwlock_t *rwlptr);

sqc_result_t
sqc_rwlock_reinitialize(sqc_rwlock_t *rwlptr);


sqc_result_t
sqc_rwlock_reader_lock(sqc_rwlock_t *rwlptr);

sqc_result_t
sqc_rwlock_reader_trylock(sqc_rwlock_t *rwlptr);

sqc_result_t
sqc_rwlock_reader_timedlock(sqc_rwlock_t *rwlptr,
                               sqc_chrono_t nsec);

sqc_result_t
sqc_rwlock_writer_lock(sqc_rwlock_t *rwlptr);

sqc_result_t
sqc_rwlock_writer_trylock(sqc_rwlock_t *rwlptr);

sqc_result_t
sqc_rwlock_writer_timedlock(sqc_rwlock_t *rwlptr,
                               sqc_chrono_t nsec);

sqc_result_t
sqc_rwlock_unlock(sqc_rwlock_t *rwlptr);


sqc_result_t
sqc_rwlock_reader_enter_critical(sqc_rwlock_t *rwlptr, int *ostateptr);

sqc_result_t
sqc_rwlock_writer_enter_critical(sqc_rwlock_t *rwlptr, int *ostateptr);

sqc_result_t
sqc_rwlock_leave_critical(sqc_rwlock_t *rwlptr, int ostate);





sqc_result_t
sqc_cond_create(sqc_cond_t *cndptr);

void
sqc_cond_destroy(sqc_cond_t *cndptr);

sqc_result_t
sqc_cond_wait(sqc_cond_t *cndptr,
                 sqc_mutex_t *mtxptr,
                 sqc_chrono_t nsec);

sqc_result_t
sqc_cond_notify(sqc_cond_t *cndptr,
                   bool for_all);





sqc_result_t
sqc_barrier_create(sqc_barrier_t *bptr, size_t n);


void
sqc_barrier_destroy(sqc_barrier_t *bptr);


sqc_result_t
sqc_barrier_wait(sqc_barrier_t *bptr, bool *is_master);





typedef pthread_spinlock_t sqc_spinlock_t;


static inline sqc_result_t
sqc_spinlock_initialize(sqc_spinlock_t *l) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(l != NULL)) {
    int st;
    errno = 0;
    if (likely((st = pthread_spin_init(l, PTHREAD_PROCESS_PRIVATE)) == 0)) {
      ret = SQC_RESULT_OK;
    } else {
      errno = st;
      ret = SQC_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
sqc_spinlock_lock(sqc_spinlock_t *l) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(l != NULL)) {
    int st;
    errno = 0;
    if (likely((st = pthread_spin_lock(l) == 0))) {
      ret = SQC_RESULT_OK;
    } else {
      errno = st;
      ret = SQC_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
sqc_spinlock_trylock(sqc_spinlock_t *l) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(l != NULL)) {
    int st;
    errno = 0;
    if (likely((st = pthread_spin_trylock(l) == 0))) {
      ret = SQC_RESULT_OK;
    } else {
      errno = st;
      ret = SQC_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
sqc_spinlock_unlock(sqc_spinlock_t *l) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(l != NULL)) {
    int st;
    errno = 0;
    if (likely((st = pthread_spin_unlock(l) == 0))) {
      ret = SQC_RESULT_OK;
    } else {
      errno = st;
      ret = SQC_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
sqc_spinlock_finalize(sqc_spinlock_t *l) {
  if (likely(l != NULL)) {
    (void)pthread_spin_destroy(l);
  }
}





__END_DECLS





#endif /* __SQC_LOCK_H__ */
