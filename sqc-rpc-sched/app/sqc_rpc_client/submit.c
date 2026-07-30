#include "sqc_apis.h"
#include "sqc_rpc_client.h"
#include "sqc_rpc_sched_conv_enums.h"


//
// Using 'session', issue 'submit_job' RPC request and retrieve a reply.
//
sqc_result_t
rpc_submit_job(rpc_session_client_t *session, uint8_t priority, const char *qprogram,
               sqc_rpc_sched_circuit_fmt_t circuit_fmt, size_t shots, sqc_rpc_sched_qc_type_t qc_type,
               sqc_rpc_sched_transpiler_t transpiler, const char *remark, const char *user_token,
               char **job_id) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t request_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t code = 0u;
  char *msg = NULL;

  if (likely(session != NULL && *session != NULL && qprogram != NULL && remark != NULL &&
             job_id != NULL)) {
    request_result = rpc_session_client_submit_job(session, priority, qprogram, circuit_fmt, shots, qc_type,
                                                   transpiler, remark, user_token, &code, &msg, job_id);
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
    if (*job_id != NULL) {
      printf("job_id: %s\n", *job_id);
    }
  }

  free(msg);
  return ret;
}


//
// Perform 'submit' sub-command according with given parameters.
//
static inline sqc_result_t
s_subcmd_submit(const char *server, bool prefer_ipv4, rpc_auth_method_t auth_method, const char *conf_dir,
                uint8_t priority, const char *qprogram, sqc_rpc_sched_circuit_fmt_t circuit_fmt, size_t shots,
                sqc_rpc_sched_qc_type_t qc_type, sqc_rpc_sched_transpiler_t transpiler,
                const char *remark, const char *user_token, bool wait_completion, uint32_t sleep_interval) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t create_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t submit_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t status_result = SQC_RESULT_ANY_FAILURES;
  rpc_session_client_t session = NULL;
  char *job_id = NULL;
  sqc_rpc_sched_job_status_t job_status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;

  if (likely(server != NULL && conf_dir != NULL && qprogram != NULL && remark != NULL)) {
    create_result = rpc_session_client_create_from_conf_dir(&session, server, prefer_ipv4,
                                                            auth_method, conf_dir);
    if (likely(create_result == SQC_RESULT_OK)) {
      submit_result = rpc_submit_job(&session, priority, qprogram, circuit_fmt, shots, qc_type,
                                     transpiler, remark, user_token, &job_id);

      if (likely(submit_result == SQC_RESULT_OK)) {
        if (wait_completion == true) {
          printf("\n");

          for (;;) {
            sleep(sleep_interval);
            status_result = rpc_job_status(&session, job_id, &job_status);
            if (likely(status_result == SQC_RESULT_OK)) {
              if (job_status == SQC_RPC_SCHED_JOB_STATUS_DONE ||
                  job_status == SQC_RPC_SCHED_JOB_STATUS_CANCELLED) {
                ret = SQC_RESULT_OK;
                break;
              } else if (job_status == SQC_RPC_SCHED_JOB_STATUS_CREATED ||
                         job_status == SQC_RPC_SCHED_JOB_STATUS_QUEUED ||
                         job_status == SQC_RPC_SCHED_JOB_STATUS_RUNNING) {
                printf("\n");
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
          ret = SQC_RESULT_OK;
        }
      } else {
        ret = submit_result;
      }

    } else {
      ret = create_result;
    }
    rpc_session_client_destroy(&session);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  free(job_id);
  return ret;
}


//
// Print the help message.
//
static inline void
s_print_help(void) {
  printf("Usage: %s submit [OPTION...] QC-TYPE PRIORITY QPROGRAM FORMAT SHOTS\n",
         program_name);
  printf("       %s submit --help\n", program_name);
  printf("\n");
  printf("Options:\n");
  print_common_options();
  printf("  --transpiler=TYPE       transpiler; none, pass or normal\n");
  printf("                          (default: none)\n");
  printf("  --remark=TEXT           remark text (default: empty text)\n");
  printf("  -w, --wait-completion   wait until the submitted job is completed\n");
  printf("  --sleep-interval=N      with -w, sleep N secs between requests (default: 1)\n");
  printf("\n");

  printf("Arguments:\n");
  printf("  QC-TYPE                 priority of the job\n");
  printf("                          rqc-rest, ibm-rest or slurm-rest\n");
  printf("  PRIORITY                priority of the job\n");
  printf("  QPROGRAM                path to a program file\n");
  printf("  FORMAT                  format type of QPROGRAM (qasm, qir or qpy)\n");
  printf("  SHOTS                   the number of shots\n");
}


//
// Main function of 'submit' sub-command.
//
sqc_result_t
subcmd_submit_main(int argc, char *argv[], int arg_index) {
  const char *server = getenv("SQC_RPC_SERVER");
  const char *user_token = getenv("SQC_RPC_USER_TOKEN");
  bool prefer_ipv4 = false;
  const char *conf_dir = default_conf_dir;
  rpc_auth_method_t auth_method = RPC_AUTH_METHOD_UNKNOWN;
  uint8_t priority = 0u;
  const char *qprogram_file = NULL;
  char *qprogram = NULL;
  sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_UNKNOWN;
  size_t shots = 0u;
  sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NONE;
  const char *remark = default_remark;
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
      } else if (parse_option_with_value(arg, "--remark", &remark) == true) {
        ;
      } else if (parse_option_with_value(arg, "--transpiler", &opt_value) == true) {
        if (unlikely(sqc_rpc_sched_transpiler_from_string(opt_value, &transpiler) == false)) {
          fprintf(stderr, "invalid value for TRANSPILER: %s\n", opt_value);
          ret = SQC_RESULT_INVALID_ARGS;
          goto end;
        }
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

  if (unlikely(arg_index + 5 != argc)) {
    fprintf(stderr, "the invalid number of arguments given to 'submit'\n");
    ret = SQC_RESULT_INVALID_ARGS;
    goto end;
  } else if (unlikely(server == NULL || *server == '\0')) {
    fprintf(stderr, "no server specified\n");
    ret = SQC_RESULT_INVALID_ARGS;
    goto end;
  }

  if (unlikely(sqc_rpc_sched_qc_type_from_string(argv[arg_index], &qc_type) == false)) {
    fprintf(stderr, "invalid value for QC_TYPE: %s\n", opt_value);
    ret = SQC_RESULT_INVALID_ARGS;
    goto end;
  }
  if (unlikely(parse_uint8(argv[arg_index + 1], &priority) == false)) {
    fprintf(stderr, "invalid PRIORITY: %s\n", argv[arg_index + 1]);
    ret = SQC_RESULT_INVALID_ARGS;
    goto end;
  }
  qprogram_file = argv[arg_index + 2];
  if (unlikely(sqc_rpc_sched_circuit_fmt_from_string(argv[arg_index + 3], &circuit_fmt) == false)) {
    fprintf(stderr, "invalid CIRCUIT-FMT: %s\n", argv[arg_index + 3]);
    ret = SQC_RESULT_INVALID_ARGS;
    goto end;
  }
  if (unlikely(parse_size_t(argv[arg_index + 4], &shots) == false)) {
    fprintf(stderr, "invalid SHOTS: %s\n", argv[arg_index + 4]);
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

  tmp_ret = read_text_file(qprogram_file, SQC_RPC_SCHED_QPROGRAM_MAX_SIZE, &qprogram);
  if (unlikely(tmp_ret != SQC_RESULT_OK)) {
    ret = tmp_ret;
    goto end;
  }

  printf("submit_job request: priority=%u, qprogram_file=%s, circuit_fmt=%d, shots=%zu, "
         "qc_type=%d, transpiler=%d, remark=%s, user_token=%s\n",
         priority, qprogram_file, circuit_fmt, shots, qc_type, transpiler, remark, user_token);
  ret = s_subcmd_submit(server, prefer_ipv4, auth_method, conf_dir, priority, qprogram, circuit_fmt,
                        shots, qc_type, transpiler, remark, user_token, wait_completion, sleep_interval);

end:
  free(qprogram);
  return ret;
}
