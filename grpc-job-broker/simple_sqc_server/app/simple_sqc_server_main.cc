#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#include <optional>
#include <thread>

#include "job_broker_ecode.h"
#include "job_broker_server.h"
#include "job_broker_logger_internal.h"
#include "user_db.h"
#include "jwt_verifier.h"
#include "job_manager.h"

#ifndef INSTALL_ETC_DIR
#define INSTALL_ETC_DIR INSTALL_PREFIX "/etc"
#endif

#ifndef SPOOL_DIR
#define SPOOL_DIR INSTALL_PREFIX "/var/qiskit_server"
#endif

#ifndef JOB_QUEUE_SIZE
#define JOB_QUEUE_SIZE 1024
#endif

static sqc_auth::JwtVerifier* s_jwt_verifier_ptr = nullptr;
static sqc_auth::UserDB* s_user_db_ptr = nullptr;
static sqc_job::JobManager* s_job_manager_ptr = nullptr;

static constexpr int num_cqs = 1;              // the default is 1.
static constexpr int min_pollers = 1;          // the default is 1.
static constexpr int max_pollers = 5;          // the default is 2.
static constexpr int cq_timeout_msec = 10000;  // the default is 10000.

///
/// Create a message text for a reply.
///
void s_create_message_text(char** reply_msg, const char* format, ...) {
  if (reply_msg != nullptr) {
    va_list ap;
    va_start(ap, format);
    vasprintf(reply_msg, format, ap);
    va_end(ap);
  }
}

//
// Authenticate a user with a JSON web token.
//
bool s_authenticate(const sqc_auth::Jwt& jwt, char** reply_msg) {
  if (s_jwt_verifier_ptr == nullptr) {
    s_create_message_text(reply_msg, "Internal error: JWT verifier is not ready");
    return false;
  } else if (s_user_db_ptr == nullptr) {
    s_create_message_text(reply_msg, "Internal error: user database is not ready");
    return false;
  }

  // Validate the token.
  try {
    s_jwt_verifier_ptr->verify(jwt);
    return s_user_db_ptr->has_user(jwt.sub());
  } catch (const std::exception& e) {
    s_create_message_text(reply_msg, "An exception is thrown during authentication: %s", e.what());
    return false;
  }
}

//
// Handles 'submit_job' request.
//
static int64_t
s_handle_submit_job(const char* token, uint32_t priority, const char* qprogram, int circuit_fmt,
                    std::size_t shots, int qc_type, int transpiler,
                    const char* remark, const char* user_token,
                    char** job_id, char** reply_msg) {
  static constexpr char log_prefix[] = "gRPC-SUBMIT_JOB";

  // Check arguments.
  if (token == nullptr || qprogram == nullptr || remark == nullptr || *remark == '\0' || job_id == nullptr ||
      reply_msg == nullptr) {
    s_create_message_text(reply_msg, "Invalid arguments");
    msg_error_with_prefix(log_prefix, *reply_msg);
    return RESULT_INVALID_ARGS;
  }

  // Validate the token.
  std::string sub;
  try {
    sqc_auth::Jwt jwt = sqc_auth::Jwt(std::string(token));
    sub = jwt.sub();
    if (!s_authenticate(jwt, reply_msg)) {
      msg_error_with_prefix(log_prefix, *reply_msg);
      return RESULT_AUTHENTICATION_ERROR;
    }
  } catch (const std::exception& e) {
    s_create_message_text(reply_msg, "An exception is thrown: %s", e.what());
    msg_error_with_prefix(log_prefix, *reply_msg);
    return RESULT_ANY_RUNTIME_ERROR;
  }

  // Submit the job.
  if (s_job_manager_ptr == nullptr) {
    s_create_message_text(reply_msg, "Job manager is not initialinzed");
    return RESULT_ANY_RUNTIME_ERROR;
  }

  try {
    sqc_job::Job job(sub, priority, qprogram, circuit_fmt, shots, qc_type, transpiler, remark,
                     user_token ? std::optional<std::string>(user_token) : std::nullopt);
    s_job_manager_ptr->submit_job(job);
    *job_id = strdup(job.id().c_str());
    if (*job_id == nullptr) {
      s_create_message_text(reply_msg, "Not enough memory");
      msg_error_with_prefix(log_prefix, *reply_msg);
      return RESULT_NO_MEMORY;
    }
  } catch (const std::exception& e) {
    s_create_message_text(reply_msg, "An exception is thrown: %s", e.what());
    msg_error_with_prefix(log_prefix, *reply_msg);
    return RESULT_ANY_RUNTIME_ERROR;
  }

  msg_info("%s: Submitted: job_id=%s\n", log_prefix, *job_id);
  return RESULT_OK;
}

