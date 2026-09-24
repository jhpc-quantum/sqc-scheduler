#include <stddef.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include "job_broker_ecode.h"
#include "job_broker_logger_internal.h"
#include "rpc_job_broker_service.h"
#include "job_broker_server.h"

//
// Strip whitespaces at the end of the string.
//
static inline std::string
strip_text(const std::string& s) {
  std::string ret = s;
  for (auto it = ret.rbegin(); it != ret.rend(); it++) {
    if (*it != ' ' && *it != '\t' && *it != '\r' && *it != '\n') {
      break;
    }
    ret.pop_back();
  }
  return ret;
}

//
// gRPC handler for 'submit_job' request.
//
grpc::Status
job_broker_service_impl::submit_job(grpc::ServerContext* context,
                                    const submit_job_request* request,
                                    submit_job_reply* reply) {
  static constexpr char log_prefix[] = "gRPC-SUBMIT_JOB";
  static_cast<void>(context);
  result_t code = RESULT_ANY_RUNTIME_ERROR;
  char* reply_msg = nullptr;

  try {
    std::string token = strip_text(request->token());
    msg_info("%s: Received a request: token=%s, priority=%u, qprogram=%zu bytes, "
             "shots=%zu, qc_type=%d, transpiler=%ld, remark=%s, "
             "has_user_token=%s, group_id=%s\n",
             log_prefix,
             token.c_str(),
             static_cast<unsigned int>(request->priority()),
             request->qprogram().size(),
             static_cast<size_t>(request->shots()),
             static_cast<int>(request->qc_type()),
             static_cast<long>(request->transpiler()),
             request->remark().c_str(),
             static_cast<bool>(request->has_user_token()) ? "true" : "false",
             static_cast<bool>(request->has_group_id()) ? "true" : "false");

    // Creates a job.
    char* job_id = nullptr;

    const char* user_token = nullptr;
    if (request->has_user_token()) {
      user_token = request->user_token().c_str();
    }

    const char* group_id = nullptr;
    if (request->has_group_id()) {
      group_id = request->group_id().c_str();
    }

    auto handle_submit_job = submit_job_handler();
    code = handle_submit_job(token.c_str(),
                             request->priority(),
                             request->qprogram().c_str(),
                             static_cast<int>(request->circuit_fmt()),
                             static_cast<size_t>(request->shots()),
                             static_cast<int>(request->qc_type()),
                             static_cast<int>(request->transpiler()),
                             request->remark().c_str(),
                             user_token,
                             group_id,
                             &job_id,
                             &reply_msg);
    if (code == RESULT_OK) {
      if (job_id == nullptr) {
        msg_warning("%s: Missing job ID in the reply\n", log_prefix);
      } else {
        reply->set_job_id(std::string(job_id));
        msg_debug(5, "%s: Created\n", log_prefix);
      }
    } else {
      msg_debug(5, "%s: Failed to create a job: code=%ld\n", log_prefix, static_cast<long>(code));
    }

    free(job_id);
  } catch (...) {
    msg_error("%s: An exception occurred while submitting a job\n", log_prefix);
  }

  try {
    // Send a reply.
    reply->set_code(static_cast<int64_t>(code));
    reply->set_message(reply_msg == nullptr ? "" : reply_msg);
    msg_info("%s: Send a reply: code=%ld, message=%s, job_id=%s\n",
             log_prefix, static_cast<long>(reply->code()), reply->message().c_str(),
             reply->has_job_id() ? reply->job_id().c_str() : "(none)");
  } catch (...) {
    msg_error("%s: An exception occurred while replying a message\n", log_prefix);
  }

  free(reply_msg);
  return grpc::Status::OK;
}

