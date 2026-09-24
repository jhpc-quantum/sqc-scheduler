#include "sqc_apis.h"
#include "sqc_rpc_sched_util.h"
#include "dbmgr.h"

#include "req_invoker.h"

#include "job_sched_scheduling_algo.h"
#include "job_sched_fair_share.h"

/*
 * Node in a priority's circular doubly-linked list of groups.
 * - one job queue per group (fair-share, not global FIFO)
 * - next: round-robin dispatch order
 * - prev: lets registration find the tail (head->prev) in O(1)
 */
typedef struct fair_share_sched_group_entry {
  char *group_id;
  sqc_bbq_t group_q; /* this group's own FIFO job queue */
  struct fair_share_sched_group_entry *next;
  struct fair_share_sched_group_entry *prev;
} fair_share_sched_group_entry_t;

/*
 * Per-priority context (one per priority level, s_pq_ctx[priority]).
 * - rwlck_: guards group_map, head/cursor, and rr_count together
 * - group_map: group_id -> group, for O(1) lookup on enqueue
 * - head: fixed anchor (set once, never moves)
 * - cursor: current round-robin position
 * - rr_count: # of groups in the list; bounds one dispatch scan
 *
 *     head --> [A] --next--> [B] --next--> [C] --+
 *               ^                                |
 *               +-----------next-----------------+
 *     prev points the opposite way around the same ring, so
 *     head->prev reaches the tail node [C] directly, with no traversal.
 *     cursor is set to cursor->next after each dispatch attempt, so
 *     every group is checked once per full round (round-robin).
 */
typedef struct fair_share_sched_priority_ctx {
  sqc_rwlock_t rwlck_;

  uint8_t priority;
  sqc_hashmap_t group_map;
  fair_share_sched_group_entry_t *head;
  fair_share_sched_group_entry_t *cursor;
  size_t rr_count;
} fair_share_sched_priority_ctx_t;

static fair_share_sched_priority_ctx_t s_pq_ctx[SQC_RPC_SCHED_PQ_NUM];
static bool s_initialized = false;

/*
 * signature matches req_invoker_invoke(). Kept as a settable function
 * pointer (not a hardcoded call) so a caller could register a different
 * invoker in the future; currently only white-box unit tests do this.
 */
typedef sqc_result_t (*fair_share_sched_req_invoker_fn_t)(const dbmgr_job_info_t);

// defaults to the real req_invoker; swappable via direct assignment
static fair_share_sched_req_invoker_fn_t s_req_invoker_invoke = req_invoker_invoke;

static void
s_fair_share_sched_group_freeup(void *val) {
  fair_share_sched_group_entry_t *entry = (fair_share_sched_group_entry_t *)val;

  if (entry != NULL) {
    sqc_bbq_destroy(&entry->group_q, true);
    free(entry->group_id);
    free(entry);
  }
}


static void
s_fair_share_sched_group_q_freeup(void **val) {
  (void) val;

  // Do nothing. Free of record is performed with DB Mgr.
}


/*
 * Free every priority context's group_map, rwlock, and round-robin state.
 * (group_map destroy also frees each group's job queue/group_id)
 */
static void
s_fair_share_sched_priority_contexts_teardown(void) {
  for (int i = 0; i < SQC_RPC_SCHED_PQ_NUM; i++) {
    if (s_pq_ctx[i].group_map != NULL) {
      sqc_hashmap_destroy(&s_pq_ctx[i].group_map, true);
      s_pq_ctx[i].group_map = NULL;
    }

    if (s_pq_ctx[i].rwlck_ != NULL) {
      sqc_rwlock_destroy(&s_pq_ctx[i].rwlck_);
      s_pq_ctx[i].rwlck_ = NULL;
    }

    s_pq_ctx[i].head = NULL;
    s_pq_ctx[i].cursor = NULL;
    s_pq_ctx[i].rr_count = 0;
  }
}


/*
 * Register a group into a priority context: create its job queue,
 * add it to the group_map, and append it to the tail of the
 * round-robin list.
 */
