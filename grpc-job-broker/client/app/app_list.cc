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
// Sends a 'job_list' requests to gRPC server.
//
static bool
s_job_list(job_broker_client& client) {
  // Sends a 'job_status' requests.
  job_list_reply reply;
  grpc::Status grpc_status = client.job_list(reply);
  std::int64_t result_code = reply.code();

  if (!grpc_status.ok()) {
    std::cout << "gRPC error: " << grpc_status.error_message() << std::endl;
    return false;
  }

  // Prints result.
  print_grpc_common_result(result_code, reply.message());

  if (result_code == RESULT_OK) {
    std::cout << "jobs: " << std::endl;
    int n_jobs = reply.jobs_size();

    for (int i = 0; i < n_jobs; i++) {
      auto const job = reply.jobs(i);
      const std::string& job_id = job.job_id();
      auto status = job.status();

      std::cout << "  id=" << job_id
                << ", status=" << static_cast<int>(status)
                << ", qc_job_id=" << (job.has_qc_job_id() ? job.qc_job_id() : "")
#ifdef JOB_LIST_WITH_RESULT
                << ", qc_result='" << (job.has_qc_result() ? job.result() : "" << "'")
#endif
                << std::endl;
    }
  }

  return (result_code == RESULT_OK);
}

//
// Prints help message for 'list' sub-command.
//
static void
s_print_help_list(const char* argv0) {
  std::string prog = get_path_basename(argv0);
  std::cout << "Usage: " << prog << " list [OPTION...]"
            << std::endl;
  std::cout << "       " << prog << " list --help"
            << std::endl
            << std::endl;

  std::cout << "Options:" << std::endl;
  std::cout << "  --conf-dir=DIR       directory where JWT token file exists" << std::endl;
  std::cout << "                       (default: " << get_default_conf_dir() << ")"
            << std::endl;
  std::cout << "  --server=HOST:PORT   gRPC server (default: env $SQC_GRPC_SERVER)"
            << std::endl;
}

//
// Main function for 'list' sub command.
//
int
do_subcmd_list(int argc, char* argv[], int optind) {
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
      s_print_help_list(argv[0]);
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

  if (optind != argc) {
    std::cerr << "The invalid number of arguments given to 'list'" << std::endl;
    return 1;
  } else if (server.length() == 0u) {
    std::cerr << "no server specified" << std::endl;
    return 1;
  }

  job_broker_client* client = create_job_broker_client(server, conf_dir);
  if (client == nullptr) {
    return 1;
  }
  bool list_result = s_job_list(*client);
  delete client;
  return list_result ? 0 : 1;
}
