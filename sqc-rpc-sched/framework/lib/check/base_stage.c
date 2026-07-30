static inline const char *
s_base_get_stage_name(const char *base, size_t idx) {
  if (likely(IS_VALID_STRING(base) == true)) {
    char buf[256];
    snprintf(buf, sizeof(buf), "%s" PFSZS(02, u),
             base, idx);
    return strdup(buf);
  } else {
    return NULL;
  }
}


static inline void
s_base_lock_stage(base_stage_t bs) {
  if (likely(bs != NULL && bs->m_lock != NULL)) {
    (void)sqc_mutex_lock(&(bs->m_lock));
  }
}


static inline void
s_base_unlock_stage(base_stage_t bs) {
  if (likely(bs != NULL && bs->m_lock != NULL)) {
    (void)sqc_mutex_unlock(&(bs->m_lock));
  }
}


static inline sqc_result_t
s_base_wait_stage(base_stage_t bs, sqc_chrono_t to) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(bs != NULL && bs->m_lock != NULL && bs->m_cond != NULL)) {
    ret = sqc_cond_wait(&(bs->m_cond), &(bs->m_lock), to);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_base_wakeup_stage(base_stage_t bs) {
  if (likely(bs != NULL && bs->m_cond != NULL)) {
    (void)sqc_cond_notify(&(bs->m_cond), true);
  }
}





/*
 * Safe bbq put/get
 */


static inline sqc_result_t
s_base_put_n(sqc_bbq_t *q, int64_t *vals, size_t n) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(q != NULL && *q != NULL &&
             vals != NULL && n > 0)) {
    size_t n_puts = 0;
    size_t n_total_puts = 0;

    do {
      ret = sqc_bbq_put_n(q,
                             (void **)(vals + n_total_puts),
                             n - n_total_puts,
                             int64_t,
                             -1LL, &n_puts);
      if (likely(ret >= 0)) {
        n_total_puts += (size_t)ret;
        continue;
      } else {
        if (likely(ret == SQC_RESULT_WAKEUP_REQUESTED)) {
          n_total_puts += n_puts;
          continue;
        } else {
          break;
        }
      }
    } while (n_total_puts < n);

    if (likely(ret >= 0)) {
      ret = (sqc_result_t)n_total_puts;
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_base_get_n(sqc_bbq_t *q, int64_t *vals, size_t n,
             sqc_chrono_t to) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(q != NULL && *q != NULL &&
             vals != NULL && n > 0)) {
    size_t n_gets = 0;

    ret = sqc_bbq_get_n(q, (void **)vals, n, 1,
                           int64_t, to, &n_gets);
    if (likely(ret >= 0)) {
      return ret;
    } else {
      if (ret == SQC_RESULT_WAKEUP_REQUESTED ||
          ret == SQC_RESULT_TIMEDOUT) {
        ret = (sqc_result_t)n_gets;
      }
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





static void
s_base_pre_pause(const sqc_pipeline_stage_t *sptr) {
  sqc_msg_debug(1, "called.\n");

  if (likely(sptr != NULL && *sptr != NULL)) {
    base_stage_t bs = (base_stage_t)(*sptr);
    size_t i;

    for (i = 0; i < bs->m_n_qs; i++) {
      (void)sqc_bbq_wakeup(&(bs->m_qs[i]), -1LL);
    }

    sqc_msg_debug(1, "woke up all workers.\n");
  }
}


static sqc_result_t
s_base_setup(const sqc_pipeline_stage_t *sptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  sqc_msg_debug(1, "called.\n");

  if (likely(sptr != NULL && *sptr != NULL)) {
    base_stage_t bs = (base_stage_t)(*sptr);
    const char *myname = NULL;

    if (bs->m_setup_proc != NULL) {
      ret = (bs->m_setup_proc)(bs);
      if (unlikely(ret != SQC_RESULT_OK)) {
        return ret;
      }
    }

    (void)sqc_pipeline_stage_get_name(sptr, &myname);

    if (bs->m_stg_idx < (bs->m_n_stgs - 1)) {
      if (likely(IS_VALID_STRING(bs->m_basename) == true)) {
        const char *name = s_base_get_stage_name(bs->m_basename,
                           bs->m_stg_idx + 1);

        if (likely(IS_VALID_STRING(name) == true)) {
          sqc_pipeline_stage_t nbs = NULL;

          ret = sqc_pipeline_stage_find(name, &nbs);
          if (ret == SQC_RESULT_OK) {
            bs->m_next_stg = (base_stage_t)nbs;
            sqc_msg_debug(1, "this is '%s', found the next stage '%s'.\n",
                             myname, name);
          } else {
            sqc_perror(ret);
            sqc_msg_error("can't find the next stage '%s'.\n", name);
          }
          free((void *)name);
        } else {
          ret = SQC_RESULT_NO_MEMORY;
        }
      } else {
        ret = SQC_RESULT_INVALID_ARGS;
      }
    } else {
      sqc_msg_debug(1, "this is '%s', the last stage.\n", myname);
      ret = SQC_RESULT_OK;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





/*
 * various schedulers
 */


static sqc_result_t
s_base_sched_single(const sqc_pipeline_stage_t *sptr,
                    void *evbuf,
                    size_t n_evs,
                    void *hint) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  (void)hint;

  if (likely(sptr != NULL && *sptr != NULL &&
             evbuf != NULL && n_evs > 0)) {
    base_stage_t bs = (base_stage_t)*sptr;
    ret = s_base_put_n(&(bs->m_qs[0]), (int64_t *)evbuf, n_evs);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static sqc_result_t
s_base_sched_hint(const sqc_pipeline_stage_t *sptr,
                  void *evbuf,
                  size_t n_evs,
                  void *hint) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL &&
             evbuf != NULL && n_evs > 0)) {
    base_stage_t bs = (base_stage_t)*sptr;
    size_t idx = (size_t)hint;

    ret = s_base_put_n(&(bs->m_qs[idx % bs->m_n_qs]), (int64_t *)evbuf, n_evs);
    if (__sync_fetch_and_add(&(bs->m_n_waiters), 0) > 0) {
      s_base_wakeup_stage(bs);
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static sqc_result_t
s_base_sched_rr(const sqc_pipeline_stage_t *sptr,
                void *evbuf,
                size_t n_evs,
                void *hint) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  (void)hint;

  if (likely(sptr != NULL && *sptr != NULL &&
             evbuf != NULL && n_evs > 0)) {
    base_stage_t bs = (base_stage_t)*sptr;
    size_t idx = __sync_fetch_and_add(&(bs->m_put_next_q_idx), 1);

    ret = s_base_put_n(&(bs->m_qs[idx % bs->m_n_qs]), (int64_t *)evbuf, n_evs);
    if (__sync_fetch_and_add(&(bs->m_n_waiters), 0) > 0) {
      s_base_wakeup_stage(bs);
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static sqc_result_t
s_base_sched_rr_para(const sqc_pipeline_stage_t *sptr,
                     void *evbuf,
                     size_t n_evs,
                     void *hint) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  (void)hint;

  if (likely(sptr != NULL && *sptr != NULL &&
             evbuf != NULL && n_evs > 0)) {
    base_stage_t bs = (base_stage_t)*sptr;
    size_t qidx = __sync_fetch_and_add(&(bs->m_put_next_q_idx), 1);
    size_t stride = n_evs / bs->m_n_qs;
    size_t mod = n_evs - stride * bs->m_n_qs;
    size_t i = 0;
    size_t n;
    size_t n_total = 0;
    int64_t *buf = (int64_t *)evbuf;
    size_t bufidx;

    do {
      qidx = (qidx + i) % bs->m_n_qs;
      bufidx = i * stride;
      n = (i == 0) ? mod : stride;

      ret = s_base_put_n(&(bs->m_qs[qidx]), buf + bufidx, n);
      if (likely(ret >= 0)) {
        n_total += (size_t)ret;
        i++;
      } else {
        break;
      }
    } while (n_total < n_evs);
    if (__sync_fetch_and_add(&(bs->m_n_waiters), 0) > 0) {
      s_base_wakeup_stage(bs);
    }
    ret = (sqc_result_t)n_total;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





/*
 * various fetchers
 */


static sqc_result_t
s_base_fetch_single(const sqc_pipeline_stage_t *sptr,
                    size_t idx, void *buf, size_t max) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  (void)idx;

  if (likely(sptr != NULL && *sptr != NULL &&
             buf != NULL && max > 0)) {
    base_stage_t bs = (base_stage_t)(*sptr);

    ret = s_base_get_n(&(bs->m_qs[0]), (int64_t *)buf, max,
                       bs->m_to);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static sqc_result_t
s_base_fetch_hint(const sqc_pipeline_stage_t *sptr,
                  size_t idx, void *buf, size_t max) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL &&
             buf != NULL && max > 0)) {
    base_stage_t bs = (base_stage_t)(*sptr);

    ret = s_base_get_n(&(bs->m_qs[idx % bs->m_n_qs]), (int64_t *)buf, max,
                       bs->m_to);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static sqc_result_t
s_base_fetch_rr(const sqc_pipeline_stage_t *sptr,
                size_t idx, void *buf, size_t max) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL &&
             buf != NULL && max > 0)) {
    base_stage_t bs = (base_stage_t)(*sptr);
    size_t baseidx = __sync_fetch_and_add(&(bs->m_get_next_q_idx), 0) +
                     bs->m_n_qs - 1;
    size_t curidx = baseidx;
    size_t i = 0;
    bool is_first_wait = true;

  recheck:
    do {

      idx = baseidx + i++;
      ret = s_base_get_n(&(bs->m_qs[idx % bs->m_n_qs]),
                         (int64_t *)buf, max, 0);
      if (likely(ret > 0)) {
        /*
         * Update the next q idx only when the old idx is not yet
         * overwritten by the bigest index.
         */
        sqc_atomic_update_max(size_t, &(bs->m_get_next_q_idx),
                                 baseidx, curidx);

        return ret;
      } else if (ret < 0) {
        return ret;
      }

    } while (i < bs->m_n_qs);

    /*
     * all the qs are checked and no data.
     */
    if (is_first_wait == true) {
      sqc_chrono_t now;
      sqc_chrono_t until;

      is_first_wait = false;
      WHAT_TIME_IS_IT_NOW_IN_NSEC(now);
      until = now + bs->m_to;

      s_base_lock_stage(bs);
      {
        (void)__sync_add_and_fetch(&(bs->m_n_waiters), 1);
        ret = s_base_wait_stage(bs, bs->m_to);
        (void)__sync_sub_and_fetch(&(bs->m_n_waiters), 1);
      }
      s_base_unlock_stage(bs);
      WHAT_TIME_IS_IT_NOW_IN_NSEC(now);

      if (likely(ret == SQC_RESULT_OK && now < until)) {
        goto recheck;
      } else if (ret == SQC_RESULT_TIMEDOUT) {
        ret = 0;
      }
    } else {
      ret = 0;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





static sqc_result_t
s_base_throw(const sqc_pipeline_stage_t *sptr,
             size_t idx, void *buf, size_t n) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL &&
             buf != NULL && n > 0)) {
    base_stage_t bs = (base_stage_t)(*sptr);

    if (likely(bs->m_next_stg != NULL)) {
      ret = sqc_pipeline_stage_submit(
              (sqc_pipeline_stage_t *)&(bs->m_next_stg), buf, n, (void *)idx);
    } else {
      ret = 0;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





static sqc_result_t
s_base_shutdown(const sqc_pipeline_stage_t *sptr,
                shutdown_grace_level_t l) {
  (void)sptr;

  sqc_msg_debug(1, "called with \"%s\".\n",
                   (l == SHUTDOWN_RIGHT_NOW) ? "right now" : "gracefully");

  return SQC_RESULT_OK;
}





static void
s_base_finalize(const sqc_pipeline_stage_t *sptr,
                bool is_canceled) {
  (void)sptr;

  sqc_msg_debug(1, "%s.\n",
                   (is_canceled == false) ? "exit normally" : "canceled");
}





static void
s_base_freeup(const sqc_pipeline_stage_t *sptr) {
  if (likely(sptr != NULL && *sptr != NULL)) {
    base_stage_t bs = (base_stage_t)(*sptr);
    size_t i;

    free((void *)bs->m_basename);

    if (bs->m_qs != NULL) {
      for (i = 0; i < bs->m_n_qs; i++) {
        sqc_bbq_destroy(&(bs->m_qs[i]), true);
      }
      free((void *)bs->m_qs);
    }

    if (bs->m_lock != NULL) {
      sqc_mutex_destroy(&(bs->m_lock));
    }
    if (bs->m_cond != NULL) {
      sqc_cond_destroy(&(bs->m_cond));
    }

  }

}





static inline sqc_result_t
s_base_create(base_stage_t *bsptr,
              size_t alloc_size,
              const char *bname,
              size_t stage_idx,
              size_t max_stage,
              size_t n_workers,
              size_t n_qs,
              size_t q_len,
              size_t batch_size,
              sqc_chrono_t to,
              sqc_pipeline_stage_sched_proc_t sched_proc,
              sqc_pipeline_stage_fetch_proc_t fetch_proc,
              sqc_pipeline_stage_main_proc_t main_proc,
              sqc_pipeline_stage_throw_proc_t throw_proc,
              sqc_pipeline_stage_freeup_proc_t freeup_proc) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(bsptr != NULL &&
             IS_VALID_STRING(bname) == true &&
             n_workers > 0 &&
             stage_idx < max_stage &&
             main_proc != NULL)) {
    sqc_mutex_t lock = NULL;
    sqc_cond_t cond = NULL;
    sqc_bbq_t *qs = NULL;
    const char *name = s_base_get_stage_name(bname, stage_idx);
    const char *basename = strdup(bname);

    if (unlikely(IS_VALID_STRING(name) == false ||
                 IS_VALID_STRING(basename) == false)) {
      goto bailout;
    }

    if (n_qs > 0 && q_len > 0) {
      qs = (sqc_bbq_t *)malloc(sizeof(sqc_bbq_t) * n_qs);
      if (likely(qs != NULL)) {
        size_t i;

        for (i = 0; i < n_qs; i++) {
          qs[i] = NULL;
          ret = sqc_bbq_create(&(qs[i]), int64_t,
                                  (int64_t)q_len, NULL);
          if (unlikely(ret != SQC_RESULT_OK)) {
            size_t j;

            for (j = 0; j < i; j++) {
              sqc_bbq_destroy(&(qs[j]), true);
            }

            goto bailout;
          }
        }
      }
    }

    if (likely(
          ((ret = sqc_mutex_create(&lock)) == SQC_RESULT_OK) &&
          ((ret = sqc_cond_create(&cond)) == SQC_RESULT_OK))) {
      base_stage_t bs = NULL;

      ret = sqc_pipeline_stage_create(
              (sqc_pipeline_stage_t *)&bs,
              (alloc_size == 0) ? sizeof(base_stage_record) : alloc_size,
              name, n_workers,
              sizeof(uint64_t), batch_size,
              s_base_pre_pause,		/* pre_pause */
              sched_proc,			/* sched */
              s_base_setup,			/* setup */
              fetch_proc,			/* fetch */
              main_proc,			/* main */
              throw_proc,			/* throw */
              s_base_shutdown,		/* shutdown */
              s_base_finalize,		/* final */
              (freeup_proc != NULL) ? freeup_proc : s_base_freeup
              /* freeup */);
      if (likely(ret == SQC_RESULT_OK)) {
        bs->m_setup_proc = NULL;
        bs->m_basename = basename;
        bs->m_n_workers = n_workers;
        bs->m_n_stgs = max_stage;
        bs->m_stg_idx = stage_idx;
        bs->m_next_stg = NULL;
        bs->m_to = to;
        bs->m_qs = qs;
        bs->m_n_qs = n_qs;
        bs->m_put_next_q_idx = 0;
        bs->m_get_next_q_idx = 0;
        bs->m_n_waiters = 0;
        bs->m_lock = lock;
        bs->m_cond = cond;

        *bsptr = bs;
      }

    }

    free((void *)name);
    name = NULL;

  bailout:
    if (ret != SQC_RESULT_OK) {
      free((void *)name);
      free((void *)basename);
      free((void *)qs);
      if (lock != NULL) {
        sqc_mutex_destroy(&lock);
      }
      if (cond != NULL) {
        sqc_cond_destroy(&cond);
      }
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_base_stage_set_setup_hook(base_stage_t bs, base_stage_setup_proc_t proc) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(bs != NULL)) {
    bs->m_setup_proc = proc;
    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





static const sqc_pipeline_stage_sched_proc_t
const s_base_stage_sched_procs[] = {
  NULL,
  s_base_sched_single,
  s_base_sched_hint,
  s_base_sched_rr,
  s_base_sched_rr_para
};


static const sqc_pipeline_stage_fetch_proc_t
const s_base_stage_fetch_procs[] = {
  NULL,
  s_base_fetch_single,
  s_base_fetch_hint,
  s_base_fetch_rr
};





static inline sqc_pipeline_stage_sched_proc_t
s_base_stage_get_sched_proc(base_stage_sched_t type) {
  if ((int)type <= (int)base_stage_sched_rr_para) {
    return s_base_stage_sched_procs[(int)type];
  } else {
    return NULL;
  }
}


static inline sqc_pipeline_stage_fetch_proc_t
s_base_stage_get_fetch_proc(base_stage_fetch_t type) {
  if ((int)type <= (int)base_stage_fetch_rr) {
    return s_base_stage_fetch_procs[(int)type];
  } else {
    return NULL;
  }
}