//
// gRPC handler for 'job_status' request.
//
grpc::Status
job_broker_service_impl::job_status(grpc::ServerContext* context,
                                    const job_status_request* request,
                                    job_status_reply* reply) {
  static constexpr char log_prefix[] = "gRPC-JOB_STATUS";
  static_cast<void>(context);
  result_t code = RESULT_ANY_RUNTIME_ERROR;
  int job_status = JOB_STATUS_UNKNOWN;
  char* qc_job_id = nullptr;
  char* job_result = nullptr;
  char* reply_msg = nullptr;

  try {
    std::string token = strip_text(request->token());
    msg_info("%s: Received a request: job_id=%s, token=%s\n",
             log_prefix, request->job_id().c_str(), token.c_str());

    // Get status of the job.
    auto handle_job_status = job_status_handler();
    code = handle_job_status(token.c_str(), request->job_id().c_str(), &job_status, &qc_job_id, &job_result,
                             &reply_msg);
    if (code == RESULT_OK) {
      msg_debug(5, "%s: Got status of the job\n", log_prefix);
    } else {
      msg_debug(5, "%s: Failed to get status of the job: code=%ld, job_id=%s\n",
                log_prefix, static_cast<long>(code), request->job_id().c_str());
    }
  } catch (...) {
    msg_error("%s: An exception occurred while getting the job status\n", log_prefix);
  }

  try {
    // Send a reply.
    reply->set_code(code);
    reply->set_message(reply_msg == nullptr ? "" : reply_msg);
    reply->set_status(static_cast<job_status_t>(job_status));
    if (code == RESULT_OK) {
      if (job_status == JOB_STATUS_DONE) {
        if (qc_job_id == nullptr) {
          msg_warning("%s: Missing qc_job_id in the reply\n", log_prefix);
        } else {
          reply->set_qc_job_id(std::string(qc_job_id));
        }

        if (job_result == nullptr) {
          msg_warning("%s: Missing job_result in the reply\n", log_prefix);
        } else {
          reply->set_result(std::string(job_result));
        }
      }
    }

    free(qc_job_id);
    free(job_result);

    if (reply->has_result()) {
      msg_info("%s: Send a reply: code=%ld, message=%s, status=%d, qc_job_id=%zu, result=%zu bytes\n",
               log_prefix, static_cast<long>(reply->code()), reply->message().c_str(),
               static_cast<int>(reply->status()), reply->qc_job_id().size(), reply->result().size());
    } else {
      msg_info("%s: Send a reply: code=%ld, message=%s, status=%d, qc_job_id=(none), result=(none)\n",
               log_prefix, static_cast<long>(reply->code()), reply->message().c_str(),
               static_cast<int>(reply->status()));
    }
  } catch (...) {
    msg_error("%s: An exception occurred while replying a message\n", log_prefix);
  }

  free(reply_msg);
  return grpc::Status::OK;
}

//
// gRPC handler for 'cancel_job' request.
//
grpc::Status
job_broker_service_impl::cancel_job(grpc::ServerContext* context,
                                    const cancel_job_request* request,
                                    cancel_job_reply* reply) {
  static constexpr char log_prefix[] = "gRPC-CANCEL_JOB";
  static_cast<void>(context);
  result_t code = RESULT_ANY_RUNTIME_ERROR;
  char* reply_msg = nullptr;

  try {
    std::string token = strip_text(request->token());
    msg_info("%s: Received a request: job_id=%s, token=%s\n",
             log_prefix, request->job_id().c_str(), token.c_str());

    // Cancel the job.
    auto handle_cancel_job = cancel_job_handler();
    code = handle_cancel_job(token.c_str(), request->job_id().c_str(), &reply_msg);
    if (code == RESULT_OK) {
      msg_debug(5, "%s: Cancelled\n", log_prefix);
    } else {
      msg_debug(5, "%s: Failed to cancel the job: code=%ld, job_id=%s\n",
                log_prefix, static_cast<long>(code), request->job_id().c_str());
    }
  } catch (...) {
    msg_error("%s: An exception occurred while cancelling the job\n", log_prefix);
  }

  try {
    // Send a reply.
    reply->set_code(code);
    reply->set_message(reply_msg == nullptr ? "" : reply_msg);
    msg_info("%s: Send a reply: code=%ld, message=%s\n",
             log_prefix, static_cast<long>(reply->code()), reply->message().c_str());
  } catch (...) {
    msg_error("%s: An exception occurred while replying a message\n", log_prefix);
  }

  free(reply_msg);
  return grpc::Status::OK;
}

