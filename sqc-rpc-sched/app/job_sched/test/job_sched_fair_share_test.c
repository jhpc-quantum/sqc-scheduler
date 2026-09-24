#include <sys/stat.h>
#include <unistd.h>

#include "unity.h"

#include "dbmgr_util.c"
#include "dbmgr_db.c"
#include "dbmgr_group_info.c"
#include "dbmgr_job_info.c"

#include "job_sched_fair_share.c"

#include "dbmgr_db_common.c"

#define S_STUB_MAX_RECORDS 32
static int s_stub_invoke_count;
static dbmgr_job_info_t s_stub_invoked_order[S_STUB_MAX_RECORDS];

/*
 * records invocation order so dispatch order can be asserted
 */
static sqc_result_t
s_stub_req_invoker_invoke(const dbmgr_job_info_t ji_ptr) {
  if ((size_t)s_stub_invoke_count < S_STUB_MAX_RECORDS) {
    s_stub_invoked_order[s_stub_invoke_count] = ji_ptr;
  }
  s_stub_invoke_count++;
  return SQC_RESULT_OK;
}

#define S_GROUP_COUNT 10

/*
 * creates grp0..grp<N-1>
 * equal remaining time ties on strcmp(group_id)
 * so the round-robin ring order is ascending
 */
static void
s_create_groups(void) {
  dbmgr_group_info_t gi_ptr = NULL;
  char group_id[16];

  for (int g = 0; g < S_GROUP_COUNT; g++) {
    snprintf(group_id, sizeof(group_id), "grp%d", g);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, dbmgr_gi_create_group(group_id, 0, 0, &gi_ptr));
  }
}

typedef struct {
  const char *group_id;
  uint8_t priority;
} s_job_spec_t;

/*
 * shared job fixture for tests 3-6
 * array order == expected dispatch() order
 */
static const s_job_spec_t s_shared_job_specs[] = {
  {"grp0", 9}, {"grp1", 9}, {"grp2", 9},
  {"grp3", 6}, {"grp5", 6}, {"grp7", 6},
  {"grp9", 2},
  {"grp0", 0}, {"grp4", 0}, {"grp6", 0}, {"grp8", 0},
};
#define S_SHARED_JOB_COUNT (sizeof(s_shared_job_specs) / sizeof(s_shared_job_specs[0]))

static void
s_create_shared_jobs(dbmgr_job_info_t jobs_out[S_SHARED_JOB_COUNT]) {
  s_create_groups();

  for (size_t i = 0; i < S_SHARED_JOB_COUNT; i++) {
    TEST_ASSERT_EQUAL(SQC_RESULT_OK,
                      dbmgr_ji_create_job("user1", s_shared_job_specs[i].group_id,
                                          s_shared_job_specs[i].priority, "qprogram-data",
                                          SQC_RPC_SCHED_CIRCUIT_FMT_QIR, 1000,
                                          SQC_RPC_SCHED_QC_TYPE_RQC_REST,
                                          SQC_RPC_SCHED_TRANSPILER_NORMAL,
                                          "remark", "user-token", &jobs_out[i]));
  }
}

static void
s_enqueue_shared_jobs(dbmgr_job_info_t jobs[S_SHARED_JOB_COUNT]) {
  for (size_t i = 0; i < S_SHARED_JOB_COUNT; i++) {
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_fair_share_sched_enqueue(jobs[i], true));
  }
}

void setUp(void) {
  s_stub_invoke_count = 0;

  remove_db_files("setUp - fair_share");
  create_db_files("setUp - fair_share");

  TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE));
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_dbmgr_group_info_initialize());
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_dbmgr_job_info_initialize());
}

void tearDown(void) {
  s_req_invoker_invoke = req_invoker_invoke;
  s_fair_share_sched_priority_contexts_teardown();
  s_initialized = false;

  s_dbmgr_job_info_finalize();
  s_dbmgr_group_info_finalize();
  s_dbmgr_db_finalize();

  remove_db_files("tearDown - fair_share");
}

/*
 * 10 groups + initialize(): every priority gets rr_count==10, all groups in
 * its hashmap, and a ring ordered grp0..grp9 that closes after 10 nodes
 */
