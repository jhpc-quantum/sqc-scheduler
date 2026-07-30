#include "unity.h"
#include "sqc_apis.h"

#define TEST_STR(_str, _test_str) {                                  \
    if (_str != NULL) {                                              \
      TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(_test_str, _str),          \
                                "string compare error.");            \
    } else {                                                         \
      TEST_FAIL_MESSAGE("string is null.");                          \
    }                                                                \
    if (_str != NULL){                                               \
      free(_str);                                                      \
      _str = NULL;                                                     \
    }                                                                \
  }

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_sqc_str_escape_01(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  bool is_escaped = false;
  char *str = NULL;
  char test_str[] = "hoge\"foo'bar";
  char test_ret_str[] = "hoge\\\"foo\\\'bar";

  ret = sqc_str_escape(test_str, "\"'", &is_escaped, &str);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_str_escap error.");
  TEST_STR(str, test_ret_str);
  TEST_ASSERT_EQUAL_MESSAGE(true, is_escaped,
                            "is_escaped error.");
}

void
test_sqc_str_escape_02(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  bool is_escaped = false;
  char *str = NULL;
  char test_str[] = "hogefoobar";
  char test_ret_str[] = "hogefoobar";

  ret = sqc_str_escape(test_str, "\"'", &is_escaped, &str);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_str_escap error.");
  TEST_STR(str, test_ret_str);
  TEST_ASSERT_EQUAL_MESSAGE(false, is_escaped,
                            "is_escaped error.");
}

void
test_sqc_str_escape_03(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str[] = "hoge\"foo'bar";
  char test_ret_str[] = "hoge\\\"foo\\\'bar";

  ret = sqc_str_escape(test_str, "\"'", NULL, &str);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_str_escap error.");
  TEST_STR(str, test_ret_str);
}

void
test_sqc_str_escape_null(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *str = NULL;
  bool is_escaped = false;
  char test_str[] = "hoge\"foo'bar";

  ret = sqc_str_escape((const char *) NULL, "\"'", &is_escaped, &str);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_str_escap error (null).");

  ret = sqc_str_escape(test_str, (const char *) NULL, &is_escaped, &str);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_str_escap error (null).");

  ret = sqc_str_escape(test_str, "\"'", &is_escaped, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_str_escap error (null).");
}

void
test_sqc_str_indexof(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  ret = sqc_str_indexof("abcdefg", "abcd");
  TEST_ASSERT_EQUAL_MESSAGE(0, ret,
                            "cmd_string_indexof error (invalid_args).");

  ret = sqc_str_indexof("abcdefg", "cde");
  TEST_ASSERT_EQUAL_MESSAGE(2, ret,
                            "cmd_string_indexof error (invalid_args).");

  ret = sqc_str_indexof("abcdefg", "efg");
  TEST_ASSERT_EQUAL_MESSAGE(4, ret,
                            "cmd_string_indexof error (invalid_args).");

  ret = sqc_str_indexof("abcdefg", "zabc");
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_NOT_FOUND, ret,
                            "cmd_string_indexof error (invalid_args).");

  ret = sqc_str_indexof("abcdefg", "efgh");
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_NOT_FOUND, ret,
                            "cmd_string_indexof error (invalid_args).");

  ret = sqc_str_indexof(NULL, "abcdefg");
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "cmd_string_indexof error (invalid_args).");

  ret = sqc_str_indexof("abcdefg", NULL);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "cmd_string_indexof error (invalid_args).");
}

#define TOKEN_MAX 5

void
test_sqc_str_tokenize01(void) {
  sqc_result_t n_tokens = SQC_RESULT_ANY_FAILURES;
  size_t test_num = 2;
  size_t i;
  char *tokens[TOKEN_MAX + 1];
  char str[] = "foo,bar";
  const char *test_str[] = {
    "foo",
    "bar"
  };

  n_tokens = sqc_str_tokenize(str, tokens, TOKEN_MAX + 1,
                                 ",");
  TEST_ASSERT_EQUAL_MESSAGE(test_num, n_tokens,
                            "n_tokens error.");

  for (i = 0; i < (size_t) n_tokens; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(test_str[i], tokens[i]),
                              "token string compare error.");
  }
}

void
test_sqc_str_tokenize_02(void) {
  sqc_result_t n_tokens = SQC_RESULT_ANY_FAILURES;
  size_t test_num = 3;
  size_t i;
  char *tokens[TOKEN_MAX + 1];
  char str[] = "foo,bar.hoge-baz";
  const char *test_str[] = {
    "foo",
    "bar",
    "hoge-baz"
  };

  n_tokens = sqc_str_tokenize(str, tokens, TOKEN_MAX + 1,
                                 ",.");
  TEST_ASSERT_EQUAL_MESSAGE(test_num, n_tokens,
                            "n_tokens error.");

  for (i = 0; i < (size_t) n_tokens; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(test_str[i], tokens[i]),
                              "token string compare error.");
  }
}

