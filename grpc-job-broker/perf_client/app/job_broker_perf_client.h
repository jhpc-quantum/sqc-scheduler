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
class job_broker_perf_client {
public:
  job_broker_perf_client(std::shared_ptr<grpc::Channel> channel, const std::string& token);
  ~job_broker_perf_client();

  // Deletes copy / move constructors and copy / move assignment operators.
  job_broker_perf_client(const job_broker_perf_client& other) = delete;
  job_broker_perf_client(job_broker_perf_client&& other) = delete;
  job_broker_perf_client& operator=(const job_broker_perf_client& other) = delete;
  job_broker_perf_client& operator=(job_broker_perf_client&& other) = delete;

  // Sends 'submit_job' request.
  grpc::Status
  submit_job(const std::string& qprogram, circuit_fmt_t circuit_fmt, std::size_t shots, qc_type_t qc_type,
             transpiler_t transpiler, const std::string& remark, std::optional<std::string> user_token,
             std::uint32_t priority, submit_job_reply& reply);

private:
  std::unique_ptr<job_broker::Stub> stub_;
  const std::string token_;
}; // class job_broker_perf_client
