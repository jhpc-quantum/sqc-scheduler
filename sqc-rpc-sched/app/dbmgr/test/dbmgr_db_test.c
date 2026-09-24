#include <sys/stat.h>

#include "unity.h"

#include "dbmgr_util.c"
#include "dbmgr_db.c"

#include "dbmgr_db_common.c"

void setUp(void) {
  remove_db_files("setUp - db");
  create_db_files("setUp - db");
}

void tearDown(void) {
  remove_db_files("tearDown - db");
}

void test_dbmgr_db_initialize_finalize(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);
  }
}

void test_dbmgr_db_user_info_insert_record(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const size_t user_id_len = strlen(user_id);
  const sqc_rpc_sched_user_role_type_t role_type = SQC_RPC_SCHED_USER_ROLE_TYPE_GENERAL;
  const sqc_rpc_sched_user_status_t status = SQC_RPC_SCHED_USER_STATUS_ENABLED;
  const int64_t created_time = 1736474012;
  const int64_t update_time = created_time;
  int actual_count = 0;
  dbmgr_user_info_t actual_ui_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_user_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_user_info_insert_record(user_id, role_type, status,
                                            created_time, update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_user_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);

    rc = s_dbmgr_db_user_info_select(user_id, &actual_ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(user_id, actual_ui_ptr->user_id);
    TEST_ASSERT_EQUAL(user_id_len, actual_ui_ptr->user_id_len);
    TEST_ASSERT_EQUAL(role_type, actual_ui_ptr->role_type);
    TEST_ASSERT_EQUAL(status, actual_ui_ptr->status);
    TEST_ASSERT_EQUAL(created_time, actual_ui_ptr->created_time);
    TEST_ASSERT_EQUAL(update_time, actual_ui_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_user_info_destroy(actual_ui_ptr);
    actual_ui_ptr = NULL;
  }
}

void test_dbmgr_db_user_info_select_all(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const size_t user_id_len = strlen(user_id);
  const sqc_rpc_sched_user_role_type_t role_type = SQC_RPC_SCHED_USER_ROLE_TYPE_GENERAL;
  const sqc_rpc_sched_user_status_t status = SQC_RPC_SCHED_USER_STATUS_ENABLED;
  const int64_t created_time = 1736474012;
  const int64_t update_time = created_time;
  int actual_count = 0;
  dbmgr_user_info_t *actual_ui_arr_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_user_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_user_info_insert_record(user_id, role_type, status,
                                            created_time, update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_user_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);

    rc = s_dbmgr_db_user_info_select_all(&actual_ui_arr_ptr, &actual_count, 1);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);
    TEST_ASSERT_EQUAL_STRING(user_id, actual_ui_arr_ptr[0]->user_id);
    TEST_ASSERT_EQUAL(user_id_len, actual_ui_arr_ptr[0]->user_id_len);
    TEST_ASSERT_EQUAL(role_type, actual_ui_arr_ptr[0]->role_type);
    TEST_ASSERT_EQUAL(status, actual_ui_arr_ptr[0]->status);
    TEST_ASSERT_EQUAL(created_time, actual_ui_arr_ptr[0]->created_time);
    TEST_ASSERT_EQUAL(update_time, actual_ui_arr_ptr[0]->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_user_info_destroy(actual_ui_arr_ptr[0]);
    actual_ui_arr_ptr[0] = NULL;

    free(actual_ui_arr_ptr);
    actual_ui_arr_ptr = NULL;
  }
}

