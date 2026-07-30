#include "unity.h"
#include "sqc_apis.h"
#include "sqc_thread_internal.h"

#define GET_TID_RETRY 10
#define N_ENTRY 100
#define Q_PUTGET_WAIT 10000000LL
#define N_LOOP 100
#define MAIN_SLEEP_USEC 2000LL
#define WAIT_NSEC 1000LL * 1000LL * 1000LL

typedef struct entry {
  int num;
} entry;

typedef SQC_BOUND_BLOCK_Q_DECL(test_bbq_t, entry *) test_bbq_t;
typedef sqc_result_t (*test_thread_proc_t)(test_bbq_t *qptr);

typedef struct {
  sqc_thread_record m_thread;  /* must be on the head. */

  test_bbq_t *m_qptr;
  test_thread_proc_t m_proc;

  int m_loop_times;
  useconds_t m_sleep_usec;
  int *m_dummy;
  sqc_mutex_t m_lock;
} test_thread_record;

typedef test_thread_record *test_thread_t;

static test_bbq_t BBQ = NULL;
static int s_entry_number = 0;

void setUp(void);
void tearDown(void);

static void s_qvalue_freeup(void **);

static entry *
s_new_entry(void);

static bool
s_test_thread_create(test_thread_t *ttptr,
                     test_bbq_t *q_ptr,
                     int loop_times,
                     useconds_t sleep_usec,
                     const char *name,
                     test_thread_proc_t proc);

static sqc_result_t
s_put(test_bbq_t *qptr);
static sqc_result_t
s_get(test_bbq_t *qptr);
static sqc_result_t
s_clear(test_bbq_t *qptr);


/*
 * setup & teardown. create/destroy BBQ.
 */
void
setUp(void) {
  sqc_result_t ret;
  ret = sqc_bbq_create(&BBQ, entry *, N_ENTRY, s_qvalue_freeup);
  if (ret != SQC_RESULT_OK) {
    sqc_perror(ret);
    exit(1);
  }
}

void
tearDown(void) {
  sqc_bbq_shutdown(&BBQ, true);
  sqc_bbq_destroy(&BBQ, false);
  BBQ=NULL;
}


/*
 * entry functions
 */
static entry *
s_new_entry(void) {
  entry *e = (entry *)malloc(sizeof(entry));
  if (e != NULL) {
    e->num = s_entry_number;
    s_entry_number++;
  }
  return e;
}

/*
 * queue functions
 */
static void
s_qvalue_freeup(void **arg) {
  if (arg != NULL) {
    entry *eptr = (entry *)*arg;
    free(eptr);
  }
}

/*
 * test_thread_proc_t functions
 */
static sqc_result_t
s_put(test_bbq_t *qptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  entry *pptr = s_new_entry();
  ret = sqc_bbq_put(qptr, &pptr, entry *, Q_PUTGET_WAIT);
  sqc_msg_debug(10, "put %p %d ... %s\n",
                   pptr, (pptr == NULL) ? -1 : pptr->num,
                   sqc_error_get_string(ret));
  return ret;
}

static sqc_result_t
s_get(test_bbq_t *qptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  entry *gptr = NULL;
  ret = sqc_bbq_get(qptr, &gptr, entry *, Q_PUTGET_WAIT);
  sqc_msg_debug(10, "get %p %d ... %s\n",
                   gptr, (gptr == NULL) ? -1 : gptr->num,
                   sqc_error_get_string(ret));
  free(gptr);
  return ret;
}


static sqc_result_t
s_clear(test_bbq_t *qptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  ret = sqc_bbq_clear(qptr, true);
  sqc_msg_debug(10, "clear %s\n", sqc_error_get_string(ret));
  return ret;
}

/*
 * thread functions
 */
static sqc_result_t
s_thread_main(const sqc_thread_t *selfptr, void *arg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  int i = 0;
  (void)arg;
  if (selfptr != NULL) {
    int cstate;
    test_thread_t tt = (test_thread_t)*selfptr;
    if (tt != NULL && tt->m_proc != NULL && tt-> m_qptr != NULL) {
      for (i = 0; i < tt->m_loop_times; i++) {
        sqc_msg_debug(30, "loop-%d start\n", i);
        sqc_mutex_enter_critical(&tt->m_lock, &cstate);
        {
          ret = tt->m_proc(tt->m_qptr);
        }
        sqc_mutex_leave_critical(&tt->m_lock, cstate);
        switch (ret) {
          case SQC_RESULT_OK:
          case SQC_RESULT_TIMEDOUT:
          case SQC_RESULT_NOT_OPERATIONAL:
            break;
          default:
            sqc_perror(ret);
            TEST_FAIL_MESSAGE("operation error");
            break;
        }
        usleep(tt->m_sleep_usec);
      }
      ret = SQC_RESULT_OK;
      sqc_msg_debug(1, "done!\n");
    } else {
      TEST_FAIL_MESSAGE("invalid thread");
    }
  } else {
    TEST_FAIL_MESSAGE("invalid thread");
  }
  return ret;
}

