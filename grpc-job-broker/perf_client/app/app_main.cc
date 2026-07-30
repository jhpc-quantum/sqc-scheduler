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
#include "app.h"

//
// Prints help message.
//
void
s_print_help_main(const char* argv0) {
  std::string prog = get_path_basename(argv0);
  std::cout << "Usage: " << prog << " SUB-COMMAND [ARG...]"
            << std::endl;
  std::cout << "       " << prog << " --help"
            << std::endl
            << std::endl;

  std::cout << "Sub-commands:" << std::endl;
  std::cout << "  submit                 submit a job" << std::endl;
  std::cout << std::endl;
  std::cout << "Try '" << prog << "' SUB-COMMAND --help' for more details"
            << std::endl;
}

//
// Main.
//
int
main(int argc, char* argv[]) {
  int optind = 1;
  for (; optind < argc; optind++) {
    std::string arg = argv[optind];
    if (arg == "--") {
      optind += 1;
      break;
    } else if (arg.compare(0u, 1, "-") != 0) {
      break;
    } else if (arg == "--help") {
      s_print_help_main(argv[0]);
      return 0;
    } else {
      std::cerr << "Invalid option '" << arg << "'" << std::endl;
      return 1;
    }
  }

  if (optind == argc) {
    std::cerr << "Missing sub-command" << std::endl;
    return 1;
  }

  job_broker_log_initialize();
  job_broker_set_log_emitter(emit_log);

  std::string subcmd = argv[optind++];
  if (subcmd == "submit") {
    return do_subcmd_submit(argc, argv, optind);
  } else {
    std::cerr << "Invalid sub-command: " << subcmd << std::endl;
    return 1;
  }
}