void test_fair_share_sched_initialize_builds_pq_ctx(void) {
  s_create_groups();

  TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_fair_share_sched_initialize());
  TEST_ASSERT_TRUE(s_initialized);

  for (int pri = 0; pri < SQC_RPC_SCHED_PQ_NUM; pri++) {
    fair_share_sched_priority_ctx_t *ctx = &s_pq_ctx[pri];
    fair_share_sched_group_entry_t *node;
    char group_id[16];
    void *val = NULL;

    TEST_ASSERT_NOT_NULL(ctx->rwlck_);
    TEST_ASSERT_NOT_NULL(ctx->group_map);
    TEST_ASSERT_NOT_NULL(ctx->head);
    TEST_ASSERT_EQUAL(S_GROUP_COUNT, ctx->rr_count);

    // every group registered, with a matching group_id and a fresh empty queue
    for (int g = 0; g < S_GROUP_COUNT; g++) {
      fair_share_sched_group_entry_t *entry;
      dbmgr_job_info_t got_job = NULL;

      snprintf(group_id, sizeof(group_id), "grp%d", g);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, sqc_hashmap_find_no_lock(&ctx->group_map, group_id, &val));
      TEST_ASSERT_NOT_NULL(val);

      entry = (fair_share_sched_group_entry_t *)val;
      TEST_ASSERT_EQUAL_STRING(group_id, entry->group_id);

      // freshly registered group's queue must start empty
      (void)sqc_bbq_get(&entry->group_q, &got_job, dbmgr_job_info_t, 0LL);
      TEST_ASSERT_NULL(got_job);
    }

    // ring:
    // - ascending group_id order
    // - closes after S_GROUP_COUNT nodes (last->next == head)
    // - each node is the same entry the hashmap holds
    node = ctx->head;
    for (int g = 0; g < S_GROUP_COUNT; g++) {
      void *hashmap_val = NULL;

      snprintf(group_id, sizeof(group_id), "grp%d", g);
      TEST_ASSERT_EQUAL_STRING(group_id, node->group_id);

      TEST_ASSERT_EQUAL(SQC_RESULT_OK, sqc_hashmap_find_no_lock(&ctx->group_map, group_id, &hashmap_val));
      TEST_ASSERT_EQUAL_PTR(hashmap_val, node);

      node = node->next;
    }
    TEST_ASSERT_EQUAL_PTR(ctx->head, node);
  }
}

/*
 * 10 groups + initialize() then finalize(): every priority's rwlck_/
 * group_map/head/cursor go back to NULL and rr_count to 0
 */
void test_fair_share_sched_finalize_releases_pq_ctx(void) {
  s_create_groups();

  TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_fair_share_sched_initialize());
  TEST_ASSERT_TRUE(s_initialized);

  s_fair_share_sched_finalize();

  TEST_ASSERT_FALSE(s_initialized);
  for (int pri = 0; pri < SQC_RPC_SCHED_PQ_NUM; pri++) {
    fair_share_sched_priority_ctx_t *ctx = &s_pq_ctx[pri];

    TEST_ASSERT_NULL(ctx->rwlck_);
    TEST_ASSERT_NULL(ctx->group_map);
    TEST_ASSERT_NULL(ctx->head);
    TEST_ASSERT_NULL(ctx->cursor);
    TEST_ASSERT_EQUAL(0, ctx->rr_count);
  }
}

/*
 * enqueue() all 11 shared jobs: each ends up QUEUED in DB and on its
 * (priority, group) queue
 */
void test_fair_share_sched_enqueue_puts_job_on_group_queue(void) {
  dbmgr_job_info_t jobs[S_SHARED_JOB_COUNT];
  sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;

  s_create_shared_jobs(jobs);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_fair_share_sched_initialize());

  s_enqueue_shared_jobs(jobs);

  for (size_t i = 0; i < S_SHARED_JOB_COUNT; i++) {
    fair_share_sched_priority_ctx_t *ctx = &s_pq_ctx[s_shared_job_specs[i].priority];
    fair_share_sched_group_entry_t *entry = NULL;
    dbmgr_job_info_t got_job = NULL;
    void *val = NULL;

    TEST_ASSERT_EQUAL(SQC_RESULT_OK, dbmgr_ji_get_status(jobs[i], &status));
    TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_QUEUED, status);

    TEST_ASSERT_EQUAL(SQC_RESULT_OK,
                      sqc_hashmap_find_no_lock(&ctx->group_map, s_shared_job_specs[i].group_id, &val));
    entry = (fair_share_sched_group_entry_t *)val;
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, sqc_bbq_get(&entry->group_q, &got_job, dbmgr_job_info_t, 0LL));
    TEST_ASSERT_EQUAL_PTR(jobs[i], got_job);
  }
}

