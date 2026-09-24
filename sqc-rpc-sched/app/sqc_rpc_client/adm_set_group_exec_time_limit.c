#include "sqc_apis.h"
#include "sqc_rpc_client.h"
#include "sqc_rpc_sched_conv_enums.h"


//
// Using 'session', issue an 'adm-set-group-exec-time-limit' RPC request.
//
static sqc_result_t
rpc_adm_set_group_exec_time_limit(rpc_session_client_t *session, char *group_id,
                                  uint64_t exec_time_limit) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t request_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t code = SQC_RESULT_ANY_FAILURES;
  char *msg = NULL;

  if (likely(session != NULL && *session != NULL && group_id != NULL)) {
    request_result = rpc_session_client_adm_set_group_exec_time_limit(session, group_id,
                                                                      exec_time_limit,
                                                                      &code, &msg);
    if (likely(request_result == SQC_RESULT_OK)) {
      ret = code;
    } else {
      ret = request_result;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    msg = strdup(sqc_error_get_string(ret));
  }
  print_rpc_common_result(ret, code, msg);

  free(msg);
  return ret;
}


//
// Perform the sub-command according with the given parameters.
//
static inline sqc_result_t
s_subcmd_adm_set_group_exec_time_limit(const char *server, bool prefer_ipv4,
                                       rpc_auth_method_t auth_method, const char *conf_dir,
                                       char *group_id, uint64_t exec_time_limit) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  rpc_session_client_t session = NULL;

  if (likely(server != NULL && conf_dir != NULL && group_id != NULL)) {
    ret = rpc_session_client_create_from_conf_dir(&session, server, prefer_ipv4,
                                                  auth_method, conf_dir);
    if (likely(ret == SQC_RESULT_OK)) {
      ret = rpc_adm_set_group_exec_time_limit(&session, group_id, exec_time_limit);
    }

    rpc_session_client_destroy(&session);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


static inline void
s_print_help(void) {
  printf("Usage: %s adm-set-group-exec-time-limit [OPTION...] GROUP LIMIT_HOUR\n", program_name);
  printf("       %s adm-set-group-exec-time-limit --help\n", program_name);
  printf("\n");

  printf("Options:\n");
  print_common_options();

  printf("\n");
  printf("Arguments:\n");
  printf("  GROUP                   group ID\n");
  printf("  LIMIT_HOUR              executable time limit in hours\n");
}


//
// Main function of the 'adm-set-group-exec-time-limit' sub-command.
//
sqc_result_t
subcmd_adm_set_group_exec_time_limit_main(int argc, char *argv[], int arg_index) {
  const char *server = getenv("SQC_RPC_SERVER");
  bool prefer_ipv4 = false;
  const char *conf_dir = default_conf_dir;
  rpc_auth_method_t auth_method = RPC_AUTH_METHOD_UNKNOWN;
  char *group_id = NULL;
  uint64_t exec_time_limit = 0u;
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t tmp_ret = SQC_RESULT_ANY_FAILURES;
  char *arg = NULL;

  while (arg_index < argc) {
    arg = argv[arg_index];

    if (arg[0] == '-') {
      if (strcmp(arg, "--") == 0) {
        arg_index += 1;
        break;
      } else if (strcmp(arg, "--help") == 0) {
        s_print_help();
        ret = SQC_RESULT_OK;
        goto end;
      } else if (parse_common_option(arg, &server, &prefer_ipv4, &auth_method, &conf_dir)
                 == true) {
        ;
      } else if (arg[1] == '\0') {
        break;
      } else {
        fprintf(stderr, "invalid option '%s'\n", arg);
        ret = SQC_RESULT_INVALID_ARGS;
        goto end;
      }
    } else {
      break;
    }
    arg_index++;
  }

  if (arg_index + 2 != argc) {
    fprintf(stderr, "the invalid number of arguments given to 'adm-set-group-exec-time-limit'\n");
    return SQC_RESULT_INVALID_ARGS;
    goto end;
  } else if (server == NULL || *server == '\0') {
    fprintf(stderr, "no server specified\n");
    ret = SQC_RESULT_INVALID_ARGS;
    goto end;
  }

  group_id = argv[arg_index];
  if (!parse_uint64(argv[arg_index + 1], &exec_time_limit)) {
    fprintf(stderr, "invalid executable time limit in hours: %s\n",
            argv[arg_index + 1]);
    ret = SQC_RESULT_INVALID_ARGS;
    goto end;
  }
  printf("adm-set-group-exec-time-limit request: group_id=%s, limit=%llu hour(s)\n",
         group_id, (unsigned long long)exec_time_limit);

  tmp_ret = sqc_log_initialize(SQC_LOG_EMIT_TO_UNKNOWN, program_name, false, false, sqc_log_get_log_level(),
                               sqc_log_get_debug_level(), sqc_log_get_rotate_size());
  if (unlikely(tmp_ret != SQC_RESULT_OK)) {
    fprintf(stderr, "failed to initialize the log module\n");
    ret = tmp_ret;
    goto end;
  }

  ret = s_subcmd_adm_set_group_exec_time_limit(server, prefer_ipv4, auth_method, conf_dir,
                                               group_id, exec_time_limit);

end:
  return ret;
}
