#include <sys/stat.h>

#include "unity.h"

#include "dbmgr_util.c"
#include "dbmgr_db.c"
#include "dbmgr_user_info.c"

#include "dbmgr_db_common.c"

void setUp(void) {
  remove_db_files("setUp - user_info");
  create_db_files("setUp - user_info");
}

void tearDown(void) {
  remove_db_files("tearDown - user_info");
}

/*
 * Positive test cases for private methods
 */

void test_s_dbmgr_user_info_initialize_finalize(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(user_info_hashmap);
  }

  // finalize
  {
    s_dbmgr_user_info_finalize();
    TEST_ASSERT_NULL(user_info_hashmap);

    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);
  }
}

void test_s_dbmgr_user_info_record_freeup(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create & add record(free in finalize)
  {
    rc = s_dbmgr_user_info_record_create(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_user_info_record_add(ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_TRUE(dbmgr_ui_user_exists(user_id));
  }

  // finalize
  s_dbmgr_user_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_s_dbmgr_user_info_record_create_destroy(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const size_t user_id_len = strlen(user_id);
  dbmgr_user_info_t ui_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_user_info_record_create(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(user_id, ui_ptr->user_id);
    TEST_ASSERT_EQUAL(user_id_len, ui_ptr->user_id_len);
  }

  // destroy record
  {
    s_dbmgr_user_info_record_destroy(ui_ptr);
    ui_ptr = NULL;
  }
}

void test_s_dbmgr_user_info_record_add_delete(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create record
  {
    rc = s_dbmgr_user_info_record_create(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // add record
  {
    rc = s_dbmgr_user_info_record_add(ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_TRUE(dbmgr_ui_user_exists(user_id));
  }

  // delete record
  {
    rc = s_dbmgr_user_info_record_delete(ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_FALSE(dbmgr_ui_user_exists(user_id));
  }

  // finalize
  s_dbmgr_user_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

/*
 * Positive test cases for public methods
 */

void test_dbmgr_ui_create_delete_user(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;
  char *actual_user_id = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create user
  {
    rc = dbmgr_ui_create_user(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get user_id
  {
    rc = dbmgr_ui_get_user_id(ui_ptr, &actual_user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(user_id, actual_user_id);

    free((void *) actual_user_id);
    actual_user_id = NULL;
  }

  // delete user
  {
    rc = dbmgr_ui_delete_user(user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_user_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_ui_user_exists(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create user
  {
    rc = dbmgr_ui_create_user(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // user exists
  {
    TEST_ASSERT_TRUE(dbmgr_ui_user_exists(user_id));
    TEST_ASSERT_FALSE(dbmgr_ui_user_exists("non-existent-user"));
  }

  // delete user
  {
    rc = dbmgr_ui_delete_user(user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_user_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_ui_user_find(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;
  char *actual_user_id = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create user
  {
    rc = dbmgr_ui_create_user(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // user find
  {
    rc = dbmgr_ui_user_find(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get user_id
  {
    rc = dbmgr_ui_get_user_id(ui_ptr, &actual_user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(user_id, actual_user_id);

    free((void *) actual_user_id);
    actual_user_id = NULL;
  }

  // delete user
  {
    rc = dbmgr_ui_delete_user(user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_user_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_ui_set_get_user_id(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const size_t user_id_len = strlen(user_id);
  const char *new_user_id = "new-test-user";
  const size_t new_user_id_len = strlen(new_user_id);
  dbmgr_user_info_t ui_ptr = NULL;
  char *actual_user_id = NULL;
  char *actual_new_user_id = NULL;
  size_t actual_user_id_len = 0;
  size_t actual_new_user_id_len = 0;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create user
  {
    rc = dbmgr_ui_create_user(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get user_id
  {
    rc = dbmgr_ui_get_user_id(ui_ptr, &actual_user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(user_id, actual_user_id);

    rc = dbmgr_ui_get_user_id_len(ui_ptr, &actual_user_id_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(user_id_len, actual_user_id_len);

    free((void *) actual_user_id);
    actual_user_id = NULL;
  }

  // set user_id
  {
    rc = dbmgr_ui_set_user_id(ui_ptr, new_user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_ui_get_user_id(ui_ptr, &actual_new_user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(new_user_id, actual_new_user_id);

    rc = dbmgr_ui_get_user_id_len(ui_ptr, &actual_new_user_id_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(new_user_id_len, actual_new_user_id_len);

    free((void *) actual_new_user_id);
    actual_new_user_id = NULL;
  }

  // delete user
  {
    rc = dbmgr_ui_delete_user(user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_user_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_ui_set_get_user_role_type(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const sqc_rpc_sched_user_role_type_t new_role_type = SQC_RPC_SCHED_USER_ROLE_TYPE_ADMIN;
  dbmgr_user_info_t ui_ptr = NULL;
  sqc_rpc_sched_user_role_type_t actual_role_type = SQC_RPC_SCHED_USER_ROLE_TYPE_UNKNOWN;
  sqc_rpc_sched_user_role_type_t actual_new_role_type = SQC_RPC_SCHED_USER_ROLE_TYPE_UNKNOWN;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create user
  {
    rc = dbmgr_ui_create_user(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get role_type
  {
    rc = dbmgr_ui_get_user_role_type(ui_ptr, &actual_role_type);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(SQC_RPC_SCHED_USER_ROLE_TYPE_GENERAL, actual_role_type);
  }

  // set role_type
  {
    rc = dbmgr_ui_set_user_role_type(ui_ptr, new_role_type);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_ui_get_user_role_type(ui_ptr, &actual_new_role_type);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(new_role_type, actual_new_role_type);
  }

  // delete user
  {
    rc = dbmgr_ui_delete_user(user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_user_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_ui_is_user_admin(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create user
  {
    rc = dbmgr_ui_create_user(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // is user admin
  {
    TEST_ASSERT_FALSE(dbmgr_ui_is_user_admin(ui_ptr));
  }

  // delete user
  {
    rc = dbmgr_ui_delete_user(user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_user_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_ui_set_user_enabled_disabled(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create user
  {
    rc = dbmgr_ui_create_user(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // is user enabled
  {
    TEST_ASSERT_TRUE(dbmgr_ui_is_user_enabled(ui_ptr));
  }

  // set user disabled
  {
    rc = dbmgr_ui_set_user_disabled(ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_FALSE(dbmgr_ui_is_user_enabled(ui_ptr));
  }

  // set user enabled
  {
    rc = dbmgr_ui_set_user_enabled(ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_TRUE(dbmgr_ui_is_user_enabled(ui_ptr));
  }

  // delete user
  {
    rc = dbmgr_ui_delete_user(user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_user_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_ui_get_created_time(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;
  sqc_chrono_t actual_created_time;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create user
  {
    rc = dbmgr_ui_create_user(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get created time
  {
    rc = dbmgr_ui_get_created_time(ui_ptr, &actual_created_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_LESS_OR_EQUAL_UINT64(sqc_chrono_now(), actual_created_time);
  }

  // delete user
  {
    rc = dbmgr_ui_delete_user(user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_user_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_ui_get_update_time(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;
  sqc_chrono_t actual_update_time;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create user
  {
    rc = dbmgr_ui_create_user(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get created time
  {
    rc = dbmgr_ui_get_update_time(ui_ptr, &actual_update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_LESS_OR_EQUAL_UINT64(sqc_chrono_now(), actual_update_time);
  }

  // delete user
  {
    rc = dbmgr_ui_delete_user(user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_user_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}


/*
 * Negative test cases for private methods
 */

void test_negative_s_dbmgr_user_info_record_create(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *excess_user_id = "01234567890123456789012345678901234567890123456789" \
                               "01234567890123456789012345678901234567890123456789" \
                               "01234567890123456789012345678901234567890123456789" \
                               "01234567890123456789012345678901234567890123456789" \
                               "01234567890123456789012345678901234567890123456789" \
                               "0123456";
  dbmgr_user_info_t ui_ptr = NULL;

  // invalid user_id
  {
    rc = s_dbmgr_user_info_record_create(excess_user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_TOO_LONG, rc);
    TEST_ASSERT_NULL(ui_ptr);

    rc = s_dbmgr_user_info_record_create("", &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(ui_ptr);

    rc = s_dbmgr_user_info_record_create(NULL, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(ui_ptr);
  }

  // invalid record
  {
    rc = s_dbmgr_user_info_record_create(user_id, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }
}

void test_negative_s_dbmgr_user_info_record_destroy(void) {
  s_dbmgr_user_info_record_destroy(NULL);
}

void test_negative_s_dbmgr_user_info_record_add(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_user_info_record_create(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // not started
  {
    rc = s_dbmgr_user_info_record_add(ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_STARTED, rc);
  }

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = s_dbmgr_user_info_record_add(NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_user_info_record_destroy(ui_ptr);
    ui_ptr = NULL;
  }

  // finalize
  s_dbmgr_user_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_negative_s_dbmgr_user_info_record_delete(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_user_info_record_create(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // not started
  {
    rc = s_dbmgr_user_info_record_delete(ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_STARTED, rc);
  }

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = s_dbmgr_user_info_record_delete(NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_user_info_record_destroy(ui_ptr);
    ui_ptr = NULL;
  }

  // finalize
  s_dbmgr_user_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

/*
 * Negative test cases for public methods
 */

void test_negative_dbmgr_ui_create_user(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;

  // invalid user_id
  {
    rc = dbmgr_ui_create_user("", &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(ui_ptr);

    rc = dbmgr_ui_create_user(NULL, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(ui_ptr);
  }

  // invalid record
  {
    rc = dbmgr_ui_create_user(user_id, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }
}

void test_negative_dbmgr_ui_delete_user(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";

  // not started
  {
    rc = dbmgr_ui_delete_user(user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_STARTED, rc);
  }

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid user_id
  {
    rc = dbmgr_ui_delete_user("");
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);

    rc = dbmgr_ui_delete_user(NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // finalize
  s_dbmgr_user_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_negative_dbmgr_ui_user_exists(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";

  // not started
  {
    TEST_ASSERT_FALSE(dbmgr_ui_user_exists(user_id));
  }

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid user_id
  {
    TEST_ASSERT_FALSE(dbmgr_ui_user_exists(""));

    TEST_ASSERT_FALSE(dbmgr_ui_user_exists(NULL));
  }

  // finalize
  s_dbmgr_user_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_negative_dbmgr_ui_user_find(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;
  dbmgr_user_info_t actual_ui_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_user_info_record_create(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // not started
  {
    rc = s_dbmgr_user_info_record_find(user_id, &actual_ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_STARTED, rc);
    TEST_ASSERT_NULL(actual_ui_ptr);
  }

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_user_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // non-existent user_id
  {
    rc = dbmgr_ui_user_find("non-existent-user", &actual_ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_FOUND, rc);
    TEST_ASSERT_NULL(actual_ui_ptr);
  }

  // invalid user_id
  {
    rc = dbmgr_ui_user_find("", &actual_ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(actual_ui_ptr);

    rc = dbmgr_ui_user_find(NULL, &actual_ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(actual_ui_ptr);
  }

  // invalid record
  {
    rc = dbmgr_ui_user_find(user_id, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_user_info_record_destroy(ui_ptr);
    ui_ptr = NULL;
  }

  // finalize
  s_dbmgr_user_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_negative_dbmgr_ui_set_user_id(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *new_user_id = "new-test-user";
  dbmgr_user_info_t ui_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_user_info_record_create(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ui_set_user_id(NULL, new_user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid user_id
  {
    rc = dbmgr_ui_set_user_id(ui_ptr, "");
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_EQUAL_STRING(user_id, ui_ptr->user_id);

    rc = dbmgr_ui_set_user_id(ui_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_EQUAL_STRING(user_id, ui_ptr->user_id);
  }

  // destroy record
  {
    s_dbmgr_user_info_record_destroy(ui_ptr);
    ui_ptr= NULL;
  }
}

void test_negative_dbmgr_ui_get_user_id(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;
  char *actual_user_id = NULL;

  // create record
  {
    rc = s_dbmgr_user_info_record_create(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ui_get_user_id(NULL, &actual_user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid user_id
  {
    rc = dbmgr_ui_get_user_id(ui_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_user_info_record_destroy(ui_ptr);
    ui_ptr = NULL;
  }
}

void test_negative_dbmgr_ui_get_user_id_len(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;
  size_t actual_user_id_len = 0;

  // create record
  {
    rc = s_dbmgr_user_info_record_create(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ui_get_user_id_len(NULL, &actual_user_id_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid user_id
  {
    rc = dbmgr_ui_get_user_id_len(ui_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_user_info_record_destroy(ui_ptr);
    ui_ptr = NULL;
  }
}

void test_negative_dbmgr_ui_set_user_role_type(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;
  sqc_rpc_sched_user_role_type_t role_type = SQC_RPC_SCHED_USER_ROLE_TYPE_ADMIN;

  // create record
  {
    rc = s_dbmgr_user_info_record_create(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ui_set_user_role_type(NULL, role_type);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_user_info_record_destroy(ui_ptr);
    ui_ptr = NULL;
  }
}

void test_negative_dbmgr_ui_get_user_role_type(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;
  sqc_rpc_sched_user_role_type_t actual_role_type = SQC_RPC_SCHED_USER_ROLE_TYPE_UNKNOWN;

  // create record
  {
    rc = s_dbmgr_user_info_record_create(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ui_get_user_role_type(NULL, &actual_role_type);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid role_type
  {
    rc = dbmgr_ui_get_user_role_type(ui_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_user_info_record_destroy(ui_ptr);
    ui_ptr = NULL;
  }
}

void test_negative_dbmgr_ui_is_user_admin(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_user_info_record_create(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    TEST_ASSERT_FALSE(dbmgr_ui_is_user_admin(NULL));
  }

  // destroy record
  {
    s_dbmgr_user_info_record_destroy(ui_ptr);
    ui_ptr = NULL;
  }
}

void test_negative_dbmgr_ui_set_user_enabled(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_user_info_record_create(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ui_set_user_enabled(NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_user_info_record_destroy(ui_ptr);
    ui_ptr = NULL;
  }
}

void test_negative_dbmgr_ui_set_user_disabled(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_user_info_record_create(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ui_set_user_disabled(NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_user_info_record_destroy(ui_ptr);
    ui_ptr = NULL;
  }
}

void test_negative_dbmgr_ui_is_user_enabled(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_user_info_record_create(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    TEST_ASSERT_FALSE(dbmgr_ui_is_user_enabled(NULL));
  }

  // destroy record
  {
    s_dbmgr_user_info_record_destroy(ui_ptr);
    ui_ptr = NULL;
  }
}

void test_negative_dbmgr_ui_get_created_time(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;
  sqc_chrono_t actual_created_time;

  // create record
  {
    rc = s_dbmgr_user_info_record_create(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ui_get_created_time(NULL, &actual_created_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid created_time
  {
    rc = dbmgr_ui_get_created_time(ui_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_user_info_record_destroy(ui_ptr);
    ui_ptr = NULL;
  }
}

void test_negative_dbmgr_ui_get_update_time(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  dbmgr_user_info_t ui_ptr = NULL;
  sqc_chrono_t actual_update_time;

  // create record
  {
    rc = s_dbmgr_user_info_record_create(user_id, &ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ui_get_update_time(NULL, &actual_update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid update_time
  {
    rc = dbmgr_ui_get_update_time(ui_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_user_info_record_destroy(ui_ptr);
    ui_ptr = NULL;
  }
}

