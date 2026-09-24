#include "unity.h"

#include "dbmgr_util.c"
#include "dbmgr_db.c"
#include "dbmgr_weight_info.c"

#include "dbmgr_db_common.c"

void setUp(void) {
  remove_db_files("setUp - weight_info");
  create_db_files("setUp - weight_info");
}

void tearDown(void) {
  remove_db_files("tearDown - weight_info");
}

/*
 * Positive test cases for private methods
 */

void test_s_dbmgr_weight_info_initialize_finalize(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_weight_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(weight_info_hashmap);
  }

  // finalize
  {
    s_dbmgr_weight_info_finalize();
    TEST_ASSERT_NULL(weight_info_hashmap);

    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);
  }
}

/*
 * Same test exists in test_s_dbmgr_weight_info_initialize_finalize
 */
//void XXXX_s_dbmgr_weight_info_record_freeup(void) {
//  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
//  const uint8_t priority = 99;
//  const uint64_t weight = 9999;
//  dbmgr_weight_info_t wi_ptr = NULL;
//
//  uint64_t actual_weight = 0;
//
//  // initialize
//  {
//    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
//    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
//    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);
//
//    rc = s_dbmgr_weight_info_initialize();
//    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
//  }
//
//  // create & add record(free in finalize)
//  {
//    rc = s_dbmgr_weight_info_record_create(priority, weight, &wi_ptr);
//    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
//
//    rc = s_dbmgr_weight_info_record_add(wi_ptr);
//    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
//
//    rc = dbmgr_wi_get_weight(priority, &actual_weight);
//    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
//    TEST_ASSERT_EQUAL(weight, actual_weight);
//  }
//
//  // finalize
//  s_dbmgr_weight_info_finalize();
//
//  s_dbmgr_db_finalize();
//  TEST_ASSERT_NULL(sqc_rpc_sched_db);
//}

void test_s_dbmgr_weight_info_record_create_destroy(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const uint8_t priority = 5;
  const uint64_t weight = 9999;
  dbmgr_weight_info_t wi_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_weight_info_record_create(priority, weight, &wi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(priority, wi_ptr->priority);
    TEST_ASSERT_EQUAL_UINT64(weight, wi_ptr->weight);
    TEST_ASSERT_NOT_NULL(wi_ptr->rwlck_);
    TEST_ASSERT_EQUAL(wi_ptr->created_time, wi_ptr->update_time);
  }

  // destroy record
  {
    s_dbmgr_weight_info_record_destroy(wi_ptr);
    wi_ptr = NULL;
  }
}


/*
 * Positive test cases for public methods
 */

void test_dbmgr_wi_get_weight(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const uint8_t priority = 5;
  uint64_t actual_weight = 0;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_weight_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get weight
  {
    rc = dbmgr_wi_get_weight(priority, &actual_weight);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_UINT64(priority * SQC_RPC_SCHED_WEIGHT_SCALE, actual_weight);
  }

  // finalize
  s_dbmgr_weight_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_wi_get_created_time(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const uint8_t priority = 5;
  sqc_chrono_t actual_created_time;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_weight_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get created time
  {
    rc = dbmgr_wi_get_created_time(priority, &actual_created_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_LESS_OR_EQUAL_UINT64(sqc_chrono_now(), actual_created_time);
  }

  // finalize
  s_dbmgr_weight_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_wi_get_update_time(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const uint8_t priority = 5;
  sqc_chrono_t actual_update_time;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_weight_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get update time
  {
    rc = dbmgr_wi_get_created_time(priority, &actual_update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_LESS_OR_EQUAL_UINT64(sqc_chrono_now(), actual_update_time);
  }

  // finalize
  s_dbmgr_weight_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}


/*
 * Negative test cases for private methods
 */

void test_negative_s_dbmgr_weight_info_record_destroy(void) {
  s_dbmgr_weight_info_record_destroy(NULL);
}

void test_negative_s_dbmgr_weight_info_record_find(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const uint8_t priority = 5;
  dbmgr_weight_info_t wi_ptr = NULL;
  dbmgr_weight_info_t actual_wi_ptr = NULL;

  // not started
  {
    rc = s_dbmgr_weight_info_record_find(priority, &actual_wi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_STARTED, rc);
    TEST_ASSERT_NULL(actual_wi_ptr);
  }

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_weight_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // non-existent priority
  {
    rc = s_dbmgr_weight_info_record_find(99, &actual_wi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_FOUND, rc);
    TEST_ASSERT_NULL(actual_wi_ptr);
  }

  // invalid record
  {
    rc = s_dbmgr_weight_info_record_find(priority, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_weight_info_record_destroy(wi_ptr);
    wi_ptr = NULL;
  }

  // finalize
  s_dbmgr_weight_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

/*
 * Negative test cases for public methods
 */

void test_negative_dbmgr_wi_get_weight(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const uint8_t priority = 5;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_weight_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid weight
  {
    rc = dbmgr_wi_get_weight(priority, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // finalize
  s_dbmgr_weight_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_negative_dbmgr_wi_get_created_time(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const uint8_t priority = 5;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_weight_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid created time
  {
    rc = dbmgr_wi_get_created_time(priority, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // finalize
  s_dbmgr_weight_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_negative_dbmgr_wi_get_update_time(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const uint8_t priority = 5;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_weight_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid update time
  {
    rc = dbmgr_wi_get_created_time(priority, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // finalize
  s_dbmgr_weight_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

