#include "sqc_apis.h"
#include "sqc_rpc_client.h"
#include "sqc_rpc_sched_conv_enums.h"


//
// Using 'session', issue 'job_status' RPC request and retrieve a reply.
//
sqc_result_t
rpc_job_status(rpc_session_client_t *session, const char *job_id,
               sqc_rpc_sched_job_status_t *status) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t request_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t code = SQC_RESULT_ANY_FAILURES;
  char *msg = NULL;
  char *qc_job_id = NULL;
  char *result = NULL;

  if (likely(session != NULL && *session != NULL && job_id != NULL && status != NULL)) {
    request_result = rpc_session_client_job_status(session, job_id, &code, &msg, status, &qc_job_id, &result);
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

  if (likely(ret == SQC_RESULT_OK)) {
    printf("job status:\n");
    printf("  status: %d (%s)\n", (int) *status, sqc_rpc_sched_job_status_to_string(*status));
    if (qc_job_id != NULL) {
      printf("  qc_job_id: %s\n", qc_job_id);
    }
    if (result != NULL) {
      printf("  result: %s\n", result);
    }
  }

  free(msg);
  free(qc_job_id);
  free(result);
  return ret;
}


//
// Perform 'status' sub-command according with given parameters.
//
static inline sqc_result_t
s_subcmd_status(const char *server, bool prefer_ipv4, rpc_auth_method_t auth_method,
                const char *conf_dir, char *job_id, bool wait_completion,
                uint32_t sleep_interval) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t create_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t status_result = SQC_RESULT_ANY_FAILURES;
  rpc_session_client_t session = NULL;
  sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;

  if (likely(server != NULL && conf_dir != NULL && job_id != NULL)) {
    create_result = rpc_session_client_create_from_conf_dir(&session, server, prefer_ipv4,
                                                            auth_method, conf_dir);
    if (likely(create_result == SQC_RESULT_OK)) {
      for (;;) {
        status_result = rpc_job_status(&session, job_id, &status);
        if (likely(status_result == SQC_RESULT_OK)) {
          if (status == SQC_RPC_SCHED_JOB_STATUS_DONE ||
              status == SQC_RPC_SCHED_JOB_STATUS_CANCELLED) {
            ret = SQC_RESULT_OK;
            break;
          } else if (status == SQC_RPC_SCHED_JOB_STATUS_CREATED ||
                     status == SQC_RPC_SCHED_JOB_STATUS_QUEUED ||
                     status == SQC_RPC_SCHED_JOB_STATUS_RUNNING) {
            if (wait_completion == true) {
              printf("\n");
              sleep(sleep_interval);
            } else {
              ret = SQC_RESULT_OK;
              break;
            }
          } else {
            ret = SQC_RESULT_INVALID_STATE;
            break;
          }
        } else {
          ret = status_result;
          break;
        }
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
  printf("Usage: %s status [OPTION...] JOB\n", program_name);
  printf("       %s status --help\n", program_name);
  printf("\n");

  printf("Options:\n");
  print_common_options();
  printf("  -w, --wait-completion   wait until the submitted job is completed\n");
  printf("  --sleep-interval=N      with -w, sleep N secs between requests (default: 1)\n");

  printf("\n");
  printf("Arguments:\n");
  printf("  JOB                    job ID\n");
}


//
// Main function of 'status' sub-command.
//
sqc_result_t
subcmd_status_main(int argc, char *argv[], int arg_index) {
  const char *server = getenv("SQC_RPC_SERVER");
  bool prefer_ipv4 = false;
  const char *conf_dir = default_conf_dir;
  rpc_auth_method_t auth_method = RPC_AUTH_METHOD_UNKNOWN;
  char *job_id = NULL;
  bool wait_completion = false;
  uint32_t sleep_interval = 0u;
  char *arg = NULL;
  const char *opt_value = NULL;

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
      } else if (strcmp(arg, "-w") == 0 || strcmp(arg, "--wait-completion") == 0) {
        wait_completion = true;
        ret = true;
      } else if (parse_option_with_value(arg, "--sleep-interval", &opt_value) == true) {
        if (unlikely(parse_uint32(opt_value, &sleep_interval) == false)) {
          fprintf(stderr, "invalid value for WAIT_INTERVAL %s\n", opt_value);
          ret = false;
        }
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
    fprintf(stderr, "the invalid number of arguments given to 'status'\n");
    ret = SQC_RESULT_INVALID_ARGS;
    goto end;
  } else if (server == NULL || *server == '\0') {
    fprintf(stderr, "no server specified\n");
    ret = SQC_RESULT_INVALID_ARGS;
    goto end;
  }

  job_id = argv[arg_index];
  printf("job_status request: job_id=%s\n", job_id);

  tmp_ret = sqc_log_initialize(SQC_LOG_EMIT_TO_UNKNOWN, program_name, false, false, sqc_log_get_log_level(),
                               sqc_log_get_debug_level(), sqc_log_get_rotate_size());
  if (unlikely(tmp_ret != SQC_RESULT_OK)) {
    fprintf(stderr, "failed to initialize the log module\n");
    ret = tmp_ret;
    goto end;
  }

  ret = s_subcmd_status(server, prefer_ipv4, auth_method, conf_dir, job_id, wait_completion,
                        sleep_interval);

end:
  return ret;
}
