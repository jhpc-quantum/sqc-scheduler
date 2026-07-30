#include <sys/types.h>
#include <errno.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <optional>

#include "job_broker.pb.h"
#include "job_broker_client.h"
#include "job_broker_ecode.h"
#include "job_broker_file_util.h"
#include "job_broker_logger_internal.h"
#include "app.h"

//
// Sends a 'submit_job' request to gRPC server.
//
static bool
s_submit_job(job_broker_client& client, std::uint32_t priority, const std::string& qprogram,
             circuit_fmt_t circuit_fmt, std::size_t shots, qc_type_t qc_type, transpiler_t transpiler,
             const std::string& remark, std::optional<std::string> user_token) {
  // Sends a 'submit_job' request.
  submit_job_reply reply;
  grpc::Status grpc_status = client.submit_job(qprogram, circuit_fmt, shots, qc_type, transpiler,
                                               remark, user_token, priority, reply);
  std::int64_t result_code = reply.code();

  // Parses the reply.
  if (!grpc_status.ok()) {
    std::cout << "gRPC error: " << grpc_status.error_message() << std::endl;
    return false;
  }

  // Prints result.
  print_grpc_common_result(result_code, reply.message());

  if (result_code == RESULT_OK) {
    if (reply.has_job_id()) {
      std::cout << "job_id: " << reply.job_id() << std::endl;
    }
  }
  return true;
}

//
// Prints help message for 'submit' sub-command.
//
static void
s_print_help_submit(const char* argv0) {
  std::string prog = get_path_basename(argv0);
  std::cout << "Usage: " << prog << " submit [OPTION...] QC-TYPE PRIORITY QPROGRAM FORMAT SHOTS"
            << std::endl;
  std::cout << "       " << prog << " submit --help"
            << std::endl
            << std::endl;

  std::cout << "Options:" << std::endl;
  std::cout << "  --conf-dir=DIR       directory where JWT token file exists" << std::endl;
  std::cout << "                       (default: " << get_default_conf_dir() << ")"
            << std::endl;
  std::cout << "  --server=HOST:PORT   gRPC server (default: env $SQC_GRPC_SERVER)"
            << std::endl;
  std::cout << "  --transpiler=TYPE    transpiler; none, pass or normal"
            << std::endl;
  std::cout << "                       (default: none)" << std::endl;
  std::cout << "  --remark=TEXT        remark text (default: empty text)" << std::endl;
  std::cout << std::endl;
  std::cout << "Arguments:" << std::endl;
  std::cout << "  QC-TYPE              QC type of the job" << std::endl;
  std::cout << "                       (rqc-rest, ibm-rest or slurm-rest)" << std::endl;
  std::cout << "  PRIORITY             priority of the job" << std::endl;
  std::cout << "  QPROGRAM             path to a program file" << std::endl;
  std::cout << "  FORMAT               format type of QPROGRAM (qasm, qir or qpy)" << std::endl;
  std::cout << "  SHOTS                the number of shots" << std::endl;
}

//
// Main function for 'submit' sub command.
//
int
do_subcmd_submit(int argc, char* argv[], int optind) {
  std::string server = get_default_server();
  std::string conf_dir = get_default_conf_dir();
  transpiler_t transpiler = TRANSPILER_NONE;
  std::string remark;
  const char* env_user_token = getenv("SQC_GRPC_USER_TOKEN");
  std::optional<std::string> user_token;

  for (; optind < argc; optind++) {
    std::string arg = argv[optind];
    std::string opt_val;
    if (arg == "--") {
      optind += 1;
      break;
    } else if (arg.compare(0u, 1, "-") != 0) {
      break;
    } else if (arg == "--help") {
      s_print_help_submit(argv[0]);
      return 0;
    } else if (parse_option_with_value(arg, "--server", server)) {
      ; // nothing to do.
    } else if (parse_option_with_value(arg, "--conf-dir", conf_dir)) {
      ; // nothing to do.
    } else if (parse_option_with_value(arg, "--remark", remark)) {
      ; // nothing to do.
    } else if (parse_option_with_value(arg, "--transpiler", opt_val)) {
      if (!parse_transpiler(opt_val, transpiler)) {
        std::cerr << "Invalid value for transpiler: " << opt_val << std::endl;
        return 1;
      }
      ; // nothing to do.
    } else {
      std::cerr << "Invalid option '" << arg << "'" << std::endl;
      return 1;
    }
  }

  // Gets the non-option arguments.
  if (optind + 5 != argc) {
    std::cerr << "The invalid number of arguments given to 'submit'" << std::endl;
    return 1;
  } else if (server.length() == 0u) {
    std::cerr << "no server specified" << std::endl;
    return 1;
  }

  qc_type_t qc_type = QC_TYPE_UNKNOWN;
  if (!parse_qc_type(argv[optind], qc_type)) {
    std::cerr << "Invalid value for QC-TYPE: " << argv[optind] << std::endl;
    return false;
  }

  std::uint32_t priority = 0u;
  try {
    priority = static_cast<std::uint32_t>(std::stoul(argv[optind + 1]));
  } catch (...) {
    std::cerr << "Invalid value for PRIORITY: " << argv[optind + 1] << std::endl;
    return false;
  }

  std::string qprogram;
  std::string qprogram_file = argv[optind + 2];
  if (!job_broker_read_file(qprogram_file, qprogram)) {
    return false;
  }

  circuit_fmt_t circuit_fmt = CIRCUIT_FMT_UNKNOWN;
  if (!parse_circuit_fmt(argv[optind + 3], circuit_fmt)) {
    std::cerr << "Invalid value for FORMAT: " << argv[optind] << std::endl;
    return false;
  }

  std::size_t shots = 0u;
  try {
    shots = static_cast<std::size_t>(std::stoul(argv[optind + 4]));
  } catch (...) {
    std::cerr << "Invalid value for SHOTS: " << argv[optind + 4] << std::endl;
    return false;
  }

  user_token = env_user_token ? std::optional<std::string>{env_user_token} : std::nullopt;

  std::cout << "Request: qc_type=" << qc_type
            << ", priority=" << priority
            << ", qprogram_file=" << qprogram_file
            << ", circuit_fmt" << circuit_fmt
            << ", shots=" << shots
            << ", transpiler=" << transpiler
            << ", remark=" << remark
            << ", user_token=" << user_token.value_or("null")
            << std::endl;

  job_broker_client* client = create_job_broker_client(server, conf_dir);
  if (client == nullptr) {
    return 1;
  }
  bool submit_result = s_submit_job(*client, priority, qprogram, circuit_fmt, shots, qc_type,
                                    transpiler, remark, user_token);
  delete client;
  return submit_result ? 0 : 1;
}
