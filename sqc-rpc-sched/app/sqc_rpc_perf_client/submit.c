#include "sqc_apis.h"
#include "sqc_rpc_perf_client.h"
#include "sqc_rpc_sched_conv_enums.h"
#include "rpc_file_util.h"

typedef struct perf_args {
  uint32_t thread_id;
  pthread_barrier_t *barrier;

  const char *server;
  bool prefer_ipv4;
  rpc_auth_method_t auth_method;
  const char *conf_dir;
  uint8_t priority;
  const char *qprogram;
  sqc_rpc_sched_circuit_fmt_t circuit_fmt;
  size_t shots;
  sqc_rpc_sched_qc_type_t qc_type;
  sqc_rpc_sched_transpiler_t transpiler;
  const char *remark;
  const char *user_token;
  uint32_t loop_count;
} perf_args_t;

//
// Create a 'sqc_session_t' object by using files under the configuration directory,
// and then connect with a server.
//
static inline sqc_result_t
s_create_perf_session_from_conf_dir(uint32_t thread_id, rpc_session_client_t *rpc_session,
                                    const char *server, bool prefer_ipv4,
                                    rpc_auth_method_t auth_method, const char *dir) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t expand_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t tls_conf_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t jwt_ctx_result = SQC_RESULT_ANY_FAILURES;
  char *exp_dir = NULL;
  sqc_tls_conf_t tls_conf = NULL;
  rpc_jwt_client_ctx_t jwt_ctx = NULL;
  sqc_chrono_t read_file_start, read_file_end, create_session_start, create_session_end;

  if (likely(rpc_session != NULL && IS_VALID_STRING(server) && IS_VALID_STRING(dir))) {
    expand_result = rpc_expand_path(dir, &exp_dir);

    if (likely(expand_result == SQC_RESULT_OK)) {
      tls_conf_result = sqc_tls_conf_create_from_conf_dir(&tls_conf, exp_dir, TLS_ROLE_CLIENT);
      if (likely(tls_conf_result == SQC_RESULT_OK)) {
        sqc_tls_conf_dump(tls_conf);

        read_file_start = sqc_chrono_now();
        jwt_ctx_result = rpc_jwt_client_create_ctx_from_conf_dir(&jwt_ctx, exp_dir);
        read_file_end = sqc_chrono_now();
        sqc_msg_info("[PERF] [thread:%d] [read file] %f nsec, %.3f usec, %.6f msec\n",
                     thread_id,
                     (double)(read_file_end - read_file_start),
                     (double)(read_file_end - read_file_start) / 1000.0,
                     (double)(read_file_end - read_file_start) / 1000.0 / 1000.0);

        if (likely(jwt_ctx_result == SQC_RESULT_OK)) {
          rpc_jwt_client_dump_ctx(&jwt_ctx);

          create_session_start = sqc_chrono_now();
          ret = rpc_session_client_create(rpc_session, server, prefer_ipv4, auth_method, tls_conf,
                                          &jwt_ctx);
          create_session_end = sqc_chrono_now();
          sqc_msg_info("[PERF] [thread:%d] [create session] %f nsec, %.3f usec, %.6f msec\n",
                       thread_id,
                       (double)(create_session_end - create_session_start),
                       (double)(create_session_end - create_session_start) / 1000.0,
                       (double)(create_session_end - create_session_start) / 1000.0 / 1000.0);
        } else {
          ret = jwt_ctx_result;
        }
      } else {
        ret = tls_conf_result;
      }
    } else {
      ret = expand_result;
    }

  } else {
    ret = SQC_RESULT_POSIX_API_ERROR;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  free(exp_dir);
  free(tls_conf);
  free(jwt_ctx);

  return ret;
}


