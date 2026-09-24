#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>

#include "job_broker.pb.h"
#include "job_broker_perf_client.h"
#include "job_broker_ecode.h"
#include "job_broker_file_util.h"
#include "job_broker_logger_internal.h"

// gRPC server / root certificate.
static constexpr char s_server_root_cert_file[] = "grpc_server_root.crt";

// JWT token file.
static constexpr char s_jwt_token_file[] = "jwt.token";

//
// Converts QC type to enum qc_type_t.
//
bool
parse_qc_type(const std::string& qc_type_str, qc_type_t& qc_type) {
  if (qc_type_str == "unknown") {
    qc_type = QC_TYPE_UNKNOWN;
  } else if (qc_type_str == "rqc-rest") {
    qc_type = QC_TYPE_RQC_REST;
  } else if (qc_type_str == "ibm-rest") {
    qc_type = QC_TYPE_IBM_REST;
  } else if (qc_type_str == "slurm-rest") {
    qc_type = QC_TYPE_SLURM_REST;
  } else if (qc_type_str == "qtm-grpc") {
    qc_type = QC_TYPE_QTM_GRPC;
  } else if (qc_type_str == "qtm-sim-grpc") {
    qc_type = QC_TYPE_QTM_SIM_GRPC;
  } else if (qc_type_str == "ibm-dacc") {
    qc_type = QC_TYPE_IBM_DACC;
  } else if (qc_type_str == "a-oqtopusrest-system-token") {
    qc_type = QC_TYPE_A_OQTOPUSREST_SYSTEM_TOKEN;
  } else if (qc_type_str == "a-oqtopusrest-user-token") {
    qc_type = QC_TYPE_A_OQTOPUSREST_USER_TOKEN;
  } else if (qc_type_str == "a-oqtopusrest-both-token") {
    qc_type = QC_TYPE_A_OQTOPUSREST_BOTH_TOKEN;
  } else if (qc_type_str == "dummy") {
    qc_type = QC_TYPE_DUMMY;
  } else {
    try {
      qc_type = static_cast<qc_type_t>(std::stoul(qc_type_str));
    } catch (...) {
      return false;
    }
  }
  return true;
}

//
// Converts tranpiler to enum transpiler_t.
//
bool
parse_transpiler(const std::string& transpiler_str, transpiler_t& transpiler) {
  if (transpiler_str == "unknown") {
    transpiler = TRANSPILER_UNKNOWN;
  } else if (transpiler_str == "none") {
    transpiler = TRANSPILER_NONE;
  } else if (transpiler_str == "pass") {
    transpiler = TRANSPILER_PASS;
  } else if (transpiler_str == "normal") {
    transpiler = TRANSPILER_NORMAL;
  } else {
    try {
      transpiler = static_cast<transpiler_t>(std::stoul(transpiler_str));
    } catch (...) {
      return false;
    }
  }
  return true;
}

//
// Converts cuircuit format to enum circuit_fmt_t.
//
bool
parse_circuit_fmt(const std::string& circuit_fmt_str, circuit_fmt_t& circuit_fmt) {
  if (circuit_fmt_str == "unknown") {
    circuit_fmt = CIRCUIT_FMT_UNKNOWN;
  } else if (circuit_fmt_str == "qasm") {
    circuit_fmt = CIRCUIT_FMT_QASM;
  } else if (circuit_fmt_str == "qir") {
    circuit_fmt = CIRCUIT_FMT_QIR;
  } else if (circuit_fmt_str == "qpy") {
    circuit_fmt = CIRCUIT_FMT_QPY;
  } else if (circuit_fmt_str == "json") {
    circuit_fmt = CIRCUIT_FMT_JSON;
  } else {
    try {
      circuit_fmt = static_cast<circuit_fmt_t>(std::stoul(circuit_fmt_str));
    } catch (...) {
      return false;
    }
  }
  return true;
}

