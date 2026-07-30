#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>
#include <grpcpp/server_builder.h>

#include "rpc_job_broker_service.h"
#include "job_broker_ecode.h"
#include "job_broker_file_util.h"
#include "job_broker_logger_internal.h"
#include "job_broker_server.h"

static std::unique_ptr<grpc::Server> s_server;
static std::unique_ptr<std::string> s_server_url;
static std::unique_ptr<std::string> s_conf_dir;
static std::mutex s_mutex;

static const char s_ca_cert_file[] = "ca/grpc_ca.crt";
static const char s_server_cert_file[] = "grpc_server.crt";
static const char s_server_key_file[] = "grpc_server.key";

static std::unique_ptr<std::string> s_ca_cert;
static std::unique_ptr<std::string> s_server_cert;
static std::unique_ptr<std::string> s_server_key;

static int s_num_cqs = 1;
static int s_min_pollers = 1;
static int s_max_pollers = 5;
static int s_cq_timeout_msec = 10000;

//
// Initialize a job_broker_handlers_t object.
//
void initialize_job_broker_handlers(job_broker_handlers_t* handlers) {
  if (handlers != nullptr) {
    handlers->submit_job = nullptr;
    handlers->job_status = nullptr;
    handlers->cancel_job = nullptr;
    handlers->delete_job = nullptr;
    handlers->job_list = nullptr;
    handlers->adm_del_jobs = nullptr;
    handlers->adm_add_user = nullptr;
    handlers->adm_set_user_status = nullptr;
  }
}

//
// Returns URL the gRPC server uses.
//
const char*
sqc_job_broker_url() {
  static const char empty[] = "";

  if (s_server_url) {
    return s_server_url->c_str();
  } else {
    return empty;
  }
}

//
// Dummy implementation of submit_job_handler.
//
static int64_t
s_dummy_submit_job_handler(const char* token, uint32_t priority, const char* qprogram, int circuit_fmt,
                           size_t shots, int qc_type, int transpiler, const char* remark, const char* user_token,
                           char** job_id, char** reply_msg) {
  static_cast<void>(token);
  static_cast<void>(priority);
  static_cast<void>(qprogram);
  static_cast<void>(circuit_fmt);
  static_cast<void>(shots);
  static_cast<void>(qc_type);
  static_cast<void>(transpiler);
  static_cast<void>(remark);
  static_cast<void>(user_token);
  static_cast<void>(job_id);
  static_cast<void>(reply_msg);
  return static_cast<int64_t>(RESULT_ANY_FAILURES);
}

//
// Dummy implementation of job_status_handler.
//
static int64_t
s_dummy_job_status_handler(const char* token, const char* job_id, int32_t* status,
                           char** qc_job_id, char** result, char** reply_msg) {
  static_cast<void>(token);
  static_cast<void>(job_id);
  static_cast<void>(status);
  static_cast<void>(qc_job_id);
  static_cast<void>(result);
  static_cast<void>(reply_msg);
  return static_cast<int64_t>(RESULT_ANY_FAILURES);
}

//
// Dummy implementation of cancel_job_handler.
//
static int64_t
s_dummy_cancel_job_handler(const char* token, const char* job_id, char** reply_msg) {
  static_cast<void>(token);
  static_cast<void>(job_id);
  static_cast<void>(reply_msg);
  return static_cast<int64_t>(RESULT_ANY_FAILURES);
}

//
// Dummy implementation of delete_job_handler.
//
static int64_t
s_dummy_delete_job_handler(const char* token, const char* job_id, char** reply_msg) {
  static_cast<void>(token);
  static_cast<void>(job_id);
  static_cast<void>(reply_msg);
  return static_cast<int64_t>(RESULT_ANY_FAILURES);
}

//
// Dummy implementation of job_list_handler.
//
static int64_t
s_dummy_job_list_handler(const char* token, grpc_job_info_t*** info, size_t* n_jobs, char** reply_msg) {
  static_cast<void>(token);
  static_cast<void>(info);
  static_cast<void>(n_jobs);
  static_cast<void>(reply_msg);
  return static_cast<int64_t>(RESULT_ANY_FAILURES);
}

