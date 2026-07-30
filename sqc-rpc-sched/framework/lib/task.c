#include "sqc_apis.h"

#include "sqc_poolable_internal.h"
#include "sqc_thread_internal.h"
#include "sqc_pool_internal.h"
#include "sqc_pooled_thread_internal.h"
#include "sqc_task_internal.h"





/*
 * sz must include a size of *arg.
 */
sqc_result_t
sqc_task_create(sqc_task_t *tptr, size_t sz, const char *name,
                   sqc_task_main_proc_t main_func,
                   sqc_task_finalize_proc_t finalize_func,
                   sqc_task_freeup_proc_t freeup_func) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  size_t alloc_sz = sz;
  sqc_task_t t = NULL;

  if (likely(tptr != NULL && main_func != NULL)) {
    if (*tptr == NULL) {
      if (alloc_sz <= 0) {
        alloc_sz = sizeof(sqc_task_record);
      } else if (alloc_sz < sizeof(sqc_task_record)) {
        ret = SQC_RESULT_TOO_SMALL;
        sqc_perror(ret);
        sqc_msg_error("task size is too small.\n");
        goto done;
      }
      *tptr = t = (sqc_task_t)malloc(alloc_sz);
      if (unlikely(*tptr == NULL)) {
        ret = SQC_RESULT_NO_MEMORY;
        sqc_perror(ret);
        sqc_msg_error("can't allocate a task.\n");
        goto done;
      }
    } else {
      t = *tptr;
    }

    t->m_state = SQC_TASK_STATE_UNKNOWN;
    if (IS_VALID_STRING(name) == true) {
      t->m_name = strdup(name);
      if (unlikely(t->m_name == NULL)) {
        ret = SQC_RESULT_NO_MEMORY;
        sqc_perror(ret);
        sqc_msg_error("can'r allocate a task name.\n");
        goto done;
      }
    } else {
      t->m_name = NULL;
    }
    if (unlikely((ret = sqc_mutex_create(&t->m_lck)) != SQC_RESULT_OK)) {
      sqc_perror(ret);
      sqc_msg_error("can't create a mutex for a task.\n");
      goto done;
    }
    if (unlikely((ret = sqc_cond_create(&t->m_cnd)) != SQC_RESULT_OK)) {
      sqc_perror(ret);
      sqc_msg_error("can't create a cond for a task.\n");
      goto done;
    }
    t->m_total_obj_size = alloc_sz;
    t->m_exit_code = SQC_RESULT_NOT_STARTED;
    t->m_is_started = false;
    t->m_is_clean_finished = false;
    t->m_is_runner_shutdown = false;
    t->m_is_cancelled = false;
    t->m_is_wait_done = false;
    t->m_main = main_func;
    t->m_finalize = finalize_func;
    t->m_freeup = freeup_func;
    t->m_core_num = -1;
    t->m_numa_node_num = -1;
    t->m_flag = 0;
    t->m_tr = NULL;
    t->m_tmp_thd = NULL;

    t->m_state = SQC_TASK_STATE_CONSTRUCTED;

    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

done:
  if (ret != SQC_RESULT_OK && t != NULL) {
    sqc_task_destroy(&t);
  }

  return ret;
}


