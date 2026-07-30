#include "unity.h"
#include "sqc_apis.h"


void
setUp(void) {
}

void
tearDown(void) {
}


void
test_zero_polls(void) {
  sqc_result_t ret = sqc_qmuxer_poll(NULL, NULL, 0,
                        1000 * 1000 * 1000);
  TEST_ASSERT_EQUAL(ret, SQC_RESULT_TIMEDOUT);
}


void
test_null_polls(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_qmuxer_t qmx = NULL;

  ret = sqc_qmuxer_create(&qmx);
  TEST_ASSERT_EQUAL(ret, SQC_RESULT_OK);

  if (ret == SQC_RESULT_OK) {
    sqc_bbq_t q = NULL;
    ret = sqc_bbq_create(&q, uint32_t, 1000, NULL);
    TEST_ASSERT_EQUAL(ret, SQC_RESULT_OK);

    if (ret == SQC_RESULT_OK) {
      sqc_qmuxer_poll_t polls[1];
      ret = sqc_qmuxer_poll_create(&polls[0], q,
                                      SQC_QMUXER_POLL_READABLE);
      TEST_ASSERT_EQUAL(ret, SQC_RESULT_OK);

      if (ret == SQC_RESULT_OK) {

        ret = sqc_qmuxer_poll(&qmx, polls, 0, 1000 * 1000 * 1000);
        TEST_ASSERT_EQUAL(ret, SQC_RESULT_TIMEDOUT);

        ret = sqc_qmuxer_poll(&qmx, NULL, 1, 1000 * 1000 * 1000);
        TEST_ASSERT_EQUAL(ret, SQC_RESULT_TIMEDOUT);

        ret = sqc_qmuxer_poll(NULL, polls, 0, 1000 * 1000 * 1000);
        TEST_ASSERT_EQUAL(ret, SQC_RESULT_TIMEDOUT);

        ret = sqc_qmuxer_poll(NULL, NULL, 1, 1000 * 1000 * 1000);
        TEST_ASSERT_EQUAL(ret, SQC_RESULT_TIMEDOUT);

        sqc_qmuxer_poll_destroy(&polls[0]);
      }

      sqc_bbq_destroy(&q, true);
    }

    sqc_qmuxer_destroy(&qmx);
  }
}
