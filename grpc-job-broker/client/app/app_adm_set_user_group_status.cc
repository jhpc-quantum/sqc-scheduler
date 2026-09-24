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
// Sends a 'adm-set-user-group-status' requests to gRPC server.
//
static bool
s_adm_set_user_group_status(job_broker_client& client, const std::string& user_id,
                            const std::string& group_id, bool enabled) {
  // Sends a 'adm_set_user_group_status' requests.
  adm_set_user_group_status_reply reply;
  grpc::Status grpc_status = client.adm_set_user_group_status(user_id, group_id, enabled, reply);
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
// Prints help message for 'adm-set-user-group-status' sub-command.
//
static void
s_print_help_adm_set_user_group_status(const char* argv0) {
  std::string prog = get_path_basename(argv0);
  std::cout << "Usage: " << prog << " adm-set-user-group-status [OPTION...] USER GROUP STATUS"
            << std::endl;
  std::cout << "       " << prog << " adm-set-user-group-status --help"
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
  std::cout << "  GROUP                group ID " << std::endl;
  std::cout << "  STATUS               status of the user-group association (enable or disable)"
            << std::endl;
}

//
// Main function for 'adm-set-user-group-status' sub command.
//
int
do_subcmd_adm_set_user_group_status(int argc, char* argv[], int optind) {
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
      s_print_help_adm_set_user_group_status(argv[0]);
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

  if (optind + 3 != argc) {
    std::cerr << "The invalid number of arguments given to 'adm-set-user-group-status'" << std::endl;
    return 1;
  } else if (server.length() == 0u) {
    std::cerr << "no server specified" << std::endl;
    return 1;
  }

  std::string user_id = argv[optind];
  std::string group_id = argv[optind + 1];
  bool enabled;
  if (::strcmp(argv[optind + 2], "enable") == 0) {
    enabled = true;
  } else if (::strcmp(argv[optind + 2], "disable") == 0) {
    enabled = false;
  } else {
    std::cerr << "Invalid STATUS: '" << argv[optind + 2] << "'" << std::endl;
    return 1;
  }

  job_broker_client* client = create_job_broker_client(server, conf_dir);
  if (client == nullptr) {
    return 1;
  }
  bool set_result = s_adm_set_user_group_status(*client, user_id, group_id, enabled);
  delete client;
  return set_result ? 0 : 1;
}
