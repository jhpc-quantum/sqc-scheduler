#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct grpc_job_info {
  char *job_id;
  int status;
  char *qc_job_id;
  char *result;
};

typedef struct grpc_job_info grpc_job_info_t;

// Function pointer of 'submit_job' handler.
typedef int64_t (*submit_job_handler_t)(const char* token, uint32_t priority, const char* qprogram, int circuit_fmt,
                                        size_t shots, int qc_type, int transpiler,
                                        const char* remark, const char* user_token,
                                        char** job_id, char** reply_msg);

// Function pointer of 'job_status' handler.
typedef int64_t (*job_status_handler_t)(const char* token, const char* job_id, int32_t* status,
                                        char** qc_job_id, char** result, char** reply_msg);

// Function pointer of 'cancel_job' handler.
typedef int64_t (*cancel_job_handler_t)(const char* token, const char* job_id, char** reply_msg);


// Function pointer of 'delete_job' handler.
typedef int64_t (*delete_job_handler_t)(const char* token, const char* job_id, char** reply_msg);


// Function pointer of 'job_list' handler.
typedef int64_t (*job_list_handler_t)(const char* token, grpc_job_info_t*** info, size_t* n_jobs,
                                      char** reply_msg);

// Function pointer of 'adm_del_jobs' handler.
typedef int64_t (*adm_del_jobs_handler_t)(const char* token, const char* user_id, int64_t from_time,
                                          int64_t to_time, char** reply_msg);

// Function pointer of 'adm_add_user' handler.
typedef int64_t (*adm_add_user_handler_t)(const char* token, const char* user_id, char** reply_msg);

// Function pointer of 'adm_set_user_status' handler.
typedef int64_t (*adm_set_user_status_handler_t)(const char* token, const char* user_id, bool enabled,
                                                 char** reply_msg);

// Set of function pointers for RPC handlers.
typedef struct  {
  submit_job_handler_t submit_job;
  job_status_handler_t job_status;
  cancel_job_handler_t cancel_job;
  delete_job_handler_t delete_job;
  job_list_handler_t job_list;
  adm_del_jobs_handler_t adm_del_jobs;
  adm_add_user_handler_t adm_add_user;
  adm_set_user_status_handler_t adm_set_user_status;
} job_broker_handlers_t;

// Initialize a job_broker_handlers_t object.
void initialize_job_broker_handlers(job_broker_handlers_t* handlers);

// Returns URL the gRPC server uses.
const char* sqc_job_broker_url(void);

// Returns a function for submitting a job.
submit_job_handler_t
submit_job_handler(void);

// Returns a function for getting status of a submitted job.
job_status_handler_t
job_status_handler(void);

// Returns a function for cancelling a submitted job.
cancel_job_handler_t
cancel_job_handler(void);

// Returns a function for deleting a submitted job.
delete_job_handler_t
delete_job_handler(void);

// Returns a function for deleting a submitted job.
job_list_handler_t
job_list_handler(void);

// Returns a function for an administrator to delete a submitted jobs.
adm_del_jobs_handler_t
adm_del_jobs_handler(void);

// Returns a function for an administrator to add an uesr.
adm_add_user_handler_t
adm_add_user_handler(void);

// Returns a function for an administrator to set status of a user.
adm_set_user_status_handler_t
adm_set_user_status_handler(void);

// Create an array of 'grpc_job_info' objects.
int64_t
grpc_job_info_create_array(grpc_job_info_t ***jobs, size_t n_jobs);

// Destroy an array of 'grpc_job_info' objects.
void
grpc_job_info_destroy_array(grpc_job_info_t **jobs, size_t n_jobs);


// 'initialize' point for the broker module.
int64_t
sqc_job_broker_initialize(const char* server_url, const char* conf_dir,
                          const job_broker_handlers_t *handlers,
                          int num_cqs, int min_pollers, int max_pollers, int cq_timeout_msec);

// 'start' point for the broker module.
int64_t
sqc_job_broker_start(void);

// 'stop' point for the broker module.
int64_t
sqc_job_broker_stop(void);

// Waits gRPC server.
int64_t
sqc_job_broker_wait(void);

#ifdef __cplusplus
} // extern "C"
#endif
