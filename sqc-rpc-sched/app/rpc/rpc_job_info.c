#include "sqc_apis.h"
#include "rpc_job_info.h"


sqc_result_t
rpc_job_info_create(rpc_job_info_t **job) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(job != NULL)) {
    *job = malloc(sizeof(rpc_job_info_t));
    if (likely(*job != NULL)) {
      (*job)->job_id = NULL;
      (*job)->status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;
      (*job)->qc_job_id = NULL;
      (*job)->result = NULL;
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_NO_MEMORY;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


void
rpc_job_info_destroy(rpc_job_info_t *job) {
  if (likely(job != NULL)) {
    free(job->job_id);
    free(job->qc_job_id);
    free(job->result);
    free(job);
  }
}


sqc_result_t
rpc_job_info_set_job_id(rpc_job_info_t *job, char *job_id) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(job != NULL)) {
    if (unlikely(job->job_id != NULL)) {
      free(job->job_id);
    }
    if (likely(job_id != NULL)) {
      job->job_id = strdup(job_id);
      if (likely(job->job_id != NULL)) {
        ret = SQC_RESULT_OK;
      } else {
        ret = SQC_RESULT_NO_MEMORY;
      }
    } else {
      job->job_id = NULL;
      ret = SQC_RESULT_OK;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


sqc_result_t
rpc_job_info_set_qc_job_id(rpc_job_info_t *job, char *qc_job_id) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(job != NULL)) {
    if (unlikely(job->qc_job_id != NULL)) {
      free(job->qc_job_id);
    }
    if (likely(qc_job_id != NULL)) {
      job->qc_job_id = strdup(qc_job_id);
      if (likely(job->qc_job_id != NULL)) {
        ret = SQC_RESULT_OK;
      } else {
        ret = SQC_RESULT_NO_MEMORY;
      }
    } else {
      job->qc_job_id = NULL;
      ret = SQC_RESULT_OK;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


sqc_result_t
rpc_job_info_set_result(rpc_job_info_t *job, char *result) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(job != NULL)) {
    if (unlikely(job->result != NULL)) {
      free(job->result);
    }
    if (likely(result != NULL)) {
      job->result = strdup(result);
      if (likely(job->result != NULL)) {
        ret = SQC_RESULT_OK;
      } else {
        ret = SQC_RESULT_NO_MEMORY;
      }
    } else {
      job->result = NULL;
      ret = SQC_RESULT_OK;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


sqc_result_t
rpc_job_info_create_array(rpc_job_info_t ***jobs, size_t n_jobs) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t tmp_ret = SQC_RESULT_ANY_FAILURES;

  if (likely(jobs != NULL && n_jobs > 0u)) {
    *jobs = malloc(sizeof(rpc_job_info_t *) * n_jobs);
    if (unlikely(*jobs == NULL)) {
      ret = SQC_RESULT_NO_MEMORY;
      goto error;
    }
    for (size_t i = 0u; i < n_jobs; i++) {
      (*jobs)[i] = NULL;
    }
    for (size_t i = 0u; i < n_jobs; i++) {
      tmp_ret = rpc_job_info_create(&(*jobs)[i]);
      if (unlikely(tmp_ret != SQC_RESULT_OK)) {
        ret = tmp_ret;
        goto error;
      }
    }
    ret = SQC_RESULT_OK;
    
  error:
    if (unlikely(ret != SQC_RESULT_OK)) {
      rpc_job_info_destroy_array(*jobs, n_jobs);
    }
  } else if (likely(jobs != NULL && n_jobs == 0u)) {
    *jobs = NULL;
    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


void
rpc_job_info_destroy_array(rpc_job_info_t **jobs, size_t n_jobs) {
  if (likely(jobs != NULL)) {
    for (size_t i = 0u; i < n_jobs; i++) {
      rpc_job_info_destroy(jobs[i]);
    }
    free(jobs);
  }
}
