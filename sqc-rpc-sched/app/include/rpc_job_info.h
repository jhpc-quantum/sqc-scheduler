#pragma once

#include "dbmgr.h"


__BEGIN_DECLS


struct rpc_job_info {
  char *job_id;
  sqc_rpc_sched_job_status_t status;
  char *qc_job_id;
  char *result;
};

typedef struct rpc_job_info rpc_job_info_t;

sqc_result_t
rpc_job_info_create(rpc_job_info_t **job);

void
rpc_job_info_destroy(rpc_job_info_t *job);

sqc_result_t
rpc_job_info_set_job_id(rpc_job_info_t *job, char *job_id);

sqc_result_t
rpc_job_info_set_qc_job_id(rpc_job_info_t *job, char *qc_job_id);

sqc_result_t
rpc_job_info_set_result(rpc_job_info_t *job, char *result);

sqc_result_t
rpc_job_info_create_array(rpc_job_info_t ***jobs, size_t n_jobs);

void
rpc_job_info_destroy_array(rpc_job_info_t **jobs, size_t n_jobs);


__END_DECLS