//
// gRPC handler for 'delete_job' request.
//
grpc::Status
job_broker_service_impl::delete_job(grpc::ServerContext* context,
                                    const delete_job_request* request,
                                    delete_job_reply* reply) {
  static constexpr char log_prefix[] = "gRPC-DELETE_JOB";
  static_cast<void>(context);
  result_t code = RESULT_ANY_RUNTIME_ERROR;
  char* reply_msg = nullptr;

  try {
    std::string token = strip_text(request->token());
    msg_info("%s: Received a request: job_id=%s, token=%s\n",
             log_prefix, request->job_id().c_str(), token.c_str());

    // Delete the job.
    auto handle_delete_job = delete_job_handler();
    code = handle_delete_job(token.c_str(), request->job_id().c_str(), &reply_msg);
    if (code == RESULT_OK) {
      msg_debug(5, "%s: Deleted\n", log_prefix);
    } else {
      msg_debug(5, "%s: Failed to delete the job: code=%ld, job_id=%s\n",
                log_prefix, static_cast<long>(code), request->job_id().c_str());
    }
  } catch (...) {
    msg_error("%s: An exception occurred while deleting the job\n", log_prefix);
  }

  try {
    // Send a reply.
    reply->set_code(code);
    reply->set_message(reply_msg == nullptr ? "" : reply_msg);
    msg_info("%s: Send a reply: code=%ld, message=%s\n",
             log_prefix, static_cast<long>(reply->code()), reply->message().c_str());
  } catch (...) {
    msg_error("%s: An exception occurred while replying a message\n", log_prefix);
  }

  free(reply_msg);
  return grpc::Status::OK;
}

//
// gRPC handler for 'job_list' request.
//
grpc::Status
job_broker_service_impl::job_list(grpc::ServerContext* context,
                                  const job_list_request* request,
                                  job_list_reply* reply) {
  static constexpr char log_prefix[] = "gRPC-JOB_LIST";
  static_cast<void>(context);
  result_t code = RESULT_ANY_RUNTIME_ERROR;
  grpc_job_info_t **jobs = nullptr;
  size_t n_jobs = 0;
  char* reply_msg = nullptr;

  try {
    std::string token = strip_text(request->token());
    msg_info("%s: Received a request: token=%s\n", log_prefix, token.c_str());

    // Get information about the submitted jobs.
    auto handle_job_list = job_list_handler();
    code = handle_job_list(token.c_str(), &jobs, &n_jobs, &reply_msg);
    if (code == RESULT_OK) {
      msg_debug(5, "%s: Got a list of the submitted jobs\n", log_prefix);
    } else {
      msg_debug(5, "%s: Failed to get information about the submitted jofb: code=%ld\n",
                log_prefix, static_cast<long>(code));
    }
  } catch (...) {
    msg_error("%s: An exception occurred while getting information about the sbumitted jobs\n",
               log_prefix);
  }

  try {
    // Send a reply.
    reply->set_code(code);
    reply->set_message(reply_msg == nullptr ? "" : reply_msg);
    if (code == RESULT_OK) {
      for (size_t i = 0u; i < n_jobs; i++) {
        job_info* reply_job = reply->add_jobs();
        reply_job->set_job_id(std::string(jobs[i]->job_id));
        reply_job->set_status(static_cast<job_status_t>(jobs[i]->status));
        if (jobs[i]->status == JOB_STATUS_DONE) {
          if (jobs[i]->qc_job_id != nullptr) {
            reply_job->set_qc_job_id(std::string(jobs[i]->qc_job_id));
          }

          if (jobs[i]->result != nullptr) {
            reply_job->set_result(std::string(jobs[i]->result));
          }
        }
      }
    }
    msg_info("%s: Send a reply: code=%ld, message=%s\n",
             log_prefix, static_cast<long>(reply->code()), reply->message().c_str());
  } catch (...) {
    msg_error("%s: An exception occurred while replying a message\n", log_prefix);
  }

  free(reply_msg);
  grpc_job_info_destroy_array(jobs, n_jobs);
  return grpc::Status::OK;
}

