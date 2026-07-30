#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "job_broker_ecode.h"
#include "job_broker_server.h"
#include "job_broker_logger_internal.h"

#ifndef SYSCONFDIR
#define SYSCONFDIR "."
#endif

#define TEST_JOB_ID "d4d22952-3159-4c7c-864c-89560700d552"

//
// Handles 'submit_job' request.
//
static int64_t
s_handle_submit_job(const char* token, uint32_t priority, const char* qprogram, int circuit_fmt,
                    size_t shots, int qc_type, int transpiler, const char* remark, const char* user_token,
                    char** job_id, char** reply_msg) {
  (void) token;
  (void) priority;
  (void) qprogram;
  (void) circuit_fmt;
  (void) shots;
  (void) qc_type;
  (void) transpiler;
  (void) remark;
  (void) user_token;

  *job_id = strdup(TEST_JOB_ID);
  *reply_msg = NULL;
  if (*job_id == NULL) {
    return RESULT_NO_MEMORY;
  }
  return RESULT_OK;
}

//
// Handles 'job_status' request.
//
static int64_t
s_handle_job_status(const char* token, const char* job_id, int32_t* status, char** qc_job_id, char** result,
                    char** reply_msg) {
  (void) token;
  (void) status;
  int64_t code = RESULT_ANY_FAILURES;

  *reply_msg = NULL;
  if (job_id == NULL || status == NULL || qc_job_id == NULL || result == NULL || strcmp(job_id, TEST_JOB_ID) == 0) {
    code = RESULT_INVALID_ARGS;
    *status = time(NULL) % 7;
    *qc_job_id = strdup("sample qc job id");
    if (*qc_job_id == NULL) {
      code = RESULT_NO_MEMORY;
    } else {
      code = RESULT_OK;
    }
    *result = strdup("sample result output");
    if (*result == NULL) {
      code = RESULT_NO_MEMORY;
    } else {
      code = RESULT_OK;
    }
  } else {
    code = RESULT_INVALID_ARGS;
  }

  return code;
}

//
// Handles 'cancel_job' request.
//
static int64_t
s_handle_cancel_job(const char* token, const char* job_id, char** reply_msg) {
  (void) token;
  int64_t code = RESULT_ANY_FAILURES;

  *reply_msg = NULL;
  if (job_id == NULL || strcmp(job_id, TEST_JOB_ID) == 0) {
    code = RESULT_OK;
  } else {
    code = RESULT_INVALID_ARGS;
  }

  return code;
}

//
// Handles 'delete_job' request.
//
static int64_t
s_handle_delete_job(const char* token, const char* job_id, char** reply_msg) {
  (void) token;
  int64_t code = RESULT_ANY_FAILURES;

  *reply_msg = NULL;
  if (job_id == NULL || strcmp(job_id, TEST_JOB_ID) == 0) {
    code = RESULT_OK;
  } else {
    code = RESULT_INVALID_ARGS;
  }

  return code;
}


//
// Handles 'job_list' request.
//
static int64_t
s_handle_job_list(const char* token, grpc_job_info_t*** info, size_t* n_jobs, char** reply_msg) {
  (void) token;
  (void) info;
  (void) n_jobs;

  *reply_msg = NULL;
  return RESULT_OK;
}

//
// Dummy implementation of adm_del_jobs_handler.
//
static int64_t
s_handle_adm_del_jobs(const char* token, const char* user_id, int64_t from_time, int64_t to_time,
                      char** reply_msg) {
  (void) token;
  (void) user_id;
  (void) from_time;
  (void) to_time;

  *reply_msg = NULL;
  return RESULT_OK;
}

//
// Dummy implementation of adm_del_jobs_handler.
//
static int64_t
s_handle_adm_add_user(const char* token, const char* user_id, char** reply_msg) {
  (void) token;
  (void) user_id;

  *reply_msg = NULL;
  return RESULT_OK;
}

//
// Dummy implementation of adm_del_jobs_handler.
//
static int64_t
s_handle_adm_set_user_status(const char* token, const char* user_id, bool enabled, char** reply_msg) {
  (void) token;
  (void) user_id;
  (void) enabled;

  *reply_msg = NULL;
  return RESULT_OK;
}

//
// Prints help message to standard out.
//
static void
s_print_help(const char* argv0) {
  printf("Usage: %s [--help] URL\n", argv0);
  printf("Options:\n");
  printf("  --help   prints this help, then exit\n");
  printf("\n");
  printf("Arguments\n");
  printf("  URL      listening address and port of gRPC service\n");
}

//
// Main.
//
int main(int argc, char* argv[]) {
  static const char default_conf_dir[] = SYSCONFDIR;
  const char* conf_dir = default_conf_dir;
  int optind = 1;

  for (; optind < argc; optind++) {
    const char* arg = argv[optind];
    if (strcmp(arg, "--") == 0) {
      optind += 1;
      break;
    } else if (*arg != '-') {
      break;
    } else if (strcmp(arg, "--help") == 0) {
      s_print_help(argv[0]);
      return 0;
    } else if (strncmp(arg, "--conf-dir=", 11) == 0) {
      conf_dir = arg + 11;
    } else {
      fprintf(stderr, "invalid option '%s'\n", arg);
      return 1;
    }
  }

  if (optind + 1 != argc) {
    fprintf(stderr, "the invalid number of arguments\n");
    return 1;
  }

  job_broker_handlers_t job_broker_handlers;
  initialize_job_broker_handlers(&job_broker_handlers);
  job_broker_handlers.submit_job = s_handle_submit_job;
  job_broker_handlers.job_status = s_handle_job_status;
  job_broker_handlers.cancel_job = s_handle_cancel_job;
  job_broker_handlers.delete_job = s_handle_delete_job;
  job_broker_handlers.job_list = s_handle_job_list;
  job_broker_handlers.adm_del_jobs = s_handle_adm_del_jobs;
  job_broker_handlers.adm_add_user = s_handle_adm_add_user;
  job_broker_handlers.adm_set_user_status = s_handle_adm_set_user_status;

  job_broker_set_log_emitter(job_broker_log_emit_to_stderr);
  if (sqc_job_broker_initialize((void*) argv[optind], conf_dir, &job_broker_handlers,
                                1, 1, 5, 10000) != RESULT_OK) {
    fprintf(stderr, "failed to initialize the server\n");
    return 1;
  }
  if (sqc_job_broker_start() != RESULT_OK) {
    return 1;
  }
  const char* url = sqc_job_broker_url();
  fprintf(stderr, "the server listens on '%s'\n", url);

  job_broker_set_log_emitter(job_broker_log_emit_to_stderr);
  sqc_job_broker_wait();
  return 0;
}
