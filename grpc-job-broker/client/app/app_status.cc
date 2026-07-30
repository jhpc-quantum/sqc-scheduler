#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <errno.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "job_broker.pb.h"
#include "job_broker_client.h"
#include "job_broker_ecode.h"
#include "job_broker_file_util.h"
#include "job_broker_logger_internal.h"
#include "app.h"

//
// Sends a 'job_status' requests to gRPC server.
//
static bool
s_job_status(job_broker_client& client, const std::string& job_id) {
  // Sends a 'job_status' requests.
  job_status_reply reply;
  grpc::Status grpc_status = client.job_status(job_id, reply);
  std::int64_t result_code = reply.code();
  job_status_t job_status = reply.status();

  if (!grpc_status.ok()) {
    std::cout << "gRPC error: " << grpc_status.error_message() << std::endl;
    return false;
  }

  // Prints result.
  print_grpc_common_result(result_code, reply.message());

  if (result_code == RESULT_OK) {
    std::cout << "job status: " << std::endl;
    std::cout << "  status: " << static_cast<long long>(job_status) << std::endl;
    if (reply.has_qc_job_id()) {
      std::cout << "  qc_job_id: " << reply.qc_job_id() << std::endl;
    }
    if (reply.has_result()) {
      std::cout << "  qc_result: " << reply.result() << std::endl;
    }
  }

  return (result_code == RESULT_OK && job_status != JOB_STATUS_UNKNOWN &&
          job_status != JOB_STATUS_ERROR);
}

//
// Prints help message for 'status' sub-command.
//
static void
s_print_help_status(const char* argv0) {
  std::string prog = get_path_basename(argv0);
  std::cout << "Usage: " << prog << " status [OPTION...] JOB"
            << std::endl;
  std::cout << "       " << prog << " status --help"
            << std::endl
            << std::endl;

  std::cout << "Options:" << std::endl;
  std::cout << "  --conf-dir=DIR       directory where JWT token file exists" << std::endl;
  std::cout << "                       (default: " << get_default_conf_dir() << ")"
            << std::endl;
  std::cout << "  --server=HOST:PORT   gRPC server (default: env $SQC_GRPC_SERVER)"
            << std::endl;
  std::cout << std::endl;
  std::cout << "Arguments:" << std::endl;
  std::cout << "  JOB                  job ID" << std::endl;
}

//
// Main function for 'status' sub command.
//
int
do_subcmd_status(int argc, char* argv[], int optind) {
  std::string server = get_default_server();
  std::string conf_dir = get_default_conf_dir();

  for (; optind < argc; optind++) {
    std::string arg = argv[optind];
    if (arg == "--") {
      optind += 1;
      break;
    } else if (arg.compare(0u, 1, "-") != 0) {
      break;
    } else if (arg == "--help") {
      s_print_help_status(argv[0]);
      return 0;
    } else if (parse_option_with_value(arg, "--server", server)) {
      ; // nothing to do.
    } else if (parse_option_with_value(arg, "--conf-dir", conf_dir)) {
      ; // nothing to do.
    } else {
      std::cerr << "Invalid option '" << arg << "'" << std::endl;
      return 1;
    }
  }

  if (optind + 1 != argc) {
    std::cerr << "The invalid number of arguments given to 'status'" << std::endl;
    return 1;
  } else if (server.length() == 0u) {
    std::cerr << "no server specified" << std::endl;
    return 1;
  }

  std::string job_id = argv[optind];
  std::cout << "Request: job_id=" << job_id << std::endl;

  job_broker_client* client = create_job_broker_client(server, conf_dir);
  if (client == nullptr) {
    return 1;
  }
  bool status_result = s_job_status(*client, job_id);
  delete client;
  return status_result ? 0 : 1;
}
