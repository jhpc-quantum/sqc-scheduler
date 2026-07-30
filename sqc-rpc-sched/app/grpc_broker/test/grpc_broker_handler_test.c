#include "unity.h"

#include "grpc_broker_handler.c"
#include "sqc_rpc_sched_conf.h"
#include "sqc_rpc_sched_paths.h"

void setUp(void) {
}

void tearDown(void) {
}

void DISABLED_grpc_broker_initialize_finalize(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  // initialize
  {
    (void) sqc_rpc_sched_conf_set_conf_dir(APP_SYSCONFDIR);
    rc = s_grpc_broker_handler_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  {
    s_grpc_broker_handler_finalize();
  }
}

void DISABLED_grpc_broker_handle_job_submit(void) {
}

void DISABLED_grpc_broker_handle_job_status(void) {
}