static sqc_result_t
s_fair_share_sched_group_register(fair_share_sched_priority_ctx_t *ctx,
                                  const char *group_id,
                                  fair_share_sched_group_entry_t **out_entry) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  fair_share_sched_group_entry_t *entry;
  void *hashmap_val;

  entry = (fair_share_sched_group_entry_t *)malloc(sizeof(*entry));
  if (entry == NULL) {
    sqc_msg_error("Memory allocation failed for group record\n");
    rc = SQC_RESULT_NO_MEMORY;
    goto error;
  }

  entry->group_id = strdup(group_id);
  if (entry->group_id == NULL) {
    sqc_msg_error("Memory allocation failed for group_id\n");
    free(entry);
    rc = SQC_RESULT_NO_MEMORY;
    goto error;
  }

  // Initialize the job queue held by this group for this priority
  rc = sqc_bbq_create(&entry->group_q, dbmgr_job_info_t, SQC_RPC_SCHED_PQ_LEN,
                      s_fair_share_sched_group_q_freeup);
  if (rc != SQC_RESULT_OK) {
    sqc_msg_error("Failed to create bbq for group %s in priority %u: %s\n",
                  group_id, ctx->priority, sqc_error_get_string(rc));
    free(entry->group_id);
    free(entry);
    goto error;
  }

  // Add to hashmap
  hashmap_val = entry;
  rc = sqc_hashmap_add(&ctx->group_map, entry->group_id, &hashmap_val, false);
  if (rc != SQC_RESULT_OK) {
    sqc_msg_error("Failed to add group %s to hashmap in priority %u: %s\n",
                  group_id, ctx->priority, sqc_error_get_string(rc));
    sqc_bbq_destroy(&entry->group_q, true);
    free(entry->group_id);
    free(entry);
    goto error;
  }

  // Insert into round-robin list at tail
  if (ctx->head == NULL) {
    ctx->head = entry;
    entry->next = entry;
    entry->prev = entry;
    ctx->cursor = entry;
  } else {
    fair_share_sched_group_entry_t *tail = ctx->head->prev;
    entry->prev = tail;
    entry->next = ctx->head;
    tail->next = entry;
    ctx->head->prev = entry;
  }

  ctx->rr_count++;

  if (out_entry != NULL) {
    *out_entry = entry;
  }

  return SQC_RESULT_OK;

error:
  return rc;
}


/*
 * Unused for now.
 * kept for a future dynamic group-addition feature.
 */
static sqc_result_t __UNUSED
s_fair_share_sched_group_find_or_register(fair_share_sched_priority_ctx_t *ctx,
                                          const char *group_id,
                                          fair_share_sched_group_entry_t **out_entry) {
  sqc_result_t rc;
  void *hashmap_val = NULL;

  (void)sqc_rwlock_writer_lock(&ctx->rwlck_);
  {
    rc = sqc_hashmap_find_no_lock(&ctx->group_map, group_id, &hashmap_val);
    if (rc == SQC_RESULT_OK && hashmap_val != NULL) {
      *out_entry = (fair_share_sched_group_entry_t *)hashmap_val;
    } else {
      rc = s_fair_share_sched_group_register(ctx, group_id, out_entry);
    }
  }
  (void)sqc_rwlock_unlock(&ctx->rwlck_);

  return rc;
}


static void
s_fair_share_sched_restore_jobs(void);