//
// Dummy implementation of adm_del_jobs_handler.
//
static int64_t
s_dummy_adm_del_jobs_handler(const char* token, const char* user_id, int64_t from_time, int64_t to_time,
                             char** reply_msg) {
  static_cast<void>(token);
  static_cast<void>(user_id);
  static_cast<void>(from_time);
  static_cast<void>(to_time);
  static_cast<void>(reply_msg);
  return static_cast<int64_t>(RESULT_ANY_FAILURES);
}

//
// Dummy implementation of adm_del_jobs_handler.
//
static int64_t
s_dummy_adm_add_user_handler(const char* token, const char* user_id, char** reply_msg) {
  static_cast<void>(token);
  static_cast<void>(user_id);
  static_cast<void>(reply_msg);
  return static_cast<int64_t>(RESULT_ANY_FAILURES);
}

//
// Dummy implementation of adm_del_jobs_handler.
//
static int64_t
s_dummy_adm_set_user_status_handler(const char* token, const char* user_id, bool enabled, char** reply_msg) {
  static_cast<void>(token);
  static_cast<void>(user_id);
  static_cast<void>(enabled);
  static_cast<void>(reply_msg);
  return static_cast<int64_t>(RESULT_ANY_FAILURES);
}

//
// Current handlers.
//
static submit_job_handler_t s_submit_job_handler = s_dummy_submit_job_handler;
static job_status_handler_t s_job_status_handler = s_dummy_job_status_handler;
static cancel_job_handler_t s_cancel_job_handler = s_dummy_cancel_job_handler;
static delete_job_handler_t s_delete_job_handler = s_dummy_delete_job_handler;
static job_list_handler_t s_job_list_handler = s_dummy_job_list_handler;
static adm_del_jobs_handler_t s_adm_del_jobs_handler = s_dummy_adm_del_jobs_handler;
static adm_add_user_handler_t s_adm_add_user_handler = s_dummy_adm_add_user_handler;
static adm_set_user_status_handler_t s_adm_set_user_status_handler = s_dummy_adm_set_user_status_handler;

//
// Returns a pointer to a function for submitting a job.
//
submit_job_handler_t
submit_job_handler() {
  return s_submit_job_handler;
}

//
// Returns a pointer to a function for submitting a job.
//
job_status_handler_t
job_status_handler() {
  return s_job_status_handler;
}

//
// Returns a pointer to a function for cancelling a job.
//
cancel_job_handler_t
cancel_job_handler() {
  return s_cancel_job_handler;
}

//
// Returns a pointer to a function for deleting a job.
//
delete_job_handler_t
delete_job_handler() {
  return s_delete_job_handler;
}

//
// Returns a pointer to a function for deleting a job.
//
job_list_handler_t
job_list_handler() {
  return s_job_list_handler;
}

//
// Returns a pointer to a function for an administrator to delete jobs.
//
adm_del_jobs_handler_t
adm_del_jobs_handler() {
  return s_adm_del_jobs_handler;
}

//
// Returns a pointer to a function for an administrator to add a user.
//
adm_add_user_handler_t
adm_add_user_handler() {
  return s_adm_add_user_handler;
}

//
// Returns a pointer to a function for an administrator to set status of a user.
//
adm_set_user_status_handler_t
adm_set_user_status_handler() {
  return s_adm_set_user_status_handler;
}