//
// Handles 'job_status' request.
//
static int64_t
s_handle_job_status(const char* token, const char* job_id, int32_t* status, char** qc_job_id, char** result,
                    char** reply_msg) {
  static constexpr char log_prefix[] = "gRPC-JOB_STATUS";

  // Check arguments.
  if (token == nullptr || job_id == nullptr || *job_id == '\0' || status == nullptr || qc_job_id == nullptr ||
      result == nullptr || reply_msg == nullptr) {
    s_create_message_text(reply_msg, "Invalid arguments");
    msg_error_with_prefix(log_prefix, *reply_msg);
    return RESULT_INVALID_ARGS;
  }

  // Validate the token.
  std::string sub;
  try {
    sqc_auth::Jwt jwt = sqc_auth::Jwt(token);
    sub = jwt.sub();
    if (!s_authenticate(jwt, reply_msg)) {
      s_create_message_text(reply_msg, "Authentication failed: user=%s", sub.c_str());
      msg_error_with_prefix(log_prefix, *reply_msg);
      return RESULT_AUTHENTICATION_ERROR;
    }
  } catch (const std::exception& e) {
    s_create_message_text(reply_msg, "An exception is thrown: %s", e.what());
    msg_error_with_prefix(log_prefix, *reply_msg);
    return RESULT_ANY_RUNTIME_ERROR;
  }

  // Get status of the job.
  if (s_job_manager_ptr == nullptr) {
    s_create_message_text(reply_msg, "Job manager is not initialinzed");
    msg_error_with_prefix(log_prefix, *reply_msg);
    return RESULT_ANY_RUNTIME_ERROR;
  }

  try {
    auto job = s_job_manager_ptr->get_job(job_id);
    if (job.user_id() == "") {
      s_create_message_text(reply_msg, "Not found");
      msg_error_with_prefix(log_prefix, *reply_msg);
      return RESULT_ANY_FAILURES;
    } else if (job.user_id() != sub) {
      s_create_message_text(reply_msg, "Not owner");
      msg_error_with_prefix(log_prefix, *reply_msg);
      return RESULT_ANY_FAILURES;
    } else {
      *status = static_cast<int32_t>(job.status());
      if (job.status() == sqc_job::JobStatus::Done) {
        *qc_job_id = strdup(job.qc_job_id().c_str());
        *result = strdup(s_job_manager_ptr->get_job_result(job_id).c_str());
        if (*qc_job_id == nullptr || *result == nullptr) {
          free(*qc_job_id);
          free(*result);
          s_create_message_text(reply_msg, "Not enough memory");
          msg_error_with_prefix(log_prefix, *reply_msg);
          return RESULT_NO_MEMORY;
        }
      }
    }
  } catch (const std::exception& e) {
    s_create_message_text(reply_msg, "An exception is thrown: %s", e.what());
    msg_error_with_prefix(log_prefix, *reply_msg);
    return RESULT_ANY_RUNTIME_ERROR;
  }

  msg_info("%s: Got job status: job_id=%s, status=%ld, qc_job_id=%s\n",
           log_prefix, job_id, static_cast<long>(*status), *qc_job_id);
  return RESULT_OK;
}

//
// Handles 'cancel_job' request.
//
static int64_t
s_handle_cancel_job(const char* token, const char* job_id, char** reply_msg) {
  static constexpr char log_prefix[] = "gRPC-CANCEL_JOB";

  // Check arguments.
  if (token == nullptr || job_id == nullptr || *job_id == '\0' || reply_msg == nullptr) {
    s_create_message_text(reply_msg, "Invalid arguments");
    msg_error_with_prefix(log_prefix, *reply_msg);
    return RESULT_INVALID_ARGS;
  }

  // Validate the token.
  std::string sub;
  try {
    sqc_auth::Jwt jwt = sqc_auth::Jwt(token);
    sub = jwt.sub();
    if (!s_authenticate(jwt, reply_msg)) {
      s_create_message_text(reply_msg, "Authentication failed: user=%s", sub.c_str());
      msg_error_with_prefix(log_prefix, *reply_msg);
      return RESULT_AUTHENTICATION_ERROR;
    }
  } catch (const std::exception& e) {
    s_create_message_text(reply_msg, "An exception is thrown: %s", e.what());
    msg_error_with_prefix(log_prefix, *reply_msg);
    return RESULT_ANY_RUNTIME_ERROR;
  }

  s_create_message_text(reply_msg, "Unsupported");
  msg_error_with_prefix(log_prefix, *reply_msg);
  return RESULT_UNSUPPORTED;
}