void
test_sqc_str_tokenize_03(void) {
  sqc_result_t n_tokens = SQC_RESULT_ANY_FAILURES;
  size_t test_num = 3;
  size_t i;
  char *tokens[TOKEN_MAX + 1];
  char str[] = "foo,\"bar.hoge-baz\"";
  const char *test_str[] = {
    "foo",
    "\"bar",
    "hoge-baz\""
  };

  n_tokens = sqc_str_tokenize(str, tokens, TOKEN_MAX + 1,
                                 ",.");
  TEST_ASSERT_EQUAL_MESSAGE(test_num, n_tokens,
                            "n_tokens error.");

  for (i = 0; i < (size_t) n_tokens; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(test_str[i], tokens[i]),
                              "token string compare error.");
  }
}

void
test_sqc_str_tokenize_quote_04(void) {
  sqc_result_t n_tokens = SQC_RESULT_ANY_FAILURES;
  size_t i;
  char *tokens[TOKEN_MAX + 1];
  char str[] = "foo,bar,hoge,baz,1,2";
  const char *test_str[] = {
    "foo",
    "bar",
    "hoge",
    "baz",
    "1",
    "2"
  };

  n_tokens = sqc_str_tokenize(str, tokens, TOKEN_MAX + 1,
                                 ",.");
  TEST_ASSERT_EQUAL_MESSAGE(TOKEN_MAX + 1, n_tokens,
                            "n_tokens error.");

  for (i = 0; i < (size_t) n_tokens; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(test_str[i], tokens[i]),
                              "token string compare error.");
  }
}

void
test_sqc_str_tokenize_with_limit_01(void) {
  sqc_result_t n_tokens = SQC_RESULT_ANY_FAILURES;
  size_t test_num = 2;
  size_t limit = 1;
  size_t i;
  char *tokens[TOKEN_MAX + 1];
  char str[] = "foo,bar,baz";
  const char *test_str[] = {
    "foo",
    "bar,baz"
  };

  n_tokens = sqc_str_tokenize_with_limit(str, tokens, TOKEN_MAX + 1,
             limit, ",");
  TEST_ASSERT_EQUAL_MESSAGE(test_num, n_tokens,
                            "n_tokens error.");

  for (i = 0; i < (size_t) n_tokens; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(test_str[i], tokens[i]),
                              "token string compare error.");
  }
}

void
test_sqc_str_tokenize_with_limit_02(void) {
  sqc_result_t n_tokens = SQC_RESULT_ANY_FAILURES;
  size_t test_num = 3;
  size_t limit = 2;
  size_t i;
  char *tokens[TOKEN_MAX + 1];
  char str[] = "foo,bar,baz";
  const char *test_str[] = {
    "foo",
    "bar",
    "baz"
  };

  n_tokens = sqc_str_tokenize_with_limit(str, tokens, TOKEN_MAX + 1,
             limit, ",");
  TEST_ASSERT_EQUAL_MESSAGE(test_num, n_tokens,
                            "n_tokens error.");

  for (i = 0; i < (size_t) n_tokens; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(test_str[i], tokens[i]),
                              "token string compare error.");
  }
}

void
test_sqc_str_tokenize_with_limit_03(void) {
  sqc_result_t n_tokens = SQC_RESULT_ANY_FAILURES;
  size_t test_num = 3;
  size_t limit = 0;
  size_t i;
  char *tokens[TOKEN_MAX + 1];
  char str[] = "foo,bar,baz";
  const char *test_str[] = {
    "foo",
    "bar",
    "baz"
  };

  n_tokens = sqc_str_tokenize_with_limit(str, tokens, TOKEN_MAX + 1,
             limit, ",");
  TEST_ASSERT_EQUAL_MESSAGE(test_num, n_tokens,
                            "n_tokens error.");

  for (i = 0; i < (size_t) n_tokens; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(test_str[i], tokens[i]),
                              "token string compare error.");
  }
}

void
test_sqc_str_tokenize_with_limit_04(void) {
  sqc_result_t n_tokens = SQC_RESULT_ANY_FAILURES;
  size_t test_num = 3;
  size_t limit = 10;
  size_t i;
  char *tokens[TOKEN_MAX + 1];
  char str[] = "foo,bar,baz";
  const char *test_str[] = {
    "foo",
    "bar",
    "baz"
  };

  n_tokens = sqc_str_tokenize_with_limit(str, tokens, TOKEN_MAX + 1,
             limit, ",");
  TEST_ASSERT_EQUAL_MESSAGE(test_num, n_tokens,
                            "n_tokens error.");

  for (i = 0; i < (size_t) n_tokens; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(test_str[i], tokens[i]),
                              "token string compare error.");
  }
}

