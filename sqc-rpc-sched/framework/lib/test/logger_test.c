#include "sqc_apis.h"
#include "unity.h"

#include "../logger.c"

void
setUp(void) {
}


void
tearDown(void) {
  unlink("hogehoge");
}

void
test_s_once_proc(void) {
  setenv("SQC_LOG_LEVEL", "2", 1);
  setenv("SQC_LOG_DEBUGLEVEL", "10", 1);
  setenv("SQC_LOG_FILE", "hogehoge", 1);
  setenv("SQC_LOG_ROTATESIZE", "512k", 1);
  s_once_proc();
  TEST_ASSERT_EQUAL(2, s_log_level);
  TEST_ASSERT_EQUAL(10, s_dbg_level);
  TEST_ASSERT_EQUAL(512*1024, s_rotate_size);
  unsetenv("SQC_LOG_LEVEL");
  unsetenv("SQC_LOG_DEBUGLEVEL");
  unsetenv("SQC_LOG_FILE");
  unsetenv("SQC_LOG_ROTATESIZE");
}

void
test_s_get_date_str(void) {
  int ret;
  char buf[BUFSIZ];
  ret = s_get_rotate_str(buf, 19, "hoge", 0);
  TEST_ASSERT_EQUAL(-1, ret);

  ret = s_get_rotate_str(buf, 21, "hoge", 1);
  TEST_ASSERT_EQUAL(-1, ret);
}

void
test_s_get_date_str_fail(void) {
  int ret;
  char buf[BUFSIZ];
  const char *fn = "hoge.";
  ret = s_get_rotate_str(buf, sizeof(buf), "hoge", 0);
  TEST_ASSERT_EQUAL(20, ret);
  TEST_ASSERT_EQUAL_STRING_LEN(fn, buf, strlen(fn));

  ret = s_get_rotate_str(buf, sizeof(buf), "hoge", 1);
  TEST_ASSERT_EQUAL(22, ret);
  TEST_ASSERT_EQUAL_STRING_LEN(fn, buf, strlen(fn));
  TEST_ASSERT_EQUAL('-', buf[ret - 2]);
  TEST_ASSERT_EQUAL('1', buf[ret - 1]);
}

void
test_s_get_date_str_checked(void) {
  int ret;
  char buf[BUFSIZ];
  char buf1[BUFSIZ];
  ret = s_get_rotate_str_checked(buf, sizeof(buf), "hoge");
  TEST_ASSERT_EQUAL(0, ret);
  s_get_rotate_str(buf, sizeof(buf), "hoge", 0);
  fopen(buf, "w");
  ret = s_get_rotate_str_checked(buf1, sizeof(buf1), "hoge");
  TEST_ASSERT_EQUAL(0, ret);
  TEST_ASSERT_EQUAL('-', buf1[strlen(buf1) - 2]);
  TEST_ASSERT_EQUAL('1', buf1[strlen(buf1) - 1]);
  remove(buf);
}

void
test_s_get_date_str_checked_fail(void) {
  int ret, o;
  char buf[BUFSIZ];
  char buf1[BUFSIZ];
  s_get_rotate_str(buf, sizeof(buf), "hoge", 0);
  fopen(buf, "w");
  o = s_max_rotate_suffix;
  s_max_rotate_suffix = 1;
  ret = s_get_rotate_str_checked(buf1, sizeof(buf1), "hoge");
  TEST_ASSERT_EQUAL(-1, ret);
  remove(buf);
  s_max_rotate_suffix = o;
}

void
test_s_get_magnification(void) {
  char units[] = "kKmMgG ";
  int mags[] = {1024, 1024, 1024*1024, 1024*1024, 1024*1024*1024, 1024*1024*1024, 1};
  for(int i = 0; i < (int) sizeof(units) - 1;i++) {
    TEST_ASSERT_EQUAL(mags[i], s_get_magnification(units[i]));
  }
}

void
test_sqc_log_set_get_log_level(void) {
  int ret;

  sqc_log_set_log_level(4);
  ret = sqc_log_get_log_level();

  TEST_ASSERT_EQUAL(4, ret);
}

void
test_sqc_log_set_get_rotate_size(void) {
  size_t ret;

  sqc_log_set_rotate_size(4*1024);
  ret = sqc_log_get_rotate_size();

  TEST_ASSERT_EQUAL(4*1024, ret);
}

void
test_sqc_log_set_get_filename(void) {
  const char *fn = "hoge.";
  const char *rfn;

  sqc_log_set_filename(fn);
  rfn = sqc_log_get_filename();

  TEST_ASSERT_EQUAL_STRING_LEN(fn, rfn, strlen(fn));
}
