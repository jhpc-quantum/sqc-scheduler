#include <sys/stat.h>

#include "unity.h"

#include "dbmgr_util.c"
#include "dbmgr_db.c"
#include "dbmgr_user_group_info.c"

#include "dbmgr_db_common.c"

void setUp(void) {
  remove_db_files("setUp - user_group_info");
  create_db_files("setUp - user_group_info");
}

void tearDown(void) {
  remove_db_files("tearDown - user_group_info");
}

/*
 * Positive test cases for private methods
 */

void test_s_dbmgr_user_group_info_initialize_finalize(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(user_group_info_hashmap);
  }

  // finalize
  {
    s_dbmgr_user_group_info_finalize();
    TEST_ASSERT_NULL(user_group_info_hashmap);

    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);
  }
}

void test_s_dbmgr_user_group_info_record_freeup(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  dbmgr_user_group_info_t ugi_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create & add record(free in finalize)
  {
    rc = s_dbmgr_user_group_info_record_create(user_id, group_id, &ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_user_group_info_record_add(ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_TRUE(dbmgr_ugi_is_user_in_group(user_id, group_id));
  }

  // finalize
  s_dbmgr_user_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_s_dbmgr_user_group_info_record_create_destroy(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const size_t user_id_len = strlen(user_id);
  const char *group_id = "test-group";
  const size_t group_id_len = strlen(group_id);
  dbmgr_user_group_info_t ugi_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_user_group_info_record_create(user_id, group_id, &ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(user_id, ugi_ptr->user_id);
    TEST_ASSERT_EQUAL(user_id_len, ugi_ptr->user_id_len);
    TEST_ASSERT_EQUAL_STRING(group_id, ugi_ptr->group_id);
    TEST_ASSERT_EQUAL(group_id_len, ugi_ptr->group_id_len);
  }

  // destroy record
  {
    s_dbmgr_user_group_info_record_destroy(ugi_ptr);
    ugi_ptr = NULL;
  }
}

void test_s_dbmgr_user_group_info_record_add_delete(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  dbmgr_user_group_info_t ugi_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create record
  {
    rc = s_dbmgr_user_group_info_record_create(user_id, group_id, &ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // add record
  {
    rc = s_dbmgr_user_group_info_record_add(ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_TRUE(dbmgr_ugi_is_user_in_group(user_id, group_id));
  }

  // delete record
  {
    rc = s_dbmgr_user_group_info_record_delete(ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_FALSE(dbmgr_ugi_is_user_in_group(user_id, group_id));
  }

  // finalize
  s_dbmgr_user_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

/*
 * Positive test cases for public methods
 */

void test_dbmgr_ugi_add_user_to_group_delete_user_from_group(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  dbmgr_user_group_info_t actual_ugi_ptr = NULL;
  char *actual_user_id = NULL;
  char *actual_group_id = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create user_group
  {
    rc = dbmgr_ugi_add_user_to_group(user_id, group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // find user_group
  {
    rc = dbmgr_ugi_user_find(user_id, group_id, &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get user_id
  {
    rc = dbmgr_ugi_get_user_id(actual_ugi_ptr, &actual_user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(user_id, actual_user_id);

    free((void *) actual_user_id);
    actual_user_id = NULL;
  }

  // get group_id
  {
    rc = dbmgr_ugi_get_group_id(actual_ugi_ptr, &actual_group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(group_id, actual_group_id);

    free((void *) actual_group_id);
    actual_group_id = NULL;
  }

  // delete user_group
  {
    rc = dbmgr_ugi_delete_user_from_group(user_id, group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_user_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_ugi_user_find(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  dbmgr_user_group_info_t actual_ugi_ptr = NULL;
  char *actual_user_id = NULL;
  char *actual_group_id = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create user_group
  {
    rc = dbmgr_ugi_add_user_to_group(user_id, group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // user_group find
  {
    rc = dbmgr_ugi_user_find(user_id, group_id, &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get user_id
  {
    rc = dbmgr_ugi_get_user_id(actual_ugi_ptr, &actual_user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(user_id, actual_user_id);

    free((void *) actual_user_id);
    actual_user_id = NULL;
  }

  // get group_id
  {
    rc = dbmgr_ugi_get_group_id(actual_ugi_ptr, &actual_group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(group_id, actual_group_id);

    free((void *) actual_group_id);
    actual_group_id = NULL;
  }

  // delete user_group
  {
    rc = dbmgr_ugi_delete_user_from_group(user_id, group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_user_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_ugi_is_user_in_group(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create user_group
  {
    rc = dbmgr_ugi_add_user_to_group(user_id, group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // user_group exists
  {
    TEST_ASSERT_TRUE(dbmgr_ugi_is_user_in_group(user_id, group_id));
    TEST_ASSERT_FALSE(dbmgr_ugi_is_user_in_group("non-existent-user", group_id));
    TEST_ASSERT_FALSE(dbmgr_ugi_is_user_in_group(user_id, "non-existent-group"));
  }

  // delete user_group
  {
    rc = dbmgr_ugi_delete_user_from_group(user_id, group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_user_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_ugi_get_user_id(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const size_t user_id_len = strlen(user_id);
  const char *group_id = "test-group";
  dbmgr_user_group_info_t actual_ugi_ptr = NULL;
  char *actual_user_id = NULL;
  size_t actual_user_id_len = 0;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create user_group
  {
    rc = dbmgr_ugi_add_user_to_group(user_id, group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // user_group find
  {
    rc = dbmgr_ugi_user_find(user_id, group_id, &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get user_id
  {
    rc = dbmgr_ugi_get_user_id(actual_ugi_ptr, &actual_user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(user_id, actual_user_id);

    rc = dbmgr_ugi_get_user_id_len(actual_ugi_ptr, &actual_user_id_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(user_id_len, actual_user_id_len);

    free((void *) actual_user_id);
    actual_user_id = NULL;
  }

  // delete user_group
  {
    rc = dbmgr_ugi_delete_user_from_group(user_id, group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_user_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_ugi_get_group_id(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const size_t group_id_len = strlen(group_id);
  dbmgr_user_group_info_t actual_ugi_ptr = NULL;
  char *actual_group_id = NULL;
  size_t actual_group_id_len = 0;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create user_group
  {
    rc = dbmgr_ugi_add_user_to_group(user_id, group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // user_group find
  {
    rc = dbmgr_ugi_user_find(user_id, group_id, &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get group_id
  {
    rc = dbmgr_ugi_get_group_id(actual_ugi_ptr, &actual_group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(group_id, actual_group_id);

    rc = dbmgr_ugi_get_group_id_len(actual_ugi_ptr, &actual_group_id_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(group_id_len, actual_group_id_len);

    free((void *) actual_group_id);
    actual_group_id = NULL;
  }

  // delete user_group
  {
    rc = dbmgr_ugi_delete_user_from_group(user_id, group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_user_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_ugi_set_get_user_group_status(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  dbmgr_user_group_info_t actual_ugi_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create user_group
  {
    rc = dbmgr_ugi_add_user_to_group(user_id, group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // user_group find
  {
    rc = dbmgr_ugi_user_find(user_id, group_id, &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // set user_group disabled
  {
    rc = dbmgr_ugi_set_user_group_disabled(actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_FALSE(dbmgr_ugi_is_user_group_enabled(actual_ugi_ptr));
  }

  // set user_group enabled
  {
    rc = dbmgr_ugi_set_user_group_enabled(actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_TRUE(dbmgr_ugi_is_user_group_enabled(actual_ugi_ptr));
  }

  // delete user_group
  {
    rc = dbmgr_ugi_delete_user_from_group(user_id, group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_user_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_ugi_get_created_time(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  dbmgr_user_group_info_t actual_ugi_ptr = NULL;
  sqc_chrono_t actual_created_time;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create user_group
  {
    rc = dbmgr_ugi_add_user_to_group(user_id, group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // user_group find
  {
    rc = dbmgr_ugi_user_find(user_id, group_id, &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get created time
  {
    rc = dbmgr_ugi_get_created_time(actual_ugi_ptr, &actual_created_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_LESS_OR_EQUAL_UINT64(sqc_chrono_now(), actual_created_time);
  }

  // delete user_group
  {
    rc = dbmgr_ugi_delete_user_from_group(user_id, group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_user_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_ugi_get_update_time(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  dbmgr_user_group_info_t actual_ugi_ptr = NULL;
  sqc_chrono_t actual_update_time;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create user_group
  {
    rc = dbmgr_ugi_add_user_to_group(user_id, group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // user_group find
  {
    rc = dbmgr_ugi_user_find(user_id, group_id, &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get update time
  {
    rc = dbmgr_ugi_get_update_time(actual_ugi_ptr, &actual_update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_LESS_OR_EQUAL_UINT64(sqc_chrono_now(), actual_update_time);
  }

  // delete user_group
  {
    rc = dbmgr_ugi_delete_user_from_group(user_id, group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_user_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

/*
 * Negative test cases for private methods
 */

void test_negative_s_dbmgr_user_group_info_record_create(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *excess_user_id = "01234567890123456789012345678901234567890123456789" \
                               "01234567890123456789012345678901234567890123456789" \
                               "01234567890123456789012345678901234567890123456789" \
                               "01234567890123456789012345678901234567890123456789" \
                               "01234567890123456789012345678901234567890123456789" \
                               "0123456";
  const char *group_id = "test-group";
  const char *excess_group_id = "01234567890123456789012345678901234567890123456789" \
                                "01234567890123456789012345678901234567890123456789" \
                                "01234567890123456789012345678901234567890123456789" \
                                "01234567890123456789012345678901234567890123456789" \
                                "01234567890123456789012345678901234567890123456789" \
                                "0123456";
  dbmgr_user_group_info_t actual_ugi_ptr = NULL;

  // invalid user_id
  {
    rc = s_dbmgr_user_group_info_record_create(excess_user_id, group_id, &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_TOO_LONG, rc);
    TEST_ASSERT_NULL(actual_ugi_ptr);

    rc = s_dbmgr_user_group_info_record_create("", group_id, &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(actual_ugi_ptr);

    rc = s_dbmgr_user_group_info_record_create(NULL, group_id, &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(actual_ugi_ptr);
  }

  // invalid group_id
  {
    rc = s_dbmgr_user_group_info_record_create(user_id, excess_group_id, &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_TOO_LONG, rc);
    TEST_ASSERT_NULL(actual_ugi_ptr);

    rc = s_dbmgr_user_group_info_record_create(user_id, "", &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(actual_ugi_ptr);

    rc = s_dbmgr_user_group_info_record_create(user_id, NULL, &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(actual_ugi_ptr);
  }

  // invalid record
  {
    rc = s_dbmgr_user_group_info_record_create(user_id, group_id, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }
}

void test_negative_s_dbmgr_user_group_info_record_destroy(void) {
  s_dbmgr_user_group_info_record_destroy(NULL);
}

void test_negative_s_dbmgr_user_group_info_record_add(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  dbmgr_user_group_info_t ugi_ptr = NULL;
  dbmgr_user_group_info_t actual_ugi_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_user_group_info_record_create(user_id, group_id, &ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // not started
  {
    rc = s_dbmgr_user_group_info_record_add(actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_STARTED, rc);
    TEST_ASSERT_NULL(actual_ugi_ptr);
  }

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = s_dbmgr_user_group_info_record_add(NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_user_group_info_record_destroy(ugi_ptr);
    ugi_ptr = NULL;
  }

  // finalize
  s_dbmgr_user_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_negative_s_dbmgr_user_group_info_record_delete(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  dbmgr_user_group_info_t ugi_ptr = NULL;
  dbmgr_user_group_info_t actual_ugi_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_user_group_info_record_create(user_id, group_id, &ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // not started
  {
    rc = s_dbmgr_user_group_info_record_delete(actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_STARTED, rc);
    TEST_ASSERT_NULL(actual_ugi_ptr);
  }

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = s_dbmgr_user_group_info_record_delete(NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_user_group_info_record_destroy(ugi_ptr);
    ugi_ptr = NULL;
  }

  // finalize
  s_dbmgr_user_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

/*
 * Negative test cases for public methods
 */

void test_negative_dbmgr_ugi_add_user_to_group(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";

  // not started
  {
    rc = dbmgr_ugi_add_user_to_group(user_id, group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_STARTED, rc);
  }

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid user_id
  {
    rc = dbmgr_ugi_add_user_to_group("", group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);

    rc = dbmgr_ugi_add_user_to_group(NULL, group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid group_id
  {
    rc = dbmgr_ugi_add_user_to_group(user_id, "");
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);

    rc = dbmgr_ugi_add_user_to_group(user_id, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // finalize
  s_dbmgr_user_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_negative_dbmgr_ugi_delete_user_from_group(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";

  // not started
  {
    rc = dbmgr_ugi_delete_user_from_group(user_id, group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_STARTED, rc);
  }

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid user_id
  {
    rc = dbmgr_ugi_delete_user_from_group("", group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);

    rc = dbmgr_ugi_delete_user_from_group(NULL, group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid group_id
  {
    rc = dbmgr_ugi_delete_user_from_group(user_id, "");
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);

    rc = dbmgr_ugi_delete_user_from_group(user_id, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // finalize
  s_dbmgr_user_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_negative_dbmgr_ugi_user_find(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  dbmgr_user_group_info_t ugi_ptr = NULL;
  dbmgr_user_group_info_t actual_ugi_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_user_group_info_record_create(user_id, group_id, &ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // not started
  {
    rc = dbmgr_ugi_user_find(user_id, group_id, &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_STARTED, rc);
    TEST_ASSERT_NULL(actual_ugi_ptr);
  }

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // non-existent user_id
  {
    rc = dbmgr_ugi_user_find("non-existent-user", group_id, &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_FOUND, rc);
    TEST_ASSERT_NULL(actual_ugi_ptr);
  }

  // invalid user_id
  {
    rc = dbmgr_ugi_user_find("", group_id, &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(actual_ugi_ptr);

    rc = dbmgr_ugi_user_find(NULL, group_id, &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(actual_ugi_ptr);
  }

  // non-existent group_id
  {
    rc = dbmgr_ugi_user_find(user_id, "non-existent-group", &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_FOUND, rc);
    TEST_ASSERT_NULL(actual_ugi_ptr);
  }

  // invalid group_id
  {
    rc = dbmgr_ugi_user_find(user_id, "", &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(actual_ugi_ptr);

    rc = dbmgr_ugi_user_find(user_id, NULL, &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(actual_ugi_ptr);
  }

  // invalid record
  {
    rc = dbmgr_ugi_user_find(user_id, group_id, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_user_group_info_record_destroy(ugi_ptr);
    ugi_ptr = NULL;
  }

  // finalize
  s_dbmgr_user_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_negative_dbmgr_ugi_is_user_in_group(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";

  // not started
  {
    TEST_ASSERT_FALSE(dbmgr_ugi_is_user_in_group(user_id, group_id));
  }

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid user_id
  {
    TEST_ASSERT_FALSE(dbmgr_ugi_is_user_in_group("", group_id));

    TEST_ASSERT_FALSE(dbmgr_ugi_is_user_in_group(NULL, group_id));
  }

  // invalid group_id
  {
    TEST_ASSERT_FALSE(dbmgr_ugi_is_user_in_group(user_id, ""));

    TEST_ASSERT_FALSE(dbmgr_ugi_is_user_in_group(user_id, NULL));
  }

  // finalize
  s_dbmgr_user_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_negative_dbmgr_ugi_get_user_id(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  dbmgr_user_group_info_t ugi_ptr = NULL;
  char *actual_user_id = NULL;

  // create record
  {
    rc = s_dbmgr_user_group_info_record_create(user_id, group_id, &ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ugi_get_user_id(NULL, &actual_user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid user_id
  {
    rc = dbmgr_ugi_get_user_id(ugi_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_user_group_info_record_destroy(ugi_ptr);
    ugi_ptr = NULL;
  }
}

void test_negative_dbmgr_ugi_get_user_id_len(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  dbmgr_user_group_info_t ugi_ptr = NULL;
  size_t actual_user_id_len = 0;

  // create record
  {
    rc = s_dbmgr_user_group_info_record_create(user_id, group_id, &ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ugi_get_user_id_len(NULL, &actual_user_id_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid user_id
  {
    rc = dbmgr_ugi_get_user_id_len(ugi_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_user_group_info_record_destroy(ugi_ptr);
    ugi_ptr = NULL;
  }
}

void test_negative_dbmgr_ugi_get_group_id(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  dbmgr_user_group_info_t ugi_ptr = NULL;
  char *actual_group_id = NULL;

  // create record
  {
    rc = s_dbmgr_user_group_info_record_create(user_id, group_id, &ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ugi_get_group_id(NULL, &actual_group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid group_id
  {
    rc = dbmgr_ugi_get_group_id(ugi_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_user_group_info_record_destroy(ugi_ptr);
    ugi_ptr = NULL;
  }
}

void test_negative_dbmgr_ugi_get_group_id_len(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  dbmgr_user_group_info_t ugi_ptr = NULL;
  size_t actual_group_id_len = 0;

  // create record
  {
    rc = s_dbmgr_user_group_info_record_create(user_id, group_id, &ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ugi_get_group_id_len(NULL, &actual_group_id_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid group_id
  {
    rc = dbmgr_ugi_get_group_id_len(ugi_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_user_group_info_record_destroy(ugi_ptr);
    ugi_ptr = NULL;
  }
}

void test_negative_dbmgr_ugi_set_user_group_enabled(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  dbmgr_user_group_info_t ugi_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_user_group_info_record_create(user_id, group_id, &ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ugi_set_user_group_enabled(NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_user_group_info_record_destroy(ugi_ptr);
    ugi_ptr = NULL;
  }
}

void test_negative_dbmgr_ugi_set_user_group_disabled(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  dbmgr_user_group_info_t ugi_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_user_group_info_record_create(user_id, group_id, &ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ugi_set_user_group_disabled(NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_user_group_info_record_destroy(ugi_ptr);
    ugi_ptr = NULL;
  }
}

void test_negative_dbmgr_ugi_is_user_group_enabled(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  dbmgr_user_group_info_t ugi_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_user_group_info_record_create(user_id, group_id, &ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    TEST_ASSERT_FALSE(dbmgr_ugi_is_user_group_enabled(NULL));
  }

  // destroy record
  {
    s_dbmgr_user_group_info_record_destroy(ugi_ptr);
    ugi_ptr = NULL;
  }
}

void test_negative_dbmgr_ugi_get_created_time(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  dbmgr_user_group_info_t ugi_ptr = NULL;
  sqc_chrono_t actual_created_time;

  // create record
  {
    rc = s_dbmgr_user_group_info_record_create(user_id, group_id, &ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ugi_get_created_time(NULL, &actual_created_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid created_time
  {
    rc = dbmgr_ugi_get_created_time(ugi_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_user_group_info_record_destroy(ugi_ptr);
    ugi_ptr = NULL;
  }
}

void test_negative_dbmgr_ugi_get_update_time(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  dbmgr_user_group_info_t ugi_ptr = NULL;
  sqc_chrono_t actual_update_time;

  // create record
  {
    rc = s_dbmgr_user_group_info_record_create(user_id, group_id, &ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ugi_get_update_time(NULL, &actual_update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid update_time
  {
    rc = dbmgr_ugi_get_update_time(ugi_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_user_group_info_record_destroy(ugi_ptr);
    ugi_ptr = NULL;
  }
}

