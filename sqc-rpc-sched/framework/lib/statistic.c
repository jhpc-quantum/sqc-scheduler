#include "sqc_apis.h"

#include <math.h>





typedef struct sqc_statistic_struct {
  const char *m_name;
  volatile size_t m_n;
  volatile int64_t m_min;
  volatile int64_t m_max;
  volatile int64_t m_sum;
  volatile int64_t m_sum2;
} sqc_statistic_struct;





static pthread_once_t s_once = PTHREAD_ONCE_INIT;
static bool s_is_inited = false;
static sqc_hashmap_t s_stat_tbl;





static void s_ctors(void) __attr_constructor__(112);
static void s_dtors(void) __attr_destructor__(112);

static void s_destroy_stat(sqc_statistic_t s, bool delhash);
static void s_stat_freeup(void *arg);

static sqc_result_t s_reset_stat(sqc_statistic_t s);





static void
s_once_proc(void) {
  sqc_result_t r;

  if ((r = sqc_hashmap_create(&s_stat_tbl,
                                 SQC_HASHMAP_TYPE_STRING,
                                 s_stat_freeup)) != SQC_RESULT_OK) {
    sqc_perror(r);
    sqc_exit_fatal("can't initialize the stattistics table.\n");
  }

  s_is_inited = true;
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();

  sqc_msg_debug(10, "The statistics module is initialized.\n");
}


static inline void
s_final(void) {
  sqc_hashmap_destroy(&s_stat_tbl, true);
}


static void
s_dtors(void) {
  if (s_is_inited == true) {
    if (sqc_module_is_unloading() &&
        sqc_module_is_finalized_cleanly()) {
      s_final();

      sqc_msg_debug(10, "The stattitics module is finalized.\n");
    } else {
      sqc_msg_debug(10, "The stattitics module is not finalized "
                       "because of module finalization problem.\n");
    }
  }
}





static void
s_stat_freeup(void *arg) {
  if (likely(arg != NULL)) {
    sqc_statistic_t s = (sqc_statistic_t)arg;
    s_destroy_stat(s, false);
  }
}





