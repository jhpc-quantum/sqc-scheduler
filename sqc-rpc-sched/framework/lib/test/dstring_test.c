#include <string.h>
#include "unity.h"
#include "sqc_apis.h"
#include "sqc_dstring.h"

static sqc_dstring_t ds = NULL;
static sqc_dstring_t ds2 = NULL;

#define TEST_DSTRING(_ret, _ds, _str, _test_str) {                    \
    _ret = sqc_dstring_str_get((_ds), &_str);                     \
    TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK,_ret,                 \
                              "sqc_dstring_str_get error.");      \
    if (_str != NULL) {                                               \
      TEST_ASSERT_EQUAL_MESSAGE(0, strcmp(_test_str, _str),           \
                                "string compare error.");             \
    } else {                                                          \
      TEST_FAIL_MESSAGE("string is null.");                           \
    }                                                                 \
    free(_str);                                                       \
  }

void
setUp(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  /* create. */
  ret = sqc_dstring_create(&ds);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_create error.");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, ds,
                                "dstring is null.");

  ret = sqc_dstring_create(&ds2);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_create error.");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, ds2,
                                "dstring is null.");
}

void
tearDown(void) {
  /* destroy. */
  sqc_dstring_destroy(&ds);
  TEST_ASSERT_EQUAL_MESSAGE(NULL, ds,
                            "dstring is not null.");

  sqc_dstring_destroy(&ds2);
  TEST_ASSERT_EQUAL_MESSAGE(NULL, ds2,
                            "dstring is not null.");

  /* double call (destroy). */
  sqc_dstring_destroy(&ds);
  TEST_ASSERT_EQUAL_MESSAGE(NULL, ds,
                            "dstring is not null.");
}

void
test_sqc_dstring_create_destroy(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str[] = "";

  /* get string. */
  TEST_DSTRING(ret, &ds, str, test_str);
}

void
test_sqc_dstring_create_null(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  /* create */
  ret = sqc_dstring_create(NULL);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_create error(NULL).");
}

void
test_sqc_dstring_clear_null(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_dstring_t ds_null = NULL;

  ret = sqc_dstring_clear(NULL);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_clear error.");

  ret = sqc_dstring_clear(&ds_null);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_clear error.");
}

void
test_sqc_dstring_appendf_clear_srt_get(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  bool empty;
  char *str = NULL;
  char test_str1[] = "1234 5678\n";
  char test_str2[] = "1234 5678\nhoge foo\n";
  char test_str3[] = "\0";
  char test_str4[] = "9876 5432\n";
  char test_str5[] = "9876 5432\nhoge foo bar\n";
  char test_str6[] = "9876 5432\nhoge foo bar\nbas\n";

  /* check empty. */
  empty = sqc_dstring_empty(&ds);
  TEST_ASSERT_EQUAL_MESSAGE(true, empty,
                            "empty error.");

  /* append int. */
  ret = sqc_dstring_appendf(&ds, "%d 5678\n", 1234);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_appendf error.");
  TEST_DSTRING(ret, &ds, str, test_str1);
  empty = sqc_dstring_empty(&ds);
  TEST_ASSERT_EQUAL_MESSAGE(false, empty,
                            "empty error.");

  /* append str. */
  ret = sqc_dstring_appendf(&ds, "hoge %s\n", "foo");

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_appendf error.");
  TEST_DSTRING(ret, &ds, str, test_str2);

  /* clear. */
  ret = sqc_dstring_clear(&ds);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_clear error.");
  TEST_DSTRING(ret, &ds, str, test_str3);

  /* append int. */
  ret = sqc_dstring_appendf(&ds, "%d 5432\n", 9876);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_appendf error.");
  TEST_DSTRING(ret, &ds, str, test_str4);

  /* append str. */
  ret = sqc_dstring_appendf(&ds, "hoge %s %s\n", "foo", "bar");

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_appendf error.");
  TEST_DSTRING(ret, &ds, str, test_str5);

  /* not args. */
  ret = sqc_dstring_appendf(&ds, "bas\n");

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_appendf error.");
  TEST_DSTRING(ret, &ds, str, test_str6);
}