/*
 * one dispatch() invokes the stub once, for jobs[0] (grp0@priority9:
 * highest priority, first in ring)
 */
void test_fair_share_sched_dispatch_invokes_req_invoker_fn(void) {
  dbmgr_job_info_t jobs[S_SHARED_JOB_COUNT];

  s_create_shared_jobs(jobs);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_fair_share_sched_initialize());
  s_enqueue_shared_jobs(jobs);

  s_req_invoker_invoke = s_stub_req_invoker_invoke;

  TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_fair_share_sched_dispatch());

  TEST_ASSERT_EQUAL(1, s_stub_invoke_count);

  // grp0@priority9: highest priority, first in ring
  TEST_ASSERT_EQUAL_PTR(jobs[0], s_stub_invoked_order[0]);
}

/*
 * 7 dispatches drain priorities 9/6/2 in jobs[0..6] order
 * priority-0 jobs must still be sitting in their queues
 */
void test_fair_share_sched_dispatch_prefers_higher_priority(void) {
  dbmgr_job_info_t jobs[S_SHARED_JOB_COUNT];
  const size_t higher_priority_count = 7; /* priority 9 (3) + 6 (3) + 2 (1) */

  s_create_shared_jobs(jobs);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_fair_share_sched_initialize());
  s_enqueue_shared_jobs(jobs);

  s_req_invoker_invoke = s_stub_req_invoker_invoke;

  for (size_t i = 0; i < higher_priority_count; i++) {
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_fair_share_sched_dispatch());
  }

  TEST_ASSERT_EQUAL((int)higher_priority_count, s_stub_invoke_count);
  for (size_t i = 0; i < higher_priority_count; i++) {
    TEST_ASSERT_EQUAL_PTR(jobs[i], s_stub_invoked_order[i]);
  }

  // the priority-0 jobs must still be untouched
  for (size_t i = higher_priority_count; i < S_SHARED_JOB_COUNT; i++) {
    fair_share_sched_priority_ctx_t *ctx = &s_pq_ctx[s_shared_job_specs[i].priority];
    fair_share_sched_group_entry_t *entry = NULL;
    dbmgr_job_info_t got_job = NULL;
    void *val = NULL;

    TEST_ASSERT_EQUAL(SQC_RESULT_OK,
                      sqc_hashmap_find_no_lock(&ctx->group_map, s_shared_job_specs[i].group_id, &val));
    entry = (fair_share_sched_group_entry_t *)val;
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, sqc_bbq_get(&entry->group_q, &got_job, dbmgr_job_info_t, 0LL));
    TEST_ASSERT_EQUAL_PTR(jobs[i], got_job);
  }
}

/*
 * 11 dispatches match s_shared_job_specs order exactly (priority
 * descending, round-robin, empty groups skipped)
 * a 12th invokes nothing
 */
void test_fair_share_sched_dispatch_round_robins_within_priority(void) {
  dbmgr_job_info_t jobs[S_SHARED_JOB_COUNT];

  s_create_shared_jobs(jobs);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_fair_share_sched_initialize());
  s_enqueue_shared_jobs(jobs);

  s_req_invoker_invoke = s_stub_req_invoker_invoke;

  for (size_t i = 0; i < S_SHARED_JOB_COUNT; i++) {
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_fair_share_sched_dispatch());
  }

  TEST_ASSERT_EQUAL((int)S_SHARED_JOB_COUNT, s_stub_invoke_count);
  for (size_t i = 0; i < S_SHARED_JOB_COUNT; i++) {
    TEST_ASSERT_EQUAL_PTR(jobs[i], s_stub_invoked_order[i]);
  }

  /* all queues empty: dispatch must not invoke anything */
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_fair_share_sched_dispatch());
  TEST_ASSERT_EQUAL((int)S_SHARED_JOB_COUNT, s_stub_invoke_count);
}