//
// Handles 'delete_job' request.
//
static int64_t
s_handle_delete_job(const char* token, const char* job_id, char** reply_msg) {
  static constexpr char log_prefix[] = "gRPC-DELETE_JOB";

  // Check arguments.
  if (token == nullptr || job_id == nullptr || *job_id == '\0' || reply_msg == nullptr) {
    s_create_message_text(reply_msg, "Invalid arguments");
    msg_error_with_prefix(log_prefix, *reply_msg);
    return RESULT_INVALID_ARGS;
  }

  // Validate the token.
  std::string sub;
  try {
    sqc_auth::Jwt jwt = sqc_auth::Jwt(token);
    sub = jwt.sub();
    if (!s_authenticate(jwt, reply_msg)) {
      s_create_message_text(reply_msg, "Authentication failed: user=%s", sub.c_str());
      msg_error_with_prefix(log_prefix, *reply_msg);
      return RESULT_AUTHENTICATION_ERROR;
    }
  } catch (const std::exception& e) {
    s_create_message_text(reply_msg, "An exception is thrown: %s", e.what());
    msg_error_with_prefix(log_prefix, *reply_msg);
    return RESULT_ANY_RUNTIME_ERROR;
  }

  s_create_message_text(reply_msg, "Unsupported");
  msg_error_with_prefix(log_prefix, *reply_msg);
  return RESULT_UNSUPPORTED;
}


//
// Handles 'job_list' request.
//
static int64_t
s_handle_job_list(const char* token, grpc_job_info_t*** info, size_t* n_jobs, char** reply_msg) {
  static constexpr char log_prefix[] = "gRPC-JOB_LIST";

  // Check arguments.
  if (token == nullptr || info == nullptr || n_jobs == nullptr || reply_msg == nullptr) {
    s_create_message_text(reply_msg, "Invalid arguments");
    msg_error_with_prefix(log_prefix, *reply_msg);
    return RESULT_INVALID_ARGS;
  }

  // Validate the token.
  std::string sub;
  try {
    sqc_auth::Jwt jwt = sqc_auth::Jwt(token);
    sub = jwt.sub();
    if (!s_authenticate(jwt, reply_msg)) {
      s_create_message_text(reply_msg, "Authentication failed: user=%s", sub.c_str());
      msg_error_with_prefix(log_prefix, *reply_msg);
      return RESULT_AUTHENTICATION_ERROR;
    }
  } catch (const std::exception& e) {
    s_create_message_text(reply_msg, "An exception is thrown: %s", e.what());
    msg_error_with_prefix(log_prefix, *reply_msg);
    return RESULT_ANY_RUNTIME_ERROR;
  }

  s_create_message_text(reply_msg, "Unsupported");
  msg_error_with_prefix(log_prefix, *reply_msg);
  return RESULT_UNSUPPORTED;
}

//
// Delete jobs submitted by the specified user.
//
static int64_t
s_handle_adm_del_jobs(const char* token, const char* user_id, int64_t from_time, int64_t to_time,
                      char** reply_msg) {
  static constexpr char log_prefix[] = "gRPC-ADM_DEL_JOBS";
  (void) from_time;
  (void) to_time;

  // Check arguments.
  if (token == nullptr || user_id == nullptr || reply_msg == nullptr) {
    s_create_message_text(reply_msg, "Invalid arguments");
    msg_error_with_prefix(log_prefix, *reply_msg);
    return RESULT_INVALID_ARGS;
  }

  // Validate the token.
  std::string sub;
  try {
    sqc_auth::Jwt jwt = sqc_auth::Jwt(token);
    sub = jwt.sub();
    if (!s_authenticate(jwt, reply_msg)) {
      s_create_message_text(reply_msg, "Authentication failed: user=%s", sub.c_str());
      msg_error_with_prefix(log_prefix, *reply_msg);
      return RESULT_AUTHENTICATION_ERROR;
    }
  } catch (const std::exception& e) {
    s_create_message_text(reply_msg, "An exception is thrown: %s", e.what());
    msg_error_with_prefix(log_prefix, *reply_msg);
    return RESULT_ANY_RUNTIME_ERROR;
  }

  s_create_message_text(reply_msg, "Unsupported");
  msg_error_with_prefix(log_prefix, *reply_msg);
  return RESULT_UNSUPPORTED;
}

