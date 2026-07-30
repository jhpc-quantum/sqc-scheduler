static sqc_result_t	s_ingress_setup(base_stage_t bs);





static inline void
s_test_stage_freeup(const sqc_pipeline_stage_t *sptr) {
  if (likely(sptr != NULL && *sptr != NULL)) {
    test_stage_t ts = (test_stage_t)(*sptr);

    if (ts->m_lock != NULL) {
      sqc_mutex_destroy(&(ts->m_lock));
    }
    if (ts->m_cond != NULL) {
      sqc_cond_destroy(&(ts->m_cond));
    }

    free((void *)ts->m_data);
    free((void *)ts->m_enq_infos);
    free((void *)ts->m_states);
  }
}


static inline sqc_result_t
s_test_stage_create(test_stage_t *tsptr,
                    test_stage_type_t type,
                    size_t n_workers,
                    size_t n_qs,
                    size_t q_len,
                    size_t n_events,
                    size_t batch_size,
                    sqc_chrono_t to,
                    sqc_pipeline_stage_sched_proc_t sched_proc,
                    sqc_pipeline_stage_fetch_proc_t fetch_proc,
                    sqc_pipeline_stage_main_proc_t main_proc,
                    size_t stage_idx,
                    size_t max_stage) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(tsptr != NULL &&
             main_proc != NULL &&
             n_workers > 0 && n_events > 0 && max_stage > 0)) {
    sqc_mutex_t lock = NULL;
    sqc_cond_t cond = NULL;
    test_stage_state_t *states = (test_stage_state_t *)
                                 malloc(sizeof(test_stage_state_t) * n_workers);
    sqc_pipeline_stage_throw_proc_t throw_proc =
      (type == test_stage_type_intermediate) ? s_base_throw : NULL;

    if (unlikely(states == NULL)) {
      ret = SQC_RESULT_NO_MEMORY;
      goto done;
    }

    if (likely((ret = sqc_mutex_create(&lock)) == SQC_RESULT_OK &&
               (ret = sqc_cond_create(&cond)) == SQC_RESULT_OK &&
               (ret = s_base_create((base_stage_t *)tsptr,
                                    sizeof(test_stage_record),
                                    "test",
                                    stage_idx,	/* stage_idx */
                                    max_stage,	/* max_stage */
                                    n_workers,	/* n_workers */
                                    n_qs,	/* n_qs */
                                    q_len,	/* q_len */
                                    batch_size,	/* batch_size */
                                    to,		/* to */
                                    sched_proc,		/* sched_proc */
                                    fetch_proc,		/* fetch_proc */
                                    main_proc,		/* main_proc */
                                    throw_proc,		/* throw_proc */
                                    s_test_stage_freeup	/* freeup_proc */
                                   )) == SQC_RESULT_OK &&
               ((type == test_stage_type_ingress ?
                 (ret = s_base_stage_set_setup_hook((base_stage_t)(*tsptr),
                        s_ingress_setup)) :
                 (ret = SQC_RESULT_OK))) == SQC_RESULT_OK)) {
      size_t i;

      for (i = 0; i < n_workers; i++) {
        states[i] = test_stage_state_initialized;
      }

      (*tsptr)->m_type = type;
      (*tsptr)->m_lock = lock;
      (*tsptr)->m_cond = cond;
      (*tsptr)->m_do_exit = false;
      (*tsptr)->m_data = NULL;
      (*tsptr)->m_n_data = n_events;
      (*tsptr)->m_enq_infos = NULL;
      (*tsptr)->m_states = states;
      (*tsptr)->m_weight = 1;
      (*tsptr)->m_sum = 0;
      (*tsptr)->m_n_events = 0;
      (*tsptr)->m_start_clock = 0;
      (*tsptr)->m_end_clock = 0;
      (*tsptr)->m_start_time = -1LL;
      (*tsptr)->m_end_time = -1LL;

      ret = SQC_RESULT_OK;
    } else {
      free((void *)states);
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}





static inline sqc_result_t
s_test_stage_wait(const test_stage_t ts) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(ts->m_lock != NULL && ts->m_cond != NULL)) {

    (void)sqc_mutex_lock(&(ts->m_lock));
    {
    recheck:
      if (ts->m_do_exit != true) {
        (void)sqc_cond_wait(&(ts->m_cond), &(ts->m_lock), -1LL);
        goto recheck;
      }
    }
    (void)sqc_mutex_unlock(&(ts->m_lock));

    ret = SQC_RESULT_OK;

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_test_stage_wakeup(const test_stage_t ts) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(ts->m_lock != NULL && ts->m_cond != NULL)) {

    (void)sqc_mutex_lock(&(ts->m_lock));
    {
      ts->m_do_exit = true;
      (void)sqc_cond_notify(&(ts->m_cond), true);
    }
    (void)sqc_mutex_unlock(&(ts->m_lock));

    ret = SQC_RESULT_OK;

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





static sqc_result_t
s_ingress_setup(base_stage_t bs) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  sqc_msg_debug(1, "called.\n");

  if (likely(bs != NULL)) {
    test_stage_t ts = (test_stage_t)bs;
    sqc_result_t n_workers = sqc_pipeline_stage_get_worker_nubmer(
                                  (sqc_pipeline_stage_t *)&ts);
    size_t n_data = ts->m_n_data;

    if (likely(n_data > 0 &&
               n_workers > 0)) {
      uint64_t *data =
        (uint64_t *)malloc(sizeof(uint64_t) * n_data);
      enqueue_info_t *enq_infos =
        (enqueue_info_t *)malloc(sizeof(enqueue_info_t) * (size_t)n_workers);

      if (likely(data != NULL && enq_infos != NULL)) {
        size_t i;
        size_t rem_size = n_data;
        size_t len = n_data / (size_t)n_workers;

        for (i = 0; i < n_data; i++) {
          data[i] = i;
        }

        if (n_data % (size_t)n_workers != 0) {
          len++;
        }

        for (i = 0; i < (size_t)n_workers; i++, rem_size -= len) {
          enq_infos[i].m_offset = i * len;
          enq_infos[i].m_length = (rem_size < len) ? rem_size : len;
        }

        ts->m_data = data;
        ts->m_enq_infos = enq_infos;

        ret = SQC_RESULT_OK;
      } else {
        ret = SQC_RESULT_NO_MEMORY;
      }
    } else {
      ret = SQC_RESULT_INVALID_ARGS;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static sqc_result_t
s_ingress_main(const sqc_pipeline_stage_t *sptr,
               size_t idx,
               void *evbuf,
               size_t n_evs) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  (void)evbuf;
  (void)n_evs;

  if (likely(sptr != NULL && *sptr != NULL)) {
    test_stage_t ts = (test_stage_t)(*sptr);
    base_stage_t bs = (base_stage_t)ts;

    if (likely(bs != NULL && bs->m_next_stg != NULL)) {
      if (likely(ts->m_states[idx] != test_stage_state_done)) {
        sqc_chrono_t start_time;
        uint64_t start_clock;
        uint64_t end_clock;
        uint64_t *addr = ts->m_data + ts->m_enq_infos[idx].m_offset;
        size_t len = ts->m_enq_infos[idx].m_length;

        WHAT_TIME_IS_IT_NOW_IN_NSEC(start_time);

        start_clock = sqc_rdtsc();
        ret = sqc_pipeline_stage_submit((sqc_pipeline_stage_t *)
                                           &(bs->m_next_stg),
                                           (void *)addr, len, (void *)idx);
        end_clock = sqc_rdtsc();

        sqc_atomic_update_min(uint64_t, &(ts->m_start_clock),
                                 0, start_clock);
        sqc_atomic_update_max(uint64_t, &(ts->m_end_clock),
                                 0, end_clock);

        sqc_atomic_update_min(sqc_chrono_t, &(ts->m_start_time),
                                 -1LL, start_time);

        ts->m_states[idx] = test_stage_state_done;
        mbar();
      } else {
        ret = s_test_stage_wait(ts);
      }
    } else {
      ret = SQC_RESULT_INVALID_ARGS;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_ingress_create(test_stage_t *tsptr,
                 size_t n_workers,
                 size_t n_events,
                 size_t max_stage) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(tsptr != NULL && n_workers > 0 && n_events > 0 &&
             max_stage > 0)) {
    ret = s_test_stage_create(tsptr,
                              test_stage_type_ingress,
                              n_workers,	/* n_workers */
                              0,		/* n_qs */
                              0,		/* q_len */
                              n_events,		/* n_events */
                              1,		/* batch_size (dummy) */
                              0,		/* to */
                              NULL,		/* sched_proc */
                              NULL,		/* fetch_proc */
                              s_ingress_main,	/* main_proc */
                              0,		/* stage_idx */
                              max_stage		/* max_stage */
                             );
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_ingress_get_data(test_stage_t *tsptr,
                   uint64_t **data, size_t *n) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(tsptr != NULL && *tsptr != NULL &&
             (*tsptr)->m_type == test_stage_type_ingress &&
             data != NULL && n != NULL)) {
    *data = (*tsptr)->m_data;
    *n = (*tsptr)->m_n_data;

    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





static sqc_result_t
s_intermediate_main(const sqc_pipeline_stage_t *sptr,
                    size_t idx,
                    void *evbuf,
                    size_t n_evs) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL)) {
    test_stage_t ts = (test_stage_t)(*sptr);
    size_t n_cur_evs = __sync_add_and_fetch(&(ts->m_n_events), 0);

    if (likely(n_cur_evs < ts->m_n_data)) {
      uint64_t *data = (uint64_t *)evbuf;
      size_t i;
      size_t j;
      uint64_t sum = 0;

      if (unlikely(ts->m_states[idx] == test_stage_state_initialized)) {
        uint64_t cur_clock = sqc_rdtsc();
        sqc_atomic_update_min(uint64_t, &(ts->m_start_clock),
                                 0, cur_clock);
        ts->m_states[idx] = test_stage_state_running;
      }

      for (i = 0; i < ts->m_weight; i++) {
        sum = 0;
        for (j = 0; j < n_evs; j++) {
          sum += data[j];
        }
      }

      (void)__sync_add_and_fetch(&(ts->m_sum), sum);
      n_cur_evs = __sync_add_and_fetch(&(ts->m_n_events), n_evs);

      if (unlikely(ts->m_n_data == n_cur_evs)) {
        uint64_t cur_clock = sqc_rdtsc();
        sqc_chrono_t end_time;

        WHAT_TIME_IS_IT_NOW_IN_NSEC(end_time);

        sqc_msg_debug(1, "got " PFSZ(u) " / " PFSZ(u)"  events.\n",
                         n_cur_evs, ts->m_n_data);

        sqc_atomic_update_min(uint64_t, &(ts->m_end_clock),
                                 0, cur_clock);

        sqc_atomic_update_max(sqc_chrono_t, &(ts->m_end_time),
                                 -1, end_time);

        ts->m_states[idx] = test_stage_state_done;
        mbar();

        if (ts->m_type == test_stage_type_egress) {
          /*
           * Wake the master up.
           */
          (void)s_test_stage_wakeup(ts);
        }
      }

      ret = (sqc_result_t)n_evs;

    } else {
      ret = s_test_stage_wait(ts);
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_intermediate_create(test_stage_t *tsptr,
                      size_t n_workers,
                      size_t n_qs,
                      size_t q_len,
                      size_t n_events,
                      size_t batch_size,
                      sqc_chrono_t to,
                      sqc_pipeline_stage_sched_proc_t sched_proc,
                      sqc_pipeline_stage_fetch_proc_t fetch_proc,
                      size_t stage_idx,
                      size_t max_stage) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(tsptr != NULL &&
             n_workers > 0 && n_qs > 0 && n_events > 0 &&
             q_len > 0 && batch_size > 0 &&
             sched_proc != NULL &&
             fetch_proc != NULL &&
             stage_idx > 0 && stage_idx < max_stage &&
             max_stage > 0)) {
    ret = s_test_stage_create(tsptr,
                              test_stage_type_intermediate,
                              n_workers,	/* n_workers */
                              n_qs,		/* n_qs */
                              q_len,		/* q_len */
                              n_events,		/* n_events */
                              batch_size,	/* batch_size */
                              to,		/* to */
                              sched_proc,	/* sched_proc */
                              fetch_proc,	/* fetch_proc */
                              s_intermediate_main,	/* main_proc */
                              stage_idx,	/* stage_idx */
                              max_stage		/* max_stage */
                             );
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline sqc_result_t
s_egress_create(test_stage_t *tsptr,
                size_t n_workers,
                size_t n_qs,
                size_t q_len,
                size_t n_events,
                size_t batch_size,
                sqc_chrono_t to,
                sqc_pipeline_stage_sched_proc_t sched_proc,
                sqc_pipeline_stage_fetch_proc_t fetch_proc,
                size_t max_stage) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(tsptr != NULL &&
             n_workers > 0 && n_qs > 0 && n_events > 0 &&
             q_len > 0 && batch_size > 0 &&
             sched_proc != NULL &&
             fetch_proc != NULL &&
             max_stage > 0)) {
    ret = s_test_stage_create(tsptr,
                              test_stage_type_egress,
                              n_workers,	/* n_workers */
                              n_qs,		/* n_qs */
                              q_len,		/* q_len */
                              n_events,		/* n_events */
                              batch_size,	/* batch_size */
                              to,		/* to */
                              sched_proc,	/* sched_proc */
                              fetch_proc,	/* fetch_proc */
                              s_intermediate_main,	/* main_proc */
                              max_stage - 1,	/* stage_idx */
                              max_stage		/* max_stage */
                             );
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





static inline sqc_result_t
s_test_stage_create_by_spec(test_stage_t *tsptr,
                            size_t stg_idx,
                            size_t max_stage,
                            test_stage_spec_t *spec) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(spec != NULL &&
             spec->m_n_workers > 0 &&
             spec->m_n_events > 0 &&
             stg_idx < max_stage)) {
    if (stg_idx == 0) {
      ret = s_ingress_create(tsptr,
                             spec->m_n_workers,
                             spec->m_n_events,
                             max_stage);
    } else {
      if (likely(spec->m_n_qs > 0 &&
                 spec->m_q_len > 0 &&
                 spec->m_batch_size > 0)) {
        sqc_pipeline_stage_sched_proc_t sched_proc = NULL;
        sqc_pipeline_stage_fetch_proc_t fetch_proc = NULL;

        if (unlikely(spec->m_n_qs == 1 &&
                     (spec->m_sched_type != base_stage_sched_single ||
                      spec->m_fetch_type != base_stage_fetch_single))) {
          ret = SQC_RESULT_INVALID_ARGS;
          goto done;
        }

        sched_proc = s_base_stage_get_sched_proc(spec->m_sched_type);
        fetch_proc = s_base_stage_get_fetch_proc(spec->m_fetch_type);
        if (unlikely(sched_proc == NULL || fetch_proc == NULL)) {
          ret = SQC_RESULT_INVALID_ARGS;
          goto done;
        }

        if (stg_idx == (max_stage - 1)) {
          ret = s_egress_create(tsptr,
                                spec->m_n_workers,
                                spec->m_n_qs,
                                spec->m_q_len,
                                spec->m_n_events,
                                spec->m_batch_size,
                                spec->m_to,
                                sched_proc,
                                fetch_proc,
                                max_stage);
        } else {
          ret = s_intermediate_create(tsptr,
                                      spec->m_n_workers,
                                      spec->m_n_qs,
                                      spec->m_q_len,
                                      spec->m_n_events,
                                      spec->m_batch_size,
                                      spec->m_to,
                                      sched_proc,
                                      fetch_proc,
                                      stg_idx,
                                      max_stage);
        }
      } else {
        ret = SQC_RESULT_INVALID_ARGS;
      }
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

done:
  return ret;
}


static inline sqc_result_t
s_test_stage_get_clocks(test_stage_t *tsptr,
                        uint64_t *start,
                        uint64_t *end) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (tsptr != NULL && *tsptr != NULL && start != NULL && end != NULL) {
    *start = (*tsptr)->m_start_clock;
    *end = (*tsptr)->m_end_clock;

    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_test_stage_get_times(test_stage_t *tsptr,
                       sqc_result_t *start,
                       sqc_result_t *end) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (tsptr != NULL && *tsptr != NULL && start != NULL && end != NULL) {
    *start = (*tsptr)->m_start_time;
    *end = (*tsptr)->m_end_time;

    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_test_stage_set_weight(test_stage_t *tsptr,
                        size_t weight) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (tsptr != NULL && *tsptr != NULL && weight > 0) {
    (*tsptr)->m_weight = weight;
    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}