//
// Create an array of 'grpc_job_info' objects.
//
int64_t
grpc_job_info_create_array(grpc_job_info_t*** jobs, size_t n_jobs) {
  int64_t ret = RESULT_ANY_RUNTIME_ERROR;

  if (jobs != NULL && n_jobs > 0u) {
    *jobs = reinterpret_cast<grpc_job_info_t**>(malloc(sizeof(grpc_job_info_t *) * n_jobs));
    if (*jobs == NULL) {
      ret = RESULT_NO_MEMORY;
      goto error;
    }

    for (size_t i = 0u; i < n_jobs; i++) {
      (*jobs)[i] = NULL;
    }
    for (size_t i = 0u; i < n_jobs; i++) {
      (*jobs)[i] = reinterpret_cast<grpc_job_info_t*>(malloc(sizeof(grpc_job_info_t)));
      if ((*jobs)[i] == NULL) {
        ret = RESULT_NO_MEMORY;
        goto error;
      }
      (*jobs)[i]->job_id = NULL;
      (*jobs)[i]->status = JOB_STATUS_UNKNOWN;
      (*jobs)[i]->qc_job_id = NULL;
      (*jobs)[i]->result = NULL;
    }
    ret = RESULT_OK;

error:
    if (ret != RESULT_OK) {
      grpc_job_info_destroy_array(*jobs, n_jobs);
    }
  } else if (jobs != NULL && n_jobs == 0u) {
    *jobs = nullptr;
    ret = RESULT_OK;
  } else {
    ret = RESULT_INVALID_ARGS;
  }

  return ret;
}

//
// Destroy an array of 'grpc_job_info' objects.
//
void
grpc_job_info_destroy_array(grpc_job_info_t** jobs, size_t n_jobs) {
  if (jobs != NULL) {
    for (size_t i = 0u; i < n_jobs; i++) {
      if (jobs[i] != NULL) {
        free(jobs[i]->job_id);
        free(jobs[i]->qc_job_id);
        free(jobs[i]->result);
        free(jobs[i]);
      }
    }
    free(jobs);
  }
}

//
// 'start' function for the broker module.
//
int64_t
sqc_job_broker_initialize(const char* server_url, const char* conf_dir,
                          const job_broker_handlers_t* handlers,
                          int num_cqs, int min_pollers, int max_pollers, int cq_timeout_msec) {
  try {
    if (server_url != nullptr) {
      s_server_url = std::make_unique<std::string>(server_url);
    }
    if (conf_dir != nullptr) {
      s_conf_dir = std::make_unique<std::string>(conf_dir);
    }
    if (handlers != nullptr) {
      if (handlers->submit_job != nullptr) {
        s_submit_job_handler = handlers->submit_job;
      }
      if (handlers->job_status != nullptr) {
        s_job_status_handler = handlers->job_status;
      }
      if (handlers->cancel_job != nullptr) {
        s_cancel_job_handler = handlers->cancel_job;
      }
      if (handlers->delete_job != nullptr) {
        s_delete_job_handler = handlers->delete_job;
      }
      if (handlers->job_list != nullptr) {
        s_job_list_handler = handlers->job_list;
      }
      if (handlers->adm_del_jobs != nullptr) {
        s_adm_del_jobs_handler = handlers->adm_del_jobs;
      }
      if (handlers->adm_add_user != nullptr) {
        s_adm_add_user_handler = handlers->adm_add_user;
      }
      if (handlers->adm_set_user_status != nullptr) {
        s_adm_set_user_status_handler = handlers->adm_set_user_status;
      }
    }
    s_num_cqs = num_cqs;
    s_min_pollers = min_pollers;
    s_max_pollers = max_pollers;
    s_cq_timeout_msec = cq_timeout_msec;
  } catch (...) {
    msg_error("An exception occurred while initializing the gRPC server\n");
    return static_cast<int64_t>(RESULT_NO_MEMORY);
  }

  try {
    auto cert = new std::string();
    if (job_broker_read_file_in_dir(conf_dir, s_ca_cert_file, *cert)) {
      msg_info("Read the CA certificate file: %s/%s\n", conf_dir, s_ca_cert_file);
    } else {
      msg_error("Failed to read the file: %s/%s\n", conf_dir, s_ca_cert_file);
      return static_cast<int64_t>(RESULT_ANY_RUNTIME_ERROR);
    }
    s_ca_cert.reset(cert);
  } catch (...) {
    msg_error("An exception occurred while reading the CA certificate file\n");
    return static_cast<int64_t>(RESULT_NO_MEMORY);
  }

  try {
    auto cert = new std::string();
    if (job_broker_read_file_in_dir(conf_dir, s_server_cert_file, *cert)) {
      msg_info("Read the server certificate file: %s/%s\n", conf_dir, s_server_cert_file);
    } else {
      msg_error("Failed to read the file: %s/%s\n", conf_dir, s_server_cert_file);
      return static_cast<int64_t>(RESULT_ANY_RUNTIME_ERROR);
    }
    s_server_cert.reset(cert);
  } catch (...) {
    msg_error("An exception occurred while reading the server certificate file\n");
    return static_cast<int64_t>(RESULT_NO_MEMORY);
  }

  try {
    auto key = new std::string();
    if (job_broker_read_file_in_dir(conf_dir, s_server_key_file, *key)) {
      msg_info("Read the server private key file: %s/%s\n", conf_dir, s_server_key_file);
    } else {
      msg_error("Failed to read the file: %s/%s\n", conf_dir, s_server_key_file);
      return static_cast<int64_t>(RESULT_ANY_RUNTIME_ERROR);
    }
    s_server_key.reset(key);
  } catch (...) {
    msg_error("An exception occurred while reading the server private key file\n");
    return static_cast<int64_t>(RESULT_NO_MEMORY);
  }

  return static_cast<int64_t>(RESULT_OK);
}

