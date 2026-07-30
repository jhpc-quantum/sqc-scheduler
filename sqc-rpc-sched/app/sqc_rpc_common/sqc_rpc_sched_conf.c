#include "sqc_apis.h"
#include "sqc_rpc_sched_conf.h"
#include "sqc_rpc_sched_conv_enums.h"

static sqc_rpc_sched_qc_type_t s_qc_type = SQC_RPC_SCHED_QC_TYPE_UNKNOWN;
static char *s_conf_dir = NULL;
static char *s_rpc_server_address = NULL;
static char *s_grpc_server_address = NULL;

/*
 * export
 */

sqc_rpc_sched_qc_type_t
sqc_rpc_sched_conf_get_qc_type(void) {
  return s_qc_type;
}


void
sqc_rpc_sched_conf_set_qc_type(sqc_rpc_sched_qc_type_t qc_type) {
  sqc_msg_info("Set qc-type=%d (%s)\n",
               (int) qc_type, sqc_rpc_sched_qc_type_to_string(qc_type));
  s_qc_type = qc_type;
}


sqc_result_t
sqc_rpc_sched_conf_set_qc_type_from_string(const char *str) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_UNKNOWN;

  if (likely(sqc_rpc_sched_qc_type_from_string(str, &qc_type) == true)) {
    sqc_rpc_sched_conf_set_qc_type(qc_type);
    ret = SQC_RESULT_OK;
  }

  return ret;
}


const char *
sqc_rpc_sched_conf_get_conf_dir(void) {
  return s_conf_dir;
}


sqc_result_t
sqc_rpc_sched_conf_set_conf_dir(const char *dir) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(dir != NULL)) {
    char *tmp_dir = strdup(dir);
    if (likely(tmp_dir != NULL)) {
      free(s_conf_dir);
      s_conf_dir = tmp_dir;
      ret = SQC_RESULT_OK;
      sqc_msg_debug(5, "Set conf-dir=%s\n", dir);
    } else {
      ret = SQC_RESULT_NO_MEMORY;
      sqc_msg_error("%s\n", sqc_error_get_string(ret));
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


const char *
sqc_rpc_sched_conf_get_rpc_server_address(void) {
  return s_rpc_server_address;
}


sqc_result_t
sqc_rpc_sched_conf_set_rpc_server_address(const char *addr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(addr != NULL)) {
    char *tmp_addr = strdup(addr);
    if (likely(tmp_addr != NULL)) {
      free(s_rpc_server_address);
      s_rpc_server_address = tmp_addr;
      ret = SQC_RESULT_OK;
      sqc_msg_info("Set rpc-server-address=%s\n", addr);
    } else {
      ret = SQC_RESULT_NO_MEMORY;
      sqc_msg_error("%s\n", sqc_error_get_string(ret));
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


const char *
sqc_rpc_sched_conf_get_grpc_server_address(void) {
  return s_grpc_server_address;
}


sqc_result_t
sqc_rpc_sched_conf_set_grpc_server_address(const char *addr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(addr != NULL)) {
    char *tmp_addr = strdup(addr);
    if (likely(tmp_addr != NULL)) {
      free(s_grpc_server_address);
      s_grpc_server_address = tmp_addr;
      ret = SQC_RESULT_OK;
      sqc_msg_info("Set grpc-gserver-address=%s\n", addr);
    } else {
      ret = SQC_RESULT_NO_MEMORY;
      sqc_msg_error("%s\n", sqc_error_get_string(ret));
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}