static inline sqc_result_t
s_fair_share_sched_initialize(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  dbmgr_group_info_t *gi_ptr_arr = NULL;
  size_t gi_arr_len = 0;
  char *group_id = NULL;
  sqc_dstring_t rr_order_ds = NULL;

  if (s_initialized) {
    return SQC_RESULT_OK;
  }

  sqc_msg_info("Priority queue config: priorities=%d, capacity=%lld, "
               "put_timeout_nsec=%lld, get_timeout_nsec=%lld\n",
               SQC_RPC_SCHED_PQ_NUM, (long long)SQC_RPC_SCHED_PQ_LEN,
               (long long)SQC_RPC_SCHED_PQ_PUT_TIMEOUT,
               (long long)SQC_RPC_SCHED_PQ_GET_TIMEOUT);

  rc = dbmgr_gi_group_find_all_sorted_by_remaining_time(&gi_ptr_arr, &gi_arr_len);
  if (rc != SQC_RESULT_OK) {
    sqc_msg_error("Failed to get all groups: %s\n", sqc_error_get_string(rc));
    goto error;
  }

  if (gi_arr_len == 0) {
    rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    sqc_msg_error("Failed to get group: Group not found\n");
    goto error;
  }

  // Initialize all priority contexts
  for (int pri = 0; pri < SQC_RPC_SCHED_PQ_NUM; pri++) {
    s_pq_ctx[pri].rwlck_ = NULL;
    rc = sqc_rwlock_create(&s_pq_ctx[pri].rwlck_);
    if (rc != SQC_RESULT_OK) {
      sqc_msg_error("Failed to create rwlock for priority %d: %s\n", pri,
                    sqc_error_get_string(rc));
      goto error;
    }

    s_pq_ctx[pri].group_map = NULL;
    rc = sqc_hashmap_create(&s_pq_ctx[pri].group_map, SQC_HASHMAP_TYPE_STRING,
                            s_fair_share_sched_group_freeup);
    if (rc != SQC_RESULT_OK) {
      sqc_msg_error("Failed to create hashmap for priority %d: %s\n", pri,
                    sqc_error_get_string(rc));
      goto error;
    }

    s_pq_ctx[pri].priority = (uint8_t)pri;
    s_pq_ctx[pri].head = NULL;
    s_pq_ctx[pri].cursor = NULL;
    s_pq_ctx[pri].rr_count = 0;
  }

  sqc_msg_info("Registering %zu groups across all priorities\n", gi_arr_len);

  // Best-effort: a dstring failure here must not fail scheduler init,
  // so rr_order_ds simply stays NULL and the RR order log is skipped.
  (void)sqc_dstring_create(&rr_order_ds);

  // Register all groups in all priorities, preserving the remaining-time
  // descending order as the initial group traversal order of each priority.
  for (size_t i = 0; i < gi_arr_len; i++) {
    rc = dbmgr_gi_get_group_id(gi_ptr_arr[i], &group_id);
    if (rc != SQC_RESULT_OK) {
      sqc_msg_error("Failed to get group_id at index %zu: %s\n", i,
                    sqc_error_get_string(rc));
      goto error;
    }

    if (rr_order_ds != NULL) {
      (void)sqc_dstring_appendf(&rr_order_ds, (i == 0) ? "%s" : ", %s", group_id);
    }

    for (int pri = 0; pri < SQC_RPC_SCHED_PQ_NUM; pri++) {
      rc = s_fair_share_sched_group_register(&s_pq_ctx[pri], group_id, NULL);
      if (rc != SQC_RESULT_OK) {
        free(group_id);
        goto error;
      }
    }

    free(group_id);
    group_id = NULL;
  }

  if (rr_order_ds != NULL) {
    char *rr_order_str = NULL;

    if (sqc_dstring_str_get(&rr_order_ds, &rr_order_str) == SQC_RESULT_OK &&
        rr_order_str != NULL) {
      sqc_msg_info("Group RR order: [%s]\n", rr_order_str);
    }
    sqc_dstring_destroy(&rr_order_ds);
  }

  // restore jobs
  s_fair_share_sched_restore_jobs();

  free(gi_ptr_arr);
  s_initialized = true;

  return SQC_RESULT_OK;

error:
  sqc_dstring_destroy(&rr_order_ds);
  s_fair_share_sched_priority_contexts_teardown();
  free(gi_ptr_arr);
  return rc;
}


static inline void
s_fair_share_sched_finalize(void) {
  if (!s_initialized) {
    return;
  }

  s_fair_share_sched_priority_contexts_teardown();
  s_initialized = false;
}


/*
 * Enqueue a job into its group's queue.
 * transition_to_queued controls whether the job's DB status is transitioned
 * to QUEUED after a successful enqueue:
 * - true: new submission, currently CREATED -> QUEUED.
 * - false: restore-time re-enqueue, already QUEUED in DB.
 */
