#include "sqc_apis.h"
#include "dbmgr.h"
#include "rpc.pb-c.h"
#include "rpc_auth_method.h"
#include "rpc_msg_id.h"
#include "rpc_msg_util.h"


//
// Return a pointer to a string that denotes an RPC message ID.
//
static inline const char *
s_message_id_string(rpc_msg_id_t id) {
  const char* ret = NULL;

  static const char *id_names[] = {
    "OPEN_NEW_SESSION_REQUEST",     //  0
    "OPEN_NEW_SESSION_REPLY",       //  1
    "SUBMIT_JOB_REQUEST",           //  2
    "SUBMIT_JOB_REPLY",             //  3
    "JOB_STATUS_REQUEST",           //  4
    "JOB_STATUS_REPLY",             //  5
    "AUTH_REQUEST",                 //  6
    "AUTH_REPLY",                   //  7
    "CANCEL_JOB_REQUEST",           //  8
    "CANCEL_JOB_REPLY",             //  9
    "DELETE_JOB_REQUEST",           // 10
    "DELETE_JOB_REPLY",             // 11
    "JOB_LIST_REQUEST",             // 12
    "JOB_LIST_REPLY"  ,             // 13
    "ADM_DEL_JOBS_REQUEST",         // 14
    "ADM_DEL_JOBS_REPLY",           // 15
    "ADM_ADD_USER_REQUEST",         // 16
    "ADM_ADD_USER_REPLY",           // 17
    "ADM_SET_USER_STATUS_REQUEST",  // 18
    "ADM_SET_USER_STATUS_REPLY",    // 19
  };
  static const char *unknown = "unknown";

  if (id < sizeof(id_names) / sizeof(char *)) {
    ret = id_names[id];
  } else {
    ret = unknown;
  }

  return ret;
}


//
// Return a pointer to a string that denotes an RPC message name of the given message ID.
//
static inline const char *
s_message_name_string(rpc_msg_id_t id) {
  const char* ret = NULL;

  static const char *id_names[] = {
    "OPEN_NEW_SESSION",     //  0
    "OPEN_NEW_SESSION",     //  1
    "SUBMIT_JOB",           //  2
    "SUBMIT_JOB",           //  3
    "JOB_STATUS",           //  4
    "JOB_STATUS",           //  5
    "AUTH",                 //  6
    "AUTH",                 //  7
    "CANCEL_JOB",           //  8
    "CANCEL_JOB",           //  9
    "DELETE_JOB",           // 10
    "DELETE_JOB",           // 11
    "JOB_LIST",             // 12
    "JOB_LIST"  ,           // 13
    "ADM_DEL_JOBS",         // 14
    "ADM_DEL_JOBS",         // 15
    "ADM_ADD_USER",         // 16
    "ADM_ADD_USER",         // 17
    "ADM_SET_USER_STATUS",  // 18
    "ADM_SET_USER_STATUS",  // 19
  };
  static const char *unknown = "unknown";

  if (id < sizeof(id_names) / sizeof(char *)) {
    ret = id_names[id];
  } else {
    ret = unknown;
  }

  return ret;
}