static inline sqc_result_t
s_create_stat(sqc_statistic_t *sptr, const char *name) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL &&
             IS_VALID_STRING(name) == true)) {
    sqc_statistic_t s = (sqc_statistic_t)malloc(sizeof(*s));
    const char *m_name = strdup(name);
    *sptr = NULL;

    if (likely(s != NULL && IS_VALID_STRING(m_name) == true)) {
      void *val = (void *)s;
      if (likely((ret = sqc_hashmap_add(&s_stat_tbl, (void *)m_name,
                                           &val, false)) ==
                 SQC_RESULT_OK)) {
        s->m_name = m_name;
        s_reset_stat(s);
        *sptr = s;
      }
    } else {
      ret = SQC_RESULT_NO_MEMORY;
    }

    if (unlikely(ret != SQC_RESULT_OK)) {
      free((void *)s);
      free((void *)m_name);
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_destroy_stat(sqc_statistic_t s, bool delhash) {
  if (likely(s != NULL)) {
    if (delhash == true) {
      (void)sqc_hashmap_delete(&s_stat_tbl,
                                  (void *)s->m_name, NULL, false);
    }
    if (s->m_name != NULL) {
      free((void *)s->m_name);
    }
    free((void *)s);
  }
}


static inline sqc_result_t
s_find_stat(sqc_statistic_t *sptr, const char *name) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL &&
             IS_VALID_STRING(name) == true)) {
    void *val = NULL;

    *sptr = NULL;

    ret = sqc_hashmap_find(&s_stat_tbl, (void *)name, &val);
    if (likely(ret == SQC_RESULT_OK)) {
      *sptr = (sqc_statistic_t)val;
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_reset_stat(sqc_statistic_t s) {
  if (likely(s != NULL)) {
    s->m_n = 0LL;
    s->m_min = LLONG_MAX;
    s->m_max = LLONG_MIN;
    s->m_sum = 0LL;
    s->m_sum2 = 0LL;
    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


static inline sqc_result_t
s_record_stat(sqc_statistic_t s, int64_t val) {
  /*
   * TODO:
   *
   *	Well. should I use a mutex to provide the atomicity? I wonder
   *	four or more atomic operations costs more than takig a mutex
   *	lock or not.
   */

  if (likely(s != NULL)) {
    int64_t sum2;

    sum2 = val * val;

    (void)__sync_add_and_fetch(&(s->m_n), 1);
    (void)__sync_add_and_fetch(&(s->m_sum), val);
    (void)__sync_add_and_fetch(&(s->m_sum2), sum2);

    sqc_atomic_update_min(int64_t, &(s->m_min), LLONG_MAX, val);
    sqc_atomic_update_max(int64_t, &(s->m_max), LLONG_MIN, val);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


static inline sqc_result_t
s_get_n_stat(sqc_statistic_t s) {
  if (likely(s != NULL)) {
    return (sqc_result_t)(__sync_fetch_and_add(&(s->m_n), 0));
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


static inline sqc_result_t
s_get_min_stat(sqc_statistic_t s, int64_t *valptr) {
  if (likely(s != NULL && valptr != NULL)) {
    *valptr = (__sync_fetch_and_add(&(s->m_min), 0));
    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


static inline sqc_result_t
s_get_max_stat(sqc_statistic_t s, int64_t *valptr) {
  if (likely(s != NULL && valptr != NULL)) {
    *valptr = (__sync_fetch_and_add(&(s->m_max), 0));
    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


static inline sqc_result_t
s_get_avg_stat(sqc_statistic_t s, double *valptr) {
  if (likely(s != NULL && valptr != NULL)) {
    int64_t n = (int64_t)__sync_fetch_and_add(&(s->m_n), 0);

    if (n > 0) {
      double sum = (double)__sync_fetch_and_add(&(s->m_sum), 0);
      *valptr = sum / (double)n;
    } else {
      *valptr = 0.0;
    }

    return SQC_RESULT_OK;

  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


static inline sqc_result_t
s_get_sd_stat(sqc_statistic_t s, double *valptr, bool is_ssd) {
  if (likely(s != NULL && valptr != NULL)) {
    int64_t n = (int64_t)__sync_fetch_and_add(&(s->m_n), 0);

    if (n == 0) {
      *valptr = 0.0;
    } else {
      double sum = (double)__sync_fetch_and_add(&(s->m_sum), 0);
      double sum2 = (double)__sync_fetch_and_add(&(s->m_sum2), 0);
      double avg = sum / (double)n;
      double ssum =
        sum2 -
        2.0 * avg * sum +
        avg * avg * (double)n;

      if (is_ssd == true) {
        if (n >= 2) {
          *valptr = sqrt((ssum / (double)(n - 1)));
        } else {
          *valptr = 0.0;
        }
      } else {
        if (n >= 1) {
          *valptr = sqrt(ssum / (double)n);
        } else {
          *valptr = 0.0;
        }
      }
    }

    return SQC_RESULT_OK;

  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}





/*
 * Exported APIs
 */


sqc_result_t
sqc_statistic_create(sqc_statistic_t *sptr, const char *name) {
  return s_create_stat(sptr, name);
}


sqc_result_t
sqc_statistic_find(sqc_statistic_t *sptr, const char *name) {
  return s_find_stat(sptr, name);
}


void
sqc_statistic_destroy(sqc_statistic_t *sptr) {
  if (likely(sptr != NULL && *sptr != NULL)) {
    s_destroy_stat(*sptr, true);
  }
}


void
sqc_statistic_destroy_by_name(const char *name) {
  if (likely(IS_VALID_STRING(name) == true)) {
    sqc_statistic_t s = NULL;
    if (likely(s_find_stat(&s, name) == SQC_RESULT_OK &&
               s != NULL)) {
      s_destroy_stat(s, true);
    }
  }
}


sqc_result_t
sqc_statistic_record(sqc_statistic_t *sptr, int64_t val) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL)) {
    ret = s_record_stat(*sptr, val);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_statistic_reset(sqc_statistic_t *sptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL)) {
    ret = s_reset_stat(*sptr);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_statistic_sample_n(sqc_statistic_t *sptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL)) {
    ret = s_get_n_stat(*sptr);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_statistic_min(sqc_statistic_t *sptr, int64_t *valptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL && valptr != NULL)) {
    ret = s_get_min_stat(*sptr, valptr);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_statistic_max(sqc_statistic_t *sptr, int64_t *valptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL && valptr != NULL)) {
    ret = s_get_max_stat(*sptr, valptr);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_statistic_average(sqc_statistic_t *sptr, double *valptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL && valptr != NULL)) {
    ret = s_get_avg_stat(*sptr, valptr);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_statistic_sd(sqc_statistic_t *sptr, double *valptr, bool is_ssd) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL && valptr != NULL)) {
    ret = s_get_sd_stat(*sptr, valptr, is_ssd);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}