/* restore_jobs(): recovers jobs left over from a previous run */

/*
 * a leftover RUNNING job -> CANCELLED after initialize()
 * an untouched CREATED job is left unchanged
 */
void test_fair_share_sched_restore_jobs_cancels_running_jobs(void) {
  dbmgr_group_info_t gi_ptr = NULL;
  dbmgr_job_info_t job_running = NULL;
  dbmgr_job_info_t job_created = NULL;
  sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;

  TEST_ASSERT_EQUAL(SQC_RESULT_OK, dbmgr_gi_create_group("grp0", 0, 0, &gi_ptr));

  TEST_ASSERT_EQUAL(SQC_RESULT_OK,
                    dbmgr_ji_create_job("user1", "grp0", 0, "qprogram-data",
                                        SQC_RPC_SCHED_CIRCUIT_FMT_QIR, 1000,
                                        SQC_RPC_SCHED_QC_TYPE_RQC_REST,
                                        SQC_RPC_SCHED_TRANSPILER_NORMAL,
                                        "remark", "user-token", &job_running));
  TEST_ASSERT_EQUAL(SQC_RESULT_OK,
                    dbmgr_ji_create_job("user1", "grp0", 0, "qprogram-data",
                                        SQC_RPC_SCHED_CIRCUIT_FMT_QIR, 1000,
                                        SQC_RPC_SCHED_QC_TYPE_RQC_REST,
                                        SQC_RPC_SCHED_TRANSPILER_NORMAL,
                                        "remark", "user-token", &job_created));

  /* CREATED -> QUEUED -> RUNNING: simulate a leftover RUNNING job */
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, dbmgr_set_job_status_queued(job_running));
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, dbmgr_set_job_status_running(job_running));

  TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_fair_share_sched_initialize());

  TEST_ASSERT_EQUAL(SQC_RESULT_OK, dbmgr_ji_get_status(job_running, &status));
  TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_CANCELLED, status);

  /* untouched job must stay CREATED */
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, dbmgr_ji_get_status(job_created, &status));
  TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_CREATED, status);
}

/*
 * 3 leftover QUEUED jobs (same group/priority, created A,B,C) are
 * re-enqueued and dispatched in creation order
 */
void test_fair_share_sched_restore_jobs_reenqueues_queued_jobs_in_creation_order(void) {
  dbmgr_group_info_t gi_ptr = NULL;
  dbmgr_job_info_t job_a = NULL;
  dbmgr_job_info_t job_b = NULL;
  dbmgr_job_info_t job_c = NULL;

  TEST_ASSERT_EQUAL(SQC_RESULT_OK, dbmgr_gi_create_group("grp0", 0, 0, &gi_ptr));

  /* same group/priority so round-robin can't affect creation-time order */
  TEST_ASSERT_EQUAL(SQC_RESULT_OK,
                    dbmgr_ji_create_job("user1", "grp0", 0, "qprogram-data",
                                        SQC_RPC_SCHED_CIRCUIT_FMT_QIR, 1000,
                                        SQC_RPC_SCHED_QC_TYPE_RQC_REST,
                                        SQC_RPC_SCHED_TRANSPILER_NORMAL,
                                        "remark", "user-token", &job_a));
  TEST_ASSERT_EQUAL(SQC_RESULT_OK,
                    dbmgr_ji_create_job("user1", "grp0", 0, "qprogram-data",
                                        SQC_RPC_SCHED_CIRCUIT_FMT_QIR, 1000,
                                        SQC_RPC_SCHED_QC_TYPE_RQC_REST,
                                        SQC_RPC_SCHED_TRANSPILER_NORMAL,
                                        "remark", "user-token", &job_b));
  TEST_ASSERT_EQUAL(SQC_RESULT_OK,
                    dbmgr_ji_create_job("user1", "grp0", 0, "qprogram-data",
                                        SQC_RPC_SCHED_CIRCUIT_FMT_QIR, 1000,
                                        SQC_RPC_SCHED_QC_TYPE_RQC_REST,
                                        SQC_RPC_SCHED_TRANSPILER_NORMAL,
                                        "remark", "user-token", &job_c));

  /* simulate leftover QUEUED jobs from a previous run */
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, dbmgr_set_job_status_queued(job_a));
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, dbmgr_set_job_status_queued(job_b));
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, dbmgr_set_job_status_queued(job_c));

  TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_fair_share_sched_initialize());

  s_req_invoker_invoke = s_stub_req_invoker_invoke;

  for (int i = 0; i < 3; i++) {
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_fair_share_sched_dispatch());
  }

  TEST_ASSERT_EQUAL(3, s_stub_invoke_count);
  TEST_ASSERT_EQUAL_PTR(job_a, s_stub_invoked_order[0]);
  TEST_ASSERT_EQUAL_PTR(job_b, s_stub_invoked_order[1]);
  TEST_ASSERT_EQUAL_PTR(job_c, s_stub_invoked_order[2]);
}

