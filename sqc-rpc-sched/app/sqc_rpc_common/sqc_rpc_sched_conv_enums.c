#include "sqc_apis.h"
#include "sqc_rpc_sched_enums.h"
#include "sqc_rpc_sched_conv_enums.h"


//
// Convert 'str' to a value of sqc_rpc_sched_circuit_fmt_t.
//
bool
sqc_rpc_sched_circuit_fmt_from_string(const char *str, sqc_rpc_sched_circuit_fmt_t *circuit_fmt) {
  bool ret = false;

  if (likely(str != NULL && circuit_fmt != NULL)) {
    if (strcmp(str, "unknown") == 0) {
      *circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_UNKNOWN;
      ret = true;
    } else if (strcmp(str, "qasm") == 0) {
      *circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QASM;
      ret = true;
    } else if (strcmp(str, "qir") == 0) {
      *circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QIR;
      ret = true;
    } else if (strcmp(str, "qpy") == 0) {
      *circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_QPY;
      ret = true;
    } else if (strcmp(str, "json") == 0) {
      *circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_JSON;
      ret = true;
    } else {
      unsigned long value;
      char *endp = NULL;
      errno = 0;
      value = strtoul(str, &endp, 10);
      if (errno == 0 && endp != NULL && *endp == '\0' && endp != str) {
        *circuit_fmt = (sqc_rpc_sched_circuit_fmt_t) value;
        ret = true;
      }
    }
  }

  return ret;
}


//
// Return a string corresponding with 'circuit_fmt'.
//
const char *
sqc_rpc_sched_circuit_fmt_to_string(sqc_rpc_sched_circuit_fmt_t circuit_fmt) {
  switch (circuit_fmt) {
    case SQC_RPC_SCHED_CIRCUIT_FMT_UNKNOWN: {
      return "unknown";
    }
    case SQC_RPC_SCHED_CIRCUIT_FMT_QASM: {
      return "qasm";
    }
    case SQC_RPC_SCHED_CIRCUIT_FMT_QIR: {
      return "qir";
    }
    case SQC_RPC_SCHED_CIRCUIT_FMT_QPY: {
      return "qpy";
    }
    case SQC_RPC_SCHED_CIRCUIT_FMT_JSON: {
      return "json";
    }
    default: {
      return "?";
    }
  }
}


//
// Convert 'str' to a value of sqc_rpc_sched_qc_type_t.
//
bool
sqc_rpc_sched_qc_type_from_string(const char *str, sqc_rpc_sched_qc_type_t *qc_type) {
  bool ret = false;

  if (likely(str != NULL && qc_type != NULL)) {
    if (strcmp(str, "unknown") == 0) {
      *qc_type = SQC_RPC_SCHED_QC_TYPE_UNKNOWN;
      ret = true;
    } else if (strcmp(str, "rqc-rest") == 0) {
      *qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
      ret = true;
    } else if (strcmp(str, "ibm-rest") == 0) {
      *qc_type = SQC_RPC_SCHED_QC_TYPE_IBM_REST;
      ret = true;
    } else if (strcmp(str, "slurm-rest") == 0) {
      *qc_type = SQC_RPC_SCHED_QC_TYPE_SLURM_REST;
      ret = true;
    } else if (strcmp(str, "qtm-grpc") == 0) {
      *qc_type = SQC_RPC_SCHED_QC_TYPE_QTM_GRPC;
      ret = true;
    } else if (strcmp(str, "qtm-sim-grpc") == 0) {
      *qc_type = SQC_RPC_SCHED_QC_TYPE_QTM_SIM_GRPC;
      ret = true;
    } else if (strcmp(str, "ibm-dacc") == 0) {
      *qc_type = SQC_RPC_SCHED_QC_TYPE_IBM_DACC;
      ret = true;
    } else if (strcmp(str, "a-oqtopusrest-system-token") == 0) {
      *qc_type = SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_SYSTEM_TOKEN;
      ret = true;
    } else if (strcmp(str, "a-oqtopusrest-user-token") == 0) {
      *qc_type = SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_USER_TOKEN;
      ret = true;
    } else if (strcmp(str, "a-oqtopusrest-both-token") == 0) {
      *qc_type = SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_BOTH_TOKEN;
      ret = true;
    } else if (strcmp(str, "dummy") == 0) {
      *qc_type = SQC_RPC_SCHED_QC_TYPE_DUMMY;
      ret = true;
    } else {
      unsigned long value;
      char *endp = NULL;
      errno = 0;
      value = strtoul(str, &endp, 10);
      if (errno == 0 && endp != NULL && *endp == '\0' && endp != str) {
        *qc_type = (sqc_rpc_sched_qc_type_t) value;
        ret = true;
      }
    }
  }

  return ret;
}