void test_dbmgr_db_user_info_deserialize_all(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const size_t user_id_len = strlen(user_id);
  const sqc_rpc_sched_user_role_type_t role_type = SQC_RPC_SCHED_USER_ROLE_TYPE_GENERAL;
  const sqc_rpc_sched_user_status_t status = SQC_RPC_SCHED_USER_STATUS_ENABLED;
  const int64_t created_time = 1736474012;
  const int64_t update_time = created_time;
  sqc_hashmap_t actual_user_info_hashmap = NULL;
  int actual_count = 0;
  dbmgr_user_info_t actual_ui_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_user_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);

    rc = sqc_hashmap_create(&actual_user_info_hashmap, SQC_HASHMAP_TYPE_STRING, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create record
  {
    rc = s_dbmgr_db_user_info_insert_record(user_id, role_type, status,
                                            created_time, update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_user_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);

    rc = s_dbmgr_db_user_info_deserialize_all(&actual_user_info_hashmap);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = sqc_hashmap_find(&actual_user_info_hashmap, (void *) user_id, (void **) &actual_ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(user_id, actual_ui_ptr->user_id);
    TEST_ASSERT_EQUAL(user_id_len, actual_ui_ptr->user_id_len);
    TEST_ASSERT_EQUAL(role_type, actual_ui_ptr->role_type);
    TEST_ASSERT_EQUAL(status, actual_ui_ptr->status);
    TEST_ASSERT_EQUAL(created_time, actual_ui_ptr->created_time);
    TEST_ASSERT_EQUAL(update_time, actual_ui_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    sqc_hashmap_destroy(&actual_user_info_hashmap, true);
    actual_user_info_hashmap = NULL;

    s_dbmgr_user_info_destroy(actual_ui_ptr);
    actual_ui_ptr = NULL;
  }
}

void test_dbmgr_db_user_info_create_destroy(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const size_t user_id_len = strlen(user_id);
  const sqc_rpc_sched_user_role_type_t role_type = SQC_RPC_SCHED_USER_ROLE_TYPE_GENERAL;
  const sqc_rpc_sched_user_status_t status = SQC_RPC_SCHED_USER_STATUS_ENABLED;
  const int64_t created_time = 1736474012;
  const int64_t update_time = created_time;
  dbmgr_user_info_t actual_ui_ptr = NULL;

  // create user_info
  {
    rc = s_dbmgr_user_info_create_from_db(user_id, role_type, status,
                                          created_time, update_time, &actual_ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(user_id, actual_ui_ptr->user_id);
    TEST_ASSERT_EQUAL(user_id_len, actual_ui_ptr->user_id_len);
    TEST_ASSERT_EQUAL(role_type, actual_ui_ptr->role_type);
    TEST_ASSERT_EQUAL(status, actual_ui_ptr->status);
    TEST_ASSERT_EQUAL(created_time, actual_ui_ptr->created_time);
    TEST_ASSERT_EQUAL(update_time, actual_ui_ptr->update_time);
  }

  // destroy record
  {
    s_dbmgr_user_info_destroy(actual_ui_ptr);
    actual_ui_ptr = NULL;
  }
}

void test_dbmgr_db_user_info_update_role_type(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const sqc_rpc_sched_user_role_type_t role_type = SQC_RPC_SCHED_USER_ROLE_TYPE_GENERAL;
  const sqc_rpc_sched_user_status_t status = SQC_RPC_SCHED_USER_STATUS_ENABLED;
  const int64_t created_time = 1736474012;
  const int64_t update_time = 1736832113;
  const sqc_rpc_sched_user_role_type_t new_role_type = SQC_RPC_SCHED_USER_ROLE_TYPE_ADMIN;
  int actual_count = 0;
  dbmgr_user_info_t actual_ui_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_user_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create user_info
  {
    rc = s_dbmgr_db_user_info_insert_record(user_id, role_type, status,
                                            created_time,
                                            created_time); // set created_time
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_user_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);
  }

  // update record
  {
    rc = dbmgr_db_user_info_update_role_type(user_id, new_role_type, update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_user_info_select(user_id, &actual_ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(user_id, actual_ui_ptr->user_id);
    TEST_ASSERT_EQUAL(new_role_type, actual_ui_ptr->role_type);
    TEST_ASSERT_EQUAL(update_time, actual_ui_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_user_info_destroy(actual_ui_ptr);
    actual_ui_ptr = NULL;
  }
}

void test_dbmgr_db_user_info_update_status(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const sqc_rpc_sched_user_role_type_t role_type = SQC_RPC_SCHED_USER_ROLE_TYPE_GENERAL;
  const sqc_rpc_sched_user_status_t status = SQC_RPC_SCHED_USER_STATUS_ENABLED;
  const int64_t created_time = 1736474012;
  const int64_t update_time = 1736832113;
  const sqc_rpc_sched_user_status_t new_status = SQC_RPC_SCHED_USER_STATUS_DISABLED;
  int actual_count = 0;
  dbmgr_user_info_t actual_ui_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_user_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create user_info
  {
    rc = s_dbmgr_db_user_info_insert_record(user_id, role_type, status,
                                            created_time,
                                            created_time); // set created_time
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_user_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);
  }

  // update record
  {
    rc = dbmgr_db_user_info_update_status(user_id, new_status, update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_user_info_select(user_id, &actual_ui_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(user_id, actual_ui_ptr->user_id);
    TEST_ASSERT_EQUAL(new_status, actual_ui_ptr->status);
    TEST_ASSERT_EQUAL(update_time, actual_ui_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_user_info_destroy(actual_ui_ptr);
    actual_ui_ptr = NULL;
  }
}

void test_dbmgr_db_group_info_insert_record(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-group";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  const int64_t created_time = 1736474012;
  const int64_t update_time = created_time;
  int actual_count = 0;
  dbmgr_group_info_t actual_gi_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_group_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_group_info_insert_record(group_id,
                                             exec_time_limit_msec,
                                             exec_time_total_msec,
                                             created_time, update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_group_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);

    rc = s_dbmgr_db_group_info_select(group_id, &actual_gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(group_id, actual_gi_ptr->group_id);
    TEST_ASSERT_EQUAL(exec_time_limit_msec, actual_gi_ptr->exec_time_limit_msec);
    TEST_ASSERT_EQUAL(exec_time_total_msec, actual_gi_ptr->exec_time_total_msec);
    TEST_ASSERT_EQUAL(created_time, actual_gi_ptr->created_time);
    TEST_ASSERT_EQUAL(update_time, actual_gi_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_group_info_destroy(actual_gi_ptr);
    actual_gi_ptr = NULL;
  }
}

void test_dbmgr_db_group_info_select_all(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-group";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  const int64_t created_time = 1736474012;
  const int64_t update_time = created_time;
  int actual_count = 0;
  dbmgr_group_info_t *actual_gi_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_group_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_group_info_insert_record(group_id,
                                             exec_time_limit_msec,
                                             exec_time_total_msec,
                                             created_time, update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_group_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);

    rc = s_dbmgr_db_group_info_select_all(&actual_gi_ptr, &actual_count, 1);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(group_id, actual_gi_ptr[0]->group_id);
    TEST_ASSERT_EQUAL(exec_time_limit_msec, actual_gi_ptr[0]->exec_time_limit_msec);
    TEST_ASSERT_EQUAL(exec_time_total_msec, actual_gi_ptr[0]->exec_time_total_msec);
    TEST_ASSERT_EQUAL(created_time, actual_gi_ptr[0]->created_time);
    TEST_ASSERT_EQUAL(update_time, actual_gi_ptr[0]->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_group_info_destroy(actual_gi_ptr[0]);
    actual_gi_ptr[0] = NULL;

    free(actual_gi_ptr);
    actual_gi_ptr = NULL;
  }
}

void test_dbmgr_db_group_info_deserialize_all(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-group";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  const int64_t created_time = 1736474012;
  const int64_t update_time = created_time;
  sqc_hashmap_t actual_group_info_hashmap = NULL;
  int actual_count = 0;
  dbmgr_group_info_t actual_gi_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_group_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);

    rc = sqc_hashmap_create(&actual_group_info_hashmap, SQC_HASHMAP_TYPE_STRING, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create record
  {
    rc = s_dbmgr_db_group_info_insert_record(group_id,
                                             exec_time_limit_msec,
                                             exec_time_total_msec,
                                             created_time, update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_group_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);

    rc = s_dbmgr_db_group_info_deserialize_all(&actual_group_info_hashmap);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = sqc_hashmap_find(&actual_group_info_hashmap, (void *) group_id, (void **) &actual_gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(group_id, actual_gi_ptr->group_id);
    TEST_ASSERT_EQUAL(exec_time_limit_msec, actual_gi_ptr->exec_time_limit_msec);
    TEST_ASSERT_EQUAL(exec_time_total_msec, actual_gi_ptr->exec_time_total_msec);
    TEST_ASSERT_EQUAL(created_time, actual_gi_ptr->created_time);
    TEST_ASSERT_EQUAL(update_time, actual_gi_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    sqc_hashmap_destroy(&actual_group_info_hashmap, true);
    actual_group_info_hashmap = NULL;

    s_dbmgr_group_info_destroy(actual_gi_ptr);
    actual_gi_ptr = NULL;
  }
}

void test_dbmgr_db_group_info_create_destroy(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-group";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  const int64_t created_time = 1736474012;
  const int64_t update_time = created_time;
  dbmgr_group_info_t actual_gi_ptr = NULL;

  // create group_info
  {
    rc = s_dbmgr_group_info_create_from_db(group_id,
                                           exec_time_limit_msec,
                                           exec_time_total_msec,
                                           created_time, update_time, &actual_gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(group_id, actual_gi_ptr->group_id);
    TEST_ASSERT_EQUAL(exec_time_limit_msec, actual_gi_ptr->exec_time_limit_msec);
    TEST_ASSERT_EQUAL(exec_time_total_msec, actual_gi_ptr->exec_time_total_msec);
    TEST_ASSERT_EQUAL(created_time, actual_gi_ptr->created_time);
    TEST_ASSERT_EQUAL(update_time, actual_gi_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_group_info_destroy(actual_gi_ptr);
    actual_gi_ptr = NULL;
  }
}

void test_dbmgr_db_group_info_update_exec_time_limit_msec(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-group";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  const int64_t created_time = 1736474012;
  const int64_t update_time = 1736832113;
  const uint64_t new_exec_time_limit_msec = 567891234;
  int actual_count = 0;
  dbmgr_group_info_t actual_gi_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_group_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_group_info_insert_record(group_id,
                                             exec_time_limit_msec,
                                             exec_time_total_msec,
                                             created_time,
                                             created_time); // set created_time
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_group_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);
  }

  // update record
  {
    rc = dbmgr_db_group_info_update_exec_time_limit_msec(group_id, new_exec_time_limit_msec, update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_group_info_select(group_id, &actual_gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(group_id, actual_gi_ptr->group_id);
    TEST_ASSERT_EQUAL(new_exec_time_limit_msec, actual_gi_ptr->exec_time_limit_msec);
    TEST_ASSERT_EQUAL(exec_time_total_msec, actual_gi_ptr->exec_time_total_msec);
    TEST_ASSERT_EQUAL(created_time, actual_gi_ptr->created_time);
    TEST_ASSERT_EQUAL(update_time, actual_gi_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_group_info_destroy(actual_gi_ptr);
    actual_gi_ptr = NULL;
  }
}

void test_dbmgr_db_group_info_update_exec_time_total_msec(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-group";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  const int64_t created_time = 1736474012;
  const int64_t update_time = 1736832113;
  const uint64_t new_exec_time_total_msec = 567891234;
  int actual_count = 0;
  dbmgr_group_info_t actual_gi_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_group_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_group_info_insert_record(group_id,
                                             exec_time_limit_msec,
                                             exec_time_total_msec,
                                             created_time,
                                             created_time); // set created_time
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_group_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);
  }

  // update record
  {
    rc = dbmgr_db_group_info_update_exec_time_total_msec(group_id, new_exec_time_total_msec, update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_group_info_select(group_id, &actual_gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(group_id, actual_gi_ptr->group_id);
    TEST_ASSERT_EQUAL(exec_time_limit_msec, actual_gi_ptr->exec_time_limit_msec);
    TEST_ASSERT_EQUAL(new_exec_time_total_msec, actual_gi_ptr->exec_time_total_msec);
    TEST_ASSERT_EQUAL(created_time, actual_gi_ptr->created_time);
    TEST_ASSERT_EQUAL(update_time, actual_gi_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_group_info_destroy(actual_gi_ptr);
    actual_gi_ptr = NULL;
  }
}

void test_dbmgr_db_user_group_info_insert_record(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const size_t user_id_len = strlen(user_id);
  const char *group_id = "test-group";
  const size_t group_id_len = strlen(group_id);
  const sqc_rpc_sched_user_group_status_t status = SQC_RPC_SCHED_USER_GROUP_STATUS_ENABLED;
  const int64_t created_time = 1736474012;
  const int64_t update_time = created_time;
  char user_group_key[SQC_RPC_SCHED_USER_GROUP_KEY_MAX_SIZE + 1];
  size_t user_group_key_len;
  int actual_count = 0;
  dbmgr_user_group_info_t actual_ugi_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_user_group_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_user_group_info_insert_record(user_id, group_id, status,
                                                  created_time, update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_user_group_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);

    rc = dbmgr_util_create_user_group_key(user_group_key, sizeof(user_group_key),
                                          user_id, group_id, &user_group_key_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_user_group_info_select(user_id, group_id, &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(user_id, actual_ugi_ptr->user_id);
    TEST_ASSERT_EQUAL(user_id_len, actual_ugi_ptr->user_id_len);
    TEST_ASSERT_EQUAL_STRING(group_id, actual_ugi_ptr->group_id);
    TEST_ASSERT_EQUAL(group_id_len, actual_ugi_ptr->group_id_len);
    TEST_ASSERT_EQUAL_STRING(user_group_key, actual_ugi_ptr->user_group_key);
    TEST_ASSERT_EQUAL(user_group_key_len, actual_ugi_ptr->user_group_key_len);
    TEST_ASSERT_EQUAL(status, actual_ugi_ptr->status);
    TEST_ASSERT_EQUAL(created_time, actual_ugi_ptr->created_time);
    TEST_ASSERT_EQUAL(update_time, actual_ugi_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_user_group_info_destroy(actual_ugi_ptr);
    actual_ugi_ptr = NULL;
  }
}

void test_dbmgr_db_user_group_info_select_all(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const size_t user_id_len = strlen(user_id);
  const char *group_id = "test-group";
  const size_t group_id_len = strlen(group_id);
  const sqc_rpc_sched_user_group_status_t status = SQC_RPC_SCHED_USER_GROUP_STATUS_ENABLED;
  const int64_t created_time = 1736474012;
  const int64_t update_time = created_time;
  int actual_count = 0;
  dbmgr_user_group_info_t *actual_ugi_arr_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_user_group_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_user_group_info_insert_record(user_id, group_id, status,
                                                  created_time, update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_user_group_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);

    rc = s_dbmgr_db_user_group_info_select_all(&actual_ugi_arr_ptr, &actual_count, 1);;
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(user_id, actual_ugi_arr_ptr[0]->user_id);
    TEST_ASSERT_EQUAL(user_id_len, actual_ugi_arr_ptr[0]->user_id_len);
    TEST_ASSERT_EQUAL_STRING(group_id, actual_ugi_arr_ptr[0]->group_id);
    TEST_ASSERT_EQUAL(group_id_len, actual_ugi_arr_ptr[0]->group_id_len);
    TEST_ASSERT_EQUAL(status, actual_ugi_arr_ptr[0]->status);
    TEST_ASSERT_EQUAL(created_time, actual_ugi_arr_ptr[0]->created_time);
    TEST_ASSERT_EQUAL(update_time, actual_ugi_arr_ptr[0]->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_user_group_info_destroy(actual_ugi_arr_ptr[0]);
    actual_ugi_arr_ptr[0] = NULL;

    free(actual_ugi_arr_ptr);
    actual_ugi_arr_ptr = NULL;
  }
}

void test_dbmgr_db_user_group_info_deserialize_all(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const size_t user_id_len = strlen(user_id);
  const char *group_id = "test-group";
  const size_t group_id_len = strlen(group_id);
  const sqc_rpc_sched_user_group_status_t status = SQC_RPC_SCHED_USER_GROUP_STATUS_ENABLED;
  const int64_t created_time = 1736474012;
  const int64_t update_time = created_time;
  sqc_hashmap_t actual_user_group_info_hashmap = NULL;
  char user_group_key[SQC_RPC_SCHED_USER_GROUP_KEY_MAX_SIZE + 1];
  size_t user_group_key_len;
  int actual_count = 0;
  dbmgr_user_group_info_t actual_ugi_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_user_group_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);

    rc = sqc_hashmap_create(&actual_user_group_info_hashmap, SQC_HASHMAP_TYPE_STRING, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create record
  {
    rc = s_dbmgr_db_user_group_info_insert_record(user_id, group_id, status,
                                                  created_time, update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_user_group_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);

    rc = s_dbmgr_db_user_group_info_deserialize_all(&actual_user_group_info_hashmap);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_util_create_user_group_key(user_group_key, sizeof(user_group_key),
                                          user_id, group_id, &user_group_key_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = sqc_hashmap_find(&actual_user_group_info_hashmap, (void *) user_group_key, (void **) &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(user_id, actual_ugi_ptr->user_id);
    TEST_ASSERT_EQUAL(user_id_len, actual_ugi_ptr->user_id_len);
    TEST_ASSERT_EQUAL_STRING(group_id, actual_ugi_ptr->group_id);
    TEST_ASSERT_EQUAL(group_id_len, actual_ugi_ptr->group_id_len);
    TEST_ASSERT_EQUAL_STRING(user_group_key, actual_ugi_ptr->user_group_key);
    TEST_ASSERT_EQUAL(user_group_key_len, actual_ugi_ptr->user_group_key_len);
    TEST_ASSERT_EQUAL(status, actual_ugi_ptr->status);
    TEST_ASSERT_EQUAL(created_time, actual_ugi_ptr->created_time);
    TEST_ASSERT_EQUAL(update_time, actual_ugi_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    sqc_hashmap_destroy(&actual_user_group_info_hashmap, true);
    actual_user_group_info_hashmap = NULL;

    s_dbmgr_user_group_info_destroy(actual_ugi_ptr);
    actual_ugi_ptr = NULL;
  }
}

void test_dbmgr_db_user_group_info_create_destroy(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const size_t user_id_len = strlen(user_id);
  const char *group_id = "test-group";
  const size_t group_id_len = strlen(group_id);
  const sqc_rpc_sched_user_group_status_t status = SQC_RPC_SCHED_USER_GROUP_STATUS_ENABLED;
  const int64_t created_time = 1736474012;
  const int64_t update_time = created_time;
  char user_group_key[SQC_RPC_SCHED_USER_GROUP_KEY_MAX_SIZE + 1];
  size_t user_group_key_len;
  dbmgr_user_group_info_t actual_ugi_ptr = NULL;


  // create user_group_info
  {
    rc = dbmgr_util_create_user_group_key(user_group_key, sizeof(user_group_key),
                                          user_id, group_id, &user_group_key_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_user_group_info_create_from_db(user_id, group_id, status,
                                                created_time, update_time, &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(user_id, actual_ugi_ptr->user_id);
    TEST_ASSERT_EQUAL(user_id_len, actual_ugi_ptr->user_id_len);
    TEST_ASSERT_EQUAL_STRING(group_id, actual_ugi_ptr->group_id);
    TEST_ASSERT_EQUAL(group_id_len, actual_ugi_ptr->group_id_len);
    TEST_ASSERT_EQUAL_STRING(user_group_key, actual_ugi_ptr->user_group_key);
    TEST_ASSERT_EQUAL(user_group_key_len, actual_ugi_ptr->user_group_key_len);
    TEST_ASSERT_EQUAL(status, actual_ugi_ptr->status);
    TEST_ASSERT_EQUAL(created_time, actual_ugi_ptr->created_time);
    TEST_ASSERT_EQUAL(update_time, actual_ugi_ptr->update_time);
  }

  // destroy record
  {
    s_dbmgr_user_group_info_destroy(actual_ugi_ptr);
    actual_ugi_ptr = NULL;
  }
}

void test_dbmgr_db_user_group_info_update_status(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const sqc_rpc_sched_user_group_status_t status = SQC_RPC_SCHED_USER_GROUP_STATUS_ENABLED;
  const int64_t created_time = 1736474012;
  const int64_t update_time = 1736832113;
  const sqc_rpc_sched_user_group_status_t new_status = SQC_RPC_SCHED_USER_GROUP_STATUS_DISABLED;
  int actual_count = 0;
  dbmgr_user_group_info_t actual_ugi_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_user_group_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_user_group_info_insert_record(user_id, group_id, status,
                                                  created_time, update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_user_group_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);
  }

  // update record
  {
    rc = dbmgr_db_user_group_info_update_status(user_id, group_id,
                                                new_status, update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_user_group_info_select(user_id, group_id, &actual_ugi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(user_id, actual_ugi_ptr->user_id);
    TEST_ASSERT_EQUAL_STRING(group_id, actual_ugi_ptr->group_id);
    TEST_ASSERT_EQUAL(new_status, actual_ugi_ptr->status);
    TEST_ASSERT_EQUAL(update_time, actual_ugi_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_user_group_info_destroy(actual_ugi_ptr);
    actual_ugi_ptr = NULL;
  }
}

void test_dbmgr_db_job_info_insert_record(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *job_id = "test-job";
  const size_t job_id_len = strlen(job_id);
  const char *user_id = "test-user";
  const size_t user_id_len = strlen(user_id);
  const char *group_id = "test-group";
  const size_t group_id_len = strlen(group_id);
  const uint8_t priority = 5;
  const sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;
  const char *qprogram= "test-qprogram-data";
  const size_t qprogram_len = strlen(qprogram);
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const size_t remark_len = strlen(remark);
  const int64_t created_time = 1736474012;
  const int64_t update_time = created_time;
  int actual_count = 0;
  dbmgr_job_info_t actual_ji_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_job_info_insert_record(job_id, user_id, group_id,
                                           priority, status,
                                           qprogram, circuit_fmt, shots,
                                           qc_type, transpiler,
                                           remark, created_time,
                                           update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);

    rc = s_dbmgr_db_job_info_select(job_id, &actual_ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(job_id, actual_ji_ptr->job_id);
    TEST_ASSERT_EQUAL(job_id_len, actual_ji_ptr->job_id_len);
    TEST_ASSERT_EQUAL_STRING(user_id, actual_ji_ptr->user_id);
    TEST_ASSERT_EQUAL(user_id_len, actual_ji_ptr->user_id_len);
    TEST_ASSERT_EQUAL_STRING(group_id, actual_ji_ptr->group_id);
    TEST_ASSERT_EQUAL(group_id_len, actual_ji_ptr->group_id_len);
    TEST_ASSERT_EQUAL(priority, actual_ji_ptr->priority);
    TEST_ASSERT_EQUAL(status, actual_ji_ptr->status);
    TEST_ASSERT_EQUAL_STRING("", actual_ji_ptr->qc_job_id);
    TEST_ASSERT_EQUAL(0, actual_ji_ptr->qc_job_id_len);
    TEST_ASSERT_EQUAL_STRING(qprogram, actual_ji_ptr->qprogram);
    TEST_ASSERT_EQUAL(qprogram_len, actual_ji_ptr->qprogram_len);
    TEST_ASSERT_EQUAL(circuit_fmt, actual_ji_ptr->circuit_fmt);
    TEST_ASSERT_EQUAL(shots, actual_ji_ptr->shots);
    TEST_ASSERT_EQUAL(qc_type, actual_ji_ptr->qc_type);
    TEST_ASSERT_EQUAL(transpiler, actual_ji_ptr->transpiler);
    TEST_ASSERT_EQUAL_STRING(remark, actual_ji_ptr->remark);
    TEST_ASSERT_EQUAL(remark_len, actual_ji_ptr->remark_len);
    TEST_ASSERT_EQUAL_STRING("", actual_ji_ptr->result);
    TEST_ASSERT_EQUAL(0, actual_ji_ptr->result_len);
    // user_token is not persistent
    {
      TEST_ASSERT_EQUAL_STRING("", actual_ji_ptr->user_token);
      TEST_ASSERT_EQUAL(0, actual_ji_ptr->user_token_len);
    }
    TEST_ASSERT_EQUAL(created_time, actual_ji_ptr->created_time);
    TEST_ASSERT_EQUAL(0, actual_ji_ptr->queued_time);
    TEST_ASSERT_EQUAL(0, actual_ji_ptr->running_time);
    TEST_ASSERT_EQUAL(0, actual_ji_ptr->done_time);
    TEST_ASSERT_EQUAL(0, actual_ji_ptr->cancelled_time);
    TEST_ASSERT_EQUAL(0, actual_ji_ptr->error_time);
    TEST_ASSERT_EQUAL(0, actual_ji_ptr->deleted_time);
    TEST_ASSERT_EQUAL(created_time, actual_ji_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_job_info_destroy(actual_ji_ptr);
    actual_ji_ptr = NULL;
  }
}

void test_dbmgr_db_job_info_select_all(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *job_id = "test-job";
  const size_t job_id_len = strlen(job_id);
  const char *user_id = "test-user";
  const size_t user_id_len = strlen(user_id);
  const char *group_id = "test-group";
  const size_t group_id_len = strlen(group_id);
  const uint8_t priority = 5;
  const sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;
  const char *qprogram = "test-qprogram-data";
  const size_t qprogram_len = strlen(qprogram);
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const size_t remark_len = strlen(remark);
  const int64_t created_time = 1736474012;
  const int64_t update_time = created_time;
  int actual_count = 0;
  dbmgr_job_info_t *actual_ji_arr_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_job_info_insert_record(job_id, user_id, group_id,
                                           priority, status,
                                           qprogram, circuit_fmt, shots,
                                           qc_type, transpiler,
                                           remark, created_time,
                                           update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);

    rc = s_dbmgr_db_job_info_select_all(&actual_ji_arr_ptr, &actual_count, 1);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);
    TEST_ASSERT_EQUAL_STRING(job_id, actual_ji_arr_ptr[0]->job_id);
    TEST_ASSERT_EQUAL(job_id_len, actual_ji_arr_ptr[0]->job_id_len);
    TEST_ASSERT_EQUAL_STRING(user_id, actual_ji_arr_ptr[0]->user_id);
    TEST_ASSERT_EQUAL(user_id_len, actual_ji_arr_ptr[0]->user_id_len);
    TEST_ASSERT_EQUAL_STRING(group_id, actual_ji_arr_ptr[0]->group_id);
    TEST_ASSERT_EQUAL(group_id_len, actual_ji_arr_ptr[0]->group_id_len);
    TEST_ASSERT_EQUAL(priority, actual_ji_arr_ptr[0]->priority);
    TEST_ASSERT_EQUAL(status, actual_ji_arr_ptr[0]->status);
    TEST_ASSERT_EQUAL_STRING("", actual_ji_arr_ptr[0]->qc_job_id);
    TEST_ASSERT_EQUAL(0, actual_ji_arr_ptr[0]->qc_job_id_len);
    TEST_ASSERT_EQUAL_STRING(qprogram, actual_ji_arr_ptr[0]->qprogram);
    TEST_ASSERT_EQUAL(qprogram_len, actual_ji_arr_ptr[0]->qprogram_len);
    TEST_ASSERT_EQUAL(circuit_fmt, actual_ji_arr_ptr[0]->circuit_fmt);
    TEST_ASSERT_EQUAL(shots, actual_ji_arr_ptr[0]->shots);
    TEST_ASSERT_EQUAL(qc_type, actual_ji_arr_ptr[0]->qc_type);
    TEST_ASSERT_EQUAL(transpiler, actual_ji_arr_ptr[0]->transpiler);
    TEST_ASSERT_EQUAL_STRING(remark, actual_ji_arr_ptr[0]->remark);
    TEST_ASSERT_EQUAL_STRING("", actual_ji_arr_ptr[0]->result);
    TEST_ASSERT_EQUAL(0, actual_ji_arr_ptr[0]->result_len);
    // user_token is not persistent
    {
      TEST_ASSERT_EQUAL_STRING("", actual_ji_arr_ptr[0]->user_token);
      TEST_ASSERT_EQUAL(0, actual_ji_arr_ptr[0]->user_token_len);
    }
    TEST_ASSERT_EQUAL(remark_len, actual_ji_arr_ptr[0]->remark_len);
    TEST_ASSERT_EQUAL(created_time, actual_ji_arr_ptr[0]->created_time);
    TEST_ASSERT_EQUAL(0, actual_ji_arr_ptr[0]->queued_time);
    TEST_ASSERT_EQUAL(0, actual_ji_arr_ptr[0]->running_time);
    TEST_ASSERT_EQUAL(0, actual_ji_arr_ptr[0]->done_time);
    TEST_ASSERT_EQUAL(0, actual_ji_arr_ptr[0]->cancelled_time);
    TEST_ASSERT_EQUAL(0, actual_ji_arr_ptr[0]->error_time);
    TEST_ASSERT_EQUAL(0, actual_ji_arr_ptr[0]->deleted_time);
    TEST_ASSERT_EQUAL(created_time, actual_ji_arr_ptr[0]->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_job_info_destroy(actual_ji_arr_ptr[0]);
    actual_ji_arr_ptr[0] = NULL;

    free(actual_ji_arr_ptr);
    actual_ji_arr_ptr = NULL;
  }
}

void test_dbmgr_db_job_info_deserialize_all(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *job_id = "test-job";
  const size_t job_id_len = strlen(job_id);
  const char *user_id = "test-user";
  const size_t user_id_len = strlen(user_id);
  const char *group_id = "test-group";
  const size_t group_id_len = strlen(group_id);
  const uint8_t priority = 5;
  const sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;
  const char *qprogram = "test-qprogram-data";
  const size_t qprogram_len = strlen(qprogram);
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const size_t remark_len = strlen(remark);
  const int64_t created_time = 1736474012;
  const int64_t update_time = created_time;
  sqc_hashmap_t actual_job_info_hashmap = NULL;
  int actual_count = 0;
  dbmgr_job_info_t actual_ji_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);

    rc = sqc_hashmap_create(&actual_job_info_hashmap, SQC_HASHMAP_TYPE_STRING, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create record
  {
    rc = s_dbmgr_db_job_info_insert_record(job_id, user_id, group_id,
                                           priority, status,
                                           qprogram, circuit_fmt, shots,
                                           qc_type, transpiler,
                                           remark, created_time,
                                           update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);
  }

  // job_info_deserialize_all
  {
    rc = s_dbmgr_db_job_info_deserialize_all(&actual_job_info_hashmap);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = sqc_hashmap_find(&actual_job_info_hashmap, (void *) job_id, (void **) &actual_ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(job_id, actual_ji_ptr->job_id);
    TEST_ASSERT_EQUAL(job_id_len, actual_ji_ptr->job_id_len);
    TEST_ASSERT_EQUAL_STRING(user_id, actual_ji_ptr->user_id);
    TEST_ASSERT_EQUAL(user_id_len, actual_ji_ptr->user_id_len);
    TEST_ASSERT_EQUAL_STRING(group_id, actual_ji_ptr->group_id);
    TEST_ASSERT_EQUAL(group_id_len, actual_ji_ptr->group_id_len);
    TEST_ASSERT_EQUAL(priority, actual_ji_ptr->priority);
    TEST_ASSERT_EQUAL(status, actual_ji_ptr->status);
    TEST_ASSERT_EQUAL_STRING("", actual_ji_ptr->qc_job_id);
    TEST_ASSERT_EQUAL(0, actual_ji_ptr->qc_job_id_len);
    TEST_ASSERT_EQUAL_STRING(qprogram, actual_ji_ptr->qprogram);
    TEST_ASSERT_EQUAL(qprogram_len, actual_ji_ptr->qprogram_len);
    TEST_ASSERT_EQUAL(circuit_fmt, actual_ji_ptr->circuit_fmt);
    TEST_ASSERT_EQUAL(shots, actual_ji_ptr->shots);
    TEST_ASSERT_EQUAL(qc_type, actual_ji_ptr->qc_type);
    TEST_ASSERT_EQUAL(transpiler, actual_ji_ptr->transpiler);
    TEST_ASSERT_EQUAL_STRING(remark, actual_ji_ptr->remark);
    TEST_ASSERT_EQUAL(remark_len, actual_ji_ptr->remark_len);
    TEST_ASSERT_EQUAL_STRING("", actual_ji_ptr->result);
    TEST_ASSERT_EQUAL(0, actual_ji_ptr->result_len);
    // user_token is not persistent
    {
      TEST_ASSERT_EQUAL_STRING("", actual_ji_ptr->user_token);
      TEST_ASSERT_EQUAL(0, actual_ji_ptr->user_token_len);
    }
    TEST_ASSERT_EQUAL(created_time, actual_ji_ptr->created_time);
    TEST_ASSERT_EQUAL(0, actual_ji_ptr->queued_time);
    TEST_ASSERT_EQUAL(0, actual_ji_ptr->running_time);
    TEST_ASSERT_EQUAL(0, actual_ji_ptr->done_time);
    TEST_ASSERT_EQUAL(0, actual_ji_ptr->cancelled_time);
    TEST_ASSERT_EQUAL(0, actual_ji_ptr->error_time);
    TEST_ASSERT_EQUAL(0, actual_ji_ptr->deleted_time);
    TEST_ASSERT_EQUAL(created_time, actual_ji_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    sqc_hashmap_destroy(&actual_job_info_hashmap, true);
    actual_job_info_hashmap = NULL;

    s_dbmgr_job_info_destroy(actual_ji_ptr);
    actual_ji_ptr = NULL;
  }
}

void test_dbmgr_db_job_info_create_destroy(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *job_id = "test-job";
  const size_t job_id_len = strlen(job_id);
  const char *user_id = "test-user";
  const size_t user_id_len = strlen(user_id);
  const char *group_id = "test-group";
  const size_t group_id_len = strlen(group_id);
  const uint8_t priority = 5;
  const sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;
  const char *qc_job_id = "test-qc-job-id";
  const size_t qc_job_id_len = strlen(qc_job_id);
  const char *qprogram = "test-qprogram-data";
  const size_t qprogram_len = strlen(qprogram);
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const size_t remark_len = strlen(remark);
  const char *result = "test-result-data";
  const size_t result_len = strlen(result);
  const uint64_t exec_time_estimate_msec = 123456789;
  const uint64_t exec_time_msec = 987654321;
  const int64_t created_time = 1736474012;
  const int64_t queued_time = 1736500222;
  const int64_t running_time = 1736500399;
  const int64_t done_time = 1736500756;
  const int64_t cancelled_time = 1736500908;
  const int64_t error_time = 1736501091;
  const int64_t deleted_time = 1736502091;
  const int64_t update_time = 1737105704;
  dbmgr_job_info_t actual_ji_ptr = NULL;

  // create job_info
  {
    rc = s_dbmgr_job_info_create_from_db(job_id, user_id, group_id, priority, status,
                                         qc_job_id, qprogram, circuit_fmt, shots, qc_type,
                                         transpiler, remark, result,
                                         exec_time_estimate_msec, exec_time_msec,
                                         created_time, queued_time, running_time,
                                         done_time, cancelled_time, error_time,
                                         deleted_time, update_time, &actual_ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(job_id, actual_ji_ptr->job_id);
    TEST_ASSERT_EQUAL(job_id_len, strlen(actual_ji_ptr->job_id));
    TEST_ASSERT_EQUAL_STRING(user_id, actual_ji_ptr->user_id);
    TEST_ASSERT_EQUAL(user_id_len, actual_ji_ptr->user_id_len);
    TEST_ASSERT_EQUAL_STRING(group_id, actual_ji_ptr->group_id);
    TEST_ASSERT_EQUAL(group_id_len, actual_ji_ptr->group_id_len);
    TEST_ASSERT_EQUAL(priority, actual_ji_ptr->priority);
    TEST_ASSERT_EQUAL(status, actual_ji_ptr->status);
    TEST_ASSERT_EQUAL_STRING(qc_job_id, actual_ji_ptr->qc_job_id);
    TEST_ASSERT_EQUAL(qc_job_id_len, actual_ji_ptr->qc_job_id_len);
    TEST_ASSERT_EQUAL_STRING(qprogram, actual_ji_ptr->qprogram);
    TEST_ASSERT_EQUAL(qprogram_len, actual_ji_ptr->qprogram_len);
    TEST_ASSERT_EQUAL(circuit_fmt, actual_ji_ptr->circuit_fmt);
    TEST_ASSERT_EQUAL(shots, actual_ji_ptr->shots);
    TEST_ASSERT_EQUAL(qc_type, actual_ji_ptr->qc_type);
    TEST_ASSERT_EQUAL(transpiler, actual_ji_ptr->transpiler);
    TEST_ASSERT_EQUAL_STRING(remark, actual_ji_ptr->remark);
    TEST_ASSERT_EQUAL(remark_len, actual_ji_ptr->remark_len);
    TEST_ASSERT_EQUAL_STRING(result, actual_ji_ptr->result);
    TEST_ASSERT_EQUAL(result_len, actual_ji_ptr->result_len);
    // user_token is not persistent
    {
      TEST_ASSERT_EQUAL_STRING("", actual_ji_ptr->user_token);
      TEST_ASSERT_EQUAL(0, actual_ji_ptr->user_token_len);
    }
    TEST_ASSERT_EQUAL(exec_time_estimate_msec, actual_ji_ptr->exec_time_estimate_msec);
    TEST_ASSERT_EQUAL(exec_time_msec, actual_ji_ptr->exec_time_msec);
    TEST_ASSERT_EQUAL(created_time, actual_ji_ptr->created_time);
    TEST_ASSERT_EQUAL(queued_time, actual_ji_ptr->queued_time);
    TEST_ASSERT_EQUAL(running_time, actual_ji_ptr->running_time);
    TEST_ASSERT_EQUAL(done_time, actual_ji_ptr->done_time);
    TEST_ASSERT_EQUAL(cancelled_time, actual_ji_ptr->cancelled_time);
    TEST_ASSERT_EQUAL(error_time, actual_ji_ptr->error_time);
    TEST_ASSERT_EQUAL(deleted_time, actual_ji_ptr->deleted_time);
    TEST_ASSERT_EQUAL(update_time, actual_ji_ptr->update_time);
  }

  // destroy record
  {
    s_dbmgr_job_info_destroy(actual_ji_ptr);
    actual_ji_ptr = NULL;
  }
}

void test_dbmgr_db_job_info_clear_result(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *job_id = "test-job";
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const int64_t created_time = 1736474012;
  const char *result = "";
  const int64_t deleted_time = 1736719129;
  const int64_t update_time = 1736832113;
  int actual_count = 0;
  dbmgr_job_info_t actual_ji_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_job_info_insert_record(job_id, user_id, group_id,
                                           priority, status,
                                           qprogram, circuit_fmt, shots,
                                           qc_type, transpiler,
                                           remark, created_time,
                                           update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);
  }

  // update record
  {
    rc = s_dbmgr_db_job_info_clear_result(job_id, deleted_time, update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select(job_id, &actual_ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(job_id, actual_ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_DELETED, actual_ji_ptr->status);
    TEST_ASSERT_EQUAL_STRING(result, actual_ji_ptr->result);
    TEST_ASSERT_EQUAL(deleted_time, actual_ji_ptr->deleted_time);
    TEST_ASSERT_EQUAL(update_time, actual_ji_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_job_info_destroy(actual_ji_ptr);
    actual_ji_ptr = NULL;
  }
}


void test_dbmgr_db_job_info_update_queued_status(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *job_id = "test-job";
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const int64_t created_time = 1736474012;
  const int64_t queued_time = 1736500222;
  const int64_t update_time = queued_time;
  int actual_count = 0;
  dbmgr_job_info_t actual_ji_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_job_info_insert_record(job_id, user_id, group_id,
                                           priority, status,
                                           qprogram, circuit_fmt, shots,
                                           qc_type, transpiler,
                                           remark, created_time,
                                           created_time); // set created_time
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);
  }

  // update record
  {
    rc = dbmgr_db_job_info_update_queued_status(job_id, queued_time,
                                                update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select(job_id, &actual_ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(job_id, actual_ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_QUEUED, actual_ji_ptr->status);
    TEST_ASSERT_EQUAL(created_time, actual_ji_ptr->created_time);
    TEST_ASSERT_EQUAL(queued_time, actual_ji_ptr->queued_time);
    TEST_ASSERT_EQUAL(update_time, actual_ji_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_job_info_destroy(actual_ji_ptr);
    actual_ji_ptr = NULL;
  }
}

void test_dbmgr_db_job_info_update_running_status(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *job_id = "test-job";
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const int64_t created_time = 1736474012;
  const int64_t running_time = 1736500399;
  const int64_t update_time = running_time;
  int actual_count = 0;
  dbmgr_job_info_t actual_ji_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_job_info_insert_record(job_id, user_id, group_id,
                                           priority, status,
                                           qprogram, circuit_fmt, shots,
                                           qc_type, transpiler,
                                           remark, created_time,
                                           created_time); // set created_time
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);
  }

  // update record
  {
    rc = dbmgr_db_job_info_update_running_status(job_id, running_time,
                                                 update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select(job_id, &actual_ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(job_id, actual_ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_RUNNING, actual_ji_ptr->status);
    TEST_ASSERT_EQUAL(created_time, actual_ji_ptr->created_time);
    TEST_ASSERT_EQUAL(running_time, actual_ji_ptr->running_time);
    TEST_ASSERT_EQUAL(update_time, actual_ji_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_job_info_destroy(actual_ji_ptr);
    actual_ji_ptr = NULL;
  }
}

void test_dbmgr_db_job_info_update_done_status(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *job_id = "test-job";
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const int64_t created_time = 1736474012;
  const int64_t done_time = 1736500756;
  const int64_t update_time = done_time;
  int actual_count = 0;
  dbmgr_job_info_t actual_ji_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_job_info_insert_record(job_id, user_id, group_id,
                                           priority, status,
                                           qprogram, circuit_fmt, shots,
                                           qc_type, transpiler,
                                           remark, created_time,
                                           created_time); // set created_time
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);
  }

  // update record
  {
    rc = dbmgr_db_job_info_update_done_status(job_id, done_time,
                                              update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select(job_id, &actual_ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(job_id, actual_ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_DONE, actual_ji_ptr->status);
    TEST_ASSERT_EQUAL(created_time, actual_ji_ptr->created_time);
    TEST_ASSERT_EQUAL(done_time, actual_ji_ptr->done_time);
    TEST_ASSERT_EQUAL(update_time, actual_ji_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_job_info_destroy(actual_ji_ptr);
    actual_ji_ptr = NULL;
  }
}

void test_dbmgr_db_job_info_update_cancelled_status(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *job_id = "test-job";
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const int64_t created_time = 1736474012;
  const int64_t cancelled_time = 1736500908;
  const int64_t update_time = cancelled_time;
  int actual_count = 0;
  dbmgr_job_info_t actual_ji_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_job_info_insert_record(job_id, user_id, group_id,
                                           priority, status,
                                           qprogram, circuit_fmt, shots,
                                           qc_type, transpiler,
                                           remark, created_time,
                                           created_time); // set created_time
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);
  }

  // update record
  {
    rc = dbmgr_db_job_info_update_cancelled_status(job_id, cancelled_time,
                                                   update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select(job_id, &actual_ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(job_id, actual_ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_CANCELLED, actual_ji_ptr->status);
    TEST_ASSERT_EQUAL(created_time, actual_ji_ptr->created_time);
    TEST_ASSERT_EQUAL(cancelled_time, actual_ji_ptr->cancelled_time);
    TEST_ASSERT_EQUAL(update_time, actual_ji_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_job_info_destroy(actual_ji_ptr);
    actual_ji_ptr = NULL;
  }
}

void test_dbmgr_db_job_info_update_error_status(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *job_id = "test-job";
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const int64_t created_time = 1736474012;
  const int64_t error_time = 1736501091;
  const int64_t update_time = error_time;
  int actual_count = 0;
  dbmgr_job_info_t actual_ji_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_job_info_insert_record(job_id, user_id, group_id,
                                           priority, status,
                                           qprogram, circuit_fmt, shots,
                                           qc_type, transpiler,
                                           remark, created_time,
                                           created_time); // set created_time
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);
  }

  // update record
  {
    rc = dbmgr_db_job_info_update_error_status(job_id, error_time,
                                               update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select(job_id, &actual_ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(job_id, actual_ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_ERROR, actual_ji_ptr->status);
    TEST_ASSERT_EQUAL(created_time, actual_ji_ptr->created_time);
    TEST_ASSERT_EQUAL(error_time, actual_ji_ptr->error_time);
    TEST_ASSERT_EQUAL(update_time, actual_ji_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_job_info_destroy(actual_ji_ptr);
    actual_ji_ptr = NULL;
  }
}

void test_dbmgr_db_job_info_update_deleted_status(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *job_id = "test-job";
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const int64_t created_time = 1736474012;
  const int64_t deleted_time = 1736502091;
  const int64_t update_time = deleted_time;
  int actual_count = 0;
  dbmgr_job_info_t actual_ji_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_job_info_insert_record(job_id, user_id, group_id,
                                           priority, status,
                                           qprogram, circuit_fmt, shots,
                                           qc_type, transpiler,
                                           remark, created_time,
                                           created_time); // set created_time
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);
  }

  // update record
  {
    rc = dbmgr_db_job_info_update_deleted_status(job_id, deleted_time,
                                                 update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select(job_id, &actual_ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(job_id, actual_ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_DELETED, actual_ji_ptr->status);
    TEST_ASSERT_EQUAL(created_time, actual_ji_ptr->created_time);
    TEST_ASSERT_EQUAL(deleted_time, actual_ji_ptr->deleted_time);
    TEST_ASSERT_EQUAL(update_time, actual_ji_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_job_info_destroy(actual_ji_ptr);
    actual_ji_ptr = NULL;
  }
}

void test_dbmgr_db_job_info_update_qc_job_id(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *job_id = "test-job";
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;
  const char *qc_job_id = "test-qc-job-id";
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const int64_t created_time = 1736474012;
  const int64_t update_time = 1736832041;
  int actual_count = 0;
  dbmgr_job_info_t actual_ji_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_job_info_insert_record(job_id, user_id, group_id,
                                           priority, status,
                                           qprogram, circuit_fmt, shots,
                                           qc_type, transpiler,
                                           remark, created_time,
                                           created_time); // set created_time
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);
  }

  // update record
  {
    rc = dbmgr_db_job_info_update_qc_job_id(job_id, qc_job_id, update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select(job_id, &actual_ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(job_id, actual_ji_ptr->job_id);
    TEST_ASSERT_EQUAL_STRING(qc_job_id, actual_ji_ptr->qc_job_id);
    TEST_ASSERT_EQUAL(update_time, actual_ji_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_job_info_destroy(actual_ji_ptr);
    actual_ji_ptr = NULL;
  }
}

void test_dbmgr_db_job_info_update_result(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *job_id = "test-job";
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *result = "test-result";
  const int64_t created_time = 1736474012;
  const int64_t update_time = 1736832113;
  int actual_count = 0;
  dbmgr_job_info_t actual_ji_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_job_info_insert_record(job_id, user_id, group_id,
                                           priority, status,
                                           qprogram, circuit_fmt, shots,
                                           qc_type, transpiler,
                                           remark, created_time,
                                           created_time); // set created_time
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);
  }

  // update record
  {
    rc = dbmgr_db_job_info_update_result(job_id, result, update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select(job_id, &actual_ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(job_id, actual_ji_ptr->job_id);
    TEST_ASSERT_EQUAL_STRING(result, actual_ji_ptr->result);
    TEST_ASSERT_EQUAL(update_time, actual_ji_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_job_info_destroy(actual_ji_ptr);
    actual_ji_ptr = NULL;
  }
}

void test_dbmgr_db_job_info_update_exec_time_estimate_msec(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *job_id = "test-job";
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const uint64_t exec_time_estimate_msec = 123456789;
  const int64_t created_time = 1736474012;
  const int64_t update_time = 1736832113;
  int actual_count = 0;
  dbmgr_job_info_t actual_ji_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_job_info_insert_record(job_id, user_id, group_id,
                                           priority, status,
                                           qprogram, circuit_fmt, shots,
                                           qc_type, transpiler,
                                           remark, created_time,
                                           created_time); // set created_time
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);
  }

  // update record
  {
    rc = dbmgr_db_job_info_update_exec_time_estimate_msec(job_id, exec_time_estimate_msec, update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select(job_id, &actual_ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(job_id, actual_ji_ptr->job_id);
    TEST_ASSERT_EQUAL(exec_time_estimate_msec, actual_ji_ptr->exec_time_estimate_msec);
    TEST_ASSERT_EQUAL(update_time, actual_ji_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_job_info_destroy(actual_ji_ptr);
    actual_ji_ptr = NULL;
  }
}

void test_dbmgr_db_job_info_update_exec_time_msec(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *job_id = "test-job";
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const uint64_t exec_time_msec = 987654321;
  const int64_t created_time = 1736474012;
  const int64_t update_time = 1736832113;
  int actual_count = 0;
  dbmgr_job_info_t actual_ji_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_job_info_insert_record(job_id, user_id, group_id,
                                           priority, status,
                                           qprogram, circuit_fmt, shots,
                                           qc_type, transpiler,
                                           remark, created_time,
                                           created_time); // set created_time
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(1, actual_count);
  }

  // update record
  {
    rc = dbmgr_db_job_info_update_exec_time_msec(job_id, exec_time_msec, update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_job_info_select(job_id, &actual_ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(job_id, actual_ji_ptr->job_id);
    TEST_ASSERT_EQUAL(exec_time_msec, actual_ji_ptr->exec_time_msec);
    TEST_ASSERT_EQUAL(update_time, actual_ji_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_job_info_destroy(actual_ji_ptr);
    actual_ji_ptr = NULL;
  }
}

void test_dbmgr_db_weight_info_select_all(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int actual_count = 0;
  dbmgr_weight_info_t *actual_wi_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_weight_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(10, actual_count);
  }

  // create record
  {
    rc = s_dbmgr_db_weight_info_select_all(&actual_wi_ptr, &actual_count, 10);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(10, actual_count);

    for (int i = 0; i < actual_count; i++) {
      TEST_ASSERT_EQUAL(i, actual_wi_ptr[i]->priority);
      TEST_ASSERT_EQUAL(i * SQC_RPC_SCHED_WEIGHT_SCALE, actual_wi_ptr[i]->weight);
      TEST_ASSERT_EQUAL(SQC_RPC_SCHED_UT_DB_WEIGHT_DEFAULT_CREATED_TIME, actual_wi_ptr[i]->created_time);
      TEST_ASSERT_EQUAL(SQC_RPC_SCHED_UT_DB_WEIGHT_DEFAULT_UPDATE_TIME, actual_wi_ptr[i]->update_time);

      s_dbmgr_weight_info_destroy(actual_wi_ptr[i]);
      actual_wi_ptr[i] = NULL;
    }

    free(actual_wi_ptr);
    actual_wi_ptr = NULL;
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);
  }
}

void test_dbmgr_db_weight_info_deserialize_all(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  sqc_hashmap_t actual_weight_info_hashmap = NULL;
  int actual_count = 0;
  dbmgr_weight_info_t actual_wi_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_weight_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(10, actual_count);

    rc = sqc_hashmap_create(&actual_weight_info_hashmap, SQC_HASHMAP_TYPE_ONE_WORD, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // deserialize all
  {
    rc = s_dbmgr_db_weight_info_deserialize_all(&actual_weight_info_hashmap);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    for (int i = 0; i < actual_count; i++) {
      rc = sqc_hashmap_find(&actual_weight_info_hashmap, (void *)(uintptr_t)i, (void **)&actual_wi_ptr);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
      TEST_ASSERT_EQUAL(i, actual_wi_ptr->priority);
      TEST_ASSERT_EQUAL(i * SQC_RPC_SCHED_WEIGHT_SCALE, actual_wi_ptr->weight);
      TEST_ASSERT_EQUAL(SQC_RPC_SCHED_UT_DB_WEIGHT_DEFAULT_CREATED_TIME, actual_wi_ptr->created_time);
      TEST_ASSERT_EQUAL(SQC_RPC_SCHED_UT_DB_WEIGHT_DEFAULT_UPDATE_TIME, actual_wi_ptr->update_time);
    }
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    for (int i = 0; i < actual_count; i++) {
      actual_wi_ptr = NULL;
      if (sqc_hashmap_find(&actual_weight_info_hashmap,
                           (void *)(uintptr_t)i, (void **)&actual_wi_ptr) == SQC_RESULT_OK) {
        s_dbmgr_weight_info_destroy(actual_wi_ptr);
      }
    }
    actual_wi_ptr = NULL;

    sqc_hashmap_destroy(&actual_weight_info_hashmap, true);
    actual_weight_info_hashmap = NULL;

    s_dbmgr_weight_info_destroy(actual_wi_ptr);
    actual_wi_ptr = NULL;
  }
}

void test_dbmgr_db_weight_info_create_destroy(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const uint8_t priority = 5;
  const uint64_t weight = 9999;
  const int64_t created_time = 1736474012;
  const int64_t update_time = created_time;
  dbmgr_weight_info_t actual_wi_ptr = NULL;

  // create weight_info
  {
    rc = s_dbmgr_weight_info_create_from_db(priority, weight,
                                            created_time, update_time, &actual_wi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(priority, actual_wi_ptr->priority);
    TEST_ASSERT_EQUAL(weight, actual_wi_ptr->weight);
    TEST_ASSERT_EQUAL(created_time, actual_wi_ptr->created_time);
    TEST_ASSERT_EQUAL(update_time, actual_wi_ptr->update_time);
  }

  // destroy record
  {
    s_dbmgr_weight_info_destroy(actual_wi_ptr);
    actual_wi_ptr = NULL;
  }
}

void test_dbmgr_db_weight_info_update_weight(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const uint8_t priority = 5;
  const int64_t created_time = 1736474012;
  const int64_t update_time = 1736832113;
  const uint64_t new_weight = 7777;
  int actual_count = 0;
  dbmgr_weight_info_t actual_wi_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_db_weight_info_select_count(&actual_count);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(10, actual_count);
  }

  // update record
  {
    rc = dbmgr_db_weight_info_update_weight(priority, new_weight, update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_db_weight_info_select(priority, &actual_wi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(priority, actual_wi_ptr->priority);
    TEST_ASSERT_EQUAL(new_weight, actual_wi_ptr->weight);
    TEST_ASSERT_EQUAL(created_time, actual_wi_ptr->created_time);
    TEST_ASSERT_EQUAL(update_time, actual_wi_ptr->update_time);
  }

  // finalize
  {
    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);

    s_dbmgr_weight_info_destroy(actual_wi_ptr);
    actual_wi_ptr = NULL;
  }
}

