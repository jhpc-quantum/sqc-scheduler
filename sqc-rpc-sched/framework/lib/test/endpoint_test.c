#include "unity.h"
#include "sqc_apis.h"
#include "../endpoint.c"

void
setUp(void) {
}

void
tearDown(void) {
}

void
test_parse_endpoint_v4addr(void) {
  char host[4096] = {0};
  int port = -1;
  af_guess_t guess = SEEMS_UNKNOWN;
  bool is_onlynum = false;
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  host[0] = '\0';
  ret = parse_endpoint("127.0.0.1", host, sizeof(host),
                       &port, &guess, &is_onlynum);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, ret);
  TEST_ASSERT_EQUAL_STRING_LEN("127.0.0.1", host, sizeof(host));
  TEST_ASSERT_EQUAL_INT(0, port);
  TEST_ASSERT_EQUAL_INT(SEEMS_V4, guess);
  TEST_ASSERT_EQUAL_INT(true, is_onlynum);
}

void
test_parse_endpoint_v4addr_with_port(void) {
  char host[4096] = {0};
  int port = -1;
  af_guess_t guess = SEEMS_UNKNOWN;
  bool is_onlynum = false;
  sqc_result_t ret = parse_endpoint("127.0.0.1:8080",
                                    host, sizeof(host), &port, &guess,
                                    &is_onlynum);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, ret);
  TEST_ASSERT_EQUAL_STRING_LEN("127.0.0.1", host, sizeof(host));
  TEST_ASSERT_EQUAL_INT(port, 8080);
  TEST_ASSERT_EQUAL_INT(SEEMS_V4, guess);
  TEST_ASSERT_EQUAL_INT(true, is_onlynum);
}

void
test_parse_endpoint_v6addr(void) {
  char host[4096] = {'\0'};
  int port = -1;
  af_guess_t guess = SEEMS_UNKNOWN;
  bool is_onlynum = false;
  sqc_result_t ret = parse_endpoint("fe80::215:5dff:fe26:2a5",
                                    host, sizeof(host), &port, &guess,
                                    &is_onlynum);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, ret);
  TEST_ASSERT_EQUAL_STRING_LEN("fe80::215:5dff:fe26:2a5", host, sizeof(host));
  TEST_ASSERT_EQUAL_INT(0, port);
  TEST_ASSERT_EQUAL_INT(SEEMS_V6, guess);
  TEST_ASSERT_EQUAL_INT(true, is_onlynum);
  TEST_ASSERT_EQUAL_INT(true, is_onlynum);
}

void
test_parse_endpoint_v6addr_localhost(void) {
  char host[4096] = {'\0'};
  int port = -1;
  af_guess_t guess = SEEMS_UNKNOWN;
  bool is_onlynum = false;
  sqc_result_t ret = parse_endpoint("::1",
                                    host, sizeof(host), &port, &guess,
                                    &is_onlynum);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, ret);
  TEST_ASSERT_EQUAL_STRING_LEN("::1", host, sizeof(host));
  TEST_ASSERT_EQUAL_INT(0, port);
  TEST_ASSERT_EQUAL_INT(SEEMS_V6, guess);
  TEST_ASSERT_EQUAL_INT(true, is_onlynum);
}

void
test_parse_endpoint_v6addr_any(void) {
  char host[4096] = {'\0'};
  int port = -1;
  af_guess_t guess = SEEMS_UNKNOWN;
  bool is_onlynum = false;
  sqc_result_t ret = parse_endpoint("::",
                                    host, sizeof(host), &port, &guess,
                                    &is_onlynum);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, ret);
  TEST_ASSERT_EQUAL_STRING_LEN("::", host, sizeof(host));
  TEST_ASSERT_EQUAL_INT(0, port);
  TEST_ASSERT_EQUAL_INT(SEEMS_V6, guess);
  TEST_ASSERT_EQUAL_INT(true, is_onlynum);
}

void
test_parse_endpoint_v6addr_with_port(void) {
  char host[4096] = {'\0'};
  int port = -1;
  af_guess_t guess = SEEMS_UNKNOWN;
  bool is_onlynum = false;
  sqc_result_t ret = parse_endpoint("[fe80::215:5dff:fe26:62a5]:8080",
                                    host, sizeof(host), &port, &guess,
                                    &is_onlynum);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, ret);
  TEST_ASSERT_EQUAL_STRING_LEN("fe80::215:5dff:fe26:62a5", host, sizeof(host));
  TEST_ASSERT_EQUAL_INT(8080, port);
  TEST_ASSERT_EQUAL_INT(SEEMS_V6, guess);
  TEST_ASSERT_EQUAL_INT(true, is_onlynum);
}

