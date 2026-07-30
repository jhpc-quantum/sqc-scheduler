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
// Sends a 'adm-set-user-status' requests to gRPC server.
//
static bool
s_adm_set_user_status(job_broker_client& client, const std::string& user_id, bool enabled) {
  // Sends a 'adm_set_user_status' requests.
  adm_set_user_status_reply reply;
  grpc::Status grpc_status = client.adm_set_user_status(user_id, enabled, reply);
  std::int64_t result_code = reply.code();

  if (!grpc_status.ok()) {
    std::cout << "gRPC error: " << grpc_status.error_message() << std::endl;
    return false;
  }

  // Prints result.
  print_grpc_common_result(result_code, reply.message());

  return (result_code == RESULT_OK);
}

//
// Prints help message for 'adm-set-user-status' sub-command.
//
static void
s_print_help_adm_set_user_status(const char* argv0) {
  std::string prog = get_path_basename(argv0);
  std::cout << "Usage: " << prog << " adm-set-user-status [OPTION...] USER STATUS"
            << std::endl;
  std::cout << "       " << prog << " adm-set-user-status --help"
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
  std::cout << "  USER                 user ID " << std::endl;
  std::cout << "  STATUS               status of USER (enable or disable)" << std::endl;
}

//
// Main function for 'adm-set-user-status' sub command.
//
int
do_subcmd_adm_set_user_status(int argc, char* argv[], int optind) {
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
      s_print_help_adm_set_user_status(argv[0]);
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

  if (optind + 2 != argc) {
    std::cerr << "The invalid number of arguments given to 'adm-set-user-status'" << std::endl;
    return 1;
  } else if (server.length() == 0u) {
    std::cerr << "no server specified" << std::endl;
    return 1;
  }

  std::string user_id = argv[optind];
  bool enabled;
  if (::strcmp(argv[optind + 1], "enable") == 0) {
    enabled = true;
  } else if (::strcmp(argv[optind + 1], "disable") == 0) {
    enabled = false;
  } else {
    std::cerr << "Invalid STATUS: '" << argv[optind + 1] << "'" << std::endl;
    return 1;
  }

  job_broker_client* client = create_job_broker_client(server, conf_dir);
  if (client == nullptr) {
    return 1;
  }
  bool set_result = s_adm_set_user_status(*client, user_id, enabled);
  delete client;
  return set_result ? 0 : 1;
}
