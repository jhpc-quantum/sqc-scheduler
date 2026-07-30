#include "unity.h"

#include "req_invoker_invoke.c"

void setUp(void) {
}

void tearDown(void) {
}

// DB became necessary, exceeding the scope of unit testing, so it was disabled.
void DISABLED_req_invoker_invoke_initialize_finalize(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  // initialize
  {
    rc = s_req_invoker_invoke_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    for (int i = 0; i <= SQC_RPC_SCHED_MAX_PRIORITY; i++) {
      TEST_ASSERT_NOT_NULL(req_invoker_priority_qs[i]);
    }
  }

  // finalize
  {
    s_req_invoker_invoke_finalize();
    for (int i = 0; i <= SQC_RPC_SCHED_MAX_PRIORITY; i++) {
      TEST_ASSERT_NULL(req_invoker_priority_qs[i]);
    }
  }
}

// DB became necessary, exceeding the scope of unit testing, so it was disabled.
void DISABLED_req_sched_schedule_main(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QASM;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_DUMMY;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  char *job_id = NULL;

  // initialize
  {
    rc = s_req_invoker_invoke_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
//    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // enqueue job
  {
    rc = req_invoker_enqueue(ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // run main
  {
    s_req_invoker_invoke_job();
  }

  // delete job
  {
    rc = dbmgr_ji_get_job_id(ji_ptr, &job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_ji_delete_job(job_id);
//    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  {
    s_req_invoker_invoke_finalize();
  }
}

