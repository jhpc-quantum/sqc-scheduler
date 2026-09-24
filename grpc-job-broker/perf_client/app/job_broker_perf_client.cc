#include <stdint.h>

#include "job_broker_perf_client.h"
#include "job_broker_ecode.h"

//
// Constructor.
//
job_broker_perf_client::job_broker_perf_client(std::shared_ptr<grpc::Channel> channel,
                                               const std::string& token)
  : stub_(job_broker::NewStub(channel)),
    token_(token) {
}

//
// Destructor.
//
job_broker_perf_client::~job_broker_perf_client() {
}

//
// Sends 'submit_job' request.
//
grpc::Status
job_broker_perf_client::submit_job(const std::string& qprorgram, ::circuit_fmt_t circuit_fmt, std::size_t shots,
                                   ::qc_type_t qc_type, transpiler_t transpiler, const std::string& remark,
                                   std::optional<std::string> user_token, std::optional<std::string> group_id,
                                   std::uint32_t priority, ::submit_job_reply& reply) {
  submit_job_request request;
  request.set_token(token_);
  request.set_qprogram(qprorgram);
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