static void
s_thread_finalize(const sqc_thread_t *selfptr,
                  bool is_canceled, void *arg) {
  (void)selfptr;
  (void)is_canceled;
  (void)arg;
  sqc_msg_debug(1, "finalize done\n");
}

static void
s_thread_freeup(const sqc_thread_t *selfptr, void *arg) {
  (void)arg;

  if (selfptr != NULL) {
    test_thread_t tt = (test_thread_t)*selfptr;
    if (tt != NULL) {
      sqc_mutex_destroy(&tt->m_lock);
      free(tt->m_dummy);
    }
  }
  sqc_msg_debug(1, "freeup done\n");
}


static inline bool
s_test_thread_create(test_thread_t *ttptr,
                     test_bbq_t *q_ptr,
                     int loop_times,
                     useconds_t sleep_usec,
                     const char *name,
                     test_thread_proc_t proc) {
  bool ret = false;
  sqc_result_t res = SQC_RESULT_ANY_FAILURES;

  if (ttptr != NULL) {
    /* init test_thread_t */
    test_thread_t tt;
    if (*ttptr == NULL) {
      tt = (test_thread_t)malloc(sizeof(*tt));
      if (tt == NULL) {
        goto done;
      }
    } else {
      tt = *ttptr;
    }
    /* init sqc_thread_t */
    res = sqc_thread_create((sqc_thread_t *)&tt,
                               s_thread_main,
                               s_thread_finalize,
                               s_thread_freeup,
                               name, NULL);
    if (res != SQC_RESULT_OK) {
      sqc_perror(res);
      goto done;
    }
    sqc_thread_free_when_destroy((sqc_thread_t *)&tt);
    /* create member */
    tt->m_dummy = (int *)malloc(sizeof(int) * N_ENTRY);
    if (tt->m_dummy == NULL) {
      sqc_msg_error("malloc error\n");
      sqc_thread_destroy((sqc_thread_t *)&tt);
      goto done;
    }
    res = sqc_mutex_create(&(tt->m_lock));
    if (res != SQC_RESULT_OK) {
      sqc_perror(res);
      free(tt->m_dummy);
      sqc_thread_destroy((sqc_thread_t *)&tt);
      goto done;
    }

    tt->m_proc = proc;
    tt->m_qptr = q_ptr;
    tt->m_loop_times = loop_times;
    tt->m_sleep_usec = sleep_usec;
    *ttptr = tt;
    ret = true;
  }

done:
  return ret;
}


/*
 * checking thread
 */
static inline bool
s_check_start(test_thread_t *ttptr, sqc_result_t require_ret) {
  bool result = false;
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  ret = sqc_thread_start((sqc_thread_t *)ttptr, false);
  if (ret == require_ret) {
    result = true;
  } else {
    sqc_perror(ret);
    sqc_msg_error("start failed");
    result = false;
  }
  return result;
}

static inline bool
s_check_wait(test_thread_t *ttptr, sqc_result_t require_ret) {
  bool result = false;
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  ret = sqc_thread_wait((sqc_thread_t *)ttptr, WAIT_NSEC);
  if (ret != require_ret) {
    sqc_perror(ret);
    sqc_msg_error("wait failed");
    result = false;
  } else {
    result = true;
  }
  return result;
}

static inline bool
s_check_cancel(test_thread_t *ttptr,
               sqc_result_t require_ret) {
  bool result = false;
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  sqc_msg_debug(1, "cancel start\n");
  ret = sqc_thread_cancel((sqc_thread_t *)ttptr);
  sqc_msg_debug(1, "cancel done, %s\n", sqc_error_get_string(ret));
  if (ret != require_ret) {
    sqc_perror(ret);
    sqc_msg_error("cancel failed");
    result = false;
  } else {
    result = true;
  }

  return result;
}


