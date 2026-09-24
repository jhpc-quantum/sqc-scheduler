#pragma once

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <optional>
#include <string>

#include <grpcpp/grpcpp.h>

#include "job_broker.grpc.pb.h"

//
// gRPC handler for client.
//
class job_broker_client {
public:
  job_broker_client(std::shared_ptr<grpc::Channel> channel, const std::string& token);
  ~job_broker_client();

  // Deletes copy / move constructors and copy / move assignment operators.
  job_broker_client(const job_broker_client& other) = delete;
  job_broker_client(job_broker_client&& other) = delete;
  job_broker_client& operator=(const job_broker_client& other) = delete;
  job_broker_client& operator=(job_broker_client&& other) = delete;

  // Sends 'submit_job' request.
  grpc::Status
  submit_job(std::uint32_t priority, const std::string& qprogram, circuit_fmt_t circuit_fmt,
             std::size_t shots, qc_type_t qc_type, transpiler_t transpiler, const std::string& remark,
             std::optional<std::string> user_token, std::optional<std::string> group_id,
             submit_job_reply& reply);

  // Sends 'job_status' request.
  grpc::Status
  job_status(const std::string& job_id, job_status_reply& reply);

  // Sends 'cancel_job' request.
  grpc::Status
  cancel_job(const std::string& job_id, cancel_job_reply& reply);

  // Sends 'delete_job' request.
  grpc::Status
  delete_job(const std::string& job_id, delete_job_reply& reply);

  // Sends 'job_list' request.
  grpc::Status
  job_list(job_list_reply& reply);

  // Sends 'adm_del_jobs' request.
  grpc::Status
  adm_del_jobs(const std::string& user_id, int64_t from_time, int64_t to_time, adm_del_jobs_reply& reply);

  // Sends 'adm_add_user' request.
  grpc::Status
  adm_add_user(const std::string& user_id, adm_add_user_reply& reply);

  // Sends 'adm_set_user_status' request.
  grpc::Status
  adm_set_user_status(const std::string& user_id, bool enabled, adm_set_user_status_reply& reply);

  // Sends 'adm_set_group_exec_time_limit' request.
  grpc::Status
  adm_set_group_exec_time_limit(const std::string& group_id, uint64_t exec_time_limit,
                                adm_set_group_exec_time_limit_reply& reply);

  // Sends 'adm_set_user_group_status' request.
  grpc::Status
  adm_set_user_group_status(const std::string& user_id, const std::string& group_id, bool enabled,
                            adm_set_user_group_status_reply& reply);

private:
  std::unique_ptr<job_broker::Stub> stub_;
  const std::string token_;
}; // class job_broker_client
