#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <errno.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "job_broker.pb.h"
#include "job_broker_client.h"
#include "job_broker_ecode.h"
#include "job_broker_file_util.h"
#include "job_broker_logger_internal.h"
#include "app.h"


//
// Conversion way of convert_yyyymmdd_to_chrono().
//
enum class yyyymmdd_to_chrono_t {
  DAY_START,
  DAY_END,
} ;


//
// Convert a string 'yyyynmmdd' to an integer representing nanoseconds since Epoch.
//
static bool
convert_yyyymmdd_to_chrono(const char *yyyymmdd, yyyymmdd_to_chrono_t way, int64_t *nanoseconds) {
  struct ::tm tm_info;

  if (yyyymmdd == NULL || nanoseconds == NULL) {
    return false;
  ::memset(&tm_info, 0, sizeof(struct tm));
  }
  if (::strptime(yyyymmdd, "%Y%m%d", &tm_info) == NULL) {
    return false;
  }
  tm_info.tm_hour = 0;
  tm_info.tm_min = 0;
  tm_info.tm_sec = 0;
  tm_info.tm_isdst = -1;
  *nanoseconds = ::mktime(&tm_info) * (1000LL * 1000 *1000);
  if (way == yyyymmdd_to_chrono_t::DAY_END) {
    *nanoseconds += (60 * 60 * 24) * (1000LL * 1000 * 1000) - 1;
  }
  return true;
}

//
// Sends a 'adm-del-jobs' requests to gRPC server.
//
static bool
s_adm_del_jobs(job_broker_client& client, const std::string& user_id, int64_t from_time, int64_t to_time) {
  // Sends a 'adm_del_jobs' requests.
  adm_del_jobs_reply reply;
  grpc::Status grpc_status = client.adm_del_jobs(user_id, from_time, to_time, reply);
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
// Prints help message for 'adm-del-jobs' sub-command.
//
static void
s_print_help_adm_del_jobs(const char* argv0) {
  std::string prog = get_path_basename(argv0);
  std::cout << "Usage: " << prog << " adm-del-jobs [OPTION...] USER FROM-TIME TO-TIME"
            << std::endl;
  std::cout << "       " << prog << " adm-del-jobs --help"
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
  std::cout << "  USER                 user ID or 'ALL'" << std::endl;
  std::cout << "  FROM-TIME            start date (YYYYMMDD)" << std::endl;
  std::cout << "  TO-TIME              end date (YYYYMMDD)" << std::endl;
}

//
// Main function for 'adm-del-jobs' sub command.
//
int
do_subcmd_adm_del_jobs(int argc, char* argv[], int optind) {
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
      s_print_help_adm_del_jobs(argv[0]);
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
    std::cerr << "The invalid number of arguments given to 'adm-del-jobs'" << std::endl;
    return 1;
  } else if (server.length() == 0u) {
    std::cerr << "no server specified" << std::endl;
    return 1;
  }

  std::string user_id = argv[optind];

  int64_t from_time;
  std::cout << "Request: user_id=" << user_id << std::endl;
  if (!convert_yyyymmdd_to_chrono(argv[optind + 1], yyyymmdd_to_chrono_t::DAY_START, &from_time)) {
    std::cerr << "invalid value for FROM-TIME: " << argv[optind + 1] << std::endl;
    return 1;
  }

  int64_t to_time;
  if (!convert_yyyymmdd_to_chrono(argv[optind + 2], yyyymmdd_to_chrono_t::DAY_END, &to_time)) {
    std::cerr << "invalid value for TO-TIME: " << argv[optind + 2] << std::endl;
    return 1;
  }

  job_broker_client* client = create_job_broker_client(server, conf_dir);
  if (client == nullptr) {
    return 1;
  }
  bool delete_result = s_adm_del_jobs(*client, user_id, from_time, to_time);
  delete client;
  return delete_result ? 0 : 1;
}
