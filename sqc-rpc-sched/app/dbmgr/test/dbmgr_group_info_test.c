#include <sys/stat.h>

#include "unity.h"

#include "dbmgr_util.c"
#include "dbmgr_db.c"
#include "dbmgr_group_info.c"

#include "dbmgr_db_common.c"

void setUp(void) {
  remove_db_files("setUp - group_info");
  create_db_files("setUp - group_info");
}

void tearDown(void) {
  remove_db_files("tearDown - group_info");
}

/*
 * Positive test cases for private methods
 */

void test_s_dbmgr_group_info_initialize_finalize(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(group_info_hashmap);
  }

  // finalize
  {
    s_dbmgr_group_info_finalize();
    TEST_ASSERT_NULL(group_info_hashmap);

    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);
  }
}

void test_s_dbmgr_group_info_record_freeup(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-group";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;

  dbmgr_group_info_t gi_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create & add record(free in finalize)
  {
    rc = s_dbmgr_group_info_record_create(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_group_info_record_add(gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_TRUE(dbmgr_gi_group_exists(group_id));
  }

  // finalize
  s_dbmgr_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_s_dbmgr_group_info_record_create_destroy(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-group";
  const size_t group_id_len = strlen(group_id);
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  dbmgr_group_info_t gi_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_group_info_record_create(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(group_id, gi_ptr->group_id);
    TEST_ASSERT_EQUAL(group_id_len, gi_ptr->group_id_len);
  }

  // destroy record
  {
    s_dbmgr_group_info_record_destroy(gi_ptr);
    gi_ptr = NULL;
  }
}

void test_s_dbmgr_group_info_record_add_delete(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-group";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  dbmgr_group_info_t gi_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create record
  {
    rc = s_dbmgr_group_info_record_create(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // add record
  {
    rc = s_dbmgr_group_info_record_add(gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_TRUE(dbmgr_gi_group_exists(group_id));
  }

  // delete record
  {
    rc = s_dbmgr_group_info_record_delete(gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_FALSE(dbmgr_gi_group_exists(group_id));
  }

  // finalize
  s_dbmgr_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

/*
 * Positive test cases for public methods
 */

void test_dbmgr_gi_create_delete_group(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-group";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  dbmgr_group_info_t gi_ptr = NULL;
  char *actual_group_id = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create group
  {
    rc = dbmgr_gi_create_group(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get group_id
  {
    rc = dbmgr_gi_get_group_id(gi_ptr, &actual_group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(group_id, actual_group_id);

    free((void *) actual_group_id);
    actual_group_id = NULL;
  }

  // delete group
  {
    rc = dbmgr_gi_delete_group(group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_gi_group_exists(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-group";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  dbmgr_group_info_t gi_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create group
  {
    rc = dbmgr_gi_create_group(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // user exists
  {
    TEST_ASSERT_TRUE(dbmgr_gi_group_exists(group_id));
    TEST_ASSERT_FALSE(dbmgr_gi_group_exists("non-existent-user"));
  }

  // delete group
  {
    rc = dbmgr_gi_delete_group(group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_gi_group_find(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-group";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  dbmgr_group_info_t gi_ptr = NULL;
  char *actual_group_id = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create group
  {
    rc = dbmgr_gi_create_group(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // group find
  {
    rc = dbmgr_gi_group_find(group_id, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get group_id
  {
    rc = dbmgr_gi_get_group_id(gi_ptr, &actual_group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(group_id, actual_group_id);

    free((void *) actual_group_id);
    actual_group_id = NULL;
  }

  // delete group
  {
    rc = dbmgr_gi_delete_group(group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}


void test_dbmgr_gi_group_find_all_sorted_by_remaining_time1(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  const char *group_id_1 = "test-group1";
  const uint64_t exec_time_limit_msec_1 = 987654321;
  const uint64_t exec_time_total_msec_1 = 123456789;

  const char *group_id_2 = "test-group2";
  const uint64_t exec_time_limit_msec_2 = 987654321;
  const uint64_t exec_time_total_msec_2 = 234567891;

  const char *group_id_3 = "test-group3";
  const uint64_t exec_time_limit_msec_3 = 987654321;
  const uint64_t exec_time_total_msec_3 = 345678912;

  const char *group_id_4 = "test-group4";
  const uint64_t exec_time_limit_msec_4 = 987654321;
  const uint64_t exec_time_total_msec_4 = 456789123;

  const char *group_id_5 = "test-group5";
  const uint64_t exec_time_limit_msec_5 = 987654321;
  const uint64_t exec_time_total_msec_5 = 567891234;

  const size_t target_data_size = 5;
  dbmgr_group_info_t *actual_gi_ptr_arr = NULL;
  size_t actual_arr_len;

  dbmgr_group_info_t gi_ptr = NULL;
  char *actual_group_id = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create group
  {
    rc = dbmgr_gi_create_group(group_id_1, exec_time_limit_msec_1, exec_time_total_msec_1, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_create_group(group_id_2, exec_time_limit_msec_2, exec_time_total_msec_2, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_create_group(group_id_3, exec_time_limit_msec_3, exec_time_total_msec_3, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_create_group(group_id_4, exec_time_limit_msec_4, exec_time_total_msec_4, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_create_group(group_id_5, exec_time_limit_msec_5, exec_time_total_msec_5, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // group find sorted by remaining time
  {
    rc = dbmgr_gi_group_find_all_sorted_by_remaining_time(&actual_gi_ptr_arr, &actual_arr_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(target_data_size, actual_arr_len);
  }

  // get group_id sorted by remaining time
  {
    for (size_t i = 0; i < target_data_size; i++) {
      rc = dbmgr_gi_get_group_id(actual_gi_ptr_arr[i], &actual_group_id);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      if (i == 0) {
        TEST_ASSERT_EQUAL_STRING(group_id_1, actual_group_id);
      } else if (i == 1) {
        TEST_ASSERT_EQUAL_STRING(group_id_2, actual_group_id);
      } else if (i == 2) {
        TEST_ASSERT_EQUAL_STRING(group_id_3, actual_group_id);
      } else if (i == 3) {
        TEST_ASSERT_EQUAL_STRING(group_id_4, actual_group_id);
      } else if (i == 4) {
        TEST_ASSERT_EQUAL_STRING(group_id_5, actual_group_id);
      } else {
        TEST_FAIL_MESSAGE("invalid group target");
      }

      free(actual_group_id);
      actual_group_id = NULL;
    }
  }

  // delete group
  {
    rc = dbmgr_gi_delete_group(group_id_1);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_delete_group(group_id_2);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_delete_group(group_id_3);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_delete_group(group_id_4);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_delete_group(group_id_5);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    free((void *) actual_gi_ptr_arr);
    actual_gi_ptr_arr = NULL;
  }

  // finalize
  s_dbmgr_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}


void test_dbmgr_gi_group_find_all_sorted_by_remaining_time2(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  const char *group_id_1 = "test-group1";
  const uint64_t exec_time_limit_msec_1 = 987654321;
  const uint64_t exec_time_total_msec_1 = 567891234;

  const char *group_id_2 = "test-group2";
  const uint64_t exec_time_limit_msec_2 = 987654321;
  const uint64_t exec_time_total_msec_2 = 456789123;

  const char *group_id_3 = "test-group3";
  const uint64_t exec_time_limit_msec_3 = 987654321;
  const uint64_t exec_time_total_msec_3 = 345678912;

  const char *group_id_4 = "test-group4";
  const uint64_t exec_time_limit_msec_4 = 987654321;
  const uint64_t exec_time_total_msec_4 = 234567891;

  const char *group_id_5 = "test-group5";
  const uint64_t exec_time_limit_msec_5 = 987654321;
  const uint64_t exec_time_total_msec_5 = 123456789;

  const size_t target_data_size = 5;
  dbmgr_group_info_t *actual_gi_ptr_arr = NULL;
  size_t actual_arr_len;

  dbmgr_group_info_t gi_ptr = NULL;
  char *actual_group_id = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create group
  {
    rc = dbmgr_gi_create_group(group_id_1, exec_time_limit_msec_1, exec_time_total_msec_1, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_create_group(group_id_2, exec_time_limit_msec_2, exec_time_total_msec_2, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_create_group(group_id_3, exec_time_limit_msec_3, exec_time_total_msec_3, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_create_group(group_id_4, exec_time_limit_msec_4, exec_time_total_msec_4, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_create_group(group_id_5, exec_time_limit_msec_5, exec_time_total_msec_5, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // group find sorted by remaining time
  {
    rc = dbmgr_gi_group_find_all_sorted_by_remaining_time(&actual_gi_ptr_arr, &actual_arr_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(target_data_size, actual_arr_len);
  }

  // get group_id sorted by remaining time
  {
    for (size_t i = 0; i < target_data_size; i++) {
      rc = dbmgr_gi_get_group_id(actual_gi_ptr_arr[i], &actual_group_id);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      if (i == 0) {
        TEST_ASSERT_EQUAL_STRING(group_id_5, actual_group_id);
      } else if (i == 1) {
        TEST_ASSERT_EQUAL_STRING(group_id_4, actual_group_id);
      } else if (i == 2) {
        TEST_ASSERT_EQUAL_STRING(group_id_3, actual_group_id);
      } else if (i == 3) {
        TEST_ASSERT_EQUAL_STRING(group_id_2, actual_group_id);
      } else if (i == 4) {
        TEST_ASSERT_EQUAL_STRING(group_id_1, actual_group_id);
      } else {
        TEST_FAIL_MESSAGE("invalid group target");
      }

      free(actual_group_id);
      actual_group_id = NULL;
    }
  }

  // delete group
  {
    rc = dbmgr_gi_delete_group(group_id_1);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_delete_group(group_id_2);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_delete_group(group_id_3);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_delete_group(group_id_4);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_delete_group(group_id_5);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    free((void *) actual_gi_ptr_arr);
    actual_gi_ptr_arr = NULL;
  }

  // finalize
  s_dbmgr_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}


void test_dbmgr_gi_group_find_all_sorted_by_remaining_time3(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  const char *group_id_1 = "test-group1";
  const uint64_t exec_time_limit_msec_1 = 987654321;
  const uint64_t exec_time_total_msec_1 = 345678912;

  const char *group_id_2 = "test-group2";
  const uint64_t exec_time_limit_msec_2 = 987654321;
  const uint64_t exec_time_total_msec_2 = 456789123;

  const char *group_id_3 = "test-group3";
  const uint64_t exec_time_limit_msec_3 = 987654321;
  const uint64_t exec_time_total_msec_3 = 567891234;

  const char *group_id_4 = "test-group4";
  const uint64_t exec_time_limit_msec_4 = 987654321;
  const uint64_t exec_time_total_msec_4 = 123456789;

  const char *group_id_5 = "test-group5";
  const uint64_t exec_time_limit_msec_5 = 987654321;
  const uint64_t exec_time_total_msec_5 = 234567891;

  const size_t target_data_size = 5;
  dbmgr_group_info_t *actual_gi_ptr_arr = NULL;
  size_t actual_arr_len;

  dbmgr_group_info_t gi_ptr = NULL;
  char *actual_group_id = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create group
  {
    rc = dbmgr_gi_create_group(group_id_1, exec_time_limit_msec_1, exec_time_total_msec_1, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_create_group(group_id_2, exec_time_limit_msec_2, exec_time_total_msec_2, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_create_group(group_id_3, exec_time_limit_msec_3, exec_time_total_msec_3, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_create_group(group_id_4, exec_time_limit_msec_4, exec_time_total_msec_4, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_create_group(group_id_5, exec_time_limit_msec_5, exec_time_total_msec_5, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // group find sorted by remaining time
  {
    rc = dbmgr_gi_group_find_all_sorted_by_remaining_time(&actual_gi_ptr_arr, &actual_arr_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(target_data_size, actual_arr_len);
  }

  // get group_id sorted by remaining time
  {
    for (size_t i = 0; i < target_data_size; i++) {
      rc = dbmgr_gi_get_group_id(actual_gi_ptr_arr[i], &actual_group_id);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      if (i == 0) {
        TEST_ASSERT_EQUAL_STRING(group_id_4, actual_group_id);
      } else if (i == 1) {
        TEST_ASSERT_EQUAL_STRING(group_id_5, actual_group_id);
      } else if (i == 2) {
        TEST_ASSERT_EQUAL_STRING(group_id_1, actual_group_id);
      } else if (i == 3) {
        TEST_ASSERT_EQUAL_STRING(group_id_2, actual_group_id);
      } else if (i == 4) {
        TEST_ASSERT_EQUAL_STRING(group_id_3, actual_group_id);
      } else {
        TEST_FAIL_MESSAGE("invalid group target");
      }

      free(actual_group_id);
      actual_group_id = NULL;
    }
  }

  // delete group
  {
    rc = dbmgr_gi_delete_group(group_id_1);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_delete_group(group_id_2);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_delete_group(group_id_3);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_delete_group(group_id_4);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_delete_group(group_id_5);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    free((void *) actual_gi_ptr_arr);
    actual_gi_ptr_arr = NULL;
  }

  // finalize
  s_dbmgr_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}


void test_dbmgr_gi_get_group_id(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-group";
  const size_t group_id_len = strlen(group_id);
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  dbmgr_group_info_t gi_ptr = NULL;
  char *actual_group_id = NULL;
  size_t actual_group_id_len = 0;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create group
  {
    rc = dbmgr_gi_create_group(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get group_id
  {
    rc = dbmgr_gi_get_group_id(gi_ptr, &actual_group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(group_id, actual_group_id);

    rc = dbmgr_gi_get_group_id_len(gi_ptr, &actual_group_id_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(group_id_len, actual_group_id_len);

    free((void *) actual_group_id);
    actual_group_id = NULL;
  }

  // delete group
  {
    rc = dbmgr_gi_delete_group(group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_gi_set_get_exec_time_limit_msec(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-group";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  const uint64_t new_exec_time_limit_msec = 567891234;
  dbmgr_group_info_t gi_ptr = NULL;
  uint64_t actual_exec_time_limit_msec = 0;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create group
  {
    rc = dbmgr_gi_create_group(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get exec_time_limit_msec
  {
    rc = dbmgr_gi_get_exec_time_limit_msec(gi_ptr, &actual_exec_time_limit_msec);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(exec_time_limit_msec, actual_exec_time_limit_msec);
  }

  // set exec_time_limit_msec
  {
    rc = dbmgr_gi_set_exec_time_limit_msec(gi_ptr, new_exec_time_limit_msec);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_get_exec_time_limit_msec(gi_ptr, &actual_exec_time_limit_msec);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(new_exec_time_limit_msec, actual_exec_time_limit_msec);
  }

  // delete group
  {
    rc = dbmgr_gi_delete_group(group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_gi_add_exec_time_total_msec(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-group";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  const uint64_t exec_time_msec = 176543211;
  dbmgr_group_info_t gi_ptr = NULL;
  uint64_t actual_exec_time_total_msec = 0;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create group
  {
    rc = dbmgr_gi_create_group(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get exec_time_total_msec
  {
    rc = dbmgr_gi_get_exec_time_total_msec(gi_ptr, &actual_exec_time_total_msec);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(exec_time_total_msec, actual_exec_time_total_msec);
  }

  // add exec_time_total_msec
  {
    rc = dbmgr_gi_add_exec_time_total_msec(gi_ptr, exec_time_msec);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_get_exec_time_total_msec(gi_ptr, &actual_exec_time_total_msec);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(exec_time_total_msec + exec_time_msec, actual_exec_time_total_msec);
  }

  // add exec_time_total_msec(exec_time_total_msec + exec_time_msec > UINT64_MAX)
  {
    rc = dbmgr_gi_add_exec_time_total_msec(gi_ptr, UINT64_MAX);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_get_exec_time_total_msec(gi_ptr, &actual_exec_time_total_msec);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(UINT64_MAX, actual_exec_time_total_msec);
  }

  // delete group
  {
    rc = dbmgr_gi_delete_group(group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_gi_subtract_exec_time_total_msec(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-group";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  const uint64_t exec_time_msec = 23456789;
  dbmgr_group_info_t gi_ptr = NULL;
  uint64_t actual_exec_time_total_msec = 0;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create group
  {
    rc = dbmgr_gi_create_group(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get exec_time_total_msec
  {
    rc = dbmgr_gi_get_exec_time_total_msec(gi_ptr, &actual_exec_time_total_msec);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(exec_time_total_msec, actual_exec_time_total_msec);
  }

  // subtract exec_time_total_msec
  {
    rc = dbmgr_gi_subtract_exec_time_total_msec(gi_ptr, exec_time_msec);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_get_exec_time_total_msec(gi_ptr, &actual_exec_time_total_msec);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(exec_time_total_msec - exec_time_msec, actual_exec_time_total_msec);
  }

  // subtract exec_time_total_msec(exec_time_msec > exec_time_total_msec)
  {
    rc = dbmgr_gi_subtract_exec_time_total_msec(gi_ptr, UINT64_MAX);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_gi_get_exec_time_total_msec(gi_ptr, &actual_exec_time_total_msec);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_exec_time_total_msec);
  }

  // delete group
  {
    rc = dbmgr_gi_delete_group(group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_gi_get_created_time(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-group";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  dbmgr_group_info_t gi_ptr = NULL;
  sqc_chrono_t actual_created_time;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create user
  {
    rc = dbmgr_gi_create_group(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get created time
  {
    rc = dbmgr_gi_get_created_time(gi_ptr, &actual_created_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_LESS_OR_EQUAL_UINT64(sqc_chrono_now(), actual_created_time);
  }

  // delete user
  {
    rc = dbmgr_gi_delete_group(group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_dbmgr_gi_get_update_time(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-group";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  dbmgr_group_info_t gi_ptr = NULL;
  sqc_chrono_t actual_update_time;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create user
  {
    rc = dbmgr_gi_create_group(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get created time
  {
    rc = dbmgr_gi_get_update_time(gi_ptr, &actual_update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_LESS_OR_EQUAL_UINT64(sqc_chrono_now(), actual_update_time);
  }

  // delete user
  {
    rc = dbmgr_gi_delete_group(group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}


/*
 * Negative test cases for private methods
 */

void test_negative_s_dbmgr_group_info_record_create(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-group";
  const char *excess_group_id = "01234567890123456789012345678901234567890123456789" \
                                "01234567890123456789012345678901234567890123456789" \
                                "01234567890123456789012345678901234567890123456789" \
                                "01234567890123456789012345678901234567890123456789" \
                                "01234567890123456789012345678901234567890123456789" \
                                "0123456";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  dbmgr_group_info_t gi_ptr = NULL;

  // invalid group_id
  {
    rc = s_dbmgr_group_info_record_create(excess_group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_TOO_LONG, rc);
    TEST_ASSERT_NULL(gi_ptr);

    rc = s_dbmgr_group_info_record_create("", exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(gi_ptr);

    rc = s_dbmgr_group_info_record_create(NULL, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(gi_ptr);
  }

  // invalid record
  {
    rc = s_dbmgr_group_info_record_create(group_id, exec_time_limit_msec, exec_time_total_msec, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }
}

void test_negative_s_dbmgr_group_info_record_destroy(void) {
  s_dbmgr_group_info_record_destroy(NULL);
}

void test_negative_s_dbmgr_group_info_record_add(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-group";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  dbmgr_group_info_t gi_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_group_info_record_create(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // not started
  {
    rc = s_dbmgr_group_info_record_add(gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_STARTED, rc);
  }

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = s_dbmgr_group_info_record_add(NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_group_info_record_destroy(gi_ptr);
    gi_ptr = NULL;
  }

  // finalize
  s_dbmgr_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_negative_s_dbmgr_group_info_record_delete(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-group";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  dbmgr_group_info_t gi_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_group_info_record_create(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // not started
  {
    rc = s_dbmgr_group_info_record_delete(gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_STARTED, rc);
  }

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = s_dbmgr_group_info_record_delete(NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_group_info_record_destroy(gi_ptr);
    gi_ptr = NULL;
  }

  // finalize
  s_dbmgr_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

/*
 * Negative test cases for public methods
 */

void test_negative_dbmgr_gi_create_group(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-user";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  dbmgr_group_info_t gi_ptr = NULL;

  // invalid group_id
  {
    rc = dbmgr_gi_create_group("", exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(gi_ptr);

    rc = dbmgr_gi_create_group(NULL, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(gi_ptr);
  }

  // invalid record
  {
    rc = dbmgr_gi_create_group(group_id, exec_time_limit_msec, exec_time_total_msec, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }
}

void test_negative_dbmgr_gi_delete_group(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-user";

  // not started
  {
    rc = dbmgr_gi_delete_group(group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_STARTED, rc);
  }

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid group_id
  {
    rc = dbmgr_gi_delete_group("");
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);

    rc = dbmgr_gi_delete_group(NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // finalize
  s_dbmgr_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_negative_dbmgr_gi_group_exists(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-user";

  // not started
  {
    TEST_ASSERT_FALSE(dbmgr_gi_group_exists(group_id));
  }

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid group_id
  {
    TEST_ASSERT_FALSE(dbmgr_gi_group_exists(""));

    TEST_ASSERT_FALSE(dbmgr_gi_group_exists(NULL));
  }

  // finalize
  s_dbmgr_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_negative_dbmgr_gi_group_find(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-user";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  dbmgr_group_info_t gi_ptr = NULL;
  dbmgr_group_info_t actual_gi_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_group_info_record_create(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // not started
  {
    rc = s_dbmgr_group_info_record_find(group_id, &actual_gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_STARTED, rc);
    TEST_ASSERT_NULL(actual_gi_ptr);
  }

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_group_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // non-existent group_id
  {
    rc = dbmgr_gi_group_find("non-existent-group", &actual_gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_FOUND, rc);
    TEST_ASSERT_NULL(actual_gi_ptr);
  }

  // invalid group_id
  {
    rc = dbmgr_gi_group_find("", &actual_gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(actual_gi_ptr);

    rc = s_dbmgr_group_info_record_find(NULL, &actual_gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(actual_gi_ptr);
  }

  // invalid record
  {
    rc = dbmgr_gi_group_find(group_id, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_group_info_record_destroy(gi_ptr);
    gi_ptr = NULL;
  }

  // finalize
  s_dbmgr_group_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_negative_dbmgr_gi_get_group_id(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-user";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  dbmgr_group_info_t gi_ptr = NULL;
  char *actual_group_id = NULL;

  // create record
  {
    rc = s_dbmgr_group_info_record_create(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_gi_get_group_id(NULL, &actual_group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid group_id
  {
    rc = dbmgr_gi_get_group_id(gi_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_group_info_record_destroy(gi_ptr);
    gi_ptr = NULL;
  }
}

void test_negative_dbmgr_gi_get_group_id_len(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-user";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  dbmgr_group_info_t gi_ptr = NULL;
  size_t actual_group_id_len = 0;

  // create record
  {
    rc = s_dbmgr_group_info_record_create(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_gi_get_group_id_len(NULL, &actual_group_id_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid group_id_len
  {
    rc = dbmgr_gi_get_group_id_len(gi_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_group_info_record_destroy(gi_ptr);
    gi_ptr = NULL;
  }
}

void test_negative_dbmgr_gi_set_exec_time_limit_msec(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-user";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  const uint64_t new_exec_time_limit_msec = 176543211;
  dbmgr_group_info_t gi_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_group_info_record_create(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_gi_set_exec_time_limit_msec(NULL, new_exec_time_limit_msec);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_group_info_record_destroy(gi_ptr);
    gi_ptr = NULL;
  }
}

void test_negative_dbmgr_gi_get_exec_time_limit_msec(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-user";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  dbmgr_group_info_t gi_ptr = NULL;
  uint64_t actual_exec_time_limit_msec = 0;

  // create record
  {
    rc = s_dbmgr_group_info_record_create(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_gi_get_exec_time_limit_msec(NULL, &actual_exec_time_limit_msec);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid exec_time_limit_msec
  {
    rc = dbmgr_gi_get_exec_time_limit_msec(gi_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_group_info_record_destroy(gi_ptr);
    gi_ptr = NULL;
  }
}

void test_negative_dbmgr_gi_get_exec_time_total_msec(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-user";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  dbmgr_group_info_t gi_ptr = NULL;
  uint64_t actual_exec_time_total_msec = 0;

  // create record
  {
    rc = s_dbmgr_group_info_record_create(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_gi_get_exec_time_total_msec(NULL, &actual_exec_time_total_msec);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid exec_time_total_msec
  {
    rc = dbmgr_gi_get_exec_time_total_msec(gi_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_group_info_record_destroy(gi_ptr);
    gi_ptr = NULL;
  }
}

void test_negative_dbmgr_gi_add_exec_time_total_msec(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-user";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  const uint64_t exec_time_msec = 176543211;
  dbmgr_group_info_t gi_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_group_info_record_create(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_gi_add_exec_time_total_msec(NULL, exec_time_msec);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_group_info_record_destroy(gi_ptr);
    gi_ptr = NULL;
  }
}

void test_negative_dbmgr_gi_subtract_exec_time_total_msec(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-user";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  const uint64_t exec_time_msec = 23456789;
  dbmgr_group_info_t gi_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_group_info_record_create(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_gi_subtract_exec_time_total_msec(NULL, exec_time_msec);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_group_info_record_destroy(gi_ptr);
    gi_ptr = NULL;
  }
}

void test_negative_dbmgr_gi_get_created_time(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-user";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  dbmgr_group_info_t gi_ptr = NULL;
  sqc_chrono_t actual_created_time;

  // create record
  {
    rc = s_dbmgr_group_info_record_create(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_gi_get_created_time(NULL, &actual_created_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid created_time
  {
    rc = dbmgr_gi_get_created_time(gi_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_group_info_record_destroy(gi_ptr);
    gi_ptr = NULL;
  }
}

void test_negative_dbmgr_gi_get_update_time(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *group_id = "test-user";
  const uint64_t exec_time_limit_msec = 987654321;
  const uint64_t exec_time_total_msec = 123456789;
  dbmgr_group_info_t gi_ptr = NULL;
  sqc_chrono_t actual_update_time;

  // create record
  {
    rc = s_dbmgr_group_info_record_create(group_id, exec_time_limit_msec, exec_time_total_msec, &gi_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_gi_get_update_time(NULL, &actual_update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid update_time
  {
    rc = dbmgr_gi_get_update_time(gi_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_group_info_record_destroy(gi_ptr);
    gi_ptr = NULL;
  }
}

