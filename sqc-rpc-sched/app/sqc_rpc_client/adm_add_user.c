#include "sqc_apis.h"
#include "sqc_rpc_client.h"
#include "sqc_rpc_sched_conv_enums.h"


//
// Using 'session', issue 'adm-add-user' RPC request and retrieve a reply.
//
static sqc_result_t
rpc_adm_add_user(rpc_session_client_t *session, char *user_id) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t request_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t code = SQC_RESULT_ANY_FAILURES;
  char *msg = NULL;

  if (likely(session != NULL && *session != NULL && user_id != NULL)) {
    request_result = rpc_session_client_adm_add_user(session, user_id, &code, &msg);
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
// Perform 'delete' sub-command according with given parameters.
//
static inline sqc_result_t
s_subcmd_adm_add_user(const char *server, bool prefer_ipv4, rpc_auth_method_t auth_method,
                      const char *conf_dir, char *user_id) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t create_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t delete_result = SQC_RESULT_ANY_FAILURES;
  rpc_session_client_t session = NULL;

  if (likely(server != NULL && conf_dir != NULL && user_id != NULL)) {
    create_result = rpc_session_client_create_from_conf_dir(&session, server, prefer_ipv4,
                                                            auth_method, conf_dir);
    if (likely(create_result == SQC_RESULT_OK)) {
      delete_result = rpc_adm_add_user(&session, user_id);
      if (likely(delete_result == SQC_RESULT_OK)) {
        ret = SQC_RESULT_OK;
      } else {
        ret = delete_result;
      }
    } else {
      ret = create_result;
    }
    rpc_session_client_destroy(&session);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


//
// Print the help message.
//
static inline void
s_print_help(void) {
  printf("Usage: %s adm-add-user [OPTION...] USER\n", program_name);
  printf("       %s adm-add-user --help\n", program_name);
  printf("\n");

  printf("Options:\n");
  print_common_options();

  printf("\n");
  printf("Arguments:\n");
  printf("  USER                    user ID\n");
}


//
// Main function of 'adm-add-user' sub-command.
//
sqc_result_t
subcmd_adm_add_user_main(int argc, char *argv[], int arg_index) {
  const char *server = getenv("SQC_RPC_SERVER");
  bool prefer_ipv4 = false;
  const char *conf_dir = default_conf_dir;
  rpc_auth_method_t auth_method = RPC_AUTH_METHOD_UNKNOWN;
  char *user_id = NULL;
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

  if (arg_index + 1 != argc) {
    fprintf(stderr, "the invalid number of arguments given to 'adm-add-user'\n");
    ret = SQC_RESULT_INVALID_ARGS;
    goto end;
  } else if (server == NULL || *server == '\0') {
    fprintf(stderr, "no server specified\n");
    ret = SQC_RESULT_INVALID_ARGS;
    goto end;
  }

  user_id = argv[arg_index];
  printf("adm-add-user request: user=%s\n", user_id);

  tmp_ret = sqc_log_initialize(SQC_LOG_EMIT_TO_UNKNOWN, program_name, false, false, sqc_log_get_log_level(),
                               sqc_log_get_debug_level(), sqc_log_get_rotate_size());
  if (unlikely(tmp_ret != SQC_RESULT_OK)) {
    fprintf(stderr, "failed to initialize the log module\n");
    ret = tmp_ret;
    goto end;
  }

  ret = s_subcmd_adm_add_user(server, prefer_ipv4, auth_method, conf_dir, user_id);

end:
  return ret;
}
