#include "unity.h"

#include "dbmgr_util.c"
#include "dbmgr_db.c"

void setUp(void) {
}

void tearDown(void) {
}

void test_dbmgr_util_create_user_group_key(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const char *expected_user_group_key= "test-user" SQC_RPC_SCHED_USER_GROUP_KEY_SEP  "test-group";
  const size_t expected_user_id_len = strlen(expected_user_group_key);
  char actual_user_group_key[SQC_RPC_SCHED_USER_GROUP_KEY_MAX_SIZE + 1];
  size_t actual_user_group_key_len;

  rc = dbmgr_util_create_user_group_key(actual_user_group_key,
                                        sizeof(actual_user_group_key),
                                        user_id, group_id,
                                        &actual_user_group_key_len);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, rc);
  TEST_ASSERT_EQUAL_STRING(expected_user_group_key, actual_user_group_key);
  TEST_ASSERT_EQUAL(expected_user_id_len, actual_user_group_key_len);
}

void test_negative_dbmgr_util_create_user_group_key_invalid_string(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  char actual_user_group_key[SQC_RPC_SCHED_USER_GROUP_KEY_MAX_SIZE + 1];
  size_t actual_user_group_key_len;

  // invalid user_group_key
  {
    rc = dbmgr_util_create_user_group_key(NULL,
                                          sizeof(actual_user_group_key),
                                          user_id, group_id,
                                          &actual_user_group_key_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);

    rc = dbmgr_util_create_user_group_key(actual_user_group_key,
                                          sizeof(actual_user_group_key),
                                          user_id, group_id,
                                          NULL);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid user_id
  {
    rc = dbmgr_util_create_user_group_key(actual_user_group_key,
                                          sizeof(actual_user_group_key),
                                          "", group_id,
                                          &actual_user_group_key_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);

    rc = dbmgr_util_create_user_group_key(actual_user_group_key,
                                          sizeof(actual_user_group_key),
                                          NULL, group_id,
                                          &actual_user_group_key_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }

  // invalid group_id
  {
    rc = dbmgr_util_create_user_group_key(actual_user_group_key,
                                          sizeof(actual_user_group_key),
                                          user_id, "",
                                          &actual_user_group_key_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);

    rc = dbmgr_util_create_user_group_key(actual_user_group_key,
                                          sizeof(actual_user_group_key),
                                          user_id, NULL,
                                          &actual_user_group_key_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }
}

void test_negative_dbmgr_util_create_user_group_key_too_long(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  const char *user_id = "test-user";
  const char *group_id = "test-group";
  const char *excess_user_id = "01234567890123456789012345678901234567890123456789" \
                               "01234567890123456789012345678901234567890123456789" \
                               "01234567890123456789012345678901234567890123456789" \
                               "01234567890123456789012345678901234567890123456789" \
                               "01234567890123456789012345678901234567890123456789" \
                               "0123456";

  const char *excess_group_id = "01234567890123456789012345678901234567890123456789" \
                                "01234567890123456789012345678901234567890123456789" \
                                "01234567890123456789012345678901234567890123456789" \
                                "01234567890123456789012345678901234567890123456789" \
                                "01234567890123456789012345678901234567890123456789" \
                                "0123456";
  size_t shortage_user_group_key_len = 5;
  char actual_user_group_key[SQC_RPC_SCHED_USER_GROUP_KEY_MAX_SIZE + 1];
  size_t actual_user_group_key_len;

  // invalid user_id
  {
    rc = dbmgr_util_create_user_group_key(actual_user_group_key,
                                          sizeof(actual_user_group_key),
                                          excess_user_id, group_id,
                                          &actual_user_group_key_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_TOO_LONG, rc);
  }

  // invalid group_id
  {
    rc = dbmgr_util_create_user_group_key(actual_user_group_key,
                                          sizeof(actual_user_group_key),
                                          user_id, excess_group_id,
                                          &actual_user_group_key_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_TOO_LONG, rc);
  }

  // invalid user_group_key_size
  {
    rc = dbmgr_util_create_user_group_key(actual_user_group_key,
                                          shortage_user_group_key_len,
                                          user_id, group_id,
                                          &actual_user_group_key_len);
    TEST_ASSERT_EQUAL(SQC_RESULT_INVALID_ARGS, rc);
  }
}

