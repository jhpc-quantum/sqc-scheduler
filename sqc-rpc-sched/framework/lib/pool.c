#include "sqc_apis.h"
#include "sqc_pool_internal.h"
#include "sqc_poolable_internal.h"





static sqc_hashmap_t s_pools = NULL;





static void
s_poolable_freeup(void **arg) {
  if (likely(arg != NULL)) {
    sqc_poolable_t p = (sqc_poolable_t)*arg;
    if (p->m_destruct != NULL) {
      p->m_destruct(&p);
    }
  }
}





sqc_result_t
sqc_pool_get_pool(const char *name, sqc_pool_t *pptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(pptr != NULL && IS_VALID_STRING(name) == true)) {
    void *v = NULL;
    ret = sqc_hashmap_find(&s_pools, name, &v);
    *pptr = (sqc_pool_t )v;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_pool_create(sqc_pool_t *pptr, size_t sz, const char *name,
                   sqc_pool_type_t type,
                   bool is_executor,
                   size_t n_max_objs,
                   size_t pobj_size,
                   sqc_poolable_methods_t m) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_pool_t p = NULL;

  if (likely(pptr != NULL && IS_VALID_STRING(name) == true &&
             (type == SQC_POOL_TYPE_QUEUE || type == SQC_POOL_TYPE_INDEX) &&
             n_max_objs > 0 && m != NULL)) {
    size_t alloc_sz = sz;
    void *add = NULL;

    if (*pptr == NULL) {
      if (alloc_sz < sizeof(sqc_pool_record)) {
        alloc_sz = sizeof(sqc_pool_record);
      }
      p = (sqc_pool_t)malloc(alloc_sz);
      if (unlikely(p == NULL)) {
        ret = SQC_RESULT_NO_MEMORY;
        sqc_perror(ret);
        sqc_msg_error("can't allocalte a pool.\n");
        goto done;
      }
    } else {
      p = *pptr;
    }

    (void)memset(p, 0, sizeof(sqc_pool_record));

    p->m_type = type;
    p->m_is_executor = is_executor;
    p->m_state = SQC_POOL_STATE_CONSTRUCTED;
    p->m_total_obj_size = alloc_sz;

    p->m_is_awakened = false;
    p->m_is_cancelled = false;
    p->m_n_waiters = 0;

    p->m_name = strdup(name);
    if (unlikely(p->m_name == NULL)) {
      ret = SQC_RESULT_NO_MEMORY;
      sqc_perror(ret);
      sqc_msg_error("can't allocalte a pool.\n");
      goto done;
    }

    ret = sqc_mutex_create_recursive(&p->m_lck);
    if (unlikely(ret != SQC_RESULT_OK)) {
      sqc_perror(ret);
      sqc_msg_error("can't create a mutex for a pool.\n");
      goto done;
    }

    p->m_obj_idx = 0;

    p->m_m = *m;
    p->m_pobj_size = pobj_size;
    p->m_n_max = n_max_objs;
    p->m_n_cur = 0;
    p->m_n_free = n_max_objs;

    p->m_objs = (sqc_poolable_t *)malloc(sizeof(sqc_poolable_t) *
                                            n_max_objs);
    if (likely(p->m_objs != NULL)) {
      (void)memset(p->m_objs, 0, sizeof(sqc_poolable_t) * n_max_objs);
    } else {
      ret = SQC_RESULT_NO_MEMORY;
      sqc_perror(ret);
      sqc_msg_error("can't allocalte a poolable methods obj. for a pool.\n");
      goto done;
    }

    if (type == SQC_POOL_TYPE_INDEX) {
      ret = sqc_cond_create(&p->m_cnd);
      if (unlikely(ret != SQC_RESULT_OK)) {
        sqc_perror(ret);
        sqc_msg_error("can't create a cond for a pool.\n");
        goto done;
      }
      ret = sqc_cond_create(&p->m_awakened_cnd);
      if (unlikely(ret != SQC_RESULT_OK)) {
        sqc_perror(ret);
        sqc_msg_error("can't create a wakeup cond for a pool.\n");
        goto done;
      }
    } else {
      ret = sqc_bbq_create(&p->m_free_q, sqc_poolable_t,
                              (int64_t)(n_max_objs + 1),
                              s_poolable_freeup);
      if (unlikely(ret != SQC_RESULT_OK)) {
        sqc_perror(ret);
        sqc_msg_error("can't allocalte a poolable obj. free "
                         "queue for a pool.\n");
        goto done;
      }
    }

    add = p;
    ret = sqc_hashmap_add(&s_pools, name, &add, false);

    *pptr = p;

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

done:

  if (ret == SQC_RESULT_OK) {
    if (p != NULL) {
      p->m_state = SQC_POOL_STATE_OPERATIONAL;
    }
  } else {
    *pptr = NULL;

    sqc_pool_destroy(&p);
  }

  return ret;
}


sqc_result_t
sqc_pool_add_poolable_by_index(sqc_pool_t *pptr, uint64_t index,
                                  void *args) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_pool_t p = NULL;

  if (likely(pptr != NULL && ((p = *pptr) != NULL) &&
             p->m_state == SQC_POOL_STATE_OPERATIONAL &&
             p->m_type == SQC_POOL_TYPE_INDEX &&
             index < p->m_n_max && p->m_objs[index] == NULL)) {
    sqc_poolable_t pobj = NULL;

    sqc_mutex_lock(&p->m_lck);
    {
      ret = sqc_poolable_create_with_size(&pobj, p->m_is_executor, &p->m_m,
                                             args, p->m_pobj_size);
      if (likely(ret == SQC_RESULT_OK)) {
        ret = sqc_poolable_setup(&pobj);
        if (likely(ret == SQC_RESULT_OK)) {
          p->m_objs[index] = pobj;
          pobj->m_obj_idx = index;
          pobj->m_pool = p;
          if (p->m_obj_idx < index) {
            p->m_obj_idx = index;
          }
          p->m_n_cur++;
        } else {
          sqc_poolable_destroy(&pobj);
        }
      }
    }
    sqc_mutex_unlock(&p->m_lck);

  } else if (p != NULL) {
    if (p->m_state != SQC_POOL_STATE_CONSTRUCTED) {
      ret = SQC_RESULT_NOT_OPERATIONAL;
    } else if (index >= p->m_n_max) {
      ret = SQC_RESULT_TOO_LARGE;
    } else if (p->m_objs[index] != NULL) {
      ret = SQC_RESULT_ALREADY_EXISTS;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_pool_acquire_poolable_by_index(sqc_pool_t *pptr, uint64_t index,
                                      sqc_chrono_t to, sqc_poolable_t *pobjptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_pool_t p = NULL;
  sqc_poolable_t pobj = NULL;

  if (likely(pobjptr != NULL && pptr != NULL && (p = *pptr) != NULL &&
             p->m_state == SQC_POOL_STATE_OPERATIONAL &&
             p->m_type == SQC_POOL_TYPE_INDEX &&
             index <= p->m_obj_idx && (pobj = p->m_objs[index]) != NULL)) {
    size_t n_waiters = 0;
    bool is_awakened = false;
    *pobjptr = NULL;

    sqc_mutex_lock(&p->m_lck);
    {

      n_waiters = __atomic_fetch_add(&p->m_n_waiters, 1, __ATOMIC_ACQ_REL);

      while (true) {
        if (pobj->m_is_used == false) {
          ret = SQC_RESULT_OK;
          break;
        } else {
          ret = sqc_cond_wait(&p->m_cnd, &p->m_lck, to);
          if (ret == SQC_RESULT_OK && p->m_is_awakened == true) {
            is_awakened = true;
            break;
          } else if (ret != SQC_RESULT_OK) {
            break;
          }
        }
      }

      n_waiters = __atomic_sub_fetch(&p->m_n_waiters, 1, __ATOMIC_ACQ_REL);

      if (is_awakened == true) {
        if (n_waiters == 0) {
          /*
           * ACK for a wakeup API caller.
           */
          (void)sqc_cond_notify(&p->m_awakened_cnd, &p->m_lck);
        }
        ret = SQC_RESULT_WAKEUP_REQUESTED;
      }

      if (ret == SQC_RESULT_OK) {
        *pobjptr = pobj;
        pobj->m_is_used = true;
        p->m_n_free--;
      }

    }
    sqc_mutex_unlock(&p->m_lck);

  } else if (p != NULL) {
    if (p->m_state != SQC_POOL_STATE_CONSTRUCTED) {
      ret = SQC_RESULT_NOT_OPERATIONAL;
    } else if (index >= p->m_n_max) {
      ret = SQC_RESULT_TOO_LARGE;
    } else if (p->m_objs[index] == NULL) {
      ret = SQC_RESULT_NOT_FOUND;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_pool_acquire_poolable(sqc_pool_t *pptr, sqc_chrono_t to,
                             sqc_poolable_t *pobjptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_pool_t p = NULL;

  if (likely(pobjptr != NULL && pptr != NULL && (p = *pptr) != NULL &&
             p->m_state == SQC_POOL_STATE_OPERATIONAL)) {
    sqc_poolable_t pobj = NULL;
    bool found = false;

    *pobjptr = NULL;

    if (likely(p->m_type == SQC_POOL_TYPE_INDEX)) {
      size_t i;
      size_t n_waiters = 0;
      bool is_awakened = false;

      sqc_mutex_lock(&p->m_lck);
      {

        n_waiters = __atomic_fetch_add(&p->m_n_waiters, 1, __ATOMIC_ACQ_REL);

        while (found == false) {
          for (i = 0; i < p->m_obj_idx; i++) {
            pobj = p->m_objs[i];
            if ((pobj = p->m_objs[i]) != NULL &&  pobj->m_is_used == false) {
              found = true;
              ret = SQC_RESULT_OK;
              break;
            }
          }
          if (found == false) {
            ret = sqc_cond_wait(&p->m_cnd, &p->m_lck, to);
            if (ret == SQC_RESULT_OK && p->m_is_awakened == true) {
              is_awakened = true;
              break;
            } else if (ret != SQC_RESULT_OK) {
              break;
            }
          }
        }

        n_waiters = __atomic_sub_fetch(&p->m_n_waiters, 1, __ATOMIC_ACQ_REL);

        if (is_awakened == true) {
          if (n_waiters == 0) {
            /*
             * ACK for a wakeup API caller.
             */
            (void)sqc_cond_notify(&p->m_awakened_cnd, &p->m_lck);
          }
          ret = SQC_RESULT_WAKEUP_REQUESTED;
        }

        if (ret == SQC_RESULT_OK && found == true) {
          *pobjptr = pobj;
          pobj->m_is_used = true;
          p->m_n_free--;
        }

      }
      sqc_mutex_unlock(&p->m_lck);

      /* SQC_POOL_TYPE_INDEX end */

    } else if (likely(p->m_type == SQC_POOL_TYPE_QUEUE)) {

      int64_t n = 0;

      /*
       * Critical region 1
       */
      sqc_mutex_lock(&p->m_lck);
      {

        n = sqc_bbq_size(&p->m_free_q);

        if (n > 0) {

          /*
           * poolables available.
           */

          ret = sqc_bbq_get(&p->m_free_q, &pobj, sqc_poolable_t, 0);
          if (likely(ret == SQC_RESULT_OK)) {
            found = true;
          }

        } else if (n == 0 && p->m_obj_idx < p->m_n_max) {

          /*
           * no poolables available and not yet fully created. so
           * create new one.
           */

          ret = sqc_poolable_create_with_size(&pobj, p->m_is_executor, &p->m_m,
                                                 NULL, p->m_pobj_size);
          if (likely(ret == SQC_RESULT_OK)) {
            pobj->m_obj_idx = p->m_obj_idx;
            ret = sqc_poolable_setup(&pobj);
            if (likely(ret == SQC_RESULT_OK)) {
              p->m_objs[p->m_obj_idx++] = pobj;
              p->m_n_cur = p->m_obj_idx;
              pobj->m_pool = p;
              found = true;
            } else {
              sqc_poolable_destroy(&pobj);
            }
          }
        }

        if (likely(found == true)) {
          *pobjptr = pobj;
          pobj->m_is_used = true;
          p->m_n_free--;
        }

      }
      sqc_mutex_unlock(&p->m_lck);

      if (unlikely(found == false)) {

        /*
         * All the poolables are created and all of them are under use.
         * Wait for someone release a poolable.
         */

        ret = sqc_bbq_get(&p->m_free_q, &pobj, sizeof(pobj), to);
        if (likely(ret == SQC_RESULT_OK && pobj != NULL)) {

          /*
           * Critical region 2
           */
          sqc_mutex_lock(&p->m_lck);
          {

            *pobjptr = pobj;
            pobj->m_is_used = true;
            p->m_n_free--;

          }
          sqc_mutex_unlock(&p->m_lck);

        }
      }

      /* SQC_POOL_TYPE_QUEUE end */

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }

  } else {
    if (p != NULL && p->m_state != SQC_POOL_STATE_OPERATIONAL) {
      ret = SQC_RESULT_NOT_OPERATIONAL;
    } else {
      ret = SQC_RESULT_INVALID_ARGS;
    }
  }

  return ret;
}


sqc_result_t
sqc_pool_release_poolable(sqc_poolable_t *pobjptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_pool_t p = NULL;
  sqc_poolable_t pobj = NULL;

  if (likely(pobjptr != NULL && (pobj = *pobjptr) != NULL &&
             (p = pobj->m_pool) != NULL &&
             p->m_state == SQC_POOL_STATE_OPERATIONAL)) {

    if (likely(p->m_type == SQC_POOL_TYPE_INDEX)) {
      size_t i = pobj->m_obj_idx;

      if (likely(p->m_objs[i] == pobj)) {

        sqc_mutex_lock(&p->m_lck);
        {

          if (likely(pobj->m_is_used == true)) {
            pobj->m_is_used = false;
            p->m_n_free++;
            ret = sqc_cond_notify(&p->m_cnd, true);
          } else {
            ret = SQC_RESULT_INVALID_OBJECT;
          }

        }
        sqc_mutex_unlock(&p->m_lck);

      } else {
        ret = SQC_RESULT_INVALID_STATE;
      }

    } else if (likely(p->m_type == SQC_POOL_TYPE_QUEUE)) {

      sqc_mutex_lock(&p->m_lck);
      {

        pobj->m_is_used = false;
        ret = sqc_bbq_put(&p->m_free_q, &pobj, sqc_poolable_t, -1LL);
        if (likely(ret == SQC_RESULT_OK)) {
          p->m_n_free++;
        }

      }
      sqc_mutex_unlock(&p->m_lck);

    }

  } else {
    if (p != NULL && p->m_state != SQC_POOL_STATE_OPERATIONAL) {
      ret = SQC_RESULT_NOT_OPERATIONAL;
    } else {
      ret = SQC_RESULT_INVALID_ARGS;
    }
  }

  return ret;
}


sqc_result_t
sqc_pool_get_outstanding_obj_num(sqc_pool_t *pptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_pool_t p = NULL;

  if (likely(pptr != NULL && (p = *pptr) != NULL)) {

    (void)sqc_mutex_lock(&p->m_lck);
    {
      ret = (sqc_result_t)(p->m_n_free);
    }
    (void)sqc_mutex_unlock(&p->m_lck);

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_pool_wakeup(sqc_pool_t *pptr, sqc_chrono_t to) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_pool_t p = NULL;

  if (likely(pptr != NULL && (p = *pptr) != NULL &&
             (p->m_state == SQC_POOL_STATE_OPERATIONAL ||
              p->m_state == SQC_POOL_STATE_SHUTTINGDOWN))) {

    if (likely(p->m_type == SQC_POOL_TYPE_INDEX)) {
      size_t n_waiters = 0;

      sqc_mutex_lock(&p->m_lck);
      {
        p->m_is_awakened = true;
        ret = sqc_cond_notify(&p->m_cnd, true);
        if (ret == SQC_RESULT_OK) {
          n_waiters = __atomic_load_n(&p->m_n_waiters, __ATOMIC_ACQUIRE);
          if (n_waiters > 0) {
            ret = sqc_cond_wait(&p->m_awakened_cnd, &p->m_lck, to);
          }
          /*
           * NOTE:
           *	Whichwver the wait result, set p->m_is_awakened false
           *	for avoiding infini-wait.
           */
          p->m_is_awakened = false;
        }

      }
      sqc_mutex_unlock(&p->m_lck);

    } else if (likely(p->m_type == SQC_POOL_TYPE_QUEUE)) {

      ret = sqc_bbq_wakeup(&p->m_free_q, to);

    }

  } else {
    if (p != NULL &&
        p->m_state != SQC_POOL_STATE_OPERATIONAL &&
        p->m_state != SQC_POOL_STATE_SHUTTINGDOWN) {
      ret = SQC_RESULT_NOT_OPERATIONAL;
    } else {
      ret = SQC_RESULT_INVALID_ARGS;
    }
  }

  return ret;
}


sqc_result_t
sqc_pool_shutdown(sqc_pool_t *pptr,  shutdown_grace_level_t lvl,
		  sqc_chrono_t to) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_pool_t p = NULL;

  if (likely(pptr != NULL && (p = *pptr) != NULL &&
             p->m_state == SQC_POOL_STATE_OPERATIONAL)) {
    size_t i;
    sqc_result_t first_err = SQC_RESULT_ANY_FAILURES;
    sqc_poolable_t pobj = NULL;
    bool got_err = false;

    (void)sqc_mutex_lock(&p->m_lck);
    {

      p->m_state = SQC_POOL_STATE_SHUTTINGDOWN;
      ret = sqc_pool_wakeup(&p, to);
      if (likely(ret == SQC_RESULT_OK)) {
        for (i = 0; i < p->m_obj_idx; i++) {
          pobj = p->m_objs[i];
          ret = sqc_poolable_shutdown(&pobj, lvl);
          if (unlikely(ret != SQC_RESULT_OK)) {
            got_err = true;
            if (first_err == SQC_RESULT_ANY_FAILURES) {
              first_err = ret;
            }
          }
        }
      }

    }
    (void)sqc_mutex_unlock(&p->m_lck);

    if (got_err == true) {
      ret = first_err;
    }

  } else {
    if (p != NULL && p->m_state != SQC_POOL_STATE_OPERATIONAL) {
      ret = SQC_RESULT_NOT_OPERATIONAL;
    } else {
      ret = SQC_RESULT_INVALID_ARGS;
    }
  }

  return ret;
}


sqc_result_t
sqc_pool_cancel(sqc_pool_t *pptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_pool_t p = NULL;

  if (likely(pptr != NULL && (p = *pptr) != NULL &&
             p->m_is_cancelled == false &&
             (p->m_state == SQC_POOL_STATE_OPERATIONAL ||
              p->m_state == SQC_POOL_STATE_SHUTTINGDOWN))) {
    size_t i;
    sqc_result_t first_err = SQC_RESULT_ANY_FAILURES;
    sqc_poolable_t pobj = NULL;
    bool got_err = false;

    (void)sqc_mutex_lock(&p->m_lck);
    {

      for (i = 0; i < p->m_obj_idx; i++) {
        pobj = p->m_objs[i];
        ret = sqc_poolable_cancel(&pobj);
        if (unlikely(ret != SQC_RESULT_OK)) {
          got_err = true;
          if (first_err == SQC_RESULT_ANY_FAILURES) {
            first_err = ret;
          }
        }
      }

    }
    (void)sqc_mutex_unlock(&p->m_lck);

    if (got_err == true) {
      ret = first_err;
    }

  } else {
    if (p != NULL) {
      if (p->m_is_cancelled == true) {
        ret = SQC_RESULT_OK;
      } else if (p->m_state != SQC_POOL_STATE_OPERATIONAL &&
                 p->m_state != SQC_POOL_STATE_SHUTTINGDOWN) {
        ret = SQC_RESULT_NOT_OPERATIONAL;
      }
    } else {
      ret = SQC_RESULT_INVALID_ARGS;
    }
  }

  return ret;
}


sqc_result_t
sqc_pool_wait(sqc_pool_t *pptr, sqc_chrono_t to) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_pool_t p = NULL;

  if (likely(pptr != NULL && (p = *pptr) != NULL &&
             p->m_state == SQC_POOL_STATE_SHUTTINGDOWN)) {
    size_t i;
    sqc_poolable_t pobj = NULL;
    size_t n_errs = 0;

    if (likely(to >= 0)) {

      sqc_chrono_t end = -1LL;
      sqc_chrono_t now;

      WHAT_TIME_IS_IT_NOW_IN_NSEC(now);
      end = now + to;

      (void)sqc_mutex_lock(&p->m_lck);
      {

        do {
          n_errs = 0;
          WHAT_TIME_IS_IT_NOW_IN_NSEC(now);

          for (i = 0; i < p->m_obj_idx; i++) {
            pobj = p->m_objs[i];
            /* 1 msec polling. */
            ret = sqc_poolable_wait(&pobj, 1000LL * 1000LL);
            if (unlikely(ret != SQC_RESULT_OK)) {
              n_errs++;
            }
          }
        } while (n_errs != 0 && now < end);

      }
      (void)sqc_mutex_unlock(&p->m_lck);

      if (n_errs == 0) {
        ret = SQC_RESULT_OK;
      } else if (now >= end) {
        ret = SQC_RESULT_TIMEDOUT;
      }

    } else {

      (void)sqc_mutex_lock(&p->m_lck);
      {

        do {
          n_errs = 0;

          for (i = 0; i < p->m_obj_idx; i++) {
            pobj = p->m_objs[i];
            /* 1 msec polling. */
            ret = sqc_poolable_wait(&pobj, 1000LL * 1000LL);
            if (unlikely(ret != SQC_RESULT_OK)) {
              n_errs++;
            }
          }
        } while (n_errs != 0);

      }
      (void)sqc_mutex_unlock(&p->m_lck);

    }

  } else {
    if (p != NULL && p->m_state != SQC_POOL_STATE_OPERATIONAL) {
      ret = SQC_RESULT_NOT_OPERATIONAL;
    } else {
      ret = SQC_RESULT_INVALID_ARGS;
    }
  }

  return ret;
}


void
sqc_pool_destroy(sqc_pool_t *pptr) {
  if (pptr != NULL && *pptr != NULL) {
    sqc_pool_t p = *pptr;
    const void *key = NULL;
    sqc_poolable_t pobj = NULL;

    sqc_msg_debug(5, "deleteing pool \"%s\"...\n",
                     (IS_VALID_STRING(p->m_name) == true) ?
                     p->m_name : "???");

    (void)sqc_pool_shutdown(pptr, SHUTDOWN_RIGHT_NOW, -1);

    if (p->m_name != NULL) {
      key = p->m_name;
      (void)sqc_hashmap_delete(&s_pools, key, NULL, false);
      free(p->m_name);
    }

    if (p->m_objs != NULL) {
      size_t i;
      size_t n_used = 0;

      if (p->m_lck != NULL) {
        (void)sqc_mutex_lock(&p->m_lck);
      }
      {

        for (i = 0; i < p->m_n_max; i++) {
          pobj = p->m_objs[i];
          if (pobj != NULL) {
            sqc_msg_debug(5, "deleteing a poolable %p...\n", pobj);
            sqc_poolable_destroy(&pobj);
            sqc_msg_debug(5, "deleteing a poolable %p done.\n", p->m_objs[i]);
            p->m_objs[i] = NULL;
          }
        }

      }
      if (p->m_lck != NULL) {
        (void)sqc_mutex_unlock(&p->m_lck);
      }

      if (n_used == 0) {
        free(p->m_objs);
      }
    }

    if (p->m_type == SQC_POOL_TYPE_INDEX) {

      if (p->m_cnd != NULL) {
        sqc_cond_destroy(&(p->m_cnd));
      }
      if (p->m_awakened_cnd != NULL) {
        sqc_cond_destroy(&(p->m_awakened_cnd));
      }

    } else {
      if (p->m_free_q != NULL) {
        sqc_bbq_destroy(&(p->m_free_q), false);
      }
    }

    if (p->m_lck != NULL) {
      sqc_mutex_destroy(&(p->m_lck));
    }

    if (p->m_total_obj_size > 0) {
      free(p);
    }
  }
}





static pthread_once_t s_once = PTHREAD_ONCE_INIT;
static bool s_is_inited = false;

static void s_ctors(void) __attr_constructor__(115);
static void s_dtors(void) __attr_destructor__(115);


static void
s_pool_destroy_hashmap(void *o) {
  if (likely(o != NULL)) {
    sqc_pool_t p = (sqc_pool_t )o;
    sqc_pool_destroy(&p);
  }
}


static void
s_once_proc(void) {
  sqc_result_t st = SQC_RESULT_ANY_FAILURES;

  if (likely((st = sqc_hashmap_create(&s_pools, SQC_HASHMAP_TYPE_STRING,
                                         s_pool_destroy_hashmap))
             == SQC_RESULT_OK)) {
    s_is_inited = true;
  } else {
    sqc_perror(st);
    sqc_exit_fatal("can't initialize a pool table.\n");
  }
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();

  sqc_msg_debug(10, "The pool module is initialized.\n");
}


static inline void
s_final(void) {
  if (s_pools != NULL) {
    sqc_hashmap_destroy(&s_pools, true);
  }
}


static void
s_dtors(void) {
  if (s_is_inited == true) {
    if (sqc_module_is_unloading() &&
        sqc_module_is_finalized_cleanly()) {
      s_final();

      sqc_msg_debug(10, "The pool module is finalized.\n");
    } else {
      sqc_msg_debug(10, "The pool module is not finalized "
                       "because of module finalization problem.\n");
    }
  }
}
