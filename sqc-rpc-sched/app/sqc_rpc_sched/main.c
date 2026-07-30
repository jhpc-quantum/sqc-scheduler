#include "sqc_apis.h"
#include "sqc_rpc_sched_conf.h"
#include "sqc_rpc_sched_paths.h"
#include "modules.h"

static bool
s_parse_option_with_value(const char *const arg, const char *const opt, const char **opt_value) {
  bool ret = false;
  size_t opt_len = 0u;

  if (likely(arg != NULL && opt != NULL && opt_value != NULL)) {
    opt_len = strlen(opt);
    if (strncmp(arg, opt, opt_len) == 0 && *(arg + opt_len) == '=') {
      *opt_value = arg + opt_len + 1;
      ret = true;
    }
  }

  return ret;
}


static sqc_result_t
s_parse_options(int argc, const char *const argv[]) {
  sqc_result_t ret = SQC_RESULT_INVALID_ARGS;
  sqc_result_t parse_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t set_result = SQC_RESULT_ANY_FAILURES;

  int i = 1;
  for (;;) {
    if (i == argc) {
      parse_result = SQC_RESULT_OK;
      break;
    }

    if (*(argv[i]) == '-') {
      const char *opt_value = NULL;

      if (strcmp(argv[i], "--") == 0) {
        parse_result = SQC_RESULT_OK;
        i++;
        break;
      } else if (s_parse_option_with_value(argv[i], "--conf-dir", &opt_value) == true) {
        if (likely(sqc_rpc_sched_conf_set_conf_dir(opt_value) != true)) {
        } else {
          parse_result = SQC_RESULT_NO_MEMORY;
          sqc_msg_error("%s\n", sqc_error_get_string(parse_result));
          break;
        }
      } else {
        parse_result = SQC_RESULT_INVALID_ARGS;
        sqc_msg_debug(5, "Invalid command line option: %s\n", argv[i]);
      }
    } else {
      parse_result = SQC_RESULT_OK;
      break;
    }

    i++;
  }

  if (likely(parse_result == SQC_RESULT_OK)) {
    if (unlikely(i + 3 != argc)) {
      parse_result = SQC_RESULT_INVALID_ARGS;
      sqc_msg_error("The invalid number of command line arguments\n");
    }
  }

  if (likely(parse_result == SQC_RESULT_OK)) {
    do {
      if (strcmp(argv[i], "") != 0 && strcmp(argv[i], "-") != 0) {
        set_result = sqc_rpc_sched_conf_set_rpc_server_address(argv[i]);
        if (likely(set_result == SQC_RESULT_OK)) {
          sqc_msg_debug(5, "Set rpc-server-address=%s\n", argv[i]);
        } else {
          ret = set_result;
          sqc_msg_error("%s\n", sqc_error_get_string(ret));
          break;
        }
      }
      i++;
      if (strcmp(argv[i], "") != 0 && strcmp(argv[i], "-") != 0) {
        set_result = sqc_rpc_sched_conf_set_grpc_server_address(argv[i]);
        if (likely(set_result == SQC_RESULT_OK)) {
          sqc_msg_debug(5, "Set grpc-server-address=%s\n", argv[i]);
        } else {
          ret = set_result;
          sqc_msg_error("%s\n", sqc_error_get_string(ret));
          break;
        }
      }
      i++;
      if (likely(sqc_rpc_sched_conf_set_qc_type_from_string(argv[i]) == SQC_RESULT_OK)) {
        sqc_msg_debug(5, "Set qc-type=%d (%s)\n",
                      (int)sqc_rpc_sched_conf_get_qc_type(), argv[i]);
      } else {
        ret = SQC_RESULT_INVALID_ARGS;
        sqc_msg_error("Invalid qc-type: %s\n", argv[i]);
        break;
      }

      if (likely(sqc_rpc_sched_conf_get_conf_dir() == NULL)) {
        set_result = sqc_rpc_sched_conf_set_conf_dir(APP_SYSCONFDIR);
        if (likely(set_result == SQC_RESULT_OK)) {
          sqc_msg_debug(5, "Set conf_dir=%s\n", APP_SYSCONFDIR);
        } else {
          ret = SQC_RESULT_NO_MEMORY;
          sqc_msg_error("%s\n", sqc_error_get_string(ret));
          break;
        }
      }
      ret = SQC_RESULT_OK;
    } while (0);
  }

  return ret;
}


int
main(int argc, const char *const argv[]) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  const char *fn = sqc_log_get_filename();

  if (strlen(fn) == 0) {
    fn = "./sqc-scheduler.log";
  }
  module_init();

  if ((ret = sqc_log_initialize(SQC_LOG_EMIT_TO_FILE,
                                fn,
                                false,
                                true,
                                sqc_log_get_log_level(),
                                sqc_log_get_debug_level(), sqc_log_get_rotate_size())) != SQC_RESULT_OK) {
    sqc_msg_error("Failed to initialize the log: %s\n", sqc_error_get_string(ret));
    goto done;
  }

  if (likely((ret = s_parse_options(argc, argv)) != SQC_RESULT_OK)) {
    sqc_msg_error("Failed to parse CLI arguments: %s\n", sqc_error_get_string(ret));
    goto done;
  }

  if ((ret = sqc_set_pidfile("./app.pid")) != SQC_RESULT_OK) {
    sqc_msg_error("Failed to set pidfile: %s\n", sqc_error_get_string(ret));
    goto done;
  }

  ret = sqc_mainloop(argc, argv, NULL, NULL, true, true, false);

done:
  return (ret == SQC_RESULT_OK) ? 0 : 1;
}