//
// Add an user.
//
static int64_t
s_handle_adm_add_user(const char* token, const char* user_id, char** reply_msg) {
  static constexpr char log_prefix[] = "gRPC-ADM_ADD_USER";

  // Check arguments.
  if (token == nullptr || user_id == nullptr || reply_msg == nullptr) {
    s_create_message_text(reply_msg, "Invalid arguments");
    msg_error_with_prefix(log_prefix, *reply_msg);
    return RESULT_INVALID_ARGS;
  }

  // Validate the token.
  std::string sub;
  try {
    sqc_auth::Jwt jwt = sqc_auth::Jwt(token);
    sub = jwt.sub();
    if (!s_authenticate(jwt, reply_msg)) {
      s_create_message_text(reply_msg, "Authentication failed: user=%s", sub.c_str());
      msg_error_with_prefix(log_prefix, *reply_msg);
      return RESULT_AUTHENTICATION_ERROR;
    }
  } catch (const std::exception& e) {
    s_create_message_text(reply_msg, "An exception is thrown: %s", e.what());
    msg_error_with_prefix(log_prefix, *reply_msg);
    return RESULT_ANY_RUNTIME_ERROR;
  }

  s_create_message_text(reply_msg, "Unsupported");
  msg_error_with_prefix(log_prefix, *reply_msg);
  return RESULT_UNSUPPORTED;
}

//
// Set user status.
//
static int64_t
s_handle_adm_set_user_status(const char* token, const char* user_id, bool enabled, char** reply_msg) {
  static constexpr char log_prefix[] = "gRPC-ADM_SET_USER_STATUS";
  (void) enabled;

  // Check arguments.
  if (token == nullptr || user_id == nullptr || reply_msg == nullptr) {
    s_create_message_text(reply_msg, "Invalid arguments");
    msg_error_with_prefix(log_prefix, *reply_msg);
    return RESULT_INVALID_ARGS;
  }

  // Validate the token.
  std::string sub;
  try {
    sqc_auth::Jwt jwt = sqc_auth::Jwt(token);
    sub = jwt.sub();
    if (!s_authenticate(jwt, reply_msg)) {
      s_create_message_text(reply_msg, "Authentication failed: user=%s", sub.c_str());
      msg_error_with_prefix(log_prefix, *reply_msg);
      return RESULT_AUTHENTICATION_ERROR;
    }
  } catch (const std::exception& e) {
    s_create_message_text(reply_msg, "An exception is thrown: %s", e.what());
    msg_error_with_prefix(log_prefix, *reply_msg);
    return RESULT_ANY_RUNTIME_ERROR;
  }

  s_create_message_text(reply_msg, "Unsupported");
  msg_error_with_prefix(log_prefix, *reply_msg);
  return RESULT_UNSUPPORTED;
}

//
// Entry point of job invoker thread.
//
static void
s_job_invoker_main(const std::string& spool_dir, size_t job_queue_size) {
  sqc_job::PyJobInvoker* job_invoker = nullptr;
  sqc_job::JobManager* job_manager = nullptr;

  try {
    job_invoker = new sqc_job::PyJobInvoker("qiskit_qasm3", "simulate_quantum_circuit", spool_dir);
    job_manager = new sqc_job::JobManager(job_queue_size, job_invoker);
  } catch (const std::exception& e) {
    msg_error("error: %s\n", e.what());
    delete job_manager;
    delete job_invoker;
    exit(1);
  }

  s_job_manager_ptr = job_manager;
  for (;;) {
    try {
      s_job_manager_ptr->invoke_next_job();
    } catch (const std::exception& e) {
      msg_error("error: %s\n", e.what());
    }
  }

  delete job_manager;
  delete job_invoker;
}

//
// Block signals.
//
static void
s_block_signals() {
  sigset_t sigset;
  sigfillset(&sigset);
  pthread_sigmask(SIG_BLOCK, &sigset, nullptr);
}

//
// Unblock signals.
//
static void
s_unblock_signals() {
  sigset_t sigset;
  sigfillset(&sigset);
  pthread_sigmask(SIG_UNBLOCK, &sigset, nullptr);
}