//
// gRPC handler for 'adm_del_jobs' request.
//
grpc::Status
job_broker_service_impl::adm_del_jobs(grpc::ServerContext* context,
                                      const adm_del_jobs_request* request,
                                      adm_del_jobs_reply* reply) {
  static constexpr char log_prefix[] = "gRPC-ADM_DEL_JOBS";
  static_cast<void>(context);
  result_t code = RESULT_ANY_RUNTIME_ERROR;
  char* reply_msg = nullptr;

  try {
    std::string token = strip_text(request->token());
    msg_info("%s: Received a request: user_id=%s, from_time=%lld, to_time=%lld, token=%s\n",
             log_prefix, request->user_id().c_str(), (long long) request->from_time(),
             (long long) request->from_time(), token.c_str());

    // Delete the job.
    auto handle_adm_del_jobs = adm_del_jobs_handler();
    code = handle_adm_del_jobs(token.c_str(), request->user_id().c_str(),
                               request->from_time(), request->to_time(), &reply_msg);
    if (code == RESULT_OK) {
      msg_debug(5, "%s: Deleted", log_prefix);
    } else {
      msg_debug(5, "%s: Failed to delete jobs: code=%ld, user_id=%s, from_time=%lld, to_time=%lld\n",
                log_prefix, static_cast<long>(code), request->user_id().c_str(),
                static_cast<long long>(request->from_time()), static_cast<long long>(request->to_time()));
    }
  } catch (...) {
    msg_error("%s: An exception occurred while deleting the job\n", log_prefix);
  }

  try {
    // Send a reply.
    reply->set_code(code);
    reply->set_message(reply_msg == nullptr ? "" : reply_msg);
    msg_info("%s: Send a reply: code=%ld, message=%s\n",
             log_prefix, static_cast<long>(reply->code()), reply->message().c_str());
  } catch (...) {
    msg_error("%s: An exception occurred while replying a message\n", log_prefix);
  }

  free(reply_msg);
  return grpc::Status::OK;
}

//
// gRPC handler for 'adm_add_user' request.
//
grpc::Status
job_broker_service_impl::adm_add_user(grpc::ServerContext* context,
                                      const adm_add_user_request* request,
                                      adm_add_user_reply* reply) {
  static constexpr char log_prefix[] = "gRPC-ADM_ADD_USER";
  static_cast<void>(context);
  result_t code = RESULT_ANY_RUNTIME_ERROR;
  char* reply_msg = nullptr;

  try {
    std::string token = strip_text(request->token());
    msg_info("%s: Received a request: user_id=%s, token=%s\n",
             log_prefix, request->user_id().c_str(), token.c_str());

    // Add a user.
    auto handle_adm_add_user = adm_add_user_handler();
    code = handle_adm_add_user(token.c_str(), request->user_id().c_str(), &reply_msg);
    if (code == RESULT_OK) {
      msg_debug(5, "%s: Added", log_prefix);
    } else {
      msg_debug(5, "%s: Failed to add the user: code=%ld, user_id=%s\n",
                log_prefix, static_cast<long>(code), request->user_id().c_str());
    }
  } catch (...) {
    msg_error("%s: An exception occurred while creating the user\n", log_prefix);
  }

  try {
    // Send a reply.
    reply->set_code(code);
    reply->set_message(reply_msg == nullptr ? "" : reply_msg);
    msg_info("%s: Send a reply: code=%ld, message=%s\n",
             log_prefix, static_cast<long>(reply->code()), reply->message().c_str());
  } catch (...) {
    msg_error("%s: An exception occurred while replying a message\n", log_prefix);
  }

  free(reply_msg);
  return grpc::Status::OK;
}

//
// gRPC handler for 'adm_set_user_status' request.
//
grpc::Status
job_broker_service_impl::adm_set_user_status(grpc::ServerContext* context,
                                             const adm_set_user_status_request* request,
                                             adm_set_user_status_reply* reply) {
  static constexpr char log_prefix[] = "gRPC-ADM_SET_USER_STATUS";
  static_cast<void>(context);
  result_t code = RESULT_ANY_RUNTIME_ERROR;
  char* reply_msg = nullptr;

  try {
    std::string token = strip_text(request->token());
    msg_info("%s: Received a request: user_id=%s, enabled=%s, token=%s\n",
             log_prefix,
             request->user_id().c_str(),
             request->enabled() ? "true" : "false",
             token.c_str());

    // Add a user.
    auto handle_adm_set_user_status = adm_set_user_status_handler();
    code = handle_adm_set_user_status(token.c_str(), request->user_id().c_str(), request->enabled(),
                                      &reply_msg);
    if (code == RESULT_OK) {
      msg_debug(5, "%s: Set status", log_prefix);
    } else {
      msg_debug(5, "%s: Failed to set user status: code=%ld, user_id=%s, enabled=%s\n",
                log_prefix,
                static_cast<long>(code),
                request->user_id().c_str(),
                request->enabled() ? "true" : "false");
    }
  } catch (...) {
    msg_error("%s: An exception occurred while setting the status of an user\n", log_prefix);
  }

  try {
    // Send a reply.
    reply->set_code(code);
    reply->set_message(reply_msg == nullptr ? "" : reply_msg);
    msg_info("%s: Send a reply: code=%ld, message=%s\n",
             log_prefix, static_cast<long>(reply->code()), reply->message().c_str());
  } catch (...) {
    msg_error("%s: An exception occurred while replying a message\n", log_prefix);
  }

  free(reply_msg);
  return grpc::Status::OK;
}