/*
 * leftover QUEUED jobs needing USER_TOKEN/BOTH_TOKEN -> CANCELLED
 * a normal QUEUED job is re-enqueued and actually dispatched
 */
void test_fair_share_sched_restore_jobs_cancels_queued_jobs_needing_user_token(void) {
  dbmgr_group_info_t gi_ptr = NULL;
  dbmgr_job_info_t job_user_token = NULL;
  dbmgr_job_info_t job_both_token = NULL;
  dbmgr_job_info_t job_normal = NULL;
  sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;

  TEST_ASSERT_EQUAL(SQC_RESULT_OK, dbmgr_gi_create_group("grp0", 0, 0, &gi_ptr));

  TEST_ASSERT_EQUAL(SQC_RESULT_OK,
                    dbmgr_ji_create_job("user1", "grp0", 0, "qprogram-data",
                                        SQC_RPC_SCHED_CIRCUIT_FMT_QIR, 1000,
                                        SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_USER_TOKEN,
                                        SQC_RPC_SCHED_TRANSPILER_NORMAL,
                                        "remark", "user-token", &job_user_token));
  TEST_ASSERT_EQUAL(SQC_RESULT_OK,
                    dbmgr_ji_create_job("user1", "grp0", 0, "qprogram-data",
                                        SQC_RPC_SCHED_CIRCUIT_FMT_QIR, 1000,
                                        SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_BOTH_TOKEN,
                                        SQC_RPC_SCHED_TRANSPILER_NORMAL,
                                        "remark", "user-token", &job_both_token));
  TEST_ASSERT_EQUAL(SQC_RESULT_OK,
                    dbmgr_ji_create_job("user1", "grp0", 0, "qprogram-data",
                                        SQC_RPC_SCHED_CIRCUIT_FMT_QIR, 1000,
                                        SQC_RPC_SCHED_QC_TYPE_RQC_REST,
                                        SQC_RPC_SCHED_TRANSPILER_NORMAL,
                                        "remark", "user-token", &job_normal));

  TEST_ASSERT_EQUAL(SQC_RESULT_OK, dbmgr_set_job_status_queued(job_user_token));
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, dbmgr_set_job_status_queued(job_both_token));
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, dbmgr_set_job_status_queued(job_normal));

  TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_fair_share_sched_initialize());

  /* token unavailable after restart: must be cancelled */
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, dbmgr_ji_get_status(job_user_token, &status));
  TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_CANCELLED, status);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, dbmgr_ji_get_status(job_both_token, &status));
  TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_CANCELLED, status);

  /* re-enqueued, stays QUEUED */
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, dbmgr_ji_get_status(job_normal, &status));
  TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_QUEUED, status);

  s_req_invoker_invoke = s_stub_req_invoker_invoke;
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_fair_share_sched_dispatch());

  TEST_ASSERT_EQUAL(1, s_stub_invoke_count);
  TEST_ASSERT_EQUAL_PTR(job_normal, s_stub_invoked_order[0]);
}

/*
 * Negative test cases for private methods
 */

/* enqueue(): error paths */

/* NULL job -> SQC_RESULT_INVALID_ARGS (no initialize() needed) */
void test_negative_fair_share_sched_enqueue_rejects_null_job(void) {
  TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, s_fair_share_sched_enqueue(NULL, true));
}

/*
 * job with an unregistered group_id -> SQC_RESULT_ANY_RUNTIME_ERROR
 * DB status stays CREATED
 */
