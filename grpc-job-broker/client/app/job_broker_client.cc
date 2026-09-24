#include <stdint.h>

#include "job_broker_client.h"
#include "job_broker_ecode.h"

//
// Constructor.
//
job_broker_client::job_broker_client(std::shared_ptr<grpc::Channel> channel,
                                     const std::string& token)
  : stub_(job_broker::NewStub(channel)),
    token_(token) {
}

//
// Destructor.
//
job_broker_client::~job_broker_client() {
}

//
// Sends 'submit_job' request.
//
grpc::Status job_broker_client::submit_job(std::uint32_t priority,
                                           const std::string& qprogram, circuit_fmt_t circuit_fmt,
                                           std::size_t shots, ::qc_type_t qc_type,
                                           transpiler_t transpiler, const std::string& remark,
                                           std::optional<std::string> user_token,
                                           std::optional<std::string> group_id,
                                           ::submit_job_reply& reply) {
  submit_job_request request;
  request.set_token(token_);
  request.set_qprogram(qprogram);
  request.set_circuit_fmt(circuit_fmt);
  request.set_shots(shots);
  request.set_qc_type(qc_type);
  request.set_transpiler(transpiler);
  request.set_remark(remark);
  if (user_token.has_value()) {
    request.set_user_token(user_token.value());
  } else {
    request.clear_user_token();
  }
  if (group_id.has_value()) {
    request.set_group_id(group_id.value());
  } else {
    request.clear_group_id();
  }
  request.set_priority(priority);
  grpc::ClientContext context;
  return stub_->submit_job(&context, request, &reply);
}

//
// Sends 'job_status' request.
//
grpc::Status
job_broker_client::job_status(const std::string& job_id, job_status_reply& reply) {
  job_status_request request;
  request.set_token(token_);
  request.set_job_id(job_id);
  grpc::ClientContext context;

  return stub_->job_status(&context, request, &reply);
}

//
// Sends 'cancel_job' request.
//
grpc::Status
job_broker_client::cancel_job(const std::string& job_id, cancel_job_reply& reply) {
  cancel_job_request request;
  request.set_token(token_);
  request.set_job_id(job_id);
  grpc::ClientContext context;
  return stub_->cancel_job(&context, request, &reply);
}

//
// Sends 'delete_job' request.
//
grpc::Status
job_broker_client::delete_job(const std::string& job_id, delete_job_reply& reply) {
  delete_job_request request;
  request.set_token(token_);
  request.set_job_id(job_id);
  grpc::ClientContext context;
  return stub_->delete_job(&context, request, &reply);
}

//
// Sends 'job_list' request.
//
grpc::Status
job_broker_client::job_list(job_list_reply& reply) {
  job_list_request request;
  request.set_token(token_);
  grpc::ClientContext context;
  return stub_->job_list(&context, request, &reply);
}

//
// Sends 'adm_del_jobs' request.
//
grpc::Status
job_broker_client::adm_del_jobs(const std::string& user_id, int64_t from_time, int64_t to_time,
                                adm_del_jobs_reply& reply) {
  adm_del_jobs_request request;
  request.set_token(token_);
  request.set_user_id(user_id);
  request.set_from_time(from_time);
  request.set_to_time(to_time);
  grpc::ClientContext context;
  return stub_->adm_del_jobs(&context, request, &reply);
}

//
// Sends 'adm_add_user' request.
//
grpc::Status
job_broker_client::adm_add_user(const std::string& user_id, adm_add_user_reply& reply) {
  adm_add_user_request request;
  request.set_token(token_);
  request.set_user_id(user_id);
  grpc::ClientContext context;
  return stub_->adm_add_user(&context, request, &reply);
}

//
// Sends 'adm_set_user_status' request.
//
grpc::Status
job_broker_client::adm_set_user_status(const std::string& user_id, bool enabled, adm_set_user_status_reply& reply) {
  adm_set_user_status_request request;
  request.set_token(token_);
  request.set_user_id(user_id);
  request.set_enabled(enabled);
  grpc::ClientContext context;
  return stub_->adm_set_user_status(&context, request, &reply);
}

//
// Sends 'adm_set_group_exec_time_limit' request.
//
grpc::Status
job_broker_client::adm_set_group_exec_time_limit(const std::string& group_id, uint64_t exec_time_limit,
                                                 adm_set_group_exec_time_limit_reply& reply) {
  adm_set_group_exec_time_limit_request request;
  request.set_token(token_);
  request.set_group_id(group_id);
  request.set_exec_time_limit(exec_time_limit);
  grpc::ClientContext context;
  return stub_->adm_set_group_exec_time_limit(&context, request, &reply);
}

//
// Sends 'adm_set_user_group_status' request.
//
grpc::Status
job_broker_client::adm_set_user_group_status(const std::string& user_id, const std::string& group_id,
                                             bool enabled, adm_set_user_group_status_reply& reply) {
  adm_set_user_group_status_request request;
  request.set_token(token_);
  request.set_user_id(user_id);
  request.set_group_id(group_id);
  request.set_enabled(enabled);
  grpc::ClientContext context;
  return stub_->adm_set_user_group_status(&context, request, &reply);
}