//
// 'start' function for the broker module.
//
int64_t
sqc_job_broker_start() {
  const std::lock_guard<std::mutex> lock(s_mutex);
  int64_t result = static_cast<int64_t>(RESULT_OK);

  if (!s_server) {
    try {
      grpc::SslServerCredentialsOptions::PemKeyCertPair key_cert_pair = {
        *s_server_key, *s_server_cert
      };
      grpc::SslServerCredentialsOptions ssl_opts;
      ssl_opts.pem_root_certs = *s_ca_cert;
      ssl_opts.pem_key_cert_pairs.push_back(key_cert_pair);

      grpc::ServerBuilder builder;
      builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::NUM_CQS,
                                  s_num_cqs);
      builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::MIN_POLLERS,
                                  s_min_pollers);
      builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::MAX_POLLERS,
                                  s_max_pollers);
      builder.SetSyncServerOption(grpc::ServerBuilder::SyncServerOption::CQ_TIMEOUT_MSEC,
                                  s_cq_timeout_msec);
      builder.AddListeningPort(*s_server_url, grpc::SslServerCredentials(ssl_opts));
      static job_broker_service_impl service;
      builder.RegisterService(&service);
      s_server = builder.BuildAndStart();
      if (!s_server) {
        result = static_cast<int64_t>(RESULT_ANY_FAILURES);
        msg_error("Failed to start the gRPC server\n");
      } else {
        std::thread server_thread(sqc_job_broker_wait);
        server_thread.detach();
      }
    } catch(...) {
      result = static_cast<int64_t>(RESULT_ANY_FAILURES);
      msg_error("An exception occurred while starting the gRPC server\n");
    }
  }
  return result;
}

//
// 'stop' function for the broker module.
//
int64_t
sqc_job_broker_stop() {
  const std::lock_guard<std::mutex> lock(s_mutex);

  if (s_server) {
    try {
      s_server->Shutdown();
      s_server.release();
    } catch(...) {
      msg_error("An exception occurred while stopping the gRPC server\n");
    }
  }

  return static_cast<int64_t>(RESULT_OK);
}

//
// Waits gRPC server.
//
int64_t
sqc_job_broker_wait() {
  const std::lock_guard<std::mutex> lock(s_mutex);

  if (s_server) {
    try {
      s_server->Wait();
    } catch (...) {
      msg_error("An exception occurred while waiting the gRPC server completes a job\n");
      return static_cast<int64_t>(RESULT_ANY_FAILURES);
    }
  }
  return static_cast<int64_t>(RESULT_OK);
}