/* wrapper */
bool s_create_put_tt(test_thread_t *put_tt) {
  return s_test_thread_create(put_tt, &BBQ, N_LOOP, MAIN_SLEEP_USEC,
                              "put_thread", s_put);
}
bool s_create_get_tt(test_thread_t *get_tt) {
  return s_test_thread_create(get_tt, &BBQ, N_LOOP, MAIN_SLEEP_USEC,
                              "get_thread", s_get);
}
bool s_create_clear_tt(test_thread_t *clear_tt) {
  return s_test_thread_create(clear_tt, &BBQ,
                              (int)(N_LOOP / 5), MAIN_SLEEP_USEC * 5,
                              "clear_thread", s_clear);
}

/*
 * tests
 */

void
test_creation(void) {
  test_thread_t put_tt = NULL, get_tt = NULL, clear_tt = NULL;
  if (s_create_put_tt(&put_tt) != true) {
    TEST_FAIL_MESSAGE("creation error");
    goto destroy;
  }
  if (s_create_get_tt(&get_tt) != true) {
    TEST_FAIL_MESSAGE("creation error");
    goto destroy;
  }
  if (s_create_clear_tt(&clear_tt) != true) {
    TEST_FAIL_MESSAGE("creation error");
    goto destroy;
  }
  usleep(10000);
destroy:
  sqc_thread_destroy((sqc_thread_t *)&put_tt);
  sqc_thread_destroy((sqc_thread_t *)&get_tt);
  sqc_thread_destroy((sqc_thread_t *)&clear_tt);
}


void
test_putget(void) {
  test_thread_t put_tt = NULL, get_tt = NULL;
  if (s_create_put_tt(&put_tt) != true) {
    TEST_FAIL_MESSAGE("creation error");
    goto destroy;
  }
  if (s_create_get_tt(&get_tt) != true) {
    TEST_FAIL_MESSAGE("creation error");
    goto destroy;
  }
  usleep(10000);

  if (s_check_start(&put_tt, SQC_RESULT_OK) == false) {
    goto destroy;
  }
  usleep(10000);
  if (s_check_start(&get_tt, SQC_RESULT_OK) == false) {
    goto destroy;
  }

  usleep(MAIN_SLEEP_USEC * (N_LOOP - 3));

  if (s_check_wait(&put_tt, SQC_RESULT_OK) == false) {
    goto destroy;
  }
  if (s_check_wait(&get_tt, SQC_RESULT_OK) == false) {
    goto destroy;
  }
  usleep(10000);

destroy:
  sqc_thread_destroy((sqc_thread_t *)&put_tt);
  sqc_thread_destroy((sqc_thread_t *)&get_tt);
}

void
test_put_clear(void) {
  test_thread_t put_tt = NULL, clear_tt = NULL;
  if (s_create_put_tt(&put_tt) != true) {
    TEST_FAIL_MESSAGE("creation error");
    goto destroy;
  }
  if (s_create_clear_tt(&clear_tt) != true) {
    TEST_FAIL_MESSAGE("creation error");
    goto destroy;
  }
  usleep(10000);

  if (s_check_start(&put_tt, SQC_RESULT_OK) == false) {
    goto destroy;
  }
  if (s_check_start(&clear_tt, SQC_RESULT_OK) == false) {
    goto destroy;
  }

  usleep(MAIN_SLEEP_USEC * (N_LOOP - 3));

  if (s_check_wait(&put_tt, SQC_RESULT_OK) == false) {
    goto destroy;
  }
  if (s_check_wait(&clear_tt, SQC_RESULT_OK) == false) {
    goto destroy;
  }
  usleep(10000);

destroy:
  sqc_thread_destroy((sqc_thread_t *)&put_tt);
  sqc_thread_destroy((sqc_thread_t *)&clear_tt);
}

/* TODO: fix deadlock at destroy */
void
test_putget_cancel(void) {
  test_thread_t put_tt = NULL, get_tt = NULL;
  if (s_create_put_tt(&put_tt) != true) {
    TEST_FAIL_MESSAGE("creation error");
    goto destroy;
  }
  if (s_create_get_tt(&get_tt) != true) {
    TEST_FAIL_MESSAGE("creation error");
    goto destroy;
  }
  usleep(10000);

  if (s_check_start(&put_tt, SQC_RESULT_OK) == false) {
    goto destroy;
  }
  if (s_check_start(&get_tt, SQC_RESULT_OK) == false) {
    goto destroy;
  }

  usleep(MAIN_SLEEP_USEC * N_LOOP);

destroy:
  sqc_thread_destroy((sqc_thread_t *)&put_tt);
  sqc_thread_destroy((sqc_thread_t *)&get_tt);

  usleep(10000);
}