static inline sqc_result_t
s_fair_share_sched_enqueue(const dbmgr_job_info_t ji_ptr, bool transition_to_queued) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  char *job_id = NULL;
  char *group_id = NULL;
  uint8_t priority = 0;
  fair_share_sched_priority_ctx_t *ctx;
  fair_share_sched_group_entry_t *entry = NULL;
  void *hashmap_val = NULL;

  if (ji_ptr == NULL) {
    rc = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("Job is invalid: err_msg=%s\n", sqc_error_get_string(rc));
    goto error;
  }

  rc = dbmgr_ji_get_job_id(ji_ptr, &job_id);
  if (rc != SQC_RESULT_OK) {
    sqc_msg_error("Failed to get db job id: err_msg=%s\n", sqc_error_get_string(rc));
    goto error;
  }

  rc = dbmgr_ji_get_group_id(ji_ptr, &group_id);
  if (rc != SQC_RESULT_OK) {
    sqc_msg_error("Failed to get group_id: job_id=%s, err_msg=%s\n",
                  job_id, sqc_error_get_string(rc));
    goto error;
  }

  rc = dbmgr_ji_get_priority(ji_ptr, &priority);
  if (rc != SQC_RESULT_OK) {
    sqc_msg_error("Failed to get priority: job_id=%s, err_msg=%s\n",
                  job_id, sqc_error_get_string(rc));
    goto error;
  }

  if (priority > SQC_RPC_SCHED_MAX_PRIORITY) {
    rc = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("Invalid priority: job_id=%s, priority=%u\n", job_id, priority);
    goto error;
  }

  ctx = &s_pq_ctx[priority];

  // Find the group's queue. Unknown group_id is an error.
  (void)sqc_rwlock_reader_lock(&ctx->rwlck_);
  {
    rc = sqc_hashmap_find_no_lock(&ctx->group_map, group_id, &hashmap_val);
    if (rc == SQC_RESULT_OK && hashmap_val != NULL) {
      entry = (fair_share_sched_group_entry_t *)hashmap_val;
    } else {
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }
  }
  (void)sqc_rwlock_unlock(&ctx->rwlck_);

  if (rc != SQC_RESULT_OK) {
    sqc_msg_error("Unknown group: job_id=%s, group_id=%s, err_msg=%s\n",
                  job_id, group_id, sqc_error_get_string(rc));
    goto error;
  }

  rc = sqc_bbq_put(&entry->group_q, &ji_ptr, dbmgr_job_info_t, SQC_RPC_SCHED_PQ_PUT_TIMEOUT);
  if (rc != SQC_RESULT_OK) {
    sqc_msg_error("Failed to put job: job_id=%s, err_msg=%s\n", job_id, sqc_error_get_string(rc));
    goto error;
  }

  sqc_msg_info("Job enqueued: job_id=%s\n", job_id);

  if (transition_to_queued == true) {
    rc = dbmgr_set_job_status_queued(ji_ptr);
    if (rc != SQC_RESULT_OK) {
      sqc_msg_error("Failed to transition status to queued: job_id=%s, err_msg=%s\n",
                    job_id, sqc_error_get_string(rc));

      if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
        sqc_msg_error("Failed to transition status to error: job_id=%s\n", job_id);
      }
    }

    rc = SQC_RESULT_OK;
  }

error:
  free(job_id);
  job_id = NULL;
  free(group_id);
  group_id = NULL;

  return rc;
}


static inline sqc_result_t
s_fair_share_sched_dispatch(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  dbmgr_job_info_t ji_ptr = NULL;

  // Always search from the highest priority queue down,
  // and search again from the top every time one job is dispatched.
  for (int pri = SQC_RPC_SCHED_MAX_PRIORITY; pri >= 0; pri--) {
    fair_share_sched_priority_ctx_t *ctx = &s_pq_ctx[pri];

    for (size_t rr_pos = 0; rr_pos < ctx->rr_count; rr_pos++) {
      fair_share_sched_group_entry_t *entry;
      sqc_chrono_t wait_time;

      // Only the first check at the top priority waits, to avoid
      // busy-spinning while idle; all other checks are non-blocking.
      if (pri == SQC_RPC_SCHED_MAX_PRIORITY && rr_pos == 0) {
        wait_time = SQC_RPC_SCHED_PQ_GET_TIMEOUT;
      } else {
        wait_time = 0LL;
      }

      // The cursor is shared state, so lock while reading/advancing it;
      // release before the sqc_bbq_get below since it may block.
      (void)sqc_rwlock_writer_lock(&ctx->rwlck_);
      {
        entry = ctx->cursor;
        ctx->cursor = entry->next;
      }
      (void)sqc_rwlock_unlock(&ctx->rwlck_);

      rc = sqc_bbq_get(&entry->group_q, &ji_ptr, dbmgr_job_info_t, wait_time);
      if (rc == SQC_RESULT_OK && ji_ptr != NULL) {
        break;
      }
      ji_ptr = NULL;
    }

    if (ji_ptr != NULL) {
      // Logging is performed within the called function.
      return s_req_invoker_invoke(ji_ptr);
    }
  }

  return SQC_RESULT_OK;
}


/*
 * Restore jobs left over from a previous run:
 * - RUNNING jobs: CANCELLED (execution was interrupted, can't be resumed)
 * - QUEUED jobs with one of the following qc_type: CANCELLED
 *   The user token isn't available after restart.
 *     - SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_USER_TOKEN
 *     - SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_BOTH_TOKEN
 * - other QUEUED jobs: re-enqueue, in creation order
 */