void
test_sqc_str_tokenize_with_limit_05(void) {
  sqc_result_t n_tokens = SQC_RESULT_ANY_FAILURES;
  size_t test_num = 3;
  size_t limit = 2;
  size_t i;
  char *tokens[TOKEN_MAX + 1];
  char str[] = "foo,bar.baz-hoge,1";
  const char *test_str[] = {
    "foo",
    "bar",
    "baz-hoge,1"
  };

  n_tokens = sqc_str_tokenize_with_limit(str, tokens, TOKEN_MAX + 1,
             limit, ",.");
  TEST_ASSERT_EQUAL_MESSAGE(test_num, n_tokens,
                            "n_tokens error.");

  for (i = 0; i < (size_t) n_tokens; i++) {
    TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(test_str[i], tokens[i]),
                              "token string compare error.");
  }
}

void
test_sqc_str_trim_right_01(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *str = NULL;
  const char *test_str = "  hoge  ";
  const char *test_ret_str = "  hoge";

  ret = sqc_str_trim_right(test_str, " ", &str);

  TEST_ASSERT_EQUAL_MESSAGE(strlen(test_ret_str), ret,
                            "sqc_str_trim_right error.");
  TEST_STR(str, test_ret_str);
}

void
test_sqc_str_trim_right_02(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *str = NULL;
  const char *test_str = "       ";
  const char *test_ret_str = "";

  ret = sqc_str_trim_right(test_str, " ", &str);

  TEST_ASSERT_EQUAL_MESSAGE(strlen(test_ret_str), ret,
                            "sqc_str_trim_right error.");
  TEST_STR(str, test_ret_str);
}

void
test_sqc_str_trim_right_03(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *str = NULL;
  const char *test_str = "  hoge";
  const char *test_ret_str = "  hoge";

  ret = sqc_str_trim_right(test_str, " ", &str);

  TEST_ASSERT_EQUAL_MESSAGE(strlen(test_ret_str), ret,
                            "sqc_str_trim_right error.");
  TEST_STR(str, test_ret_str);
}

void
test_sqc_str_trim_right_04(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *str = NULL;
  const char *test_str = "";

  ret = sqc_str_trim_right(test_str, " ", &str);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_str_trim_right error.");
}

void
test_sqc_str_trim_left_01(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *str = NULL;
  const char *test_str = "  hoge  ";
  const char *test_ret_str = "hoge  ";

  ret = sqc_str_trim_left(test_str, " ", &str);

  TEST_ASSERT_EQUAL_MESSAGE(strlen(test_ret_str), ret,
                            "sqc_str_trim_left error.");
  TEST_STR(str, test_ret_str);
}

void
test_sqc_str_trim_left_02(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *str = NULL;
  const char *test_str = "        ";
  const char *test_ret_str = "";

  ret = sqc_str_trim_left(test_str, " ", &str);

  TEST_ASSERT_EQUAL_MESSAGE(strlen(test_ret_str), ret,
                            "sqc_str_trim_left error.");
  TEST_STR(str, test_ret_str);
}

void
test_sqc_str_trim_left_03(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *str = NULL;
  const char *test_str = "hoge  ";
  const char *test_ret_str = "hoge  ";

  ret = sqc_str_trim_left(test_str, " ", &str);

  TEST_ASSERT_EQUAL_MESSAGE(strlen(test_ret_str), ret,
                            "sqc_str_trim_left error.");
  TEST_STR(str, test_ret_str);
}

void
test_sqc_str_trim_left_04(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *str = NULL;
  const char *test_str = "";

  ret = sqc_str_trim_left(test_str, " ", &str);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_str_trim_left error.");
}

void
test_sqc_str_trim_01(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *str = NULL;
  const char *test_str = "  hoge  ";
  const char *test_ret_str = "hoge";

  ret = sqc_str_trim(test_str, " ", &str);

  TEST_ASSERT_EQUAL_MESSAGE(strlen(test_ret_str), ret,
                            "sqc_str_trim error.");
  TEST_STR(str, test_ret_str);
}

void
test_sqc_str_trim_02(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *str = NULL;
  const char *test_str = "        ";
  const char *test_ret_str = "";

  ret = sqc_str_trim(test_str, " ", &str);

  TEST_ASSERT_EQUAL_MESSAGE(strlen(test_ret_str), ret,
                            "sqc_str_trim error.");
  TEST_STR(str, test_ret_str);
}

void
test_sqc_str_trim_03(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *str = NULL;
  const char *test_str = "hoge";
  const char *test_ret_str = "hoge";

  ret = sqc_str_trim(test_str, " ", &str);

  TEST_ASSERT_EQUAL_MESSAGE(strlen(test_ret_str), ret,
                            "sqc_str_trim error.");
  TEST_STR(str, test_ret_str);
}

void
test_sqc_str_trim_04(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *str = NULL;
  const char *test_str = "";

  ret = sqc_str_trim(test_str, " ", &str);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_str_trim error.");
}
