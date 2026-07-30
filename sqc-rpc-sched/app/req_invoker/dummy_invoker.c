#include <unistd.h>

#include "sqc_apis.h"
#include "sqc_rpc_sched.h"

#include "dbmgr.h"

#define SQC_RPC_SCHED_DUMMY_JOB_EXE_TIME 10 * 1000

#define DUMMY_RESULT "{" \
                     "   \"counts\": {" \
                     "      \"0011\": 227," \
                     "      \"0100\": 273," \
                     "      \"1001\": 257," \
                     "      \"1010\": 267 " \
                     "    }," \
                     "   \"properties\": {" \
                     "     \"0\": {" \
                     "       \"qubit_index\": 0," \
                     "       \"measurement_window_index\": 0" \
                     "     }," \
                     "     \"1\": {" \
                     "       \"qubit_index\": 1," \
                     "       \"measurement_window_index\": 0" \
                     "     }," \
                     "     \"2\": {" \
                     "       \"qubit_index\": 2," \
                     "       \"measurement_window_index\": 0" \
                     "     }," \
                     "     \"3\": {" \
                     "       \"qubit_index\": 3," \
                     "       \"measurement_window_index\": 0" \
                     "     }" \
                     "   }," \
                     "   \"transpiler_info\": {" \
                     "     \"physical_virtual_mapping\": {" \
                     "       \"0\": 3," \
                     "       \"1\": 1," \
                     "       \"2\": 0," \
                     "       \"3\": 2 " \
                     "     }" \
                     "   }," \
                     "   \"message\": \"SUCCESS!(dummy)\"" \
                     "}"

static inline void
s_dummy_invoke(const dbmgr_job_info_t ji_ptr) {
  const size_t dummy_result_len = strlen(DUMMY_RESULT);
  char *remark = NULL;

  if (dbmgr_set_job_status_running(ji_ptr) == SQC_RESULT_OK) {
    // Sleep for the virtual job's execution time.
    usleep(SQC_RPC_SCHED_DUMMY_JOB_EXE_TIME);

    // If "fail" is set to remark, the job fails.
    if (dbmgr_ji_get_remark(ji_ptr, &remark) == SQC_RESULT_OK &&
        IS_VALID_STRING(remark) == true &&
        strcmp(remark, "fail") == 0) {
      sqc_msg_error("Failed to retrieve information to calculate.\n");

      if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
        sqc_msg_error("Failed to transition status to error.\n");
      }
    } else {
      if (dbmgr_ji_set_result(ji_ptr, DUMMY_RESULT, dummy_result_len) == SQC_RESULT_OK) {
        sqc_msg_info("Successfully saved the results.\n");

        if (dbmgr_set_job_status_done(ji_ptr) != SQC_RESULT_OK) {
          sqc_msg_error("Failed to transition status to done.\n");
        }
      } else {
        sqc_msg_error("Failed to save results.\n");

        if (dbmgr_set_job_status_error(ji_ptr) != SQC_RESULT_OK) {
          sqc_msg_error("Failed to transition status to error.\n");
        }
      }
    }
  } else {
    sqc_msg_error("Failed to transition status to running.\n");
  }
}