void
test_parse_endpoint_v6addr_localhost_with_port(void) {
  char host[4096] = {'\0'};
  int port = -1;
  af_guess_t guess = SEEMS_UNKNOWN;
  bool is_onlynum = false;
  sqc_result_t ret = parse_endpoint("[::1]:8080",
                                    host, sizeof(host), &port, &guess,
                                    &is_onlynum);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, ret);
  TEST_ASSERT_EQUAL_STRING_LEN("::1", host, sizeof(host));
  TEST_ASSERT_EQUAL_INT(8080, port);
  TEST_ASSERT_EQUAL_INT(SEEMS_V6, guess);
  TEST_ASSERT_EQUAL_INT(true, is_onlynum);
}

void
test_parse_endpoint_v6addr_any_with_port(void) {
  char host[4096] = {'\0'};
  int port = -1;
  af_guess_t guess = SEEMS_UNKNOWN;
  bool is_onlynum = false;
  sqc_result_t ret = parse_endpoint("[::]:8080",
                                    host, sizeof(host), &port, &guess,
                                    &is_onlynum);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, ret);
  TEST_ASSERT_EQUAL_STRING_LEN("::", host, sizeof(host));
  TEST_ASSERT_EQUAL_INT(8080, port);
  TEST_ASSERT_EQUAL_INT(SEEMS_V6, guess);
  TEST_ASSERT_EQUAL_INT(true, is_onlynum);
}

void
test_parse_endpoint_hostname(void) {
  char host[4096] = {'\0'};
  int port = -1;
  af_guess_t guess = SEEMS_V4;
  bool is_onlynum = true;
  sqc_result_t ret = parse_endpoint("example",
                                    host, sizeof(host), &port, &guess,
                                    &is_onlynum);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, ret);
  TEST_ASSERT_EQUAL_STRING_LEN("example", host, sizeof(host));
  TEST_ASSERT_EQUAL_INT(0, port);
  TEST_ASSERT_EQUAL_INT(SEEMS_UNKNOWN, guess);
  TEST_ASSERT_EQUAL_INT(false, is_onlynum);
}

void
test_parse_endpoint_hostname_with_port(void) {
  char host[4096] = {'\0'};
  int port = -1;
  af_guess_t guess = SEEMS_V4;
  bool is_onlynum = true;
  sqc_result_t ret = parse_endpoint("example:8080",
                                    host, sizeof(host), &port, &guess,
                                    &is_onlynum);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, ret);
  TEST_ASSERT_EQUAL_STRING_LEN("example", host, sizeof(host));
  TEST_ASSERT_EQUAL_INT(8080, port);
  TEST_ASSERT_EQUAL_INT(SEEMS_UNKNOWN, guess);
  TEST_ASSERT_EQUAL_INT(false, is_onlynum);
}

void
test_parse_endpoint_domainname(void) {
  char host[4096] = {'\0'};
  int port = -1;
  af_guess_t guess = SEEMS_V4;
  bool is_onlynum = true;
  sqc_result_t ret = parse_endpoint("example.com",
                                    host, sizeof(host), &port, &guess,
                                    &is_onlynum);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, ret);
  TEST_ASSERT_EQUAL_STRING_LEN("example.com", host, sizeof(host));
  TEST_ASSERT_EQUAL_INT(0, port);
  TEST_ASSERT_EQUAL_INT(SEEMS_UNKNOWN, guess);
  TEST_ASSERT_EQUAL_INT(false, is_onlynum);
}

void
test_parse_endpoint_domainname_with_port(void) {
  char host[4096] = {'\0'};
  int port = -1;
  af_guess_t guess = SEEMS_V4;
  bool is_onlynum = true;
  sqc_result_t ret = parse_endpoint("example.com:8080",
                                    host, sizeof(host), &port, &guess,
                                    &is_onlynum);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, ret);
  TEST_ASSERT_EQUAL_STRING_LEN("example.com", host, sizeof(host));
  TEST_ASSERT_EQUAL_INT(8080, port);
  TEST_ASSERT_EQUAL_INT(SEEMS_UNKNOWN, guess);
  TEST_ASSERT_EQUAL_INT(false, is_onlynum);
}
