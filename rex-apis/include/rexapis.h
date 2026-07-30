#ifndef REXAPIS_H_
#define REXAPIS_H_

#include <stdio.h>
#include <stdint.h>

#define REXAPIS_RESULT_OK (0)
#define REXAPIS_RESULT_ANY_FAILURES (-1)
#define REXAPIS_RESULT_INVALID_ARGS (-2)
#define REXAPIS_RESULT_NO_MEMORY (-3)
#define REXAPIS_RESULT_UNSUPPORTED (-4)
#define REXAPIS_RESULT_INVALID_JSON (-5)
#define REXAPIS_RESULT_RUNTIME_ERROR (-6)

typedef enum {
  REXAPIS_HTTP_STATUS_CODE_UNKNOWN = -1,
  REXAPIS_HTTP_STATUS_CODE_OK = 200,
  REXAPIS_HTTP_STATUS_CODE_CREATED = 201,
  REXAPIS_HTTP_STATUS_CODE_BADREQUEST = 400,
  REXAPIS_HTTP_STATUS_CODE_UNAUTHORIZED = 401,
  REXAPIS_HTTP_STATUS_CODE_NOTFOUND = 404,
  REXAPIS_HTTP_STATUS_CODE_REQUESTTIMEOUT = 408,
  REXAPIS_HTTP_STATUS_CODE_INTERNALSERVERERROR = 500,
} rexapis_http_status_code_t;

typedef enum {
  REXAPIS_QC_TYPE_UNKNOW = 0,
  REXAPIS_QC_TYPE_RQC_REST = 1,
  REXAPIS_QC_TYPE_IBM_REST = 2,
  REXAPIS_QC_TYPE_SLURM_REST = 3,
  REXAPIS_QC_TYPE_QTM_GRPC = 4,
  REXAPIS_QC_TYPE_QTM_SIM_GRPC = 5,
  REXAPIS_QC_TYPE_IBM_DACC = 6,
  REXAPIS_QC_TYPE_A_OQTOPUSREST_SYSTEM_TOKEN = 7,
  REXAPIS_QC_TYPE_A_OQTOPUSREST_USER_TOKEN = 8,
  REXAPIS_QC_TYPE_A_OQTOPUSREST_BOTH_TOKEN = 9,
  REXAPIS_QC_TYPE_NUM = 10,
} rexapis_qc_type_t;

typedef enum {
  REXAPIS_CIRCUIT_FMT_UNKNOW = 0,
  REXAPIS_CIRCUIT_FMT_QASM = 1,
  REXAPIS_CIRCUIT_FMT_QIR = 2,
  REXAPIS_CIRCUIT_FMT_QPY = 3,
  REXAPIS_CIRCUIT_FMT_JSON = 4,
  REXAPIS_CIRCUIT_FMT_NUM = 5,
} rexapis_circuit_fmt_t;

typedef enum {
  REXAPIS_TRANSPILER_UNKNOW = 0,
  REXAPIS_TRANSPILER_NONE = 1,
  REXAPIS_TRANSPILER_PASS = 2,
  REXAPIS_TRANSPILER_NORMAL = 3,
  REXAPIS_TRANSPILER_NUM = 4,
} rexapis_transpiler_t;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// deprecated
int calculate(uint32_t qc_type, const char *base_url, const char *token, const char *qprogram,
              uint32_t circuit_fmt, uint32_t shots, uint32_t transpiler, const char *remark,
              uint32_t polling_interval, uint32_t max_polling_count,
              char **output, size_t *output_len);

int scrape_response(uint32_t qc_type, const char *resp, uint32_t shots, int **patterns,
                    float **probs, size_t *n_patterns);

int submit_job(uint32_t qc_type, const char *base_url, const char *token, const char *qprogram,
               uint32_t circuit_fmt, uint32_t shots, uint32_t transpiler, const char *remark,
               char **qc_job_id, size_t *qc_job_id_len, char **err_msg, size_t *err_msg_len);

int get_job_status(uint32_t qc_type, const char *base_url, const char *token, const char *qc_job_id,
                   char **status, size_t *status_len, char **err_msg, size_t *err_msg_len);

int get_job_result(uint32_t qc_type, const char *base_url, const char *token, const char *qc_job_id,
                   char **result, size_t *result_len, char **err_msg, size_t *err_msg_len);

int cancel_job(uint32_t qc_type, const char *base_url, const char *token, const char *qc_job_id,
               char **err_msg, size_t *err_msg_len);

int delete_job(uint32_t qc_type, const char *base_url, const char *token, const char *qc_job_id,
               char **err_msg, size_t *err_msg_len);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // REXAPIS_H_