//
// gRPC handler for 'adm_set_group_exec_time_limit' request.
//
grpc::Status
job_broker_service_impl::adm_set_group_exec_time_limit(grpc::ServerContext *context,
                                                       const adm_set_group_exec_time_limit_request *request,
                                                       adm_set_group_exec_time_limit_reply *reply) {
  static constexpr char log_prefix[] = "gRPC-ADM_SET_GROUP_EXEC_TIME_LIMIT";
  static_cast<void>(context);
  result_t code = RESULT_ANY_RUNTIME_ERROR;
  char* reply_msg = nullptr;

  try {
    std::string token = strip_text(request->token());
    msg_info("%s: Received a request: user_id=%s, exec_time_limit=%ld, token=%s\n",
             log_prefix, request->group_id().c_str(),
             request->exec_time_limit(), token.c_str());


    // Set exec_time_limit.
    auto handle_adm_set_group_exec_time_limit = adm_set_group_exec_time_limit_handler();
    code = handle_adm_set_group_exec_time_limit(token.c_str(), request->group_id().c_str(),
                                                request->exec_time_limit(), &reply_msg);
    if (code == RESULT_OK) {
      msg_debug(5, "%s: Set exec_time_limit", log_prefix);
    } else {
      msg_debug(5, "%s: Failed to set group exec_time_limit: code=%ld, group_id=%s, exec_time_limit=%ld\n",
                log_prefix, static_cast<long>(code),
                request->group_id().c_str(), request->exec_time_limit());
    }
  } catch (...) {
    msg_error("%s: An exception occurred while setting the exec_time_limit of an group\n", log_prefix);
  }

  try {
    // Send a reply.
    reply->set_code(code);
    reply->set_message(reply_msg == nullptr ? "" : reply_msg);
    msg_info("%s: Send a reply: code=%ld, message=%s\n",
             log_prefix, static_cast<long>(reply->code()), reply->message().c_str());
  } catch (...) {
    msg_error("%s: An exception occurred while replying a message\n", log_prefix);
  }

  free(reply_msg);
  return grpc::Status::OK;
}

//
// gRPC handler for 'adm_set_user_group_status' request.
//
grpc::Status
job_broker_service_impl::adm_set_user_group_status(grpc::ServerContext *context,
                                                   const adm_set_user_group_status_request *request,
                                                   adm_set_user_group_status_reply *reply) {
  static constexpr char log_prefix[] = "gRPC-ADM_SET_USER_GROUP_STATUS";
  static_cast<void>(context);
  result_t code = RESULT_ANY_RUNTIME_ERROR;
  char* reply_msg = nullptr;

  try {
    std::string token = strip_text(request->token());
    msg_info("%s: Received a request: user_id=%s, group_id=%s, enabled=%s, token=%s\n",
             log_prefix, request->user_id().c_str(), request->group_id().c_str(),
             request->enabled() ? "true" : "false", token.c_str());

    // Set the user-group association status.
    auto handle_adm_set_user_group_status = adm_set_user_group_status_handler();
    code = handle_adm_set_user_group_status(token.c_str(), request->user_id().c_str(),
                                            request->group_id().c_str(), request->enabled(),
                                            &reply_msg);
    if (code == RESULT_OK) {
      msg_debug(5, "%s: Set status", log_prefix);
    } else {
      msg_debug(5, "%s: Failed to set user-group status: code=%ld, user_id=%s, group_id=%s, enabled=%s\n",
                log_prefix, static_cast<long>(code),
                request->user_id().c_str(), request->group_id().c_str(),
                request->enabled() ? "true" : "false");
    }
  } catch (...) {
    msg_error("%s: An exception occurred while setting the status of a user-group association\n",
              log_prefix);
  }

  try {
    // Send a reply.
    reply->set_code(code);
    reply->set_message(reply_msg == nullptr ? "" : reply_msg);
    msg_info("%s: Send a reply: code=%ld, message=%s\n",
             log_prefix, static_cast<long>(reply->code()), reply->message().c_str());
  } catch (...) {
    msg_error("%s: An exception occurred while replying a message\n", log_prefix);
  }

  free(reply_msg);
  return grpc::Status::OK;
}