//
// Parses an option with the value.
// It assumes the option has the form '--<option name>=<value>'.
//
bool
parse_option_with_value(const std::string& arg, const std::string& option_name,
                          std::string& option_value) {
  if (arg.size() < option_name.size() + 1 || arg[option_name.size()] != '=') {
    return false;
  } else if (arg.compare(0u, option_name.size(), option_name) != 0) {
    return false;
  }

  option_value = arg.substr(option_name.size() + 1);
  return true;
}

bool
parse_uint32(const std::string& arg, std::uint32_t *value) {
  try {
    unsigned long parsed_val = std::stoul(arg, nullptr, 10);
    if (parsed_val > static_cast<unsigned long>(std::numeric_limits<std::uint32_t>::max())) {
        return false;
    }

    *value = static_cast<std::uint32_t>(parsed_val);
    return true;
  } catch (const std::invalid_argument& e) {
      return false;
  } catch (const std::out_of_range& e) {
      return false;
  }
}

//
// Creates a gRPC client.
//
job_broker_perf_client*
create_job_broker_perf_client(const std::string& server, const std::string& conf_dir) {
  std::string cert;
  try {
    if (job_broker_read_file_in_dir(conf_dir, s_server_root_cert_file, cert)) {
      msg_debug(5,"Read the server root certificate: file=%s/%s\n", conf_dir.c_str(),
                s_server_root_cert_file);
    } else {
      msg_error("Failed to read the server root certificate: file=%s/%s\n", conf_dir.c_str(),
                s_server_root_cert_file);
    }
  } catch (...) {
    msg_error("An exception occurred whild reading the server root certificate: file=%s/%s\n",
              conf_dir.c_str(), s_server_root_cert_file);
    return nullptr;
  }

  std::string token;
  try {
    if (job_broker_read_file_in_dir(conf_dir, s_jwt_token_file, token)) {
      msg_debug(5, "Read the JWT token: file=%s/%s\n", conf_dir.c_str(), s_jwt_token_file);
    } else {
      msg_debug(5, "Failed to read the JWT token: file=%s/%s\n", conf_dir.c_str(),
                s_jwt_token_file);
    }

    grpc::SslCredentialsOptions ssl_opts;
    ssl_opts.pem_root_certs = cert;
    return new job_broker_perf_client(grpc::CreateChannel(server, grpc::SslCredentials(ssl_opts)),
                                      token);
  } catch (...) {
    msg_error("An exception occurred whild reading the JWT token: file=%s/%s\n",
              conf_dir.c_str(), s_jwt_token_file);
    return nullptr;
  }
}

//
// Prints a log message to stderr.
//
void emit_log(int log_level, uint64_t debug_level, const char* file, int line,
                const char* func, const char* fmt, ...) {
  static const char* const s_log_level_strs[] = {
    "",
    "[DEBUG] ",
    "[INFO ] ",
    "[NOTE ] ",
    "[WARN ] ",
    "[ERROR] ",
    "[FATAL] ",
    nullptr
  };

  static_cast<void>(debug_level);
  static_cast<void>(file);
  static_cast<void>(line);
  static_cast<void>(func);

  if (log_level >= LOG_LEVEL_UNKNOWN && log_level <= LOG_LEVEL_FATAL) {
    std::cerr << s_log_level_strs[log_level];
  }

  va_list args;
  char msg[8192];

  va_start(args, fmt);
  vsnprintf(msg, sizeof(msg), fmt, args);
  va_end(args);

  std::cerr << msg;
}

//
// Get location of the default server.
//
const std::string
get_default_server() {
  const char* value = getenv("SQC_GRPC_SERVER");
  if (value != nullptr) {
    return value;
  } else {
    return "";
  }
}

//
// Get path to the default configuration directory.
//
const std::string
get_default_conf_dir() {
  static const std::string default_conf_dir = "~/.sqc-scheduler";
  return default_conf_dir;
}

//
// Get basename of the given path.
//
std::string
get_path_basename(const std::string& path) {
  size_t last_slash = path.find_last_of('/');
  if (last_slash == std::string::npos) {
    return path;
  }
  return path.substr(last_slash + 1);
}
