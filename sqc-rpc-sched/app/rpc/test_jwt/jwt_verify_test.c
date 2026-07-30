#include "unity.h"
#include "sqc_apis.h"
#include "rpc_jwt_server.h"
#include "rpc_file_util.h"

#define MAX_TOKEN_SIZE 16384
#define MAX_SUBJECT_SIZE 1024

static char conf_dir[PATH_MAX + 1];
static char subject_file[PATH_MAX + 1];
static char token_file[PATH_MAX + 1];
static bool pass;

void setUp(void) {}


void tearDown(void) {}


static void
do_test(void) {
  sqc_result_t read_token_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t read_subject_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t create_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t validate_result = SQC_RESULT_ANY_FAILURES;
  char* expected_subject = NULL;
  char* token = NULL;
  char* token_subject = NULL;
  rpc_jwt_server_ctx_t ctx = NULL;

  read_token_result = rpc_read_text_file(token_file, &token, NULL);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, read_token_result);
  TEST_ASSERT_NOT_NULL(token);

  read_subject_result = rpc_read_text_file(subject_file, &expected_subject, NULL);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, read_subject_result);
  TEST_ASSERT_NOT_NULL(expected_subject);

  create_result = rpc_jwt_server_create_ctx_from_conf_dir(&ctx, conf_dir);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, create_result);
  TEST_ASSERT_NOT_NULL(ctx);

  validate_result = rpc_jwt_server_validate_token(&ctx, token, 0u, &token_subject);

  if (pass) {
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, validate_result);
    TEST_ASSERT_EQUAL_STRING(expected_subject, token_subject);
  } else {
    TEST_ASSERT_NOT_EQUAL(SQC_RESULT_OK, validate_result);
    TEST_ASSERT_NULL(token_subject);
  }

  free(token);
  free(token_subject);
  free(ctx);
}


int main(int argc, char *argv[]) {
  if (argc == 3) {
    snprintf(conf_dir, sizeof(conf_dir), "%s", argv[1]);
    snprintf(token_file, sizeof(token_file), "%s/jwt.token", argv[1]);
    snprintf(subject_file, sizeof(subject_file), "%s/jwt_sub.txt", argv[1]);
    pass = (strcmp(argv[2], "pass") == 0);

    UNITY_BEGIN();
    RUN_TEST(do_test);
    return UNITY_END();
  } else {
    fprintf(stderr, "Usage: %s TEST-DATA-DIR pass\n", argv[0]);
    fprintf(stderr, "       %s TEST-DATA-DIR fail\n", argv[0]);
    return 1;
  }
}
