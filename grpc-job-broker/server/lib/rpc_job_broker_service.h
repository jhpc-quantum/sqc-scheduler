#pragma once

#include <stddef.h>
#include <stdint.h>

#include <grpcpp/grpcpp.h>

#include "job_broker.grpc.pb.h"

//
// gRPC handler for 'sqc_rpc_job_broker' service.
//
class job_broker_service_impl final : public ::job_broker::Service {
  // handler for 'submit_job' request.
  grpc::Status
  submit_job(grpc::ServerContext* context, const submit_job_request* request,
             submit_job_reply* reply) override;

  // handler for 'job_status' request.
  grpc::Status
  job_status(grpc::ServerContext* context, const job_status_request* request,
             job_status_reply* reply) override;

  // handler for 'cancel_job' request.
  grpc::Status
  cancel_job(grpc::ServerContext* context, const cancel_job_request* request,
             cancel_job_reply* reply) override;

  // handler for 'delete_job' request.
  grpc::Status
  delete_job(grpc::ServerContext* context, const delete_job_request* request,
             delete_job_reply* reply) override;

  // handler for 'job_list' request.
  grpc::Status
  job_list(grpc::ServerContext* context, const job_list_request* request,
           job_list_reply* reply) override;

  // handler for 'adm_del_jobs' request.
  grpc::Status
  adm_del_jobs(grpc::ServerContext* context, const adm_del_jobs_request* request,
               adm_del_jobs_reply* reply) override;

  // handler for 'adm_add_user' request.
  grpc::Status
  adm_add_user(grpc::ServerContext* context, const adm_add_user_request* request,
               adm_add_user_reply* reply) override;

  // handler for 'adm_set_user_status' request.
  grpc::Status
  adm_set_user_status(grpc::ServerContext* context, const adm_set_user_status_request* request,
                      adm_set_user_status_reply* reply) override;

  // handler for 'adm_set_user_status' request.
  grpc::Status adm_set_group_exec_time_limit(grpc::ServerContext *context,
                                             const adm_set_group_exec_time_limit_request *request,
                                             adm_set_group_exec_time_limit_reply *reply) override;

  // handler for 'adm_set_user_group_status' request.
  grpc::Status adm_set_user_group_status(grpc::ServerContext *context,
                                         const adm_set_user_group_status_request *request,
                                         adm_set_user_group_status_reply *reply) override;

  // Parses a <std::string> instance of an user ID and converts it into <char*>.
  bool
  s_parse_user_id(const std::string& id_string, char* id_c_ptr, std::size_t c_ptr_size);

  // Parses a <std::string> instance of qprogram and and converts it into <char*>.
  bool
  s_parse_qprogram(const std::string& qprogram_string, char* qprogram_c_ptr, std::size_t c_ptr_size);

  // Parses a <std::string> instance of a remark and converts it into <char*>.
  bool
  s_parse_remark(const std::string& remark_string, char* remark_c_ptr, std::size_t c_ptr_size);

  // Parses a <std::string> instance of a job ID and converts it into <char*>.
  bool
  s_parse_job_id(const std::string& id_string, char* id_c_ptr, std::size_t c_ptr_size);

  // Returns a status message corresponding with the staus code.
  const std::string& s_get_status_message(int job_status) const;
}; // class job_broker_service_impl