//
// Return a string corresponding with 'qc_type'.
//
const char *
sqc_rpc_sched_qc_type_to_string(sqc_rpc_sched_qc_type_t qc_type) {
  switch (qc_type) {
    case SQC_RPC_SCHED_QC_TYPE_UNKNOWN: {
      return "unknown";
    }
    case SQC_RPC_SCHED_QC_TYPE_RQC_REST: {
      return "rqc-rest";
    }
    case SQC_RPC_SCHED_QC_TYPE_IBM_REST: {
      return "ibm-rest";
    }
    case SQC_RPC_SCHED_QC_TYPE_SLURM_REST: {
      return "slurm-rest";
    }
    case SQC_RPC_SCHED_QC_TYPE_QTM_GRPC: {
      return "qtm-grpc";
    }
    case SQC_RPC_SCHED_QC_TYPE_QTM_SIM_GRPC: {
      return "qtm-sim-grpc";
    }
    case SQC_RPC_SCHED_QC_TYPE_IBM_DACC: {
      return "ibm-dacc";
    }
    case SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_SYSTEM_TOKEN: {
      return "a-oqtopusrest-system-token";
    }
    case SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_USER_TOKEN: {
      return "a-oqtopusrest-user-token";
    }
    case SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_BOTH_TOKEN: {
      return "a-oqtopusrest-both-token";
    }
    case SQC_RPC_SCHED_QC_TYPE_DUMMY: {
      return "dummy";
    }
    default: {
      return "?";
    }
  }
}


//
// Convert 'str' to a value of sqc_rpc_sched_transpiler_t.
//
bool
sqc_rpc_sched_transpiler_from_string(const char *str,
                                     sqc_rpc_sched_transpiler_t *transpiler) {
  bool ret = false;

  if (likely(str != NULL && transpiler != NULL)) {
    if (strcmp(str, "unknown") == 0) {
      *transpiler = SQC_RPC_SCHED_TRANSPILER_UNKNOWN;
      ret = true;
    } else if (strcmp(str, "none") == 0) {
      *transpiler = SQC_RPC_SCHED_TRANSPILER_NONE;
      ret = true;
    } else if (strcmp(str, "pass") == 0) {
      *transpiler = SQC_RPC_SCHED_TRANSPILER_PASS;
      ret = true;
    } else if (strcmp(str, "normal") == 0) {
      *transpiler = SQC_RPC_SCHED_TRANSPILER_NORMAL;
      ret = true;
    } else {
      unsigned long value;
      char *endp = NULL;
      errno = 0;
      value = strtoul(str, &endp, 10);
      if (errno == 0 && endp != NULL && *endp == '\0' && endp != str) {
        *transpiler = (sqc_rpc_sched_transpiler_t) value;
        ret = true;
      }
    }
  }

  return ret;
}


//
// Return a string corresponding with 'transpiler'.
//
const char *
sqc_rpc_sched_transpiler_to_string(sqc_rpc_sched_transpiler_t transpiler) {
  switch (transpiler) {
    case SQC_RPC_SCHED_TRANSPILER_UNKNOWN: {
      return "unknown";
    }
    case SQC_RPC_SCHED_TRANSPILER_NONE: {
      return "none";
    }
    case SQC_RPC_SCHED_TRANSPILER_PASS: {
      return "pass";
    }
    case SQC_RPC_SCHED_TRANSPILER_NORMAL: {
      return "normal";
    }
    default: {
      return "?";
    }
  }
}


//
// Return a string corresponding with 'status'.
//
const char *
sqc_rpc_sched_job_status_to_string(sqc_rpc_sched_job_status_t status) {
  switch (status) {
    case SQC_RPC_SCHED_JOB_STATUS_UNKNOWN: {
      return "unknown";
    }
    case SQC_RPC_SCHED_JOB_STATUS_CREATED: {
      return "created";
    }
    case SQC_RPC_SCHED_JOB_STATUS_QUEUED: {
      return "queued";
    }
    case SQC_RPC_SCHED_JOB_STATUS_RUNNING: {
      return "running";
    }
    case SQC_RPC_SCHED_JOB_STATUS_DONE: {
      return "done";
    }
    case SQC_RPC_SCHED_JOB_STATUS_CANCELLED: {
      return "cancelled";
    }
    case SQC_RPC_SCHED_JOB_STATUS_ERROR: {
      return "error";
    }
    case SQC_RPC_SCHED_JOB_STATUS_DELETED: {
      return "deleted";
    }
    default: {
      return "?";
    }
  }
}