void
test_sqc_dstring_appendf_empty(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str1[] = "";
  char test_str2[] = "1234 5678\n";

  /* 1st */
  ret = sqc_dstring_appendf(&ds, "");
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_appendf error.");
  TEST_DSTRING(ret, &ds, str, test_str1);

  /* 2nd */
  ret = sqc_dstring_appendf(&ds, "");
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_appendf error.");
  TEST_DSTRING(ret, &ds, str, test_str1);

  /* append int. */
  ret = sqc_dstring_appendf(&ds, "%d 5678\n", 1234);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_appendf error.");
  TEST_DSTRING(ret, &ds, str, test_str2);

  /* 3nd */
  ret = sqc_dstring_appendf(&ds, "");
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_appendf error.");
  TEST_DSTRING(ret, &ds, str, test_str2);
}

void
test_sqc_dstring_appendf_null(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  static sqc_dstring_t ds_null = NULL;

  ret = sqc_dstring_appendf(NULL, "hoge\n");
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_appendf error.");

  ret = sqc_dstring_appendf(&ds_null, "hoge\n");
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_appendf error.");

  ret = sqc_dstring_appendf(&ds, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_appendf error.");
}

static void
va_list_test(const char *format, ...) __attribute__((format(printf, 1, 2)));

static void
va_list_test(const char *format, ...) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  va_list args;
  bool empty;
  char *str = NULL;
  char test_str1[] = "1234 5678\n";

  va_start(args, format);
  /* append int. */
  ret = sqc_dstring_vappendf(&ds, format, &args);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_vappendf error.");
  TEST_DSTRING(ret, &ds, str, test_str1);
  empty = sqc_dstring_empty(&ds);
  TEST_ASSERT_EQUAL_MESSAGE(false, empty,
                            "empty error.");
  va_end(args);
}

void
test_sqc_dstring_vappendf(void) {
  va_list_test("%d 5678\n", 1234);
}

void
test_sqc_dstring_vappendf_null(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  va_list args;
  static sqc_dstring_t ds_null = NULL;

  ret = sqc_dstring_vappendf(NULL, "hoge\n", &args);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_vappendf error.");

  ret = sqc_dstring_vappendf(&ds_null, "hoge\n", &args);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_vappendf error.");

  ret = sqc_dstring_vappendf(&ds, NULL, &args);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_vappendf error.");

  ret = sqc_dstring_vappendf(&ds, "hoge\n", NULL);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_vappendf error.");
}

void
test_sqc_dstring_concat_clear_srt_get(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str1[] = "hoge foo\n";
  char test_str2[] = "bas\n";
  char test_str3[] = "hoge foo\nbas\n";
  char test_str4[] = "\0";
  char test_str5[] = "1234 5678\n";
  char test_str6[] = "hoge foo\nbas\n1234 5678\n";

  /* append ds. */
  ret = sqc_dstring_appendf(&ds, "%s foo\n", "hoge");
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_appendf error.");
  TEST_DSTRING(ret, &ds, str, test_str1);

  /* append ds2. */
  ret = sqc_dstring_appendf(&ds2, "%s\n", "bas");
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_appendf error.");
  TEST_DSTRING(ret, &ds2, str, test_str2);

  /* concat (ds + ds2). */
  ret = sqc_dstring_concat(&ds, &ds2);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_concat error.");
  TEST_DSTRING(ret, &ds, str, test_str3);

  /* check const ds2. */
  TEST_DSTRING(ret, &ds2, str, test_str2);

  /* clear ds2. */
  ret = sqc_dstring_clear(&ds2);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_clear error.");
  TEST_DSTRING(ret, &ds2, str, test_str4);

  /* append ds2. */
  ret = sqc_dstring_appendf(&ds2, "1234 %d\n", 5678);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_appendf error.");
  TEST_DSTRING(ret, &ds2, str, test_str5);

  /* concat (ds + ds2).*/
  ret = sqc_dstring_concat(&ds, &ds2);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_concat error.");
  TEST_DSTRING(ret, &ds, str, test_str6);

  /* check const ds2. */
  TEST_DSTRING(ret, &ds2, str, test_str5);
}