static void
s_fair_share_sched_restore_jobs(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  dbmgr_job_info_t *ji_ptr_arr = NULL;
  size_t arr_len = 0;
  char *job_id = NULL;
  size_t running_found = 0;
  size_t running_cancelled = 0;
  size_t queued_found = 0;
  size_t queued_cancelled = 0;
  size_t queued_restored = 0;

  // RUNNING jobs -> CANCELLED
  rc = dbmgr_ji_job_find_by_job_status(SQC_RPC_SCHED_JOB_STATUS_RUNNING,
                                       &ji_ptr_arr, &arr_len);
  if (rc == SQC_RESULT_OK) {
    running_found = arr_len;
    sqc_msg_info("Found job(running): %zu\n", arr_len);

    for (size_t i = 0; i < arr_len; i++) {
      rc = dbmgr_ji_get_job_id(ji_ptr_arr[i], &job_id);
      if (rc == SQC_RESULT_OK) {
        sqc_msg_warning("Scheduler terminated during job execution. "
                        "Therefore, the job execution cancelled: job_id=%s\n", job_id);

        if (dbmgr_set_job_status_cancelled(ji_ptr_arr[i]) != SQC_RESULT_OK) {
          sqc_msg_error("Failed to transition status to cancelled: job_id=%s\n", job_id);
        } else {
          running_cancelled++;
        }

        free(job_id);
        job_id = NULL;
      } else {
        sqc_msg_error("Failed to get job_id for running job at index %zu: %s\n", i,
                      sqc_error_get_string(rc));
      }
    }
  } else {
    sqc_msg_error("Failed to find job with status running: %s\n", sqc_error_get_string(rc));
  }
  free(ji_ptr_arr);
  ji_ptr_arr = NULL;
  arr_len = 0;

  // QUEUED jobs -> re-enqueue or CANCELLED
  rc = dbmgr_ji_job_find_by_job_status(SQC_RPC_SCHED_JOB_STATUS_QUEUED,
                                       &ji_ptr_arr, &arr_len);
  if (rc == SQC_RESULT_OK) {
    queued_found = arr_len;
    sqc_msg_info("Found job(queued): %zu\n", arr_len);

    if (arr_len > 0) {
      rc = dbmgr_ji_job_arr_sort_by_created_time(ji_ptr_arr, arr_len);
      if (rc == SQC_RESULT_OK) {
        for (size_t i = 0; i < arr_len; i++) {
          sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_UNKNOWN;

          rc = dbmgr_ji_get_qc_type(ji_ptr_arr[i], &qc_type);
          if (rc == SQC_RESULT_OK &&
              (qc_type == SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_USER_TOKEN ||
               qc_type == SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_BOTH_TOKEN)) {
            rc = dbmgr_ji_get_job_id(ji_ptr_arr[i], &job_id);
            if (rc == SQC_RESULT_OK) {
              sqc_msg_warning("User token unavailable after restart. Job cancelled: "
                              "job_id=%s, qc_type=%d\n", job_id, qc_type);

              if (dbmgr_set_job_status_cancelled(ji_ptr_arr[i]) != SQC_RESULT_OK) {
                sqc_msg_error("Failed to transition status to cancelled: job_id=%s\n", job_id);
              } else {
                queued_cancelled++;
              }

              free(job_id);
              job_id = NULL;
            } else {
              sqc_msg_error("Failed to get job_id for queued job at index %zu: %s\n", i,
                            sqc_error_get_string(rc));
            }

            continue;
          }

          if (rc != SQC_RESULT_OK) {
            // qc_type unknown: fall through and attempt normal enqueue anyway.
            // If this job actually needed a token, it will fail later at execution time.
            sqc_msg_error("Failed to get qc_type for queued job at index %zu: %s\n", i,
                          sqc_error_get_string(rc));
          }

          if (s_fair_share_sched_enqueue(ji_ptr_arr[i], false) != SQC_RESULT_OK) {
            break;
          }
          queued_restored++;
        }
      } else {
        sqc_msg_error("Failed to sort job array for status queued: %s\n", sqc_error_get_string(rc));
      }
    }
  } else {
    sqc_msg_error("Failed to find job with status queued: %s\n", sqc_error_get_string(rc));
  }

  free(ji_ptr_arr);
  ji_ptr_arr = NULL;

  sqc_msg_info("Job restore summary: running(found=%zu, cancelled=%zu), "
               "queued(found=%zu, restored=%zu, cancelled=%zu)\n",
               running_found, running_cancelled,
               queued_found, queued_restored, queued_cancelled);
}


static sqc_result_t
s_fair_share_sched_submit(const dbmgr_job_info_t ji_ptr) {
  return s_fair_share_sched_enqueue(ji_ptr, true);
}


/*
 * export
 */


const job_sched_scheduling_algo_t *
job_sched_fair_share_get_scheduling_algo(void) {
  static const job_sched_scheduling_algo_t algo = {
    .name = "fair_share",
    .initialize = s_fair_share_sched_initialize,
    .finalize = s_fair_share_sched_finalize,
    .submit = s_fair_share_sched_submit,
    .dispatch = s_fair_share_sched_dispatch,
  };

  return &algo;
}