//
// Serialize a 'basic_reply' message using protobuf-c.
//
static inline sqc_result_t
s_pack_basic_reply(sqc_result_t code, const char *msg, char **body, size_t *len) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char empty_msg = '\0';

  if (likely(body != NULL && len != NULL)) {
    BasicReply reply = BASIC_REPLY__INIT;

    reply.code = (int64_t) code;
    if (msg == NULL) {
      reply.message.data = (uint8_t *) &empty_msg;
      reply.message.len = 1u;
    } else {
      reply.message.data = (uint8_t *) msg;
      reply.message.len = strlen(msg) + 1u;
    }
    *len = basic_reply__get_packed_size(&reply);
    *body = malloc(*len);

    if (likely(*body != NULL)) {
      (void) basic_reply__pack(&reply, (uint8_t *) *body);
      ret = SQC_RESULT_OK;
      sqc_msg_debug(5, "Built a basic_reply message\n");
    } else {
      ret = SQC_RESULT_NO_MEMORY;
      sqc_msg_error("Failed to build a basic_reply message, %s\n",
                    sqc_error_get_string(ret));
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


//
// Deserialize a 'basic_reply' message using protobuf-c.
//
static inline sqc_result_t
s_unpack_basic_reply(char *body, size_t len, sqc_result_t *code, char **msg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(body != NULL && code != NULL && msg != NULL)) {
    BasicReply *reply = basic_reply__unpack(NULL, len, (uint8_t *) body);

    if (likely(reply != NULL && RPC_VALIDATE_PROTOC_BYTES(&reply->message) == true)) {
      *code = (sqc_result_t) reply->code;
      *msg = strdup((char *) reply->message.data);

      if (likely(*msg != NULL)) {
        ret = SQC_RESULT_OK;
      } else {
        ret = SQC_RESULT_NO_MEMORY;
      }

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
    basic_reply__free_unpacked(reply, NULL);

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


//
// Serialize a 'open_new_session_request' message using protobuf-c.
//
static inline sqc_result_t
s_pack_open_new_session_request(rpc_auth_method_t auth_method, char **body, size_t *len) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(body != NULL && len != NULL)) {
    OpenNewSessionRequest request = OPEN_NEW_SESSION_REQUEST__INIT;
    request.auth_method = (AuthMethodT) auth_method;
    *len = open_new_session_request__get_packed_size(&request);
    *body = malloc(*len);

    if (likely(*body != NULL)) {
      (void) open_new_session_request__pack(&request, (uint8_t *) *body);
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_NO_MEMORY;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


//
// Serialize a 'submit_job_request' message using protobuf-c.
//
static inline sqc_result_t
s_pack_submit_job_request(uint8_t priority, const char *qprogram, sqc_rpc_sched_circuit_fmt_t circuit_fmt,
                          size_t shots, sqc_rpc_sched_qc_type_t qc_type,
                          sqc_rpc_sched_transpiler_t transpiler,
                          const char *remark, const char *user_token,
                          char **body, size_t *len) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(qprogram != NULL && remark != NULL && body != NULL && len != NULL)) {
    SubmitJobRequest request = SUBMIT_JOB_REQUEST__INIT;

    request.priority = (uint32_t) priority;
    request.qprogram.data = (uint8_t *) qprogram;
    request.qprogram.len = strlen(qprogram) + 1u;
    request.circuit_fmt = (int) circuit_fmt;
    request.shots = (uint32_t) shots;
    request.qc_type = (int) qc_type;
    request.transpiler = (int) transpiler;
    request.remark.data = (uint8_t *) remark;
    request.remark.len = strlen(remark) + 1u;

    if (user_token != NULL) {
      request.has_user_token = true;
      request.user_token.data = (uint8_t *) user_token;
      request.user_token.len = strlen(user_token) + 1u;
    }

    *len = submit_job_request__get_packed_size(&request);
    *body = malloc(*len);

    if (likely(*body != NULL)) {
      (void) submit_job_request__pack(&request, (uint8_t *) *body);
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_NO_MEMORY;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


//
// Serialize a 'submit_job_reply' message using protobuf-c.
//
static inline sqc_result_t
s_pack_submit_job_reply(sqc_result_t code, const char *msg, const char *job_id,
                        char **body, size_t *len) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char empty_msg = '\0';

  if (likely(body != NULL && len != NULL)) {
    SubmitJobReply reply = SUBMIT_JOB_REPLY__INIT;

    reply.code = (int64_t) code;
    if (msg == NULL) {
      reply.message.data = (uint8_t *) &empty_msg;
      reply.message.len = 1u;
    } else {
      reply.message.data = (uint8_t *) msg;
      reply.message.len = strlen(msg) + 1u;
    }
    if (job_id == NULL) {
      reply.has_job_id = false;
    } else {
      reply.has_job_id = true;
      reply.job_id.data = (uint8_t *) job_id;
      reply.job_id.len = strlen(job_id) + 1u;
    }
    *len = submit_job_reply__get_packed_size(&reply);
    *body = malloc(*len);

    if (likely(*body != NULL)) {
      (void) submit_job_reply__pack(&reply, (uint8_t *) *body);
      ret = SQC_RESULT_OK;
      sqc_msg_debug(5, "Built a submit_job_reply message\n");
    } else {
      ret = SQC_RESULT_NO_MEMORY;
      sqc_msg_error("Failed to build a submit_job_reply message, %s\n",
                    sqc_error_get_string(ret));
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


//
// Deserialize a 'submit_job_reply' message using protobuf-c.
//
static inline sqc_result_t
s_unpack_submit_job_reply(char *body, size_t len, sqc_result_t *code, char **msg,
                          char **job_id) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(body != NULL && code != NULL && msg != NULL && job_id != NULL)) {
    SubmitJobReply *reply = submit_job_reply__unpack(NULL, len, (uint8_t *) body);

    if (likely(reply != NULL &&
               RPC_VALIDATE_PROTOC_BYTES(&reply->message) == true &&
               (!reply->has_job_id || RPC_VALIDATE_PROTOC_BYTES(&reply->job_id) == true))) {
      *code = (sqc_result_t) reply->code;
      *msg = strdup((char *) reply->message.data);
      if (reply->has_job_id) {
        *job_id = strdup((char *) reply->job_id.data);
      } else {
        *job_id = strdup("");
      }

      if (likely(*msg != NULL && *job_id != NULL)) {
        ret = SQC_RESULT_OK;
      } else {
        free(*msg);
        free(*job_id);
        *msg = NULL;
        *job_id = NULL;
        ret = SQC_RESULT_NO_MEMORY;
      }

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
    submit_job_reply__free_unpacked(reply, NULL);

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


//
// Serialize a 'job_status_request' message using protobuf-c.
//
static inline sqc_result_t
s_pack_job_status_request(const char *job_id, char **body, size_t *len) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(job_id != NULL && body != NULL && len != NULL)) {
    JobStatusRequest request = JOB_STATUS_REQUEST__INIT;
    request.job_id.data = (uint8_t *) job_id;
    request.job_id.len = strlen(job_id) + 1u;
    *len = job_status_request__get_packed_size(&request);
    *body = malloc(*len);

    if (likely(*body != NULL)) {
      (void) job_status_request__pack(&request, (uint8_t *) *body);
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_NO_MEMORY;
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


//
// Serialize a 'job_status_reply' message using protobuf-c.
//
static inline sqc_result_t
s_pack_job_status_reply(sqc_result_t code, const char *msg,
                        sqc_rpc_sched_job_status_t status, const char *qc_job_id, const char *result,
                        char **body, size_t *len) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  JobStatusReply reply = JOB_STATUS_REPLY__INIT;
  char empty_msg = '\0';

  if (likely(body != NULL && len != NULL)) {
    reply.code = (int64_t) code;
    if (msg == NULL) {
      reply.message.data = (uint8_t *) &empty_msg;
      reply.message.len = 1u;
    } else {
      reply.message.data = (uint8_t *) msg;
      reply.message.len = strlen(msg) + 1u;
    }
    reply.status = (int) status;

    if (qc_job_id == NULL) {
      reply.has_qc_job_id = false;
    } else {
      reply.has_qc_job_id = true;
      reply.qc_job_id.data = (uint8_t *) qc_job_id;
      reply.qc_job_id.len = strlen(qc_job_id) + 1u;
    }

    if (result == NULL) {
      reply.has_result = false;
    } else {
      reply.has_result = true;
      reply.result.data = (uint8_t *) result;
      reply.result.len = strlen(result) + 1u;
    }
    *len = job_status_reply__get_packed_size(&reply);
    *body = malloc(*len);

    if (likely(*body != NULL)) {
      (void) job_status_reply__pack(&reply, (uint8_t *) *body);
      ret = SQC_RESULT_OK;
      sqc_msg_debug(5, "Built a job_status_reply message\n");
    } else {
      ret = SQC_RESULT_NO_MEMORY;
      sqc_msg_error("Failed to build a job_status_reply message, %s\n",
                    sqc_error_get_string(ret));
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


//
// Deserialize a 'job_status_reply' message using protobuf-c.
//
static inline sqc_result_t
s_unpack_job_status_reply(char *body, size_t len, sqc_result_t *code, char **msg,
                          sqc_rpc_sched_job_status_t *status, char **qc_job_id, char **result) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(body != NULL && code != NULL && msg != NULL && qc_job_id != NULL && result != NULL)) {
    JobStatusReply *reply = job_status_reply__unpack(NULL, len, (uint8_t *) body);

    if (likely(reply != NULL &&
               RPC_VALIDATE_PROTOC_BYTES(&reply->message) == true &&
               (!reply->has_qc_job_id || RPC_VALIDATE_PROTOC_BYTES(&reply->qc_job_id) == true) &&
               (!reply->has_result || RPC_VALIDATE_PROTOC_BYTES(&reply->result) == true))) {
      *code = (sqc_result_t) reply->code;
      *msg = strdup((char *) reply->message.data);
      *status = (sqc_rpc_sched_job_status_t) reply->status;
      if (reply->has_qc_job_id) {
        *qc_job_id = strdup((char *) reply->qc_job_id.data);
      } else {
        *qc_job_id = strdup("");
      }
      if (reply->has_result) {
        *result = strdup((char *) reply->result.data);
      } else {
        *result = strdup("");
      }

      if (likely(*msg != NULL && *qc_job_id != NULL && *result != NULL)) {
        ret = SQC_RESULT_OK;
      } else {
        free(*msg);
        free(*qc_job_id);
        free(*result);
        *msg = NULL;
        *result = NULL;
        ret = SQC_RESULT_NO_MEMORY;
      }

    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }
    job_status_reply__free_unpacked(reply, NULL);

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


//
// Serialize a 'auth_request' message using protobuf-c.
//
static inline sqc_result_t
s_pack_auth_request(const uint8_t *data, size_t datalen, char **body, size_t *len) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(body != NULL && len != NULL)) {
    AuthRequest request = AUTH_REQUEST__INIT;

    if (data != NULL) {
      request.data.data = (uint8_t *) data;
      request.data.len = datalen;
    } else {
      static const char* empty_data = "";
      request.data.data = (uint8_t *) empty_data;
      request.data.len = 0;
    }
    *len = auth_request__get_packed_size(&request);
    *body = malloc(*len);

    if (likely(*body != NULL)) {
      (void) auth_request__pack(&request, (uint8_t *) *body);
      ret = SQC_RESULT_OK;
      sqc_msg_debug(5, "Built a auth_request message\n");
    } else {
      ret = SQC_RESULT_NO_MEMORY;
      sqc_msg_error("Failed to build a auth_request message, %s\n",
                    sqc_error_get_string(ret));
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


//
// Serialize a 'cancel_job_request' message using protobuf-c.
//
static inline sqc_result_t
s_pack_cancel_job_request(const char *job_id, char **body, size_t *len) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(job_id != NULL && body != NULL && len != NULL)) {
    CancelJobRequest request = CANCEL_JOB_REQUEST__INIT;
    request.job_id.data = (uint8_t *) job_id;
    request.job_id.len = strlen(job_id) + 1u;
    *len = cancel_job_request__get_packed_size(&request);
    *body = malloc(*len);

    if (likely(*body != NULL)) {
      (void) cancel_job_request__pack(&request, (uint8_t *) *body);
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_NO_MEMORY;
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


//
// Serialize a 'delete_job_request' message using protobuf-c.
//
static inline sqc_result_t
s_pack_delete_job_request(const char *job_id, char **body, size_t *len) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(job_id != NULL && body != NULL && len != NULL)) {
    DeleteJobRequest request = DELETE_JOB_REQUEST__INIT;
    request.job_id.data = (uint8_t *) job_id;
    request.job_id.len = strlen(job_id) + 1u;
    *len = delete_job_request__get_packed_size(&request);
    *body = malloc(*len);

    if (likely(*body != NULL)) {
      (void) delete_job_request__pack(&request, (uint8_t *) *body);
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_NO_MEMORY;
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


//
// Serialize a 'job_list_reply' message using protobuf-c.
//
static inline sqc_result_t
s_pack_job_list_reply(sqc_result_t code, const char *msg,
                      const dbmgr_job_info_t *jobs, size_t n_jobs,
                      char **body, size_t *len) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  JobListReply reply = JOB_LIST_REPLY__INIT;
  char empty_msg = '\0';

  if (likely((jobs != NULL || n_jobs == 0) && body != NULL && len != NULL)) {
    reply.code = (int64_t) code;
    if (msg == NULL) {
      reply.message.data = (uint8_t *) &empty_msg;
      reply.message.len = 1u;
    } else {
      reply.message.data = (uint8_t *) msg;
      reply.message.len = strlen(msg) + 1u;
    }
    reply.n_jobs = n_jobs;

    reply.jobs = malloc(sizeof(JobInfo *) * n_jobs);
    if (unlikely(reply.jobs == NULL)) {
      ret = SQC_RESULT_NO_MEMORY;
      goto error;
    }
    for (size_t i = 0u; i < n_jobs; i++) {
      reply.jobs[i] = NULL;
    }
    for (size_t i = 0u; i < n_jobs; i++) {
      sqc_result_t get_result = SQC_RESULT_ANY_FAILURES;
      sqc_rpc_sched_job_status_t status;

      reply.jobs[i] = malloc(sizeof(JobInfo));
      if (unlikely(reply.jobs[i] == NULL)) {
        ret = SQC_RESULT_NO_MEMORY;
        goto error;
      }
      job_info__init(reply.jobs[i]);

      get_result = dbmgr_ji_get_job_id(jobs[i], (char **) &reply.jobs[i]->job_id.data);
      if (unlikely(get_result != SQC_RESULT_OK)) {
        ret = get_result;
        goto error;
      }
      reply.jobs[i]->job_id.len = strlen((char *) reply.jobs[i]->job_id.data) + 1u;

      get_result = dbmgr_ji_get_status(jobs[i], &status);
      if (unlikely(get_result != SQC_RESULT_OK)) {
        ret = get_result;
        goto error;
      }
      reply.jobs[i]->status = (int) status;

      get_result = dbmgr_ji_get_qc_job_id(jobs[i], (char **) &reply.jobs[i]->qc_job_id.data);
      if (unlikely(get_result != SQC_RESULT_OK)) {
        ret = get_result;
        goto error;
      }
      reply.jobs[i]->qc_job_id.len = strlen((char *) reply.jobs[i]->qc_job_id.data) + 1u;
      reply.jobs[i]->has_qc_job_id = (reply.jobs[i]->qc_job_id.len > 1u) ? 1 : 0;

 #ifdef JOB_LIST_WITH_RESULT
      get_result = dbmgr_ji_get_result(jobs[i], (char **) &reply.jobs[i]->result.data);
      if (unlikely(get_result != SQC_RESULT_OK)) {
        ret = get_result;
        goto error;
      }
      reply.jobs[i]->result.len = strlen((char *) reply.jobs[i]->result.data) + 1u;
      reply.jobs[i]->has_result = (reply.jobs[i]->result.len > 1u) ? 1 : 0;
#else
      reply.jobs[i]->has_result = 0;
#endif
    }

    *len = job_list_reply__get_packed_size(&reply);
    *body = malloc(*len);
    if (unlikely(*body == NULL)) {
      ret = SQC_RESULT_NO_MEMORY;
      goto error;
    }
    (void) job_list_reply__pack(&reply, (uint8_t *) *body);
    ret = SQC_RESULT_OK;

  error:
    if (unlikely(ret != SQC_RESULT_OK)) {
      if (likely(reply.jobs != NULL)) {
        for (size_t i = 0u; i < n_jobs; i++) {
          free(reply.jobs[i]->job_id.data);
          free(reply.jobs[i]->qc_job_id.data);
          free(reply.jobs[i]->result.data);
          free(reply.jobs[i]);
        }
        free(reply.jobs);
      }
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


//
// Convert a 'JobInfo' to 'rpc_job_info_t' object.
//
static inline sqc_result_t
s_convert_job_info(const JobInfo *src, rpc_job_info_t *dst) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t tmp_ret = SQC_RESULT_ANY_FAILURES;

  if (likely(src != NULL &&
             RPC_VALIDATE_PROTOC_BYTES(&src->job_id) &&
             (src->has_qc_job_id == false || RPC_VALIDATE_PROTOC_BYTES(&src->qc_job_id)) &&
             (src->has_result == false || RPC_VALIDATE_PROTOC_BYTES(&src->result)) &&
             dst != NULL)) {
    tmp_ret = rpc_job_info_set_job_id(dst, (char *) src->job_id.data);
    if (unlikely(tmp_ret != SQC_RESULT_OK)) {
      ret = SQC_RESULT_NO_MEMORY;
      goto error;
    }

    dst->status = (sqc_rpc_sched_job_status_t) src->status;

    tmp_ret = rpc_job_info_set_qc_job_id(dst, (char *) src->qc_job_id.data);
    if (unlikely(tmp_ret != SQC_RESULT_OK)) {
      ret = SQC_RESULT_NO_MEMORY;
      goto error;
    }

    tmp_ret = rpc_job_info_set_result(dst, (char *) src->result.data);
    if (unlikely(tmp_ret != SQC_RESULT_OK)) {
      ret = SQC_RESULT_NO_MEMORY;
      goto error;
    }

    ret = SQC_RESULT_OK;

  error:
    if (unlikely(ret != SQC_RESULT_OK)) {
      rpc_job_info_set_job_id(dst, NULL);
      rpc_job_info_set_qc_job_id(dst, NULL);
      rpc_job_info_set_result(dst, NULL);
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


//
// Deserialize a 'job_list_reply' message using protobuf-c.
//
static inline sqc_result_t
s_unpack_job_list_reply(char *body, size_t len, sqc_result_t *code, char **msg,
                        rpc_job_info_t ***jobs, size_t *n_jobs) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t tmp_ret = SQC_RESULT_ANY_FAILURES;

  if (likely(body != NULL && code != NULL && msg != NULL && jobs != NULL && n_jobs != NULL)) {
    JobListReply *reply = job_list_reply__unpack(NULL, len, (uint8_t *) body);
    *msg = NULL;
    *jobs = NULL;

    if (likely(reply != NULL && RPC_VALIDATE_PROTOC_BYTES(&reply->message) == true)) {
      *code = (sqc_result_t) reply->code;
      *msg = strdup((char *) reply->message.data);
      if (unlikely(msg == NULL)) {
        ret = SQC_RESULT_NO_MEMORY;
        sqc_msg_error("Failed to build a job_list_reply message, %s\n",
                      sqc_error_get_string(ret));
        goto error;
      }

      *n_jobs = reply->n_jobs;
      tmp_ret = rpc_job_info_create_array(jobs, reply->n_jobs);
      if (unlikely(tmp_ret != SQC_RESULT_OK)) {
        ret = tmp_ret;
        goto error;
      }

      for (size_t i = 0u; i < reply->n_jobs; i++) {
        tmp_ret = s_convert_job_info(reply->jobs[i], (*jobs)[i]);
        if (unlikely(tmp_ret != SQC_RESULT_OK)) {
          ret = tmp_ret;
          goto error;
        }
      }
      ret = SQC_RESULT_OK;

  error:
      if (unlikely(ret != SQC_RESULT_OK)) {
        free(*msg);
        rpc_job_info_destroy_array(*jobs, reply->n_jobs);
        *msg = NULL;
        *jobs = NULL;
      }
    } else {
      ret = SQC_RESULT_INVALID_OBJECT;
    }

    job_list_reply__free_unpacked(reply, NULL);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}

//
// Free an array of JobInfo objects.
//
static inline void
s_free_job_list(JobInfo **jobs, size_t n_jobs) {
  if (likely(jobs != NULL)) {
    for (size_t i = 0u; i < n_jobs; i++) {
      free(jobs[i]);
    }
    free(jobs);
  }
}


//
// Serialize a 'adm_del_jobs_request' message using protobuf-c.
//
static inline sqc_result_t
s_pack_adm_del_jobs_request(const char *user_id, sqc_chrono_t from_time, sqc_chrono_t to_time,
                            char **body, size_t *len) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(user_id != NULL && body != NULL && len != NULL)) {
    AdmDelJobsRequest request = ADM_DEL_JOBS_REQUEST__INIT;
    request.user_id.data = (uint8_t *) user_id;
    request.user_id.len = strlen(user_id) + 1u;
    request.from_time = (int64_t) from_time;
    request.to_time = (int64_t) to_time;
    *len = adm_del_jobs_request__get_packed_size(&request);
    *body = malloc(*len);

    if (likely(*body != NULL)) {
      (void) adm_del_jobs_request__pack(&request, (uint8_t *) *body);
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_NO_MEMORY;
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


//
// Serialize a 'adm_add_user_request' message using protobuf-c.
//
static inline sqc_result_t
s_pack_adm_add_user_request(const char *user_id, char **body, size_t *len) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(user_id != NULL && body != NULL && len != NULL)) {
    AdmAddUserRequest request = ADM_ADD_USER_REQUEST__INIT;
    request.user_id.data = (uint8_t *) user_id;
    request.user_id.len = strlen(user_id) + 1u;
    *len = adm_add_user_request__get_packed_size(&request);
    *body = malloc(*len);

    if (likely(*body != NULL)) {
      (void) adm_add_user_request__pack(&request, (uint8_t *) *body);
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_NO_MEMORY;
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


//
// Serialize a 'adm_add_user_request' message using protobuf-c.
//
static inline sqc_result_t
s_pack_adm_set_user_status_request(const char *user_id, bool enabled, char **body, size_t *len) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(user_id != NULL && body != NULL && len != NULL)) {
    AdmSetUserStatusRequest request = ADM_SET_USER_STATUS_REQUEST__INIT;
    request.user_id.data = (uint8_t *) user_id;
    request.user_id.len = strlen(user_id) + 1u;
    request.enabled = enabled;
    *len = adm_set_user_status_request__get_packed_size(&request);
    *body = malloc(*len);

    if (likely(*body != NULL)) {
      (void) adm_set_user_status_request__pack(&request, (uint8_t *) *body);
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_NO_MEMORY;
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


/*
 * Exported APIs
 */

const char *
rpc_message_id_string(rpc_msg_id_t id) {
  return s_message_id_string(id);
}


const char *
rpc_message_name_string(rpc_msg_id_t id) {
  return s_message_name_string(id);
}


void
rpc_create_message_text(char **text, const char *format, ...) {
  va_list args;
  va_start(args, format);
  if (text != NULL) {
    if (vasprintf(text, format, args) == -1) {
      *text = NULL;
    }
  }
  va_end(args);
}


sqc_result_t
rpc_pack_open_new_session_request(rpc_auth_method_t auth_method, char **body, size_t *len) {
  return s_pack_open_new_session_request(auth_method, body, len);
}


sqc_result_t
rpc_pack_open_new_session_reply(sqc_result_t code, const char *msg, char **body, size_t *len) {
  return s_pack_basic_reply(code, msg, body, len);
}


sqc_result_t
rpc_unpack_open_new_session_reply(char *body, size_t len, sqc_result_t *code, char **msg) {
  return s_unpack_basic_reply(body, len, code, msg);
}


sqc_result_t
rpc_pack_submit_job_request(uint8_t priority, const char *qprogram, sqc_rpc_sched_circuit_fmt_t circuit_fmt,
                            size_t shots, sqc_rpc_sched_qc_type_t qc_type, sqc_rpc_sched_transpiler_t transpiler,
                            const char *remark, const char *user_token, char **body, size_t *len) {
  return s_pack_submit_job_request(priority, qprogram, circuit_fmt, shots, qc_type, transpiler,
                                   remark, user_token, body, len);
}


sqc_result_t
rpc_pack_submit_job_reply(sqc_result_t code, const char *msg, const char *job_id, char **body,
                          size_t *len) {
  return s_pack_submit_job_reply(code, msg, job_id, body, len);
}


sqc_result_t
rpc_unpack_submit_job_reply(char *body, size_t len, sqc_result_t *code, char **msg,
                            char **job_id) {
  return s_unpack_submit_job_reply(body, len, code, msg, job_id);
}


sqc_result_t
rpc_pack_job_status_request(const char *job_id, char **body, size_t *len) {
  return s_pack_job_status_request(job_id, body, len);
}


sqc_result_t
rpc_pack_job_status_reply(sqc_result_t code, const char *msg, sqc_rpc_sched_job_status_t status,
                          const char *qc_job_id, const char *result, char **body, size_t *len) {
  return s_pack_job_status_reply(code, msg, status, qc_job_id, result, body, len);
}


sqc_result_t
rpc_unpack_job_status_reply(char *body, size_t len, sqc_result_t *code, char **msg,
                            sqc_rpc_sched_job_status_t *status, char **qc_job_id, char **result) {
  return s_unpack_job_status_reply(body, len, code, msg, status, qc_job_id, result);
}


sqc_result_t
rpc_pack_auth_request(const uint8_t *data, size_t datalen, char **body, size_t *len) {
  return s_pack_auth_request(data, datalen, body, len);
}


sqc_result_t
rpc_pack_auth_reply(sqc_result_t code, const char *msg, char **body, size_t *len) {
  return s_pack_basic_reply(code, msg, body, len);
}


sqc_result_t
rpc_unpack_auth_reply(char *body, size_t len, sqc_result_t *code, char **msg) {
  return s_unpack_basic_reply(body, len, code, msg);
}


sqc_result_t
rpc_pack_cancel_job_request(const char *job_id, char **body, size_t *len) {
  return s_pack_cancel_job_request(job_id, body, len);
}


sqc_result_t
rpc_pack_cancel_job_reply(sqc_result_t code, const char *msg, char **body, size_t *len) {
  return s_pack_basic_reply(code, msg, body, len);
}


sqc_result_t
rpc_unpack_cancel_job_reply(char *body, size_t len, sqc_result_t *code, char **msg) {
  return s_unpack_basic_reply(body, len, code, msg);
}


sqc_result_t
rpc_pack_delete_job_request(const char *job_id, char **body, size_t *len) {
  return s_pack_delete_job_request(job_id, body, len);
}


sqc_result_t
rpc_pack_delete_job_reply(sqc_result_t code, const char *msg, char **body, size_t *len) {
  return s_pack_basic_reply(code, msg, body, len);
}


sqc_result_t
rpc_unpack_delete_job_reply(char *body, size_t len, sqc_result_t *code, char **msg) {
  return s_unpack_basic_reply(body, len, code, msg);
}


sqc_result_t
rpc_pack_job_list_reply(sqc_result_t code, const char *msg,
                        const dbmgr_job_info_t *jobs, size_t n_jobs,
                        char **body, size_t *len) {
  return s_pack_job_list_reply(code, msg, jobs, n_jobs, body, len);
}


sqc_result_t
rpc_unpack_job_list_reply(char *body, size_t len, sqc_result_t *code, char **msg,
                          rpc_job_info_t ***jobs, size_t *n_jobs) {
  return s_unpack_job_list_reply(body, len, code, msg, jobs, n_jobs);
}


sqc_result_t
rpc_pack_adm_del_jobs_request(const char *user_id, sqc_chrono_t from_time, sqc_chrono_t to_time,
                              char **body, size_t *len) {
  return s_pack_adm_del_jobs_request(user_id, from_time, to_time, body, len);
}


sqc_result_t
rpc_pack_adm_del_jobs_reply(sqc_result_t code, const char *msg, char **body, size_t *len) {
  return s_pack_basic_reply(code, msg, body, len);
}


sqc_result_t
rpc_unpack_adm_del_jobs_reply(char *body, size_t len, sqc_result_t *code, char **msg) {
  return s_unpack_basic_reply(body, len, code, msg);
}


sqc_result_t
rpc_pack_adm_add_user_request(const char *user_id, char **body, size_t *len) {
  return s_pack_adm_add_user_request(user_id, body, len);
}


sqc_result_t
rpc_pack_adm_add_user_reply(sqc_result_t code, const char *msg, char **body, size_t *len) {
  return s_pack_basic_reply(code, msg, body, len);
}


sqc_result_t
rpc_unpack_adm_add_user_reply(char *body, size_t len, sqc_result_t *code, char **msg) {
  return s_unpack_basic_reply(body, len, code, msg);
}


sqc_result_t
rpc_pack_adm_set_user_status_request(const char *user_id, bool enabled, char **body, size_t *len) {
  return s_pack_adm_set_user_status_request(user_id, enabled, body, len);
}


sqc_result_t
rpc_pack_adm_set_user_status_reply(sqc_result_t code, const char *msg, char **body, size_t *len) {
  return s_pack_basic_reply(code, msg, body, len);
}


sqc_result_t
rpc_unpack_adm_set_user_status_reply(char *body, size_t len, sqc_result_t *code, char **msg) {
  return s_unpack_basic_reply(body, len, code, msg);
}
