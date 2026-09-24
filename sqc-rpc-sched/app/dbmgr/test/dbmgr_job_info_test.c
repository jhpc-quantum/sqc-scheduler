#include <sys/stat.h>
#include <unistd.h>

#include "unity.h"

#include "dbmgr_util.c"
#include "dbmgr_db.c"
#include "dbmgr_job_info.c"

#include "dbmgr_db_common.c"

void setUp(void) {
  remove_db_files("setUp - job_info");
  create_db_files("setUp - job_info");
}

void tearDown(void) {
  remove_db_files("tearDown - job_info");
}

void test_job_info_initialize_finalize(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(job_info_hashmap);
  }

  // finalize
  {
    s_dbmgr_job_info_finalize();
    TEST_ASSERT_NULL(job_info_hashmap);

    s_dbmgr_db_finalize();
    TEST_ASSERT_NULL(sqc_rpc_sched_db);
  }
}

void test_job_info_free_in_finalize(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create & add record(free in finalize)
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = s_dbmgr_job_info_record_add(ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_TRUE(dbmgr_ji_job_exists(ji_ptr->job_id));
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_job_info_record_create_destroy(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const size_t user_id_len = strlen(user_id);
  const char *qprogram = "test-qprogram-data";
  const size_t qprogram_len = strlen(qprogram);
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const size_t remark_len = strlen(remark);
  const char *user_token = "test-user_token-data";
  const size_t user_token_len = strlen(user_token);
  dbmgr_job_info_t ji_ptr = NULL;
  const sqc_rpc_sched_job_status_t expected_status = SQC_RPC_SCHED_JOB_STATUS_CREATED;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_ID_MAX_SIZE, strlen(ji_ptr->job_id));
    TEST_ASSERT_EQUAL_STRING(user_id, ji_ptr->user_id);
    TEST_ASSERT_EQUAL(user_id_len, ji_ptr->user_id_len);
    TEST_ASSERT_EQUAL(priority, ji_ptr->priority);
    TEST_ASSERT_EQUAL(expected_status, ji_ptr->status);
    TEST_ASSERT_EQUAL_STRING(qprogram, ji_ptr->qprogram);
    TEST_ASSERT_EQUAL(qprogram_len, ji_ptr->qprogram_len);
    TEST_ASSERT_EQUAL(circuit_fmt, ji_ptr->circuit_fmt);
    TEST_ASSERT_EQUAL(shots, ji_ptr->shots);
    TEST_ASSERT_EQUAL(qc_type, ji_ptr->qc_type);
    TEST_ASSERT_EQUAL(transpiler, ji_ptr->transpiler);
    TEST_ASSERT_EQUAL_STRING(remark, ji_ptr->remark);
    TEST_ASSERT_EQUAL(remark_len, ji_ptr->remark_len);
    TEST_ASSERT_EQUAL_STRING(user_token, ji_ptr->user_token);
    TEST_ASSERT_EQUAL(user_token_len, ji_ptr->user_token_len);
    TEST_ASSERT_EQUAL(0, strlen(ji_ptr->qc_job_id));
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

void test_job_info_record_add_delete(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  char *actual_job_id = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_ji_get_job_id(ji_ptr, &actual_job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // add record
  {
    rc = s_dbmgr_job_info_record_add(ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_TRUE(dbmgr_ji_job_exists(actual_job_id));
  }

  // delete record
  {
    rc = s_dbmgr_job_info_record_delete(ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_FALSE(dbmgr_ji_job_exists(actual_job_id));
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);

  free((void *) actual_job_id);
  actual_job_id = NULL;
}

void test_ji_job_create_delete(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_job_exists(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // job exists
  {
    TEST_ASSERT_TRUE(dbmgr_ji_job_exists(ji_ptr->job_id));
    TEST_ASSERT_FALSE(dbmgr_ji_job_exists("non-existent-job"));
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_is_deletable(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  const size_t target_data_size = 7;

  const char *user_id = "test-user";
  const char *group_id = "test-group";

  dbmgr_job_info_t ji_ptr = NULL;

  sqc_rpc_sched_job_status_t actual_status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    // create target job
    for (size_t i = 0; i < target_data_size; i++) {
      rc = dbmgr_ji_create_job(user_id, group_id, 5,
                               "test-qprogram-data", SQC_RPC_SCHED_CIRCUIT_FMT_QIR, 30000 + i,
                               SQC_RPC_SCHED_QC_TYPE_RQC_REST, SQC_RPC_SCHED_TRANSPILER_NORMAL,
                               "test-remark-data", "test-user_token-data", &ji_ptr);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      if (i == 0) {  // SQC_RPC_SCHED_JOB_STATUS_CREATED
        // do nothing

        rc = dbmgr_ji_get_status(ji_ptr, &actual_status);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
        TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_CREATED, actual_status);

        TEST_ASSERT_FALSE(dbmgr_ji_is_deletable(ji_ptr));
      } else if (i == 1) {  // SQC_RPC_SCHED_JOB_STATUS_QUEUED
        rc = dbmgr_set_job_status_queued(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_ji_get_status(ji_ptr, &actual_status);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
        TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_QUEUED, actual_status);

        TEST_ASSERT_FALSE(dbmgr_ji_is_deletable(ji_ptr));
      } else if (i == 2) {  // SQC_RPC_SCHED_JOB_STATUS_RUNNING
        rc = dbmgr_set_job_status_queued(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_set_job_status_running(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_ji_get_status(ji_ptr, &actual_status);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
        TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_RUNNING, actual_status);

        TEST_ASSERT_FALSE(dbmgr_ji_is_deletable(ji_ptr));
      } else if (i == 3) {  // SQC_RPC_SCHED_JOB_STATUS_DONE
        rc = dbmgr_set_job_status_queued(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_set_job_status_running(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_set_job_status_done(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_ji_get_status(ji_ptr, &actual_status);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
        TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_DONE, actual_status);

        TEST_ASSERT_TRUE(dbmgr_ji_is_deletable(ji_ptr));
      } else if (i == 4) {  // SQC_RPC_SCHED_JOB_STATUS_CANCELLED
        rc = dbmgr_set_job_status_cancelled(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_ji_get_status(ji_ptr, &actual_status);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
        TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_CANCELLED, actual_status);

        TEST_ASSERT_TRUE(dbmgr_ji_is_deletable(ji_ptr));
      } else if (i == 5) {  // SQC_RPC_SCHED_JOB_STATUS_ERROR
        rc = dbmgr_set_job_status_error(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_ji_get_status(ji_ptr, &actual_status);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
        TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_ERROR, actual_status);

        TEST_ASSERT_TRUE(dbmgr_ji_is_deletable(ji_ptr));
      } else if (i == 6) {  // SQC_RPC_SCHED_JOB_STATUS_DELETED
        rc = dbmgr_set_job_status_cancelled(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_set_job_status_deleted(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_ji_get_status(ji_ptr, &actual_status);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
        TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_DELETED, actual_status);

        TEST_ASSERT_FALSE(dbmgr_ji_is_deletable(ji_ptr));
      }

      rc = dbmgr_ji_delete_job(ji_ptr->job_id);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    }
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_job_find(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  dbmgr_job_info_t actual_ji_ptr = NULL;
  char *actual_job_id = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // job find
  {
    rc = dbmgr_ji_job_find(ji_ptr->job_id, &actual_ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get job_id
  {
    rc = dbmgr_ji_get_job_id(ji_ptr, &actual_job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(ji_ptr->job_id, actual_job_id);

    free((void *) actual_job_id);
    actual_job_id = NULL;
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_job_find_by_user_id(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  const size_t target_data_size = 10;
  dbmgr_job_info_t target_job_info_arr[target_data_size];

  const size_t other_data_size = 5;
  dbmgr_job_info_t other_job_info_arr[other_data_size];

  const char *target_user_id = "test-user";
  const char *other_user_id = "other-user";

  const char *group_id = "test-group";

  dbmgr_job_info_t ji_ptr = NULL;

  dbmgr_job_info_t *actual_ji_ptr_arr = NULL;
  size_t actual_arr_len;
  size_t actual_shots;
  char *actual_user_id = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    // create target job
    for (size_t i = 0; i < target_data_size; i++) {
      rc = dbmgr_ji_create_job(target_user_id, group_id, 5,
                               "test-qprogram-data", SQC_RPC_SCHED_CIRCUIT_FMT_QIR, 30000 + i,
                               SQC_RPC_SCHED_QC_TYPE_RQC_REST, SQC_RPC_SCHED_TRANSPILER_NORMAL,
                               "test-remark-data", "test-user_token-data", &ji_ptr);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      target_job_info_arr[i] = ji_ptr;
      ji_ptr = NULL;
    }

    // create other job
    for (size_t i = 0; i < other_data_size; i++) {
      rc = dbmgr_ji_create_job(other_user_id, group_id, 1,
                               "other-qprogram-data", SQC_RPC_SCHED_CIRCUIT_FMT_QIR, 50000 + i,
                               SQC_RPC_SCHED_QC_TYPE_RQC_REST, SQC_RPC_SCHED_TRANSPILER_NORMAL,
                               "other-remark-data", "other-user_token-data", &ji_ptr);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      other_job_info_arr[i] = ji_ptr;
      ji_ptr = NULL;
    }
  }

  // job find by user_id
  {
    rc = dbmgr_ji_job_find_by_user_id(target_user_id, &actual_ji_ptr_arr, &actual_arr_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(target_data_size, actual_arr_len);
  }

  // get job
  {
    for (size_t i = 0; i < target_data_size; i++) {
      rc = dbmgr_ji_get_user_id(actual_ji_ptr_arr[i], &actual_user_id);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
      TEST_ASSERT_EQUAL_STRING(target_user_id, actual_user_id);

      rc = dbmgr_ji_get_shots(actual_ji_ptr_arr[i], &actual_shots);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
      TEST_ASSERT_INT_WITHIN(5, 30005, actual_shots); // 30000 - 30010

      free((void *) actual_user_id);
      actual_user_id = NULL;
    }
  }

  // delete job
  {
    for (size_t i = 0; i < target_data_size; i++) {
      rc = dbmgr_ji_delete_job(target_job_info_arr[i]->job_id);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    }

    for (size_t i = 0; i < other_data_size; i++) {
      rc = dbmgr_ji_delete_job(other_job_info_arr[i]->job_id);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    }

    free((void *) actual_ji_ptr_arr);
    actual_ji_ptr_arr = NULL;
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_job_find_by_job_status(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  const size_t target_data_size = 10;
  dbmgr_job_info_t target_job_info_arr[target_data_size];

  const size_t other_data_size = 5;
  dbmgr_job_info_t other_job_info_arr[other_data_size];

  const sqc_rpc_sched_job_status_t target_status = SQC_RPC_SCHED_JOB_STATUS_QUEUED;

  const char *user_id = "test-user";
  const char *group_id = "test-group";

  dbmgr_job_info_t ji_ptr = NULL;

  dbmgr_job_info_t *actual_ji_ptr_arr = NULL;
  size_t actual_arr_len;
  size_t actual_shots;
  char *actual_user_id = NULL;
  sqc_rpc_sched_job_status_t actual_status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    // create target job
    for (size_t i = 0; i < target_data_size; i++) {
      rc = dbmgr_ji_create_job(user_id, group_id, 5,
                               "test-qprogram-data", SQC_RPC_SCHED_CIRCUIT_FMT_QIR, 30000 + i,
                               SQC_RPC_SCHED_QC_TYPE_RQC_REST, SQC_RPC_SCHED_TRANSPILER_NORMAL,
                               "test-remark-data", "test-user_token-data", &ji_ptr);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      rc = dbmgr_set_job_status_queued(ji_ptr);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      target_job_info_arr[i] = ji_ptr;
      ji_ptr = NULL;
    }

    // create other job
    for (size_t i = 0; i < other_data_size; i++) {
      rc = dbmgr_ji_create_job(user_id, group_id, 1,
                               "other-qprogram-data", SQC_RPC_SCHED_CIRCUIT_FMT_QIR, 50000 + i,
                               SQC_RPC_SCHED_QC_TYPE_RQC_REST, SQC_RPC_SCHED_TRANSPILER_NORMAL,
                               "other-remark-data", "other-user_token-data", &ji_ptr);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      if (i == 0) {  // SQC_RPC_SCHED_JOB_STATUS_CREATED
        // do nothing

        rc = dbmgr_ji_get_status(ji_ptr, &actual_status);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
        TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_CREATED, actual_status);
      } else if (i == 1) {  // SQC_RPC_SCHED_JOB_STATUS_RUNNING
        rc = dbmgr_set_job_status_queued(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_set_job_status_running(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_ji_get_status(ji_ptr, &actual_status);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
        TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_RUNNING, actual_status);
      } else if (i == 2) {  // SQC_RPC_SCHED_JOB_STATUS_DONE
        rc = dbmgr_set_job_status_queued(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_set_job_status_running(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_set_job_status_done(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_ji_get_status(ji_ptr, &actual_status);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
        TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_DONE, actual_status);
      } else if (i == 3) {  // SQC_RPC_SCHED_JOB_STATUS_CANCELLED
        rc = dbmgr_set_job_status_cancelled(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_ji_get_status(ji_ptr, &actual_status);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
        TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_CANCELLED, actual_status);
      } else if (i == 4) {  // SQC_RPC_SCHED_JOB_STATUS_ERROR
        rc = dbmgr_set_job_status_error(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_ji_get_status(ji_ptr, &actual_status);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
        TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_ERROR, actual_status);
      } else if (i == 5) {  // SQC_RPC_SCHED_JOB_STATUS_DELETED
        rc = dbmgr_set_job_status_cancelled(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_set_job_status_deleted(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_ji_get_status(ji_ptr, &actual_status);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
        TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_DELETED, actual_status);
      }

      other_job_info_arr[i] = ji_ptr;
      ji_ptr = NULL;
    }
  }


  // job find by job status
  {
    rc = dbmgr_ji_job_find_by_job_status(target_status, &actual_ji_ptr_arr, &actual_arr_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(target_data_size, actual_arr_len);
  }

  // get job
  {
    for (size_t i = 0; i < target_data_size; i++) {
      rc = dbmgr_ji_get_user_id(actual_ji_ptr_arr[i], &actual_user_id);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
      TEST_ASSERT_EQUAL_STRING(user_id, actual_user_id);

      rc = dbmgr_ji_get_shots(actual_ji_ptr_arr[i], &actual_shots);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
      TEST_ASSERT_INT_WITHIN(5, 30005, actual_shots); // 30000 - 30010

      rc = dbmgr_ji_get_status(actual_ji_ptr_arr[i], &actual_status);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
      TEST_ASSERT_EQUAL(target_status, actual_status);

      free((void *) actual_user_id);
      actual_user_id = NULL;
    }
  }

  // delete job
  {
    for (size_t i = 0; i < target_data_size; i++) {
      rc = dbmgr_ji_delete_job(target_job_info_arr[i]->job_id);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    }

    for (size_t i = 0; i < other_data_size; i++) {
      rc = dbmgr_ji_delete_job(other_job_info_arr[i]->job_id);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    }

    free((void *) actual_ji_ptr_arr);
    actual_ji_ptr_arr = NULL;
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_job_find_by_created_time(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  const size_t target_data_size = 10;
  dbmgr_job_info_t target_job_info_arr[target_data_size];

  const size_t other_data_size = 5;
  dbmgr_job_info_t other_job_info_arr[other_data_size];

  const char *user_id = "test-user";
  const char *other_user_id = "other-user";

  const char *group_id = "test-group";

  dbmgr_job_info_t ji_ptr = NULL;

  sqc_chrono_t target_from_time = sqc_chrono_now();
  sqc_chrono_t target_to_time;

  dbmgr_job_info_t *actual_ji_ptr_arr = NULL;
  size_t actual_arr_len;
  char *actual_user_id = NULL;
  sqc_chrono_t actual_created_time;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    // create target job
    for (size_t i = 0; i < target_data_size; i++) {
      rc = dbmgr_ji_create_job(user_id, group_id, 5,
                               "test-qprogram-data", SQC_RPC_SCHED_CIRCUIT_FMT_QIR, 30000 + i,
                               SQC_RPC_SCHED_QC_TYPE_RQC_REST, SQC_RPC_SCHED_TRANSPILER_NORMAL,
                               "test-remark-data", "test-user_token-data", &ji_ptr);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      rc = dbmgr_set_job_status_queued(ji_ptr);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      target_job_info_arr[i] = ji_ptr;
      ji_ptr = NULL;
    }

    // Leave a gap between the creation times of the target job and other jobs.
    target_to_time = sqc_chrono_now();
    sleep(1);

    // create other job
    for (size_t i = 0; i < other_data_size; i++) {
      rc = dbmgr_ji_create_job(other_user_id, group_id, 1,
                               "other-qprogram-data", SQC_RPC_SCHED_CIRCUIT_FMT_QIR, 50000 + i,
                               SQC_RPC_SCHED_QC_TYPE_RQC_REST, SQC_RPC_SCHED_TRANSPILER_NORMAL,
                               "other-remark-data", "other-user_token-data", &ji_ptr);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      other_job_info_arr[i] = ji_ptr;
      ji_ptr = NULL;
    }
  }

  // job find by created_time
  {
    rc = dbmgr_ji_job_find_by_created_time(target_from_time, target_to_time, &actual_ji_ptr_arr, &actual_arr_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(target_data_size, actual_arr_len);
  }

  // get job
  {
    for (size_t i = 0; i < target_data_size; i++) {
      rc = dbmgr_ji_get_user_id(actual_ji_ptr_arr[i], &actual_user_id);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
      TEST_ASSERT_EQUAL_STRING(user_id, actual_user_id);

      rc = dbmgr_ji_get_created_time(actual_ji_ptr_arr[i], &actual_created_time);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
      TEST_ASSERT_GREATER_THAN(target_from_time, actual_created_time);
      TEST_ASSERT_LESS_THAN(target_to_time, actual_created_time);

      free((void *) actual_user_id);
      actual_user_id = NULL;
    }
  }

  // delete job
  {
    for (size_t i = 0; i < target_data_size; i++) {
      rc = dbmgr_ji_delete_job(target_job_info_arr[i]->job_id);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    }

    for (size_t i = 0; i < other_data_size; i++) {
      rc = dbmgr_ji_delete_job(other_job_info_arr[i]->job_id);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    }

    free((void *) actual_ji_ptr_arr);
    actual_ji_ptr_arr = NULL;
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_job_find_by_delete_target(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  const size_t target_data_size = 3;
  dbmgr_job_info_t target_job_info_arr[target_data_size];

  const size_t other_data_size = 3;
  dbmgr_job_info_t other_job_info_arr[other_data_size];

  const char *target_user_id = "test-user";
  const char *other_user_id = "other-user";

  const char *group_id = "test-group";

  dbmgr_job_info_t ji_ptr = NULL;

  sqc_chrono_t target_from_time = sqc_chrono_now();
  sqc_chrono_t target_to_time;

  dbmgr_job_info_t *actual_ji_ptr_arr = NULL;
  size_t actual_arr_len;
  char *actual_user_id = NULL;
  sqc_chrono_t actual_created_time;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    // create target job
    for (size_t i = 0; i < target_data_size; i++) {
      rc = dbmgr_ji_create_job(target_user_id, group_id, 5,
                               "test-qprogram-data", SQC_RPC_SCHED_CIRCUIT_FMT_QIR, 30000 + i,
                               SQC_RPC_SCHED_QC_TYPE_RQC_REST, SQC_RPC_SCHED_TRANSPILER_NORMAL,
                               "test-remark-data", "test-user_token-data", &ji_ptr);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      if (i == 0) {  // SQC_RPC_SCHED_JOB_STATUS_DONE
        rc = dbmgr_set_job_status_queued(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_set_job_status_running(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_set_job_status_done(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
      } else if (i == 1) {  // SQC_RPC_SCHED_JOB_STATUS_CANCELLED
        rc = dbmgr_set_job_status_cancelled(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
      } else if (i == 2) {  // SQC_RPC_SCHED_JOB_STATUS_ERROR
        rc = dbmgr_set_job_status_error(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
      }

      target_job_info_arr[i] = ji_ptr;
      ji_ptr = NULL;
    }

    // Leave a gap between the creation times of the target job and other jobs.
    target_to_time = sqc_chrono_now();
    sleep(1);

    // create other job
    for (size_t i = 0; i < other_data_size; i++) {
      rc = dbmgr_ji_create_job(other_user_id, group_id, 1,
                               "other-qprogram-data", SQC_RPC_SCHED_CIRCUIT_FMT_QIR, 50000 + i,
                               SQC_RPC_SCHED_QC_TYPE_RQC_REST, SQC_RPC_SCHED_TRANSPILER_NORMAL,
                               "other-remark-data", "other-user_token-data", &ji_ptr);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      if (i == 0) {  // SQC_RPC_SCHED_JOB_STATUS_CREATED
        // do nothing
      } else if (i == 1) {  // SQC_RPC_SCHED_JOB_STATUS_RUNNING
        rc = dbmgr_set_job_status_queued(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_set_job_status_running(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
      } else if (i == 2) {  // SQC_RPC_SCHED_JOB_STATUS_DELETED
        rc = dbmgr_set_job_status_queued(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_set_job_status_running(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_set_job_status_done(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_set_job_status_deleted(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
      }

      other_job_info_arr[i] = ji_ptr;
      ji_ptr = NULL;
    }
  }

  // job find by delete_target
  {
    rc = dbmgr_ji_job_find_by_delete_target(target_user_id, target_from_time, target_to_time,
                                            &actual_ji_ptr_arr, &actual_arr_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(target_data_size, actual_arr_len);
  }

  // get job
  {
    for (size_t i = 0; i < target_data_size; i++) {
      rc = dbmgr_ji_get_user_id(actual_ji_ptr_arr[i], &actual_user_id);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
      TEST_ASSERT_EQUAL_STRING(target_user_id, actual_user_id);

      rc = dbmgr_ji_get_created_time(actual_ji_ptr_arr[i], &actual_created_time);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
      TEST_ASSERT_GREATER_THAN(target_from_time, actual_created_time);
      TEST_ASSERT_LESS_THAN(target_to_time, actual_created_time);

      free((void *) actual_user_id);
      actual_user_id = NULL;
    }
  }

  // delete job
  {
    for (size_t i = 0; i < target_data_size; i++) {
      rc = dbmgr_ji_delete_job(target_job_info_arr[i]->job_id);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    }

    for (size_t i = 0; i < other_data_size; i++) {
      rc = dbmgr_ji_delete_job(other_job_info_arr[i]->job_id);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    }

    free((void *) actual_ji_ptr_arr);
    actual_ji_ptr_arr = NULL;
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_job_find_by_delete_target_all(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  const size_t target_data_size = 3;
  dbmgr_job_info_t target_job_info_arr[target_data_size];

  const size_t other_data_size = 3;
  dbmgr_job_info_t other_job_info_arr[other_data_size];

  const char *target_user_id_arr[] = {
      "test-user1",
      "test-user2",
      "test-user3"
  };
  const char *other_user_id = "other-user";
  const size_t target_base_shots = 30000;
  const size_t other_shots = 50000;

  const char *group_id = "test-group";

  dbmgr_job_info_t ji_ptr = NULL;

  sqc_chrono_t target_from_time = sqc_chrono_now();
  sqc_chrono_t target_to_time;

  dbmgr_job_info_t *actual_ji_ptr_arr = NULL;
  size_t actual_arr_len;
  char *actual_user_id = NULL;
  size_t actual_shots;
  sqc_chrono_t actual_created_time;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    // create target job
    for (size_t i = 0; i < target_data_size; i++) {
      rc = dbmgr_ji_create_job(target_user_id_arr[i], group_id, 5,
                               "test-qprogram-data", SQC_RPC_SCHED_CIRCUIT_FMT_QIR, target_base_shots + i,
                               SQC_RPC_SCHED_QC_TYPE_RQC_REST, SQC_RPC_SCHED_TRANSPILER_NORMAL,
                               "test-remark-data", "test-user_token-data", &ji_ptr);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      if (i == 0) {  // SQC_RPC_SCHED_JOB_STATUS_DONE
        rc = dbmgr_set_job_status_queued(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_set_job_status_running(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_set_job_status_done(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
      } else if (i == 1) {  // SQC_RPC_SCHED_JOB_STATUS_CANCELLED
        rc = dbmgr_set_job_status_cancelled(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
      } else if (i == 2) {  // SQC_RPC_SCHED_JOB_STATUS_ERROR
        rc = dbmgr_set_job_status_error(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
      }

      target_job_info_arr[i] = ji_ptr;
      ji_ptr = NULL;
    }

    // Leave a gap between the creation times of the target job and other jobs.
    target_to_time = sqc_chrono_now();
    sleep(1);

    // create other job
    for (size_t i = 0; i < other_data_size; i++) {
      rc = dbmgr_ji_create_job(other_user_id, group_id, 1,
                               "other-qprogram-data", SQC_RPC_SCHED_CIRCUIT_FMT_QIR, other_shots + i,
                               SQC_RPC_SCHED_QC_TYPE_RQC_REST, SQC_RPC_SCHED_TRANSPILER_NORMAL,
                               "other-remark-data", "other-user_token-data", &ji_ptr);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      if (i == 0) {  // SQC_RPC_SCHED_JOB_STATUS_CREATED
        // do nothing
      } else if (i == 1) {  // SQC_RPC_SCHED_JOB_STATUS_RUNNING
        rc = dbmgr_set_job_status_queued(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_set_job_status_running(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
      } else if (i == 2) {  // SQC_RPC_SCHED_JOB_STATUS_DELETED
        rc = dbmgr_set_job_status_queued(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_set_job_status_running(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_set_job_status_done(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

        rc = dbmgr_set_job_status_deleted(ji_ptr);
        TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
      }

      other_job_info_arr[i] = ji_ptr;
      ji_ptr = NULL;
    }
  }

  // job find by delete_target(ALL)
  {
    rc = dbmgr_ji_job_find_by_delete_target(NULL, target_from_time, target_to_time,
                                            &actual_ji_ptr_arr, &actual_arr_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(target_data_size, actual_arr_len);
  }

  // get job
  {
    for (size_t i = 0; i < target_data_size; i++) {
      rc = dbmgr_ji_get_shots(actual_ji_ptr_arr[i], &actual_shots);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      rc = dbmgr_ji_get_user_id(actual_ji_ptr_arr[i], &actual_user_id);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
      // target data: user_id -> test-userX, shots: 3000X
      TEST_ASSERT_EQUAL_STRING(target_user_id_arr[actual_shots - target_base_shots], actual_user_id);

      rc = dbmgr_ji_get_created_time(actual_ji_ptr_arr[i], &actual_created_time);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
      TEST_ASSERT_GREATER_THAN(target_from_time, actual_created_time);
      TEST_ASSERT_LESS_THAN(target_to_time, actual_created_time);

      free((void *) actual_user_id);
      actual_user_id = NULL;
    }
  }

  // delete job
  {
    for (size_t i = 0; i < target_data_size; i++) {
      rc = dbmgr_ji_delete_job(target_job_info_arr[i]->job_id);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    }

    for (size_t i = 0; i < other_data_size; i++) {
      rc = dbmgr_ji_delete_job(other_job_info_arr[i]->job_id);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    }

    free((void *) actual_ji_ptr_arr);
    actual_ji_ptr_arr = NULL;
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_get_job_id(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  char *actual_job_id = NULL;
  size_t actual_job_id_len = 0;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get job_id
  {
    rc = dbmgr_ji_get_job_id(ji_ptr, &actual_job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(ji_ptr->job_id, actual_job_id);

    rc = dbmgr_ji_get_job_id_len(ji_ptr, &actual_job_id_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_ID_MAX_SIZE, actual_job_id_len);

    free((void *) actual_job_id);
    actual_job_id = NULL;
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_get_user_id(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  char *actual_user_id = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get user_id
  {
    rc = dbmgr_ji_get_user_id(ji_ptr, &actual_user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(user_id, actual_user_id);

    free((void *) actual_user_id);
    actual_user_id = NULL;
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_get_user_id_len(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const size_t user_id_len = strlen(user_id);
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  size_t actual_user_id_len = 0;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get user_id_len
  {
    rc = dbmgr_ji_get_user_id_len(ji_ptr, &actual_user_id_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(user_id_len, actual_user_id_len);
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_get_group_id(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  char *actual_group_id = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get group_id
  {
    rc = dbmgr_ji_get_group_id(ji_ptr, &actual_group_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(group_id, actual_group_id);

    free((void *) actual_group_id);
    actual_group_id = NULL;
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_get_group_id_len(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const size_t group_id_len = strlen(group_id);
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  size_t actual_group_id_len = 0;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get group_id_len
  {
    rc = dbmgr_ji_get_group_id_len(ji_ptr, &actual_group_id_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(group_id_len, actual_group_id_len);
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_get_priority(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  uint8_t actual_priority = 0;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get priority
  {
    rc = dbmgr_ji_get_priority(ji_ptr, &actual_priority);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(priority, actual_priority);
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_set_get_status(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  sqc_rpc_sched_job_status_t actual_status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;
  sqc_chrono_t actual_queued_time = 0;
  sqc_chrono_t actual_running_time = 0;
  sqc_chrono_t actual_done_time = 0;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get status
  {
    rc = dbmgr_ji_get_status(ji_ptr, &actual_status);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_CREATED, actual_status);
  }

  // get status
  {
    // to queued
    rc = dbmgr_set_job_status_queued(ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    rc = dbmgr_ji_get_status(ji_ptr, &actual_status);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_QUEUED, actual_status);
    rc = dbmgr_ji_get_queued_time(ji_ptr, &actual_queued_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_GREATER_THAN(0, actual_queued_time);

    // to running
    rc = dbmgr_set_job_status_running(ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    rc = dbmgr_ji_get_status(ji_ptr, &actual_status);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_RUNNING, actual_status);
    rc = dbmgr_ji_get_running_time(ji_ptr, &actual_running_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_GREATER_THAN(0, actual_running_time);

    // to done
    rc = dbmgr_set_job_status_done(ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    rc = dbmgr_ji_get_status(ji_ptr, &actual_status);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_DONE, actual_status);
    rc = dbmgr_ji_get_done_time(ji_ptr, &actual_done_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_GREATER_THAN(0, actual_done_time);
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_set_get_status_cancelled(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  sqc_rpc_sched_job_status_t actual_status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get status
  {
    rc = dbmgr_ji_get_status(ji_ptr, &actual_status);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_CREATED, actual_status);
  }

  // get status
  {
    // to cancelled
    rc = dbmgr_set_job_status_cancelled(ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    rc = dbmgr_ji_get_status(ji_ptr, &actual_status);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_CANCELLED, actual_status);
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_set_get_status_error(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  sqc_rpc_sched_job_status_t actual_status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get status
  {
    rc = dbmgr_ji_get_status(ji_ptr, &actual_status);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_CREATED, actual_status);
  }

  // get status
  {
    // to error
    rc = dbmgr_set_job_status_error(ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    rc = dbmgr_ji_get_status(ji_ptr, &actual_status);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(SQC_RPC_SCHED_JOB_STATUS_ERROR, actual_status);
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}


void test_ji_set_get_qc_job_id(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qc_job_id = "test-qc-job";
  const size_t qc_job_id_len = strlen(qc_job_id);
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  char *actual_qc_job_id = NULL;
  size_t actual_qc_job_id_len = 0;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // set/get qc_job_id/qc_job_id_len
  {
    rc = dbmgr_ji_set_qc_job_id(ji_ptr, qc_job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_ji_get_qc_job_id(ji_ptr, &actual_qc_job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(ji_ptr->qc_job_id, actual_qc_job_id);

    rc = dbmgr_ji_get_qc_job_id_len(ji_ptr, &actual_qc_job_id_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(qc_job_id_len, actual_qc_job_id_len);

    free((void *) actual_qc_job_id);
    actual_qc_job_id = NULL;
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_get_qprogram(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  char *actual_qprogram = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get qprogram
  {
    rc = dbmgr_ji_get_qprogram(ji_ptr, &actual_qprogram);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(qprogram, actual_qprogram);

    free((void *) actual_qprogram);
    actual_qprogram = NULL;
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_get_qprogram_len(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const size_t qprogram_len = strlen(qprogram);
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  size_t actual_qprogram_len = 0;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get qprogram_len
  {
    rc = dbmgr_ji_get_qprogram_len(ji_ptr, &actual_qprogram_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(qprogram_len, actual_qprogram_len);
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_get_circuit_fmt(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  sqc_rpc_sched_circuit_fmt_t actual_circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_UNKNOWN;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get circuit_fmt
  {
    rc = dbmgr_ji_get_circuit_fmt(ji_ptr, &actual_circuit_fmt);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(circuit_fmt, actual_circuit_fmt);
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_get_shots(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  size_t actual_shots = 0;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get shots
  {
    rc = dbmgr_ji_get_shots(ji_ptr, &actual_shots);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(shots, actual_shots);
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_get_qc_type(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  sqc_rpc_sched_qc_type_t actual_qc_type = SQC_RPC_SCHED_QC_TYPE_UNKNOWN;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get qc_type
  {
    rc = dbmgr_ji_get_qc_type(ji_ptr, &actual_qc_type);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(qc_type, actual_qc_type);
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_get_transpiler(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  sqc_rpc_sched_transpiler_t actual_transpiler = SQC_RPC_SCHED_TRANSPILER_UNKNOWN;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get transpiler
  {
    rc = dbmgr_ji_get_transpiler(ji_ptr, &actual_transpiler);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(transpiler, actual_transpiler);
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_get_remark(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  char *actual_remark = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get remark
  {
    rc = dbmgr_ji_get_remark(ji_ptr, &actual_remark);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(remark, actual_remark);

    free((void *) actual_remark);
    actual_remark = NULL;
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_get_remark_len(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  const size_t remark_len = strlen(remark);
  dbmgr_job_info_t ji_ptr = NULL;
  size_t actual_remark_len = 0;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get remark_len
  {
    rc = dbmgr_ji_get_remark_len(ji_ptr, &actual_remark_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(remark_len, actual_remark_len);
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_set_get_result(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *result = "test-result-data";
  const char *user_token = "test-user_token-data";
  const size_t result_len = strlen(result);
  dbmgr_job_info_t ji_ptr = NULL;
  char *actual_result = NULL;
  size_t actual_result_len = 0;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // set/get result/result_len
  {
    rc = dbmgr_ji_set_result(ji_ptr, result, result_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_ji_get_result(ji_ptr, &actual_result);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(ji_ptr->result, actual_result);

    rc = dbmgr_ji_get_result_len(ji_ptr, &actual_result_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(result_len, actual_result_len);

    free((void *) actual_result);
    actual_result = NULL;
  }

  // too long
  {
    rc = dbmgr_ji_set_result(ji_ptr, result, 1000);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_ji_get_result(ji_ptr, &actual_result);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(ji_ptr->result, actual_result);

    rc = dbmgr_ji_get_result_len(ji_ptr, &actual_result_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(result_len, actual_result_len);

    free((void *) actual_result);
    actual_result = NULL;
  }

  // too short
  {
    rc = dbmgr_ji_set_result(ji_ptr, result, 11);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_ji_get_result(ji_ptr, &actual_result);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING("test-result", actual_result);

    rc = dbmgr_ji_get_result_len(ji_ptr, &actual_result_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(11, actual_result_len);

    free((void *) actual_result);
    actual_result = NULL;
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_get_user_token(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  char *actual_user_token = NULL;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get user_token
  {
    rc = dbmgr_ji_get_user_token(ji_ptr, &actual_user_token);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL_STRING(user_token, actual_user_token);

    free((void *) actual_user_token);
    actual_user_token = NULL;
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_get_user_token_len(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  const size_t user_token_len = strlen(user_token);
  dbmgr_job_info_t ji_ptr = NULL;
  size_t actual_user_token_len = 0;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get user_token_len
  {
    rc = dbmgr_ji_get_user_token_len(ji_ptr, &actual_user_token_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(user_token_len, actual_user_token_len);
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_set_get_exec_time_estimate_msec(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  uint64_t new_exec_time_estimate_msec = 123456789;
  dbmgr_job_info_t ji_ptr = NULL;
  uint64_t actual_exec_time_estimate_msec = 0;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // set/get exec_time_estimate_msec
  {
    rc = dbmgr_ji_get_exec_time_estimate_msec(ji_ptr, &actual_exec_time_estimate_msec);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_exec_time_estimate_msec);

    rc = dbmgr_ji_set_exec_time_estimate_msec(ji_ptr, new_exec_time_estimate_msec);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_ji_get_exec_time_estimate_msec(ji_ptr, &actual_exec_time_estimate_msec);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(new_exec_time_estimate_msec, actual_exec_time_estimate_msec);
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_set_get_exec_time_msec(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  uint64_t new_exec_time_msec = 123456789;
  dbmgr_job_info_t ji_ptr = NULL;
  uint64_t actual_exec_time_msec = 0;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // set/get exec_time_msec
  {
    rc = dbmgr_ji_get_exec_time_msec(ji_ptr, &actual_exec_time_msec);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(0, actual_exec_time_msec);

    rc = dbmgr_ji_set_exec_time_msec(ji_ptr, new_exec_time_msec);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    rc = dbmgr_ji_get_exec_time_msec(ji_ptr, &actual_exec_time_msec);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_EQUAL(new_exec_time_msec, actual_exec_time_msec);
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_get_created_time(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  sqc_chrono_t actual_created_time;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get create_time
  {
    rc = dbmgr_ji_get_created_time(ji_ptr, &actual_created_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_LESS_OR_EQUAL_UINT64(sqc_chrono_now(), actual_created_time);
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_get_update_time(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  sqc_chrono_t actual_update_time;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // get update_time
  {
    rc = dbmgr_ji_get_update_time(ji_ptr, &actual_update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

    TEST_ASSERT_LESS_OR_EQUAL_UINT64(sqc_chrono_now(), actual_update_time);
  }

  // delete job
  {
    rc = dbmgr_ji_delete_job(ji_ptr->job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_arr_sort_by_created_time(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  const size_t data_size = 3;
  dbmgr_job_info_t job_info_arr[data_size];

  const char *user_id = "test-user";
  const char *group_id = "test-group";

  dbmgr_job_info_t ji_ptr = NULL;

  sqc_chrono_t target_created_time = 0;
  sqc_chrono_t actual_created_time = 0;

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // create job
  {
    // create target job
    for (size_t i = 0; i < data_size; i++) {
      rc = dbmgr_ji_create_job(user_id, group_id, 5,
                               "test-qprogram-data", SQC_RPC_SCHED_CIRCUIT_FMT_QIR, 30000 + i,
                               SQC_RPC_SCHED_QC_TYPE_RQC_REST, SQC_RPC_SCHED_TRANSPILER_NORMAL,
                               "test-remark-data", "test-user_token-data", &ji_ptr);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      job_info_arr[i] = ji_ptr;
      ji_ptr = NULL;
    }
  }

  // sort by created_time1
  {
    dbmgr_job_info_t target_job_info_arr[data_size];

    target_job_info_arr[0] = job_info_arr[0];
    target_job_info_arr[1] = job_info_arr[1];
    target_job_info_arr[2] = job_info_arr[2];

    rc = dbmgr_ji_job_arr_sort_by_created_time(target_job_info_arr, data_size);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    for (size_t i = 0; i < data_size; i++) {
      rc = dbmgr_ji_get_created_time(job_info_arr[i], &target_created_time);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      rc = dbmgr_ji_get_created_time(target_job_info_arr[i], &actual_created_time);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      TEST_ASSERT_EQUAL(target_created_time, actual_created_time);

      target_created_time = 0;
      actual_created_time = 0;
    }
  }

  // sort by created_time2
  {
    dbmgr_job_info_t target_job_info_arr[data_size];

    target_job_info_arr[0] = job_info_arr[1];
    target_job_info_arr[1] = job_info_arr[2];
    target_job_info_arr[2] = job_info_arr[0];

    rc = dbmgr_ji_job_arr_sort_by_created_time(target_job_info_arr, data_size);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    for (size_t i = 0; i < data_size; i++) {
      rc = dbmgr_ji_get_created_time(job_info_arr[i], &target_created_time);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      rc = dbmgr_ji_get_created_time(target_job_info_arr[i], &actual_created_time);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      TEST_ASSERT_EQUAL(target_created_time, actual_created_time);

      target_created_time = 0;
      actual_created_time = 0;
    }
  }

  // sort by created_time3
  {
    dbmgr_job_info_t target_job_info_arr[data_size];

    target_job_info_arr[0] = job_info_arr[2];
    target_job_info_arr[1] = job_info_arr[1];
    target_job_info_arr[2] = job_info_arr[0];

    rc = dbmgr_ji_job_arr_sort_by_created_time(target_job_info_arr, data_size);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    for (size_t i = 0; i < data_size; i++) {
      rc = dbmgr_ji_get_created_time(job_info_arr[i], &target_created_time);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      rc = dbmgr_ji_get_created_time(target_job_info_arr[i], &actual_created_time);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      TEST_ASSERT_EQUAL(target_created_time, actual_created_time);

      target_created_time = 0;
      actual_created_time = 0;
    }
  }

  // sort by created_time4
  {
    dbmgr_job_info_t target_job_info_arr[data_size];

    target_job_info_arr[0] = job_info_arr[0];
    target_job_info_arr[1] = job_info_arr[2];
    target_job_info_arr[2] = job_info_arr[1];

    rc = dbmgr_ji_job_arr_sort_by_created_time(target_job_info_arr, data_size);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    for (size_t i = 0; i < data_size; i++) {
      rc = dbmgr_ji_get_created_time(job_info_arr[i], &target_created_time);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      rc = dbmgr_ji_get_created_time(target_job_info_arr[i], &actual_created_time);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);

      TEST_ASSERT_EQUAL(target_created_time, actual_created_time);

      target_created_time = 0;
      actual_created_time = 0;
    }
  }

  // delete job
  {
    for (size_t i = 0; i < data_size; i++) {
      rc = dbmgr_ji_delete_job(job_info_arr[i]->job_id);
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    }
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}


/*
 * Negative testing
 */

void test_job_info_record_create_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  const char *excess_user_id = "01234567890123456789012345678901234567890123456789" \
                               "01234567890123456789012345678901234567890123456789" \
                               "01234567890123456789012345678901234567890123456789" \
                               "01234567890123456789012345678901234567890123456789" \
                               "01234567890123456789012345678901234567890123456789" \
                               "0123456";
  dbmgr_job_info_t ji_ptr = NULL;

  // invalid job_id
  {
    rc = s_dbmgr_job_info_record_create(excess_user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_TOO_LONG, rc);
    TEST_ASSERT_NULL(ji_ptr);

    rc = s_dbmgr_job_info_record_create("", group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(ji_ptr);

    rc = s_dbmgr_job_info_record_create(NULL, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(ji_ptr);
  }

  // invalid record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }
}

void test_job_info_record_destroy_negative(void) {
  s_dbmgr_job_info_record_destroy(NULL);
}

void test_job_info_record_add_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // not started
  {
    rc = s_dbmgr_job_info_record_add(ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_STARTED, rc);
  }

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = s_dbmgr_job_info_record_delete(NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_job_info_record_delete_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // not started
  {
    rc = s_dbmgr_job_info_record_delete(ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_STARTED, rc);
  }

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = s_dbmgr_job_info_record_delete(NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_job_info_record_find_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  dbmgr_job_info_t actual_ji_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // not started
  {
    rc = s_dbmgr_job_info_record_find(ji_ptr->job_id, &actual_ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_STARTED, rc);
    TEST_ASSERT_NULL(actual_ji_ptr);
  }

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // non-existent job_id
  {
    rc = s_dbmgr_job_info_record_find("non-existent-job", &actual_ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_FOUND, rc);
    TEST_ASSERT_NULL(actual_ji_ptr);
  }

  // invalid job_id
  {
    rc = s_dbmgr_job_info_record_find("", &actual_ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(actual_ji_ptr);

    rc = s_dbmgr_job_info_record_find(NULL, &actual_ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(actual_ji_ptr);
  }

  // invalid record
  {
    rc = s_dbmgr_job_info_record_find(ji_ptr->job_id, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_create_job_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;

  // invalid user_id
  {
    rc = dbmgr_ji_create_job("", group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(ji_ptr);

    rc = dbmgr_ji_create_job(NULL, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(ji_ptr);
  }

  // invalid priority
  {
    rc = dbmgr_ji_create_job(user_id, group_id, 100,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(ji_ptr);
  }

  // invalid qprogram
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             "", circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(ji_ptr);

    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             NULL, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(ji_ptr);
  }

  // invalid remark
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             "", user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(ji_ptr);

    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             NULL, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_NULL(ji_ptr);
  }

  // invalid record
  {
    rc = dbmgr_ji_create_job(user_id, group_id, priority,
                             qprogram, circuit_fmt, shots,
                             qc_type, transpiler,
                             remark, user_token, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }
}

void test_ji_delete_job_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *job_id = "test-job";

  // not started
  {
    rc = dbmgr_ji_delete_job(job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_NOT_STARTED, rc);
  }

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid job_id
  {
    rc = dbmgr_ji_delete_job("");
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);

    rc = dbmgr_ji_delete_job(NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_job_exists_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *job_id = "test-job";

  // not started
  {
    TEST_ASSERT_FALSE(dbmgr_ji_job_exists(job_id));
  }

  // initialize
  {
    rc = s_dbmgr_db_initialize(SQC_RPC_SCHED_UT_DB_FILE);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
    TEST_ASSERT_NOT_NULL(sqc_rpc_sched_db);

    rc = s_dbmgr_job_info_initialize();
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid job_id
  {
    TEST_ASSERT_FALSE(dbmgr_ji_job_exists(""));

    TEST_ASSERT_FALSE(dbmgr_ji_job_exists(NULL));
  }

  // finalize
  s_dbmgr_job_info_finalize();

  s_dbmgr_db_finalize();
  TEST_ASSERT_NULL(sqc_rpc_sched_db);
}

void test_ji_job_find_negative(void) {
  // same as test_job_info_record_find_negative test
}

void test_ji_get_job_id_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  char *actual_job_id = NULL;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ji_get_job_id(NULL, &actual_job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid job_id
  {
    rc = dbmgr_ji_get_job_id(ji_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

void test_ji_get_job_id_len_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  size_t actual_job_id_len = 0;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ji_get_job_id_len(NULL, &actual_job_id_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid job_id_len
  {
    rc = dbmgr_ji_get_job_id_len(ji_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

void test_ji_get_user_id_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  char *actual_user_id = NULL;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ji_get_user_id(NULL, &actual_user_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid user_id
  {
    rc = dbmgr_ji_get_user_id(ji_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

void test_ji_get_user_id_len_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  size_t actual_user_id_len = 0;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ji_get_user_id_len(NULL, &actual_user_id_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid user_id_len
  {
    rc = dbmgr_ji_get_user_id_len(ji_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

void test_ji_get_priority_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  uint8_t actual_priority = 0;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ji_get_priority(NULL, &actual_priority);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid user_id_len
  {
    rc = dbmgr_ji_get_priority(ji_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

void test_ji_set_qc_job_id_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qc_job_id = "test-qc-job";
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ji_set_qc_job_id(NULL, qc_job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid qc_job_id
  {
    rc = dbmgr_ji_set_qc_job_id(ji_ptr, "");
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_EQUAL(0, strlen(ji_ptr->qc_job_id));

    rc = dbmgr_ji_set_qc_job_id(ji_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_EQUAL(0, strlen(ji_ptr->qc_job_id));
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

void test_ji_get_qc_job_id_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  char *actual_qc_job_id = NULL;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ji_get_qc_job_id(NULL, &actual_qc_job_id);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid qc_job_id
  {
    rc = dbmgr_ji_get_qc_job_id(ji_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

void test_ji_get_qc_job_id_len_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  size_t actual_qc_job_id_len = 0;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ji_get_qc_job_id_len(NULL, &actual_qc_job_id_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid qc_job_id_len
  {
    rc = dbmgr_ji_get_qc_job_id_len(ji_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

void test_ji_get_qprogram_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  char *actual_qprogram = NULL;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ji_get_qprogram(NULL, &actual_qprogram);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid qprogram
  {
    rc = dbmgr_ji_get_qprogram(ji_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

void test_ji_get_qprogram_len_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  size_t actual_qprogram_len = 0;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ji_get_qprogram_len(NULL, &actual_qprogram_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid qprogram_len
  {
    rc = dbmgr_ji_get_qprogram_len(ji_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

void test_ji_get_shots_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  size_t actual_shots = 0;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ji_get_shots(NULL, &actual_shots);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid shots
  {
    rc = dbmgr_ji_get_shots(ji_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

void test_ji_get_qc_type_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  sqc_rpc_sched_qc_type_t actual_qc_type = SQC_RPC_SCHED_QC_TYPE_UNKNOWN;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ji_get_qc_type(NULL, &actual_qc_type);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid qc_type
  {
    rc = dbmgr_ji_get_qc_type(ji_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

void test_ji_get_transpiler_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  sqc_rpc_sched_transpiler_t actual_transpiler = SQC_RPC_SCHED_TRANSPILER_UNKNOWN;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ji_get_transpiler(NULL, &actual_transpiler);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid qc_type
  {
    rc = dbmgr_ji_get_transpiler(ji_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

void test_ji_get_remark_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  char *actual_remark = NULL;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ji_get_remark(NULL, &actual_remark);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid remark
  {
    rc = dbmgr_ji_get_remark(ji_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

void test_ji_get_remark_len_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  size_t actual_remark_len = 0;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ji_get_remark_len(NULL, &actual_remark_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid remark_len
  {
    rc = dbmgr_ji_get_remark_len(ji_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

void test_ji_set_result_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *result = "test-result-data";
  const size_t result_len = strlen(result);
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ji_set_result(NULL, result, result_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid result
  {
    rc = dbmgr_ji_set_result(ji_ptr, "", result_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_EQUAL(0, strlen(ji_ptr->qc_job_id));

    rc = dbmgr_ji_set_result(ji_ptr, NULL, result_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
    TEST_ASSERT_EQUAL(0, strlen(ji_ptr->result));
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

void test_ji_get_result_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  char *actual_result = NULL;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ji_get_result(NULL, &actual_result);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid result
  {
    rc = dbmgr_ji_get_result(ji_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

void test_ji_get_result_len_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  size_t actual_result_len = 0;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid result
  {
    rc = dbmgr_ji_get_result_len(NULL, &actual_result_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid result_len
  {
    rc = dbmgr_ji_get_result_len(ji_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

void test_ji_get_user_token_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  char *actual_user_token = NULL;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ji_get_user_token(NULL, &actual_user_token);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid user_token
  {
    rc = dbmgr_ji_get_user_token(ji_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

void test_ji_get_user_token_len_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  size_t actual_user_token_len = 0;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ji_get_user_token_len(NULL, &actual_user_token_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid remark_len
  {
    rc = dbmgr_ji_get_user_token_len(ji_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

void test_ji_get_create_time_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  sqc_chrono_t actual_created_time;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ji_get_created_time(NULL, &actual_created_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid remark_len
  {
    rc = dbmgr_ji_get_created_time(ji_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

void test_ji_get_update_time_negative(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const uint8_t priority = 5;
  const char *qprogram = "test-qprogram-data";
  const sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
  const size_t shots = 30000;
  const sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  const sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
  const char *remark = "test-remark-data";
  const char *user_token = "test-user_token-data";
  dbmgr_job_info_t ji_ptr = NULL;
  sqc_chrono_t actual_update_time;

  // create record
  {
    rc = s_dbmgr_job_info_record_create(user_id, group_id, priority,
                                        qprogram, circuit_fmt, shots,
                                        qc_type, transpiler,
                                        remark, user_token, &ji_ptr);
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  }

  // invalid record
  {
    rc = dbmgr_ji_get_update_time(NULL, &actual_update_time);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid remark_len
  {
    rc = dbmgr_ji_get_update_time(ji_ptr, NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // destroy record
  {
    s_dbmgr_job_info_record_destroy(ji_ptr);
    ji_ptr = NULL;
  }
}