sqc_result_t
sqc_task_set_cpu_affinity(const sqc_task_t *tptr, int cpu) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(tptr != NULL && *tptr != NULL)) {
    (*tptr)->m_core_num = cpu;
    if (likely((*tptr)->m_tr != NULL)) {
      if (likely(sqc_thread_set_cpu_affinity((sqc_thread_t *)&(*tptr)->m_tr,
                 -1) ==
                 SQC_RESULT_OK)) {
        ret = sqc_thread_set_cpu_affinity((sqc_thread_t *)&(*tptr)->m_tr,
                                             cpu);
      }
    } else {
      ret = SQC_RESULT_OK;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_task_set_numa_node_affinity(const sqc_task_t *tptr, int node) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(tptr != NULL && *tptr != NULL)) {
    (*tptr)->m_numa_node_num = node;
    if (likely((*tptr)->m_tr != NULL)) {
      ret = sqc_thread_set_numa_node_affinity((sqc_thread_t *)&(*tptr)->m_tr,
            node);
    } else {
      ret = SQC_RESULT_OK;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_task_run(sqc_task_t *tptr, sqc_pooled_thread_t *ptptr,
                int flag) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_task_t t = NULL;

  if (likely(tptr != NULL && (t = *tptr) != NULL && t->m_main != NULL &&
             t->m_tr == NULL)) {
    sqc_pooled_thread_t pt = NULL;
    sqc_poolable_t pobj = NULL;
    sqc_task_runner_thread_t tr = NULL;

    if (likely(ptptr != NULL && (pt = *ptptr) != NULL &&
               (pobj = (sqc_poolable_t)pt) != NULL &&
               pobj->m_is_used == true &&
               (tr = &pt->m_trthd) != NULL)) {

      (void)sqc_mutex_lock(&tr->m_lck);
      {

        (void)sqc_mutex_lock(&t->m_lck);
        {

          t->m_state = SQC_TASK_STATE_ATTACHED;
          t->m_flag = flag;

        }
        (void)sqc_mutex_unlock(&t->m_lck);

        tr->m_tsk = t;
        tr->m_is_got_a_task = true;

        /*
         * Signal task runner to exec this task.
         */
        ret = sqc_cond_notify(&tr->m_cnd, true);

      }
      (void)sqc_mutex_unlock(&tr->m_lck);

      if (likely(ret == SQC_RESULT_OK)) {
        if ((t->m_flag & SQC_TASK_DELETE_CONTEXT_AFTER_EXEC) != 0) {
          /*
           * no one can touch this task anymore.
           */
          *tptr = NULL;
        }
      }

    } else if (pt == NULL) {
      /*
       * TODO:
       *	Launch temp thread to run the task.
       */
      ret = SQC_RESULT_NOT_OPERATIONAL;
    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
sqc_task_finalize(sqc_task_t *tptr, bool is_cancelled) {
  sqc_task_t t = NULL;

  sqc_msg_debug(5, "called, cancel %s.\n",
                   (is_cancelled == true) ? "yes" : "no");

  if (likely(tptr != NULL && (t = *tptr) != NULL)) {

    if (t->m_finalize != NULL) {
      t->m_finalize(tptr, is_cancelled);
    }

    if (unlikely(is_cancelled == true)) {
      (void)sqc_mutex_unlock(&t->m_lck);
    }

    sqc_msg_debug(5, "about to wake task \"%s\" waiters up...\n", t->m_name);

    (void)sqc_mutex_lock(&t->m_lck);
    {

      t->m_is_wait_done = true;

      (void)sqc_cond_notify(&t->m_cnd, true);

      sqc_msg_debug(5, "woke task \"%s\" waiters up.\n", t->m_name);

    }
    (void)sqc_mutex_unlock(&t->m_lck);
  }
}


sqc_result_t
sqc_task_wait(sqc_task_t *tptr, sqc_chrono_t to) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_task_t t = NULL;

  if (likely(tptr != NULL && (t = *tptr) != NULL)) {

    sqc_msg_debug(5, "wait for task \"%s\" done...\n", t->m_name);

    (void)sqc_mutex_lock(&t->m_lck);
    {

      do {
        if (likely(t->m_is_wait_done == true)) {
          ret = SQC_RESULT_OK;
          break;
        } else {
          ret = sqc_cond_wait(&t->m_cnd, &t->m_lck, to);
          if (unlikely(ret != SQC_RESULT_OK)) {
            break;
          }
        }
      } while (true);

    }
    (void)sqc_mutex_unlock(&t->m_lck);

    sqc_msg_debug(5, "task \"%s\" done, returns %ld.\n",
                     t->m_name, t->m_exit_code);

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_task_get_exit_code(sqc_task_t *tptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_task_t t = NULL;

  if (likely(tptr != NULL && (t = *tptr) != NULL)) {

    (void)sqc_mutex_lock(&t->m_lck);
    {

      ret = t->m_exit_code;

    }
    (void)sqc_mutex_unlock(&t->m_lck);

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_task_get_state(sqc_task_t *tptr, sqc_task_state_t *stptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_task_t t = NULL;

  if (likely(tptr != NULL && (t = *tptr) != NULL && stptr != NULL)) {

    (void)sqc_mutex_lock(&t->m_lck);
    {

      *stptr = t->m_state;

    }
    (void)sqc_mutex_unlock(&t->m_lck);

    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
sqc_task_destroy(sqc_task_t *tptr) {
  sqc_task_t t = NULL;
  if (tptr != NULL && (t = *tptr) != NULL &&
      t->m_state != SQC_TASK_STATE_RUNNING &&
      t->m_state != SQC_TASK_STATE_CANCELLED) {

    if (t->m_freeup != NULL) {
      t->m_freeup(tptr);
    }

    if (t->m_cnd != NULL) {

      if (t->m_lck != NULL) {
        (void)sqc_mutex_lock(&t->m_lck);
      }
      {

        sqc_cond_notify(&t->m_cnd, true);

      }
      if (t->m_lck != NULL) {
        (void)sqc_mutex_unlock(&t->m_lck);
      }
      sqc_cond_destroy(&t->m_cnd);

    }

    if (t->m_lck != NULL) {
      sqc_mutex_destroy(&t->m_lck);
    }

    free(t->m_name);

    if (t->m_total_obj_size != 0) {
      free(t);
      *tptr = NULL;
    }
  }
}


