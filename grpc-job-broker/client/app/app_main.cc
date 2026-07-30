#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <pwd.h>
#include <errno.h>
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
// Prints help message.
//
static void
s_print_help_main(const char* argv0) {
  std::string prog = get_path_basename(argv0);
  std::cout << "Usage: " << prog << " SUB-COMMAND [ARG...]"
            << std::endl;
  std::cout << "       " << prog << " --help"
            << std::endl
            << std::endl;

  std::cout << "Sub-commands:" << std::endl;
  std::cout << "  submit                 submit a job" << std::endl;
  std::cout << "  status                 get status of the submitted job" << std::endl;
  std::cout << "  cancel                 cancel the submitted job" << std::endl;
  std::cout << "  delete                 delete the submitted job" << std::endl;
  std::cout << "  list                   get information about submitted jobs" << std::endl;
  std::cout << "  adm-del-jobs           delete submitted jobs" << std::endl;
  std::cout << "  adm-add-user           add a user" << std::endl;
  std::cout << "  adm-set-user-status    set user status\n" << std::endl;
  std::cout << std::endl;
  std::cout << "Try '" << prog << "' SUB-COMMAND --help' for more details"  << std::endl;
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
  } else if (subcmd == "status") {
    return do_subcmd_status(argc, argv, optind);
  } else if (subcmd == "cancel") {
    return do_subcmd_cancel(argc, argv, optind);
  } else if (subcmd == "delete") {
    return do_subcmd_delete(argc, argv, optind);
  } else if (subcmd == "list") {
    return do_subcmd_list(argc, argv, optind);
  } else if (subcmd == "adm-del-jobs") {
    return do_subcmd_adm_del_jobs(argc, argv, optind);
  } else if (subcmd == "adm-add-user") {
    return do_subcmd_adm_add_user(argc, argv, optind);
  } else if (subcmd == "adm-set-user-status") {
    return do_subcmd_adm_set_user_status(argc, argv, optind);
  } else {
    std::cerr << "Invalid sub-command: " << subcmd << std::endl;
    return 1;
  }
}