//
// Using 'session', issue 'submit_job' RPC request and retrieve a reply.
//
sqc_result_t
rpc_submit_job(rpc_session_client_t *session, uint8_t priority, const char *qprogram,
               sqc_rpc_sched_circuit_fmt_t circuit_fmt, size_t shots, sqc_rpc_sched_qc_type_t qc_type,
               sqc_rpc_sched_transpiler_t transpiler, const char *remark, const char *user_token,
               char **job_id, uint32_t thread_id, uint32_t loop_count) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t request_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t code = 0u;
  char *msg = NULL;

  sqc_chrono_t submit_start, submit_end;

  if (likely(session != NULL && *session != NULL && qprogram != NULL && remark != NULL &&
             job_id != NULL)) {
    submit_start = sqc_chrono_now();
    request_result = rpc_session_client_submit_job(session, priority, qprogram, circuit_fmt, shots, qc_type,
                                                   transpiler, remark, user_token, &code, &msg, job_id);
    submit_end = sqc_chrono_now();

    if (likely(request_result == SQC_RESULT_OK)) {
      sqc_msg_info("[PERF] [thread:%d] [loop:%d] [submit] %f nsec, %.3f usec, %.6f msec, code: %lld, job_id: %s, "
                   "priority=%u, shots=%zu, qc_type=%d, transpiler=%d, remark=%s, user_token=%s\n",
                   thread_id,
                   loop_count,
                   (double)(submit_end - submit_start),
                   (double)(submit_end - submit_start) / 1000.0,
                   (double)(submit_end - submit_start) / 1000.0 / 1000.0,
                   (long long) code, *job_id, priority, shots, qc_type, transpiler, remark, user_token);
    } else {
      sqc_msg_info("[PERF] [thread:%d] [loop:%d] [submit] %f nsec, %.3f usec, %.6f msec, err: %s, "
                   "priority=%u, shots=%zu, qc_type=%d, transpiler=%d, remark=%s, user_token=%s\n",
                   thread_id,
                   loop_count,
                   (double)(submit_end - submit_start),
                   (double)(submit_end - submit_start) / 1000.0,
                   (double)(submit_end - submit_start) / 1000.0 / 1000.0,
                   sqc_error_get_string(ret), priority, shots, qc_type, transpiler, remark, user_token);
    }
    free(msg);
    ret = request_result;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


//
// Perform 'submit' sub-command according with given parameters.
//
static inline sqc_result_t
s_subcmd_submit(const char *server, bool prefer_ipv4, rpc_auth_method_t auth_method, const char *conf_dir,
                uint8_t priority, const char *qprogram, sqc_rpc_sched_circuit_fmt_t circuit_fmt, size_t shots,
                sqc_rpc_sched_qc_type_t qc_type, sqc_rpc_sched_transpiler_t transpiler,
                const char *remark, const char *user_token, uint32_t thread_id, uint32_t loop_count) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t create_result = SQC_RESULT_ANY_FAILURES;
  rpc_session_client_t session = NULL;
  char *job_id = NULL;

  if (likely(server != NULL && conf_dir != NULL && qprogram != NULL && remark != NULL)) {
    create_result = s_create_perf_session_from_conf_dir(thread_id, &session, server, prefer_ipv4,
                                                        auth_method, conf_dir);
    if (likely(create_result == SQC_RESULT_OK)) {
      for (uint32_t i = 0; i < loop_count; i++) {
        ret = rpc_submit_job(&session, priority, qprogram, circuit_fmt, shots,
                             qc_type, transpiler, remark, user_token, &job_id,
                             thread_id, i);
        if (ret != SQC_RESULT_OK) {
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

  free(job_id);
  return ret;
}


void*
thread_entry(void *arg) {
  perf_args_t *args = (perf_args_t *)arg;

  int ret = pthread_barrier_wait(args->barrier);
  if (ret != 0 && ret != PTHREAD_BARRIER_SERIAL_THREAD) {
    fprintf(stderr, "failed pthread_barrier_wait: %s\n", strerror(ret));
    pthread_exit(NULL);
  }

  (void)s_subcmd_submit(args->server, args->prefer_ipv4, args->auth_method, args->conf_dir,
                        args->priority, args->qprogram, args->circuit_fmt, args->shots,
                        args->qc_type, args->transpiler, args->remark, args->user_token,
                        args->thread_id, args->loop_count);

  pthread_exit(NULL);
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
  printf("  --thread-num=N          thread num (default: 1)\n");
  printf("  --loop-count=N          loop count (default: 1)\n");
  printf("\n");

  printf("Arguments:\n");
  printf("  QC-TYPE                 priority of the job\n");
  printf("                          rqc-rest, ibm-rest or slurm-rest\n");
  printf("  PRIORITY                priority of the job\n");
  printf("  QPROGRAM                path to a qprogram file\n");
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
  uint32_t thread_num = 1u;
  uint32_t loop_count = 1u;
  uint8_t priority = 0u;
  const char *qprogram_file = NULL;
  char *qprogram = NULL;
  sqc_rpc_sched_circuit_fmt_t circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_UNKNOWN;
  size_t shots = 0u;
  sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_RQC_REST;
  sqc_rpc_sched_transpiler_t transpiler = SQC_RPC_SCHED_TRANSPILER_NONE;
  const char *remark = default_remark;
  char *arg = NULL;
  const char *opt_value = NULL;

  pthread_t *threads = NULL;
  perf_args_t *perf_args = NULL;
  pthread_barrier_t barrier;
  bool barrier_inited = false;

  int rc;
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
      } else if (parse_option_with_value(arg, "--thread-num", &opt_value) == true) {
        if (unlikely(parse_uint32(opt_value, &thread_num) == false)) {
          fprintf(stderr, "invalid value for thread num: %s\n", opt_value);
          ret = false;
        }
      } else if (parse_option_with_value(arg, "--loop-count", &opt_value) == true) {
        if (unlikely(parse_uint32(opt_value, &loop_count) == false)) {
          fprintf(stderr, "invalid value for loop cound: %s\n", opt_value);
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
    fprintf(stderr, "invalid PRIORITY: %s\n", argv[arg_index]);
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

  sqc_msg_info("submit_job request: priority=%u, qprogram_file=%s, circuit_fmt=%d, "
               "shots=%zu, qc_type=%d, transpiler=%d, remark=%s, user_token=%s, "
               "thread_num=%d, loop_count=%d\n",
               priority, qprogram_file, circuit_fmt, shots, qc_type, transpiler,
               remark, user_token, thread_num, loop_count);

  threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_num);
  perf_args = (perf_args_t *)malloc(sizeof(perf_args_t) * thread_num);

  rc = pthread_barrier_init(&barrier, NULL, thread_num);
  if (rc != 0) {
    fprintf(stderr, "failed pthread_barrier_init: %s\n", strerror(rc));
    ret = SQC_RESULT_POSIX_API_ERROR;
    goto end;
  }
  barrier_inited = true;

  // create thread
  for (uint32_t i = 0; i < thread_num; i++) {
    perf_args[i].thread_id = i;
    perf_args[i].barrier = &barrier;
    perf_args[i].server = server;
    perf_args[i].prefer_ipv4 = prefer_ipv4;
    perf_args[i].auth_method = auth_method;
    perf_args[i].conf_dir = conf_dir;
    perf_args[i].priority = priority;
    perf_args[i].qprogram = qprogram;
    perf_args[i].circuit_fmt = circuit_fmt;
    perf_args[i].shots = shots;
    perf_args[i].qc_type = qc_type;
    perf_args[i].transpiler = transpiler;
    perf_args[i].remark = remark;
    perf_args[i].user_token = user_token;
    perf_args[i].loop_count = loop_count;

    rc = pthread_create(&threads[i], NULL, thread_entry, &perf_args[i]);
    if (rc != 0) {
      fprintf(stderr, "failed pthread_create: %s\n", strerror(rc));
      ret = SQC_RESULT_POSIX_API_ERROR;
      goto end;
    }
  }

  // wait thread
  for (uint32_t i = 0; i < thread_num; i++) {
    pthread_join(threads[i], NULL);
  }

end:
  if (barrier_inited) {
      pthread_barrier_destroy(&barrier);
  }
  free(threads);
  free(perf_args);
  free(qprogram);
  return ret;
}