void
test_sqc_dstring_concat_null(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_dstring_t ds_null = NULL;

  ret = sqc_dstring_concat(NULL, &ds2);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_concat error.");

  ret = sqc_dstring_concat(&ds_null, &ds2);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_concat error.");

  ret = sqc_dstring_concat(&ds, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_concat error.");

  ret = sqc_dstring_concat(&ds, &ds_null);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_concat error.");
}

void
test_sqc_dstring_str_get_null(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_dstring_t ds_null = NULL;
  char *str = NULL;

  ret = sqc_dstring_str_get(NULL, &str);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_str_get error.");

  ret = sqc_dstring_str_get(&ds_null, &str);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_str_get error.");

  ret = sqc_dstring_str_get(&ds, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_str_get error.");
}

void
test_sqc_dstring_len_get(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str1[] = "1 2\n";
  char test_str2[] = "1 2\na b\n";
  char test_str3[] = "\0";
  char test_str4[] = "98 76\n";

  /* check len. */
  ret = sqc_dstring_len_get(&ds);
  TEST_ASSERT_EQUAL_MESSAGE(0, ret,
                            "sqc_dstring_len_get error.");

  /* append int. */
  ret = sqc_dstring_appendf(&ds, "%d 2\n", 1);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_appendf error.");
  TEST_DSTRING(ret, &ds, str, test_str1);
  ret = sqc_dstring_len_get(&ds);
  TEST_ASSERT_EQUAL_MESSAGE(4, ret,
                            "sqc_dstring_len_get error.");

  /* append str. */
  ret = sqc_dstring_appendf(&ds, "a %s\n", "b");

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_appendf error.");
  TEST_DSTRING(ret, &ds, str, test_str2);
  ret = sqc_dstring_len_get(&ds);
  TEST_ASSERT_EQUAL_MESSAGE(4 + 4, ret,
                            "sqc_dstring_len_get error.");

  /* clear. */
  ret = sqc_dstring_clear(&ds);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_clear error.");
  TEST_DSTRING(ret, &ds, str, test_str3);
  ret = sqc_dstring_len_get(&ds);
  TEST_ASSERT_EQUAL_MESSAGE(0, ret,
                            "sqc_dstring_len_get error.");

  /* append int. */
  ret = sqc_dstring_appendf(&ds, "%d 76\n", 98);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_appendf error.");
  TEST_DSTRING(ret, &ds, str, test_str4);
  ret = sqc_dstring_len_get(&ds);
  TEST_ASSERT_EQUAL_MESSAGE(6, ret,
                            "sqc_dstring_len_get error.");
}

void
test_sqc_dstring_len_get_null(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  static sqc_dstring_t ds_null = NULL;

  ret = sqc_dstring_len_get(NULL);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_len_get error.");

  ret = sqc_dstring_len_get(&ds_null);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_len_get error.");
}

static void
prepend_va_list_test(const char *format, ...) __attribute__((format(printf, 1,
    2)));

static void
prepend_va_list_test(const char *format, ...) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  va_list args;
  bool empty;
  char *str = NULL;
  char test_str1[] = "hoge foo 1234 5678\n";

  va_start(args, format);
  /* append int. */
  ret = sqc_dstring_vprependf(&ds, format, &args);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_vprependf error.");
  TEST_DSTRING(ret, &ds, str, test_str1);
  empty = sqc_dstring_empty(&ds);
  TEST_ASSERT_EQUAL_MESSAGE(false, empty,
                            "empty error.");
  va_end(args);
}

void
test_sqc_dstring_vprependf(void) {
  va_list_test("%d 5678\n", 1234);
  prepend_va_list_test("hoge %s ", "foo");
}

void
test_sqc_dstring_vprependf_null(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  va_list args;
  static sqc_dstring_t ds_null = NULL;

  ret = sqc_dstring_vprependf(NULL, "hoge\n", &args);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_vprependf error.");

  ret = sqc_dstring_vprependf(&ds_null, "hoge\n", &args);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_vprependf error.");

  ret = sqc_dstring_vprependf(&ds, NULL, &args);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_vprependf error.");

  ret = sqc_dstring_vprependf(&ds, "hoge\n", NULL);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_vprependf error.");
}

void
test_sqc_dstring_prependf_clear_srt_get(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  bool empty;
  char *str = NULL;
  char test_str1[] = "1234 5678\n";
  char test_str2[] = "hoge foo\n1234 5678\n";
  char test_str3[] = "\0";
  char test_str4[] = "9876 5432\n";
  char test_str5[] = "hoge foo bar\n9876 5432\n";
  char test_str6[] = "bas\nhoge foo bar\n9876 5432\n";

  /* check empty. */
  empty = sqc_dstring_empty(&ds);
  TEST_ASSERT_EQUAL_MESSAGE(true, empty,
                            "empty error.");
  /* prepend int. */
  ret = sqc_dstring_prependf(&ds, "%d 5678\n", 1234);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_prependf error.");
  TEST_DSTRING(ret, &ds, str, test_str1);
  empty = sqc_dstring_empty(&ds);
  TEST_ASSERT_EQUAL_MESSAGE(false, empty,
                            "empty error.");

  /* prepend str. */
  ret = sqc_dstring_prependf(&ds, "hoge %s\n", "foo");

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_prependf error.");
  TEST_DSTRING(ret, &ds, str, test_str2);

  /* clear. */
  ret = sqc_dstring_clear(&ds);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_clear error.");
  TEST_DSTRING(ret, &ds, str, test_str3);

  /* prepend int. */
  ret = sqc_dstring_prependf(&ds, "%d 5432\n", 9876);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_prependf error.");
  TEST_DSTRING(ret, &ds, str, test_str4);

  /* prepend str. */
  ret = sqc_dstring_prependf(&ds, "hoge %s %s\n", "foo", "bar");

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_prependf error.");
  TEST_DSTRING(ret, &ds, str, test_str5);

  /* not args. */
  ret = sqc_dstring_prependf(&ds, "bas\n");

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_prependf error.");
  TEST_DSTRING(ret, &ds, str, test_str6);
}

void
test_sqc_dstring_prependf_empty(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *str = NULL;
  char test_str1[] = "";
  char test_str2[] = "1234 5678\n";

  /* 1st */
  ret = sqc_dstring_prependf(&ds, "");
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_prependf error.");
  TEST_DSTRING(ret, &ds, str, test_str1);

  /* 2nd */
  ret = sqc_dstring_prependf(&ds, "");
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_prependf error.");
  TEST_DSTRING(ret, &ds, str, test_str1);

  /* prepend int. */
  ret = sqc_dstring_prependf(&ds, "%d 5678\n", 1234);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_prependf error.");
  TEST_DSTRING(ret, &ds, str, test_str2);

  /* 3nd */
  ret = sqc_dstring_prependf(&ds, "");
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_prependf error.");
  TEST_DSTRING(ret, &ds, str, test_str2);
}

void
test_sqc_dstring_prependf_null(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  static sqc_dstring_t ds_null = NULL;

  ret = sqc_dstring_prependf(NULL, "hoge\n");
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_prependf error.");

  ret = sqc_dstring_prependf(&ds_null, "hoge\n");
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_prependf error.");

  ret = sqc_dstring_prependf(&ds, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_prependf error.");
}

static void
insert_va_list_test(const char *format, ...) __attribute__((format(printf, 1,
    2)));

static void
insert_va_list_test(const char *format, ...) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  va_list args;
  bool empty;
  char *str = NULL;
  size_t test_offset1 = 5;
  char test_str1[] = "1234 hoge foo 5678\n";

  va_start(args, format);
  /* insert. */
  ret = sqc_dstring_vinsertf(&ds, test_offset1, format, &args);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_vinsertf error.");
  TEST_DSTRING(ret, &ds, str, test_str1);
  empty = sqc_dstring_empty(&ds);
  TEST_ASSERT_EQUAL_MESSAGE(false, empty,
                            "empty error.");
  va_end(args);
}

void
test_sqc_dstring_vinsertf(void) {
  va_list_test("%d 5678\n", 1234);
  insert_va_list_test("hoge %s ", "foo");
}

void
test_sqc_dstring_vinsertf_null(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  va_list args;
  size_t test_offset1 = 0;
  static sqc_dstring_t ds_null = NULL;

  ret = sqc_dstring_vinsertf(NULL, test_offset1, "hoge\n", &args);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_vinsertf error.");

  ret = sqc_dstring_vinsertf(&ds_null, test_offset1, "hoge\n", &args);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_vinsertf error.");

  ret = sqc_dstring_vinsertf(&ds, test_offset1, NULL, &args);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_vinsertf error.");

  ret = sqc_dstring_vinsertf(&ds, test_offset1, "hoge\n", NULL);
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_vinsertf error.");
}

void
test_sqc_dstring_insertf_clear_srt_get(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  bool empty;
  char *str = NULL;
  size_t test_offset1 = 0;
  char test_str1[] = "1234 5678\n";
  size_t test_offset2 = 5;
  char test_str2[] = "1234 hoge foo\n5678\n";
  char test_str3[] = "\0";
  size_t test_offset4 = 0;
  char test_str4[] = "9876 5432\n";
  size_t test_offset5 = 5;
  char test_str5[] = "9876 hoge foo bar\n5432\n";
  size_t test_offset6 = 0;
  char test_str6[] = "bas\n9876 hoge foo bar\n5432\n";
  size_t test_offset7 = 0;
  char test_str7[] = "1 2\nbas\n9876 hoge foo bar\n5432\n";
  size_t test_offset8 = 31;
  char test_str8[] = "1 2\nbas\n9876 hoge foo bar\n5432\n3 4\n";
  size_t test_offset9 = 36;

  /* check empty. */
  empty = sqc_dstring_empty(&ds);
  TEST_ASSERT_EQUAL_MESSAGE(true, empty,
                            "empty error.");
  /* insert int. */
  ret = sqc_dstring_insertf(&ds, test_offset1, "%d 5678\n", 1234);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_insertf error.");
  TEST_DSTRING(ret, &ds, str, test_str1);
  empty = sqc_dstring_empty(&ds);
  TEST_ASSERT_EQUAL_MESSAGE(false, empty,
                            "empty error.");

  /* insert str. */
  ret = sqc_dstring_insertf(&ds, test_offset2, "hoge %s\n", "foo");

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_insertf error.");
  TEST_DSTRING(ret, &ds, str, test_str2);

  /* clear. */
  ret = sqc_dstring_clear(&ds);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_clear error.");
  TEST_DSTRING(ret, &ds, str, test_str3);

  /* insert int. */
  ret = sqc_dstring_insertf(&ds, test_offset4, "%d 5432\n", 9876);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_insertf error.");
  TEST_DSTRING(ret, &ds, str, test_str4);

  /* insert str. */
  ret = sqc_dstring_insertf(&ds, test_offset5,
                               "hoge %s %s\n", "foo", "bar");

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_insertf error.");
  TEST_DSTRING(ret, &ds, str, test_str5);

  /* not args */
  ret = sqc_dstring_insertf(&ds, test_offset6, "bas\n");

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_insertf error.");
  TEST_DSTRING(ret, &ds, str, test_str6);

  /* offset = 0. */
  ret = sqc_dstring_insertf(&ds, test_offset7, "%d 2\n", 1);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_insertf error.");
  TEST_DSTRING(ret, &ds, str, test_str7);

  /* offset = end. */
  ret = sqc_dstring_insertf(&ds, test_offset8, "%d 4\n", 3);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_insertf error.");
  TEST_DSTRING(ret, &ds, str, test_str8);

  /* offset = over. */
  ret = sqc_dstring_insertf(&ds, test_offset9, "%d 5\n", 6);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_INVALID_ARGS, ret,
                            "sqc_dstring_insertf error.");
}

void
test_sqc_dstring_insertf_empty(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *str = NULL;
  size_t test_offset1 = 0;
  char test_str1[] = "";
  char test_str2[] = "1234 5678\n";

  /* 1st */
  ret = sqc_dstring_insertf(&ds, test_offset1, "");
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_insertf error.");
  TEST_DSTRING(ret, &ds, str, test_str1);

  /* 2nd */
  ret = sqc_dstring_insertf(&ds, test_offset1, "");
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_insertf error.");
  TEST_DSTRING(ret, &ds, str, test_str1);

  /* insert int. */
  ret = sqc_dstring_insertf(&ds, test_offset1, "%d 5678\n", 1234);

  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_insertf error.");
  TEST_DSTRING(ret, &ds, str, test_str2);

  /* 3nd */
  ret = sqc_dstring_insertf(&ds, test_offset1, "");
  TEST_ASSERT_EQUAL_MESSAGE(SQC_RESULT_OK, ret,
                            "sqc_dstring_insertf error.");
  TEST_DSTRING(ret, &ds, str, test_str2);
}