//
// Set signal handler.
//
static void
s_set_signal_handler() {
  auto handler = [](int signo) -> void { static_cast<void>(signo); };
  struct sigaction sa;
  sa.sa_handler = handler;
  sa.sa_sigaction = nullptr;
  sigemptyset(&sa.sa_mask);
  sa.sa_restorer = nullptr;
  sigaction(SIGHUP, &sa, nullptr);
  sigaction(SIGINT, &sa, nullptr);
  sigaction(SIGQUIT, &sa, nullptr);
  sigaction(SIGTERM, &sa, nullptr);
}

//
// Prints help message to standard out.
//
static void
s_print_help(const char* argv0) {
  printf("Usage: %s [--help] URL\n", argv0);
  printf("Options:\n");
  printf("  --help           prints this help, then exit\n");
  printf("  --conf-dir=DIR   read configuration files in DIR\n");
  printf("  --spool-dir=DIR  output job files in DIR\n");
  printf("                   (default: %s\n", INSTALL_ETC_DIR);
  printf("\n");
  printf("Arguments\n");
  printf("  URL      listening address and port of gRPC service\n");
}

//
// Parse command line arguments.
//
static bool
s_parse_argv(int argc, char* argv[], std::string& conf_dir, std::string& spool_dir,
             size_t& job_queue_size, std::string& url) {
  conf_dir = INSTALL_ETC_DIR;
  spool_dir = SPOOL_DIR;
  job_queue_size = JOB_QUEUE_SIZE;

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
      exit(0);
    } else if (strncmp(arg, "--conf-dir=", 11) == 0) {
      conf_dir = arg + 11;
    } else if (strncmp(arg, "--job-queue-size=", 17) == 0) {
      job_queue_size = static_cast<size_t>(std::stoull(std::string(arg + 17)));
    } else if (strncmp(arg, "--spool-dir=", 12) == 0) {
      spool_dir = arg + 12;
    } else {
      fprintf(stderr, "invalid option '%s'\n", arg);
      return false;
    }
  }

  if (optind + 1 != argc) {
    fprintf(stderr, "the invalid number of arguments\n");
    return false;
  }
  url = argv[optind];

  return true;
}

//
// Main.
//
int
main(int argc, char* argv[]) {
  std::string conf_dir;
  std::string spool_dir;
  size_t job_queue_size = 0u;
  std::string url;

  // Parse command line arguments.
  if (!s_parse_argv(argc, argv, conf_dir, spool_dir, job_queue_size, url)) {
    return 1;
  }

  // Set log handler.
  job_broker_set_log_emitter(job_broker_log_emit_to_stderr);

  // Output the settings.
  msg_info("conf_dir = %s\n", conf_dir.c_str());
  msg_info("spool_dir = %s\n", spool_dir.c_str());
  msg_info("job_queue_size = %zu\n", job_queue_size);
  msg_info("listen address = %s\n", url.c_str());

  // Set up job broker.
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

  // Set up user database.
  sqc_auth::UserDB user_db;
  try {
    user_db = sqc_auth::UserDB::from_conf_dir(conf_dir);
  } catch (const std::exception& e) {
    msg_error("failed to initialize the user database: %s\n", e.what());
    return 1;
  }
  s_user_db_ptr = &user_db;

  // Set up JWT verifier.
  sqc_auth::JwtVerifier jwt_verifier;
  try {
    jwt_verifier = sqc_auth::JwtVerifier::from_conf_dir(conf_dir);
  } catch (const std::exception& e) {
    msg_error("failed to initialize the JWT verifier: %s\n", e.what());
    return 1;
  }
  s_jwt_verifier_ptr = &jwt_verifier;

  // Set up job manager.
  Py_Initialize();
  PyEval_SaveThread();


  // Start job invoker thread.
  s_block_signals();
  std::thread thd(s_job_invoker_main, spool_dir, job_queue_size);
  thd.detach();
  s_set_signal_handler();
  s_unblock_signals();

  // Start job broker.
  if (sqc_job_broker_initialize(url.c_str(), conf_dir.c_str(), &job_broker_handlers,
                                num_cqs, min_pollers, max_pollers, cq_timeout_msec) != RESULT_OK) {
    msg_error("failed to initialize the server\n");
    Py_Finalize();
    return 1;
  }

  if (sqc_job_broker_start() != RESULT_OK) {
    msg_error("failed to start the server\n");
    Py_Finalize();
    return 1;
  }
  sqc_job_broker_wait();

  // thd.join();
  // Py_Finalize();

  return 0;
}
