#include <cstdint>
#include <iostream>
#include <string>

#include "job_broker.pb.h"
#include "job_broker_client.h"
#include "job_broker_ecode.h"
#include "app.h"

//
// Sends an 'adm-set-group-exec-time-limit' request to the gRPC server.
//
static bool
s_adm_set_group_exec_time_limit(job_broker_client& client,
                                const std::string& group_id, std::uint64_t exec_time_limit) {
  adm_set_group_exec_time_limit_reply reply;
  grpc::Status grpc_status = client.adm_set_group_exec_time_limit(group_id, exec_time_limit, reply);
  std::int64_t result_code = reply.code();

  if (!grpc_status.ok()) {
    std::cout << "gRPC error: " << grpc_status.error_message() << std::endl;
    return false;
  }

  // Prints result.
  print_grpc_common_result(reply.code(), reply.message());

  return (result_code == RESULT_OK);
}

//
// Prints the help message for the sub-command.
//
static void
s_print_help_adm_set_group_exec_time_limit(const char* argv0) {
  const std::string prog = get_path_basename(argv0);
  std::cout << "Usage: " << prog
            << " adm-set-group-exec-time-limit [OPTION...] GROUP LIMIT_HOUR" << std::endl;
  std::cout << "       " << prog << " adm-set-group-exec-time-limit --help" << std::endl
            << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  --conf-dir=DIR       directory where JWT token file exists" << std::endl;
  std::cout << "                       (default: " << get_default_conf_dir() << ")" << std::endl;
  std::cout << "  --server=HOST:PORT   gRPC server (default: env $SQC_GRPC_SERVER)" << std::endl;
  std::cout << std::endl;
  std::cout << "Arguments:" << std::endl;
  std::cout << "  GROUP                group ID" << std::endl;
  std::cout << "  LIMIT_HOUR           executable time limit in hours" << std::endl;
}

//
// Main function for the 'adm-set-group-exec-time-limit' sub-command.
//
int
do_subcmd_adm_set_group_exec_time_limit(int argc, char* argv[], int optind) {
  std::string server = get_default_server();
  std::string conf_dir = get_default_conf_dir();

  for (; optind < argc; optind++) {
    const std::string arg = argv[optind];
    if (arg == "--") {
      optind += 1;
      break;
    } else if (arg.compare(0u, 1, "-") != 0) {
      break;
    } else if (arg == "--help") {
      s_print_help_adm_set_group_exec_time_limit(argv[0]);
      return 0;
    } else if (parse_option_with_value(arg, "--server", server)) {
      ;
    } else if (parse_option_with_value(arg, "--conf-dir", conf_dir)) {
      ;
    } else {
      std::cerr << "Invalid option '" << arg << "'" << std::endl;
      return 1;
    }
  }

  if (optind + 2 != argc) {
    std::cerr << "The invalid number of arguments given to 'adm-set-group-exec-time-limit'" << std::endl;
    return 1;
  } else if (server.length() == 0u) {
    std::cerr << "no server specified" << std::endl;
    return 1;
  }

  const std::string group_id = argv[optind];
  std::uint64_t exec_time_limit = 0u;
  if (!parse_uint64(argv[optind + 1], &exec_time_limit)) {
    std::cerr << "Invalid executable time limit in hours: '" << argv[optind + 1] << "'" << std::endl;
    return 1;
  }

  job_broker_client* client = create_job_broker_client(server, conf_dir);
  if (client == nullptr) {
    return 1;
  }
  const bool set_result = s_adm_set_group_exec_time_limit(*client, group_id, exec_time_limit);
  delete client;
  return set_result ? 0 : 1;
}