void test_negative_fair_share_sched_enqueue_rejects_unknown_group(void) {
  dbmgr_job_info_t ji_ptr = NULL;
  sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;

  s_create_groups();
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_fair_share_sched_initialize());

  TEST_ASSERT_EQUAL(SQC_RESULT_OK,
                    dbmgr_ji_create_job("user1", "ghost-group", 0, "qprogram-data",
                                        SQC_RPC_SCHED_CIRCUIT_FMT_QIR, 1000,
                                        SQC_RPC_SCHED_QC_TYPE_RQC_REST,
                                        SQC_RPC_SCHED_TRANSPILER_NORMAL,
                                        "remark", "user-token", &ji_ptr));

  TEST_ASSERT_EQUAL(SQC_RESULT_ANY_RUNTIME_ERROR, s_fair_share_sched_enqueue(ji_ptr, true));

  TEST_ASSERT_EQUAL(SQC_RESULT_OK, dbmgr_ji_get_status(ji_ptr, &status));
  TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_CREATED, status);
}

/*
 * queue filled to SQC_RPC_SCHED_PQ_LEN -> enqueue returns SQC_RESULT_TIMEDOUT
 * DB status stays CREATED
 */
void test_negative_fair_share_sched_enqueue_returns_timedout_when_queue_full(void) {
  dbmgr_group_info_t gi_ptr = NULL;
  dbmgr_job_info_t ji_ptr = NULL;
  dbmgr_job_info_t dummy_job = (dbmgr_job_info_t)0x1;
  const uint8_t priority = 3;
  fair_share_sched_priority_ctx_t *ctx = &s_pq_ctx[priority];
  fair_share_sched_group_entry_t *entry = NULL;
  sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;
  void *val = NULL;

  TEST_ASSERT_EQUAL(SQC_RESULT_OK, dbmgr_gi_create_group("grp0", 0, 0, &gi_ptr));
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_fair_share_sched_initialize());

  TEST_ASSERT_EQUAL(SQC_RESULT_OK, sqc_hashmap_find_no_lock(&ctx->group_map, "grp0", &val));
  entry = (fair_share_sched_group_entry_t *)val;
  for (size_t i = 0; i < (size_t)SQC_RPC_SCHED_PQ_LEN; i++) {
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, sqc_bbq_put(&entry->group_q, &dummy_job, dbmgr_job_info_t, 0LL));
  }

  TEST_ASSERT_EQUAL(SQC_RESULT_OK,
                    dbmgr_ji_create_job("user1", "grp0", priority, "qprogram-data",
                                        SQC_RPC_SCHED_CIRCUIT_FMT_QIR, 1000,
                                        SQC_RPC_SCHED_QC_TYPE_RQC_REST,
                                        SQC_RPC_SCHED_TRANSPILER_NORMAL,
                                        "remark", "user-token", &ji_ptr));

  TEST_ASSERT_EQUAL(SQC_RESULT_TIMEDOUT, s_fair_share_sched_enqueue(ji_ptr, true));

  TEST_ASSERT_EQUAL(SQC_RESULT_OK, dbmgr_ji_get_status(ji_ptr, &status));
  TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_CREATED, status);
}

/* initialize(): error/boundary paths */

/*
 * no groups in DB -> initialize() returns non-OK, s_initialized stays false
 * (exact rc is a dbmgr-layer quirk, see job-sched-ut.md)
 */
void test_negative_fair_share_sched_initialize_returns_error_when_no_groups(void) {
  TEST_ASSERT_NOT_EQUAL(SQC_RESULT_OK, s_fair_share_sched_initialize());
  TEST_ASSERT_FALSE(s_initialized);
}

/* a second initialize() call returns OK without rebuilding s_pq_ctx */
void test_fair_share_sched_initialize_is_idempotent(void) {
  fair_share_sched_group_entry_t *head_before;

  s_create_groups();
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_fair_share_sched_initialize());
  head_before = s_pq_ctx[0].head;

  TEST_ASSERT_EQUAL(SQC_RESULT_OK, s_fair_share_sched_initialize());

  TEST_ASSERT_EQUAL_PTR(head_before, s_pq_ctx[0].head);
}

