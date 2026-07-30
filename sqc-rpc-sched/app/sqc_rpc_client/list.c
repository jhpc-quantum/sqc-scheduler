#include "sqc_apis.h"
#include "sqc_rpc_client.h"
#include "sqc_rpc_sched_conv_enums.h"


//
// Using 'session', issue 'job_list' RPC request and retrieve a reply.
//
sqc_result_t
rpc_job_list(rpc_session_client_t *session, rpc_job_info_t ***jobs, size_t *n_jobs) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t request_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t code = SQC_RESULT_ANY_FAILURES;
  char *msg = NULL;

  if (likely(session != NULL && *session != NULL)) {
    request_result = rpc_session_client_job_list(session, &code, &msg, jobs, n_jobs);

    if (likely(request_result == SQC_RESULT_OK)) {
      ret = SQC_RESULT_OK;
    } else {
      ret = request_result;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    msg = strdup(sqc_error_get_string(ret));
  }
  print_rpc_common_result(ret, code, msg);

  if (likely(ret == SQC_RESULT_OK)) {
    printf("jobs:\n");
    for (size_t i = 0u; i < *n_jobs; i++) {
#ifdef JOB_LIST_WITH_RESULT
      printf("  id=%s, status=%d (%s), qc_job_id=%s, qc_result='%s'\n",
             (*jobs)[i]->job_id, (*jobs)[i]->status,
             sqc_rpc_sched_job_status_to_string((*jobs)[i]->status),
             (*jobs)[i]->qc_job_id != NULL ? (*jobs)[i]->qc_job_id : "",
             (*jobs)[i]->result != NULL ? (*jobs)[i]->result : "");
#else
      printf("  id=%s, status=%d (%s), qc_job_id=%s\n",
             (*jobs)[i]->job_id, (*jobs)[i]->status,
             sqc_rpc_sched_job_status_to_string((*jobs)[i]->status),
             (*jobs)[i]->qc_job_id != NULL ? (*jobs)[i]->qc_job_id : "");
#endif
    }
  }

  free(msg);
  return ret;
}


//
// Perform 'list' sub-command according with given parameters.
//
static inline sqc_result_t
s_subcmd_list(const char *server, bool prefer_ipv4, rpc_auth_method_t auth_method,
              const char *conf_dir) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t create_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t status_result = SQC_RESULT_ANY_FAILURES;
  rpc_session_client_t session = NULL;
  rpc_job_info_t **jobs = NULL;
  size_t n_jobs;

  if (likely(server != NULL && conf_dir != NULL)) {
    create_result = rpc_session_client_create_from_conf_dir(&session, server, prefer_ipv4,
                                                            auth_method, conf_dir);
    if (likely(create_result == SQC_RESULT_OK)) {
      status_result = rpc_job_list(&session, &jobs, &n_jobs);
      if (likely(status_result == SQC_RESULT_OK)) {
        rpc_job_info_destroy_array(jobs, n_jobs);
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
  printf("Usage: %s list [OPTION...] \n", program_name);
  printf("       %s list --help\n", program_name);
  printf("\n");

  printf("Options:\n");
  print_common_options();
}


//
// Main function of 'list' sub-command.
//
sqc_result_t
subcmd_list_main(int argc, char *argv[], int arg_index) {
  const char *server = getenv("SQC_RPC_SERVER");
  bool prefer_ipv4 = false;
  const char *conf_dir = default_conf_dir;
  rpc_auth_method_t auth_method = RPC_AUTH_METHOD_UNKNOWN;
  char *arg = NULL;

  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t tmp_ret = SQC_RESULT_ANY_FAILURES;

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

  if (arg_index != argc) {
    fprintf(stderr, "the invalid number of arguments given to 'list'\n");
    ret = SQC_RESULT_INVALID_ARGS;
    goto end;
  } else if (server == NULL || *server == '\0') {
    fprintf(stderr, "no server specified\n");
    ret = SQC_RESULT_INVALID_ARGS;
    goto end;
  }

  tmp_ret = sqc_log_initialize(SQC_LOG_EMIT_TO_UNKNOWN, program_name, false, false, sqc_log_get_log_level(),
                               sqc_log_get_debug_level(), sqc_log_get_rotate_size());
  if (unlikely(tmp_ret != SQC_RESULT_OK)) {
    fprintf(stderr, "failed to initialize the log module\n");
    ret = tmp_ret;
    goto end;
  }

  ret = s_subcmd_list(server, prefer_ipv4, auth_method, conf_dir);

end:
  return ret;
}
