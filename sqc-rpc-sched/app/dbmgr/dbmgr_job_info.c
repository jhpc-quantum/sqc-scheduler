#include <sys/time.h>
#include <uuid/uuid.h>

#include "sqc_apis.h"
#include "sqc_rpc_sched_conf.h"
#include "sqc_rpc_sched_conv_enums.h"

#include "dbmgr_db.h"
#include "dbmgr_job.h"
#include "dbmgr_types_internal.h"

static sqc_hashmap_t job_info_hashmap = NULL;

/*
 * job_info methods
 */


static inline void
s_dbmgr_job_info_record_rlock(dbmgr_job_info_t ji_ptr) {
  if (likely(ji_ptr != NULL)) {
    (void)sqc_rwlock_reader_lock(&(ji_ptr->rwlck_));
  }
}


static inline void
s_dbmgr_job_info_record_wlock(dbmgr_job_info_t ji_ptr) {
  if (likely(ji_ptr != NULL)) {
    (void)sqc_rwlock_writer_lock(&(ji_ptr->rwlck_));
  }
}


static inline void
s_dbmgr_job_info_record_unlock(dbmgr_job_info_t ji_ptr) {
  if (likely(ji_ptr != NULL)) {
    (void)sqc_rwlock_unlock(&(ji_ptr->rwlck_));
  }
}


static inline sqc_result_t
s_dbmgr_job_info_record_create(const char *user_id, const uint8_t priority,
                               const char *qprogram, sqc_rpc_sched_circuit_fmt_t circuit_fmt, size_t shots,
                               sqc_rpc_sched_qc_type_t qc_type, sqc_rpc_sched_transpiler_t transpiler,
                               const char *remark, const char *user_token, dbmgr_job_info_t *ji_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  uuid_t job_id;
  size_t user_id_len, qprogram_len, remark_len, user_token_len;

  if (likely(ji_ptr != NULL &&
             user_id != NULL && IS_VALID_STRING(user_id) == true &&
             priority <= SQC_RPC_SCHED_MAX_PRIORITY &&
             qprogram != NULL && IS_VALID_STRING(qprogram) == true &&
             remark != NULL && IS_VALID_STRING(remark) == true)) {
    user_id_len = strlen(user_id);
    if (user_id_len > (SQC_RPC_SCHED_USER_ID_MAX_SIZE)) {
      return SQC_RESULT_TOO_LONG;
    }

    qprogram_len = strlen(qprogram);
    if (qprogram_len > (SQC_RPC_SCHED_QPROGRAM_MAX_SIZE)) {
      return SQC_RESULT_INVALID_ARGS;
    }

    remark_len = strlen(remark);
    if (remark_len > (SQC_RPC_SCHED_REMARK_MAX_SIZE)) {
      return SQC_RESULT_INVALID_ARGS;
    }

    if (user_token != NULL && IS_VALID_STRING(user_token) == true) {
      user_token_len = strlen(user_token);
      if (user_token_len > (SQC_RPC_SCHED_USER_TOKEN_MAX_SIZE)) {
        return SQC_RESULT_INVALID_ARGS;
      }
    } else {
      user_token_len = 0;
    }

    *ji_ptr = (dbmgr_job_info_t) malloc(sizeof(dbmgr_job_info_record));
    if (*ji_ptr != NULL) {
      (*ji_ptr)->rwlck_ = NULL;
      if ((rc = sqc_rwlock_create(&((*ji_ptr)->rwlck_))) != SQC_RESULT_OK) {
        goto error;
      }

      // create job_id
      uuid_generate(job_id);
      uuid_unparse(job_id, (*ji_ptr)->job_id);
      (*ji_ptr)->job_id[SQC_RPC_SCHED_JOB_ID_MAX_SIZE] = '\0';
      (*ji_ptr)->job_id_len = SQC_RPC_SCHED_JOB_ID_MAX_SIZE;

      memcpy((*ji_ptr)->user_id, user_id, user_id_len);
      (*ji_ptr)->user_id[user_id_len] = '\0';
      (*ji_ptr)->user_id_len = user_id_len;

      (*ji_ptr)->priority = priority;

      (*ji_ptr)->status = SQC_RPC_SCHED_JOB_STATUS_CREATED;

      (*ji_ptr)->qc_job_id[0] = '\0';
      (*ji_ptr)->qc_job_id_len = 0;

      memcpy((*ji_ptr)->qprogram, qprogram, qprogram_len);
      (*ji_ptr)->qprogram[qprogram_len] = '\0';
      (*ji_ptr)->qprogram_len = qprogram_len;

      (*ji_ptr)->circuit_fmt = circuit_fmt;

      (*ji_ptr)->shots = shots;

      (*ji_ptr)->qc_type = qc_type;

      (*ji_ptr)->transpiler = transpiler;

      memcpy((*ji_ptr)->remark, remark, remark_len);
      (*ji_ptr)->remark[remark_len] = '\0';
      (*ji_ptr)->remark_len = remark_len;

      (*ji_ptr)->result[0] = '\0';
      (*ji_ptr)->result_len = 0;

      if (user_token_len > 0) {
        memcpy((*ji_ptr)->user_token, user_token, user_token_len);
        (*ji_ptr)->user_token[user_token_len] = '\0';
      } else {
        (*ji_ptr)->user_token[0] = '\0';
      }
      (*ji_ptr)->user_token_len = user_token_len;

      (*ji_ptr)->created_time = sqc_chrono_now();
      (*ji_ptr)->queued_time = 0;
      (*ji_ptr)->running_time = 0;
      (*ji_ptr)->done_time = 0;
      (*ji_ptr)->cancelled_time = 0;
      (*ji_ptr)->error_time = 0;
      (*ji_ptr)->deleted_time = 0;
      (*ji_ptr)->update_time = (*ji_ptr)->created_time;

      return SQC_RESULT_OK;
    } else {
      rc = SQC_RESULT_NO_MEMORY;
      goto error;
    }
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }

error:
  if (*ji_ptr != NULL) {
    if ((*ji_ptr)->rwlck_ != NULL) {
      sqc_rwlock_destroy(&(*ji_ptr)->rwlck_);
    }
    free((void *) *ji_ptr);
    *ji_ptr = NULL;
  }
  return rc;
}


static inline void
s_dbmgr_job_info_record_destroy(dbmgr_job_info_t ji_ptr) {
  if (ji_ptr != NULL) {
    if (ji_ptr->rwlck_ != NULL) {
      (void)sqc_rwlock_destroy(&(ji_ptr->rwlck_));
    }
  }

  free((void *) ji_ptr);
}


static inline sqc_result_t
s_dbmgr_job_info_record_add(dbmgr_job_info_t ji_ptr) {
  if (likely(job_info_hashmap != NULL)) {
    if (likely(ji_ptr != NULL && IS_VALID_STRING(ji_ptr->job_id) == true)) {
      return sqc_hashmap_add(&job_info_hashmap,
                             (char *) (ji_ptr->job_id),
                             (void **) &ji_ptr, false);
    }

    return SQC_RESULT_INVALID_ARGS;
  } else {
    return SQC_RESULT_NOT_STARTED;
  }
}


static inline sqc_result_t
s_dbmgr_job_info_record_delete(dbmgr_job_info_t ji_ptr) {
  if (likely(job_info_hashmap != NULL)) {
    if (likely(ji_ptr != NULL && IS_VALID_STRING(ji_ptr->job_id) == true)) {
      return sqc_hashmap_delete(&job_info_hashmap,
                                (void *) ji_ptr->job_id,
                                NULL, true);
    }

    return SQC_RESULT_INVALID_ARGS;
  } else {
    return SQC_RESULT_NOT_STARTED;
  }
}


static inline sqc_result_t
s_dbmgr_job_info_record_find(const char *job_id, dbmgr_job_info_t *ji_ptr) {
  if (likely(job_info_hashmap != NULL)) {
    if (likely(ji_ptr != NULL && IS_VALID_STRING(job_id) == true)) {
      return sqc_hashmap_find(&job_info_hashmap,
                              (void *) job_id, (void **) ji_ptr);
    }

    return SQC_RESULT_INVALID_ARGS;
  } else {
    return SQC_RESULT_NOT_STARTED;
  }
}

static inline bool
s_dbmgr_job_info_is_deletable(const dbmgr_job_info_t ji_ptr) {
  if (ji_ptr != NULL && ((ji_ptr->status == SQC_RPC_SCHED_JOB_STATUS_DONE) ||
                         (ji_ptr->status == SQC_RPC_SCHED_JOB_STATUS_CANCELLED) ||
                         (ji_ptr->status == SQC_RPC_SCHED_JOB_STATUS_ERROR))) {
    return true;
  }

  return false;
}


typedef struct {
  const char *target_user_id;
  dbmgr_job_info_t *results;
  size_t count;
  size_t max_count;
} user_filter_ctx_t;


static bool
s_find_filter_by_user_id(const void *key, void *val, sqc_hashentry_t he, void *arg) {
  bool result = false;
  dbmgr_job_info_t ji_ptr = (dbmgr_job_info_t)val;
  user_filter_ctx_t *ctx = (user_filter_ctx_t *)arg;

  (void)key;
  (void)he;

  if (strcmp(ji_ptr->user_id, ctx->target_user_id) == 0) {
    if (ctx->count < ctx->max_count) {
      ctx->results[ctx->count] = ji_ptr;
      ctx->count++;
      result = true;
    } else {
      result = false;
    }
  } else {
    result = true;
  }

  return result;
}


static inline sqc_result_t
s_dbmgr_job_info_record_find_by_user_id(const char *target_user_id,
                                        dbmgr_job_info_t **ji_ptr_arr, size_t *arr_len) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  size_t job_info_size;
  user_filter_ctx_t ctx;

  if (likely(job_info_hashmap != NULL)) {
    if (likely(IS_VALID_STRING(target_user_id) == true)) {
      if ((rc = sqc_hashmap_size(&job_info_hashmap)) >= 0) {
        job_info_size = (size_t)rc;
        if (job_info_size > 0) {
          ctx.target_user_id = target_user_id;
          ctx.results = (dbmgr_job_info_t *)malloc(sizeof(dbmgr_job_info_t) * job_info_size);
          if (ctx.results == NULL) {
            rc = SQC_RESULT_NO_MEMORY;
            sqc_msg_error("Failed to malloc Job record array.\n");
            return rc;
          }
          ctx.count = 0;
          ctx.max_count = job_info_size;

          rc = sqc_hashmap_iterate(&job_info_hashmap, s_find_filter_by_user_id, &ctx);
          if (rc == SQC_RESULT_OK) {
            *ji_ptr_arr = ctx.results;
            *arr_len = ctx.count;
            return rc;
          } else {
            sqc_msg_error("Failed to hash iterate: %s\n", sqc_error_get_string(rc));
            free(ctx.results);
            return rc;
          }
        } else {
          rc = SQC_RESULT_OK;
          *arr_len = 0;
          return rc;
        }
      } else {
        sqc_msg_error("Failed to get hash size: %s\n", sqc_error_get_string(rc));
        return rc;
      }
    } else {
      rc = SQC_RESULT_INVALID_ARGS;
      sqc_msg_error("Failed to find job info: %s\n", sqc_error_get_string(rc));
      return rc;
    }
  } else {
    return SQC_RESULT_NOT_STARTED;
  }
}


typedef struct {
  sqc_rpc_sched_job_status_t target_status;
  dbmgr_job_info_t *results;
  size_t count;
  size_t max_count;
} job_status_filter_ctx_t;


static bool
s_find_filter_by_job_status(const void *key, void *val, sqc_hashentry_t he, void *arg) {
  bool result = false;
  dbmgr_job_info_t ji_ptr = (dbmgr_job_info_t)val;
  job_status_filter_ctx_t *ctx = (job_status_filter_ctx_t *)arg;

  (void)key;
  (void)he;

  if (ji_ptr->status == ctx->target_status) {
    if (ctx->count < ctx->max_count) {
      ctx->results[ctx->count] = ji_ptr;
      ctx->count++;
      result = true;
    } else {
      result = false;
    }
  } else {
    result = true;
  }

  return result;
}


static inline sqc_result_t
s_dbmgr_job_info_record_find_by_job_status(const sqc_rpc_sched_job_status_t target_status,
                                           dbmgr_job_info_t **ji_ptr_arr, size_t *arr_len) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  size_t job_info_size;
  job_status_filter_ctx_t ctx;

  if (likely(job_info_hashmap != NULL)) {
    if ((rc = sqc_hashmap_size(&job_info_hashmap)) >= 0) {
      job_info_size = (size_t)rc;
      if (job_info_size > 0) {
        ctx.target_status = target_status;
        ctx.results = (dbmgr_job_info_t *)malloc(sizeof(dbmgr_job_info_t) * job_info_size);
        if (ctx.results == NULL) {
          rc = SQC_RESULT_NO_MEMORY;
          sqc_msg_error("Failed to malloc Job record array.\n");
          return rc;
        }
        ctx.count = 0;
        ctx.max_count = job_info_size;

        rc = sqc_hashmap_iterate(&job_info_hashmap, s_find_filter_by_job_status, &ctx);
        if (rc == SQC_RESULT_OK) {
          *ji_ptr_arr = ctx.results;
          *arr_len = ctx.count;
          return rc;
        } else {
          sqc_msg_error("Failed to hash iterate: %s\n", sqc_error_get_string(rc));
          free(ctx.results);
          return rc;
        }
      } else {
        rc = SQC_RESULT_OK;
        *arr_len = 0;
        return rc;
      }
    } else {
      sqc_msg_error("Failed to get hash size: %s\n", sqc_error_get_string(rc));
      return rc;
    }
  } else {
    return SQC_RESULT_NOT_STARTED;
  }
}


typedef struct {
  sqc_chrono_t from_time;
  sqc_chrono_t to_time;
  dbmgr_job_info_t *results;
  size_t count;
  size_t max_count;
} created_time_filter_ctx_t;


static bool
s_find_filter_by_created_time(const void *key, void *val, sqc_hashentry_t he, void *arg) {
  bool result = false;
  dbmgr_job_info_t ji_ptr = (dbmgr_job_info_t)val;
  created_time_filter_ctx_t *ctx = (created_time_filter_ctx_t *)arg;

  (void)key;
  (void)he;

  if (ji_ptr->created_time >= ctx->from_time && ji_ptr->created_time <= ctx->to_time) {
    if (ctx->count < ctx->max_count) {
      ctx->results[ctx->count] = ji_ptr;
      ctx->count++;
      result = true;
    } else {
      result = false;
    }
  } else {
    result = true;
  }

  return result;
}


static inline sqc_result_t
s_dbmgr_job_info_record_find_by_created_time(const sqc_chrono_t from_time, const sqc_chrono_t to_time,
                                             dbmgr_job_info_t **ji_ptr_arr, size_t *arr_len) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  size_t job_info_size;
  created_time_filter_ctx_t ctx;

  if (likely(job_info_hashmap != NULL)) {
    if ((rc = sqc_hashmap_size(&job_info_hashmap)) >= 0) {
      job_info_size = (size_t)rc;
      if (job_info_size > 0) {
        ctx.from_time = from_time;
        ctx.to_time = to_time;
        ctx.results = (dbmgr_job_info_t *)malloc(sizeof(dbmgr_job_info_t) * job_info_size);
        if (ctx.results == NULL) {
          rc = SQC_RESULT_NO_MEMORY;
          sqc_msg_error("Failed to malloc Job record array.\n");
          return rc;
        }
        ctx.count = 0;
        ctx.max_count = job_info_size;

        rc = sqc_hashmap_iterate(&job_info_hashmap, s_find_filter_by_created_time, &ctx);
        if (rc == SQC_RESULT_OK) {
          *ji_ptr_arr = ctx.results;
          *arr_len = ctx.count;
          return rc;
        } else {
          sqc_msg_error("Failed to hash iterate: %s\n", sqc_error_get_string(rc));
          free(ctx.results);
          return rc;
        }
      } else {
        rc = SQC_RESULT_OK;
        *arr_len = 0;
        return rc;
      }
    } else {
      sqc_msg_error("Failed to get hash size: %s\n", sqc_error_get_string(rc));
      return rc;
    }
  } else {
    return SQC_RESULT_NOT_STARTED;
  }
}


typedef struct {
  const char *target_user_id;
  sqc_chrono_t from_time;
  sqc_chrono_t to_time;
  dbmgr_job_info_t *results;
  size_t count;
  size_t max_count;
} delete_target_filter_ctx_t;

static bool
s_is_delete_target(const dbmgr_job_info_t ji_ptr, const char *target_user_id,
                   const sqc_chrono_t from_time, const sqc_chrono_t to_time) {
  if ((IS_VALID_STRING(target_user_id) == false) ||
      ((IS_VALID_STRING(target_user_id) == true) && (strcmp(ji_ptr->user_id, target_user_id) == 0))) {
    if (ji_ptr->created_time >= from_time && ji_ptr->created_time <= to_time) {
      if (s_dbmgr_job_info_is_deletable(ji_ptr) == true) {
        return true;
      }
    }
  }

  return false;
}

static bool
s_find_filter_by_delete_target(const void *key, void *val, sqc_hashentry_t he, void *arg) {
  bool result = false;
  dbmgr_job_info_t ji_ptr = (dbmgr_job_info_t)val;
  delete_target_filter_ctx_t *ctx = (delete_target_filter_ctx_t *)arg;

  (void)key;
  (void)he;

  if (s_is_delete_target(ji_ptr, ctx->target_user_id, ctx->from_time, ctx->to_time) == true) {
    if (ctx->count < ctx->max_count) {
      ctx->results[ctx->count] = ji_ptr;
      ctx->count++;
      result = true;
    } else {
      result = false;
    }
  } else {
    result = true;
  }

  return result;
}


static inline sqc_result_t
s_dbmgr_job_info_record_find_by_delete_target(const char *target_user_id,
                                              const sqc_chrono_t from_time, const sqc_chrono_t to_time,
                                              dbmgr_job_info_t **ji_ptr_arr, size_t *arr_len) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  size_t job_info_size;
  delete_target_filter_ctx_t ctx;

  if (likely(job_info_hashmap != NULL)) {
    if ((rc = sqc_hashmap_size(&job_info_hashmap)) >= 0) {
      job_info_size = (size_t)rc;
      if (job_info_size > 0) {
        // If anything other than a string is specified, it applies to all users
        if (IS_VALID_STRING(target_user_id) == true) {
          ctx.target_user_id = target_user_id;
        } else {
          ctx.target_user_id = NULL;
        }
        ctx.from_time = from_time;
        ctx.to_time = to_time;
        ctx.results = (dbmgr_job_info_t *)malloc(sizeof(dbmgr_job_info_t) * job_info_size);
        if (ctx.results == NULL) {
          rc = SQC_RESULT_NO_MEMORY;
          sqc_msg_error("Failed to malloc Job record array.\n");
          return rc;
        }
        ctx.count = 0;
        ctx.max_count = job_info_size;

        rc = sqc_hashmap_iterate(&job_info_hashmap, s_find_filter_by_delete_target, &ctx);
        if (rc == SQC_RESULT_OK) {
          *ji_ptr_arr = ctx.results;
          *arr_len = ctx.count;
          return rc;
        } else {
          sqc_msg_error("Failed to hash iterate: %s\n", sqc_error_get_string(rc));
          free(ctx.results);
          return rc;
        }
      } else {
        rc = SQC_RESULT_OK;
        *arr_len = 0;
        return rc;
      }
    } else {
      sqc_msg_error("Failed to get hash size: %s\n", sqc_error_get_string(rc));
      return rc;
    }
  } else {
    return SQC_RESULT_NOT_STARTED;
  }
}


static void
s_dbmgr_job_info_record_freeup(void *ji_ptr) {
  s_dbmgr_job_info_record_destroy((dbmgr_job_info_t) ji_ptr);
}


static sqc_result_t
s_dbmgr_job_info_initialize(void) {
  sqc_rpc_sched_qc_type_t qc_type = SQC_RPC_SCHED_QC_TYPE_UNKNOWN;

  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  if ((rc = sqc_hashmap_create(&job_info_hashmap,
                               SQC_HASHMAP_TYPE_STRING,
                               s_dbmgr_job_info_record_freeup)) != SQC_RESULT_OK) {
    sqc_perror(rc);
  }

  qc_type = sqc_rpc_sched_conf_get_qc_type();
  if (qc_type == SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_USER_TOKEN ||
      qc_type == SQC_RPC_SCHED_QC_TYPE_A_OQTOPUSREST_BOTH_TOKEN) {
    sqc_msg_info("Excluded from job info deserialization: qc-type=%d (%s)\n",
                 (int) qc_type, sqc_rpc_sched_qc_type_to_string(qc_type));
  } else {
    if ((rc = dbmgr_db_job_info_deserialize_all(&job_info_hashmap)) == SQC_RESULT_OK) {
      sqc_msg_info("Job info deserialized successfully\n");
    } else {
      sqc_msg_error("Failed to deserialize the job info: %s\n", sqc_error_get_string(rc));
    }
  }

  return rc;
}


static void
s_dbmgr_job_info_finalize(void) {
  sqc_hashmap_destroy(&job_info_hashmap, true);
  job_info_hashmap = NULL;
}


static inline bool
s_dbmgr_is_state_transition_possible(const dbmgr_job_info_t ji_ptr,
                                     const sqc_rpc_sched_job_status_t status) {
  if (ji_ptr != NULL) {
    switch (ji_ptr->status) {
      case SQC_RPC_SCHED_JOB_STATUS_UNKNOWN: {
        switch (status) {
          case SQC_RPC_SCHED_JOB_STATUS_CREATED:
          case SQC_RPC_SCHED_JOB_STATUS_CANCELLED:
          case SQC_RPC_SCHED_JOB_STATUS_ERROR:
          case SQC_RPC_SCHED_JOB_STATUS_DELETED: {
            return true;
          }
          default: {
            break;
          }
        }
        break;
      }
      case SQC_RPC_SCHED_JOB_STATUS_CREATED: {
        switch (status) {
          case SQC_RPC_SCHED_JOB_STATUS_QUEUED:
          case SQC_RPC_SCHED_JOB_STATUS_CANCELLED:
          case SQC_RPC_SCHED_JOB_STATUS_ERROR:
          case SQC_RPC_SCHED_JOB_STATUS_DELETED: {
            return true;
          }
          default: {
            break;
          }
        }
        break;
      }
      case SQC_RPC_SCHED_JOB_STATUS_QUEUED: {
        switch (status) {
          case SQC_RPC_SCHED_JOB_STATUS_RUNNING:
          case SQC_RPC_SCHED_JOB_STATUS_CANCELLED:
          case SQC_RPC_SCHED_JOB_STATUS_ERROR:
          case SQC_RPC_SCHED_JOB_STATUS_DELETED: {
            return true;
          }
          default: {
            break;
          }
        }
        break;
      }
      case SQC_RPC_SCHED_JOB_STATUS_RUNNING: {
        switch (status) {
          case SQC_RPC_SCHED_JOB_STATUS_DONE:
          case SQC_RPC_SCHED_JOB_STATUS_CANCELLED:
          case SQC_RPC_SCHED_JOB_STATUS_ERROR: {
            return true;
          }
          default: {
            break;
          }
        }
        break;
      }
      case SQC_RPC_SCHED_JOB_STATUS_DONE: {
        switch (status) {
          case SQC_RPC_SCHED_JOB_STATUS_DELETED: {
            return true;
          }
          default: {
            break;
          }
        }
        break;
      }
      case SQC_RPC_SCHED_JOB_STATUS_CANCELLED: {
        switch (status) {
          case SQC_RPC_SCHED_JOB_STATUS_DELETED: {
            return true;
          }
          default: {
            break;
          }
        }
        break;
      }
      case SQC_RPC_SCHED_JOB_STATUS_ERROR: {
        switch (status) {
          case SQC_RPC_SCHED_JOB_STATUS_DELETED: {
            return true;
          }
          default: {
            break;
          }
        }
        break;
      }
      case SQC_RPC_SCHED_JOB_STATUS_DELETED:
      default: {
        break;
      }
    }
  }

  return false;
}


static int
s_cmp_created_time_proc(const void *v0, const void *v1, void *arg) {
  (void) arg;

  if (v0 != NULL && v1 != NULL) {
    dbmgr_job_info_t p0 = *(dbmgr_job_info_t *)v0;
    dbmgr_job_info_t p1 = *(dbmgr_job_info_t *)v1;
    sqc_chrono_t p0_created_time = p0->created_time;
    sqc_chrono_t p1_created_time = p1->created_time;

    if (p0_created_time < p1_created_time) return -1;
    if (p0_created_time > p1_created_time) return 1;
  }

  return 0;
}


static inline sqc_result_t
s_dbmgr_job_info_arr_sort_by_created_time(dbmgr_job_info_t *ji_ptr_arr,
                                          const size_t arr_len) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  if (ji_ptr_arr != NULL) {
    sqc_qsort_r((void *)ji_ptr_arr, arr_len, sizeof(dbmgr_job_info_t),
                s_cmp_created_time_proc, NULL);
    rc = SQC_RESULT_OK;
  } else {
    rc = SQC_RESULT_INVALID_ARGS;
  }

  return rc;
}


static inline sqc_result_t
s_dbmgr_job_info_clear_result(const dbmgr_job_info_t ji_ptr) {
  if (likely(ji_ptr != NULL)) {
    if (s_dbmgr_is_state_transition_possible(ji_ptr, SQC_RPC_SCHED_JOB_STATUS_DELETED) == true) {
      s_dbmgr_job_info_record_wlock(ji_ptr);
      {
        ji_ptr->status = SQC_RPC_SCHED_JOB_STATUS_DELETED;

        ji_ptr->result[0] = '\0';
        ji_ptr->result_len = 0;

        ji_ptr->qprogram[0] = '\0';
        ji_ptr->qprogram_len = 0;

        ji_ptr->deleted_time = sqc_chrono_now();

        ji_ptr->update_time = ji_ptr->deleted_time;
      }
      s_dbmgr_job_info_record_unlock(ji_ptr);

      // Update DB
      (void)dbmgr_db_job_info_clear_result(ji_ptr->job_id, ji_ptr->deleted_time,
                                           ji_ptr->update_time);

      return SQC_RESULT_OK;
    }

    return SQC_RESULT_INVALID_STATE_TRANSITION;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


static inline sqc_result_t
s_dbmgr_job_info_arr_clear_result(dbmgr_job_info_t *ji_ptr_arr, const size_t arr_len) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  // TODO: Consider using transactions to perform batch processing.
  for (size_t i = 0; i < arr_len; i++) {
    rc = s_dbmgr_job_info_clear_result(ji_ptr_arr[i]);
    if (rc != SQC_RESULT_OK) {
      sqc_msg_error("Failed to clear result: job_id=%s, err_msg=%s\n",
                    ji_ptr_arr[i]->job_id, sqc_error_get_string(rc));
    }
  }

  return SQC_RESULT_OK;
}


/*
 * export
 */


sqc_result_t
dbmgr_ji_create_job(const char *user_id, const uint8_t priority,
                    const char *qprogram, sqc_rpc_sched_circuit_fmt_t circuit_fmt, size_t shots,
                    sqc_rpc_sched_qc_type_t qc_type, sqc_rpc_sched_transpiler_t transpiler,
                    const char *remark, const char *user_token, dbmgr_job_info_t *ji_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  rc = s_dbmgr_job_info_record_create(user_id, priority,
                                      qprogram, circuit_fmt, shots,
                                      qc_type, transpiler,
                                      remark, user_token, ji_ptr);
  if (rc == SQC_RESULT_OK) {
    rc = s_dbmgr_job_info_record_add(*ji_ptr);
    if (rc == SQC_RESULT_OK) {
      // do not add user_token to the database
      rc = dbmgr_db_job_info_insert_record((*ji_ptr)->job_id, (*ji_ptr)->user_id,
                                           (*ji_ptr)->priority, (*ji_ptr)->status,
                                           (*ji_ptr)->qprogram, (*ji_ptr)->circuit_fmt, (*ji_ptr)->shots,
                                           (*ji_ptr)->qc_type, (*ji_ptr)->transpiler,
                                           (*ji_ptr)->remark, (*ji_ptr)->created_time,
                                           (*ji_ptr)->update_time);
      if (rc != SQC_RESULT_OK) {
        sqc_msg_error("Failed to Insert Job record: %s\n", sqc_error_get_string(rc));
      }
    } else {
      sqc_msg_error("Failed to add Job record: %s\n", sqc_error_get_string(rc));
    }
  } else {
    sqc_msg_error("Failed to create Job record: %s\n", sqc_error_get_string(rc));
  }

  return rc;
}


sqc_result_t
dbmgr_ji_delete_job(const char *job_id) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  dbmgr_job_info_t ji_ptr = NULL;

  rc = s_dbmgr_job_info_record_find(job_id, &ji_ptr);
  if (rc == SQC_RESULT_OK) {
    rc = s_dbmgr_job_info_record_delete(ji_ptr);
  }

  return rc;
}


bool
dbmgr_ji_job_exists(const char *job_id) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  dbmgr_job_info_t ji_ptr = NULL;

  rc = s_dbmgr_job_info_record_find(job_id, &ji_ptr);
  if (rc == SQC_RESULT_OK && ji_ptr != NULL) {
    return true;
  }
  return false;
}


bool
dbmgr_ji_is_deletable(const dbmgr_job_info_t ji_ptr) {
  return s_dbmgr_job_info_is_deletable(ji_ptr);
}


sqc_result_t
dbmgr_ji_job_find(const char *job_id, dbmgr_job_info_t *ji_ptr) {
  return s_dbmgr_job_info_record_find(job_id, ji_ptr);
}


sqc_result_t
dbmgr_ji_job_find_by_user_id(const char *target_user_id,
                             dbmgr_job_info_t **ji_ptr_arr, size_t *arr_len) {
  return s_dbmgr_job_info_record_find_by_user_id(target_user_id, ji_ptr_arr, arr_len);
}


sqc_result_t
dbmgr_ji_job_find_by_job_status(const sqc_rpc_sched_job_status_t target_status,
                                dbmgr_job_info_t **ji_ptr_arr, size_t *arr_len) {
  return s_dbmgr_job_info_record_find_by_job_status(target_status, ji_ptr_arr, arr_len);
}


sqc_result_t
dbmgr_ji_job_find_by_created_time(const sqc_chrono_t from_time, const sqc_chrono_t to_time,
                                  dbmgr_job_info_t **ji_ptr_arr, size_t *arr_len) {
  return s_dbmgr_job_info_record_find_by_created_time(from_time, to_time, ji_ptr_arr, arr_len);
}


sqc_result_t
dbmgr_ji_job_find_by_delete_target(const char *target_user_id,
                                   const sqc_chrono_t from_time, const sqc_chrono_t to_time,
                                   dbmgr_job_info_t **ji_ptr_arr, size_t *arr_len) {
  return s_dbmgr_job_info_record_find_by_delete_target(target_user_id, from_time, to_time, ji_ptr_arr, arr_len);
}


sqc_result_t
dbmgr_ji_get_job_id(const dbmgr_job_info_t ji_ptr, char **job_id) {
  if (likely(ji_ptr != NULL && job_id != NULL && *job_id == NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *job_id = strdup(ji_ptr->job_id);
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    if (*job_id == NULL) {
      return SQC_RESULT_NO_MEMORY;
    }

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_job_id_len(const dbmgr_job_info_t ji_ptr, size_t *job_id_len) {
  if (likely(ji_ptr != NULL && job_id_len != NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *job_id_len = ji_ptr->job_id_len;
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_user_id(const dbmgr_job_info_t ji_ptr, char **user_id){
  if (likely(ji_ptr != NULL && user_id != NULL && *user_id == NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *user_id = strdup(ji_ptr->user_id);
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    if (*user_id == NULL) {
      return SQC_RESULT_NO_MEMORY;
    }

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_user_id_len(const dbmgr_job_info_t ji_ptr, size_t *user_id_len) {
  if (likely(ji_ptr != NULL && user_id_len != NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *user_id_len = ji_ptr->user_id_len;
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_priority(const dbmgr_job_info_t ji_ptr, uint8_t *priority) {
  if (likely(ji_ptr != NULL && priority != NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *priority = ji_ptr->priority;
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_status(const dbmgr_job_info_t ji_ptr, sqc_rpc_sched_job_status_t *status) {
  if (likely(ji_ptr != NULL && status != NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *status = ji_ptr->status;
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_set_job_status_queued(const dbmgr_job_info_t ji_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  if (likely(ji_ptr != NULL)) {
    s_dbmgr_job_info_record_wlock(ji_ptr);
    {
      if (s_dbmgr_is_state_transition_possible(ji_ptr, SQC_RPC_SCHED_JOB_STATUS_QUEUED) == true) {
        ji_ptr->status = SQC_RPC_SCHED_JOB_STATUS_QUEUED;
        ji_ptr->queued_time = sqc_chrono_now();
        ji_ptr->update_time = ji_ptr->queued_time;

        // Update DB
        (void)dbmgr_db_job_info_update_queued_status(ji_ptr->job_id, ji_ptr->queued_time,
                                                     ji_ptr->update_time);

        rc = SQC_RESULT_OK;
      } else {
        rc = SQC_RESULT_INVALID_STATE_TRANSITION;
      }
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);
  } else {
    rc = SQC_RESULT_INVALID_ARGS;
  }

  return rc;
}


sqc_result_t
dbmgr_set_job_status_running(const dbmgr_job_info_t ji_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  if (likely(ji_ptr != NULL)) {
    s_dbmgr_job_info_record_wlock(ji_ptr);
    {
      if (s_dbmgr_is_state_transition_possible(ji_ptr, SQC_RPC_SCHED_JOB_STATUS_RUNNING) == true) {
        ji_ptr->status = SQC_RPC_SCHED_JOB_STATUS_RUNNING;
        ji_ptr->running_time = sqc_chrono_now();
        ji_ptr->update_time = ji_ptr->running_time;

        // Update DB
        (void)dbmgr_db_job_info_update_running_status(ji_ptr->job_id, ji_ptr->running_time,
                                                      ji_ptr->update_time);

        rc = SQC_RESULT_OK;
      } else {
        rc = SQC_RESULT_INVALID_STATE_TRANSITION;
      }
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);
  } else {
    rc = SQC_RESULT_INVALID_ARGS;
  }

  return rc;
}


sqc_result_t
dbmgr_set_job_status_done(const dbmgr_job_info_t ji_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  if (likely(ji_ptr != NULL)) {
    s_dbmgr_job_info_record_wlock(ji_ptr);
    {
      if (s_dbmgr_is_state_transition_possible(ji_ptr, SQC_RPC_SCHED_JOB_STATUS_DONE) == true) {
        ji_ptr->status = SQC_RPC_SCHED_JOB_STATUS_DONE;
        ji_ptr->done_time = sqc_chrono_now();
        ji_ptr->update_time = ji_ptr->done_time;

        // Update DB
        (void)dbmgr_db_job_info_update_done_status(ji_ptr->job_id, ji_ptr->done_time,
                                                   ji_ptr->update_time);

        rc = SQC_RESULT_OK;
      } else {
        rc = SQC_RESULT_INVALID_STATE_TRANSITION;
      }
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);
  } else {
    rc = SQC_RESULT_INVALID_ARGS;
  }

  return rc;
}


sqc_result_t
dbmgr_set_job_status_cancelled(const dbmgr_job_info_t ji_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  if (likely(ji_ptr != NULL)) {
    s_dbmgr_job_info_record_wlock(ji_ptr);
    {
      if (s_dbmgr_is_state_transition_possible(ji_ptr, SQC_RPC_SCHED_JOB_STATUS_CANCELLED) == true) {
        ji_ptr->status = SQC_RPC_SCHED_JOB_STATUS_CANCELLED;
        ji_ptr->cancelled_time = sqc_chrono_now();
        ji_ptr->update_time = ji_ptr->cancelled_time;

        // Update DB
        (void)dbmgr_db_job_info_update_cancelled_status(ji_ptr->job_id, ji_ptr->cancelled_time,
                                                        ji_ptr->update_time);

        rc = SQC_RESULT_OK;
      } else {
        rc = SQC_RESULT_INVALID_STATE_TRANSITION;
      }
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);
  } else {
    rc = SQC_RESULT_INVALID_ARGS;
  }

  return rc;
}


sqc_result_t
dbmgr_set_job_status_error(const dbmgr_job_info_t ji_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  if (likely(ji_ptr != NULL)) {
    s_dbmgr_job_info_record_wlock(ji_ptr);
    {
      if (s_dbmgr_is_state_transition_possible(ji_ptr, SQC_RPC_SCHED_JOB_STATUS_ERROR) == true) {
        ji_ptr->status = SQC_RPC_SCHED_JOB_STATUS_ERROR;
        ji_ptr->error_time = sqc_chrono_now();
        ji_ptr->update_time = ji_ptr->error_time;

        // Update DB
        (void)dbmgr_db_job_info_update_error_status(ji_ptr->job_id, ji_ptr->error_time,
                                                    ji_ptr->update_time);

        rc = SQC_RESULT_OK;
      } else {
        rc = SQC_RESULT_INVALID_STATE_TRANSITION;
      }
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);
  } else {
    rc = SQC_RESULT_INVALID_ARGS;
  }

  return rc;
}


sqc_result_t
dbmgr_set_job_status_deleted(const dbmgr_job_info_t ji_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  if (likely(ji_ptr != NULL)) {
    s_dbmgr_job_info_record_wlock(ji_ptr);
    {
      if (s_dbmgr_is_state_transition_possible(ji_ptr, SQC_RPC_SCHED_JOB_STATUS_DELETED) == true) {
        ji_ptr->status = SQC_RPC_SCHED_JOB_STATUS_DELETED;
        ji_ptr->deleted_time = sqc_chrono_now();
        ji_ptr->update_time = ji_ptr->deleted_time;

        // Update DB
        (void)dbmgr_db_job_info_update_deleted_status(ji_ptr->job_id, ji_ptr->deleted_time,
                                                      ji_ptr->update_time);

        rc = SQC_RESULT_OK;
      } else {
        rc = SQC_RESULT_INVALID_STATE_TRANSITION;
      }
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);
  } else {
    rc = SQC_RESULT_INVALID_ARGS;
  }

  return rc;
}


sqc_result_t
dbmgr_ji_set_qc_job_id(dbmgr_job_info_t ji_ptr, const char *qc_job_id) {
  if (likely(ji_ptr != NULL && qc_job_id != NULL && IS_VALID_STRING(qc_job_id) == true)) {
    size_t len = strlen(qc_job_id);
    if (len > SQC_RPC_SCHED_QC_JOB_ID_MAX_SIZE) {
      return SQC_RESULT_TOO_LONG;
    }

    s_dbmgr_job_info_record_wlock(ji_ptr);
    {
      strncpy(ji_ptr->qc_job_id, qc_job_id, len);
      ji_ptr->qc_job_id[len] = '\0';

      ji_ptr->qc_job_id_len = len;

      ji_ptr->update_time = sqc_chrono_now();
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    // Update DB
    (void)dbmgr_db_job_info_update_qc_job_id(ji_ptr->job_id, ji_ptr->qc_job_id,
                                             ji_ptr->update_time);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}

sqc_result_t
dbmgr_ji_get_qc_job_id(const dbmgr_job_info_t ji_ptr, char **qc_job_id) {
  if (likely(ji_ptr != NULL && qc_job_id != NULL && *qc_job_id == NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *qc_job_id = strdup(ji_ptr->qc_job_id);
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    if (*qc_job_id == NULL) {
      return SQC_RESULT_NO_MEMORY;
    }

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_qc_job_id_len(const dbmgr_job_info_t ji_ptr, size_t *qc_job_id_len) {
  if (likely(ji_ptr != NULL && qc_job_id_len != NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *qc_job_id_len = ji_ptr->qc_job_id_len;
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_qprogram(const dbmgr_job_info_t ji_ptr, char **qprogram) {
  if (likely(ji_ptr != NULL && qprogram != NULL && *qprogram == NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *qprogram = strdup(ji_ptr->qprogram);
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    if (*qprogram == NULL) {
      return SQC_RESULT_NO_MEMORY;
    }

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_qprogram_len(const dbmgr_job_info_t ji_ptr, size_t *qprogram_len) {
  if (likely(ji_ptr != NULL && qprogram_len != NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *qprogram_len = ji_ptr->qprogram_len;
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_circuit_fmt(const dbmgr_job_info_t ji_ptr, sqc_rpc_sched_circuit_fmt_t *circuit_fmt) {
  if (likely(ji_ptr != NULL && circuit_fmt != NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *circuit_fmt = ji_ptr->circuit_fmt;
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_shots(const dbmgr_job_info_t ji_ptr, size_t *shots) {
  if (likely(ji_ptr != NULL && shots != NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *shots = ji_ptr->shots;
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_qc_type(const dbmgr_job_info_t ji_ptr, sqc_rpc_sched_qc_type_t *qc_type) {
  if (likely(ji_ptr != NULL && qc_type != NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *qc_type = ji_ptr->qc_type;
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_transpiler(const dbmgr_job_info_t ji_ptr, sqc_rpc_sched_transpiler_t *transpiler) {
  if (likely(ji_ptr != NULL && transpiler != NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *transpiler = ji_ptr->transpiler;
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_remark(const dbmgr_job_info_t ji_ptr, char **remark) {
  if (likely(ji_ptr != NULL && remark != NULL && *remark == NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *remark = strdup(ji_ptr->remark);
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    if (*remark == NULL) {
      return SQC_RESULT_NO_MEMORY;
    }

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_remark_len(const dbmgr_job_info_t ji_ptr, size_t *remark_len) {
  if (likely(ji_ptr != NULL && remark_len != NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *remark_len = ji_ptr->remark_len;
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_set_result(dbmgr_job_info_t ji_ptr, const char *result, size_t result_len) {
  if (likely(ji_ptr != NULL && result != NULL && IS_VALID_STRING(result) == true)) {
    size_t len = strlen(result);
    if (len > SQC_RPC_SCHED_RESULT_MAX_SIZE) {
      len = SQC_RPC_SCHED_RESULT_MAX_SIZE;
    }
    if (len > result_len) {
      len = result_len;
    }

    s_dbmgr_job_info_record_wlock(ji_ptr);
    {
      strncpy(ji_ptr->result, result, len);
      ji_ptr->result[len] = '\0';

      ji_ptr->result_len = len;

      ji_ptr->update_time = sqc_chrono_now();
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    // Update DB
    (void)dbmgr_db_job_info_update_result(ji_ptr->job_id, ji_ptr->result,
                                          ji_ptr->update_time);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_result(const dbmgr_job_info_t ji_ptr, char **result) {
  if (likely(ji_ptr != NULL && result != NULL && *result == NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *result = strdup(ji_ptr->result);
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    if (*result == NULL) {
      return SQC_RESULT_NO_MEMORY;
    }

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_result_len(const dbmgr_job_info_t ji_ptr, size_t *result_len) {
  if (likely(ji_ptr != NULL && result_len != NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *result_len = ji_ptr->result_len;
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_user_token(const dbmgr_job_info_t ji_ptr, char **user_token) {
  if (likely(ji_ptr != NULL && user_token != NULL && *user_token == NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *user_token = strdup(ji_ptr->user_token);
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    if (*user_token == NULL) {
      return SQC_RESULT_NO_MEMORY;
    }

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_user_token_len(const dbmgr_job_info_t ji_ptr, size_t *user_token_len) {
  if (likely(ji_ptr != NULL && user_token_len != NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *user_token_len = ji_ptr->user_token_len;
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_created_time(const dbmgr_job_info_t ji_ptr, sqc_chrono_t *created_time) {
  if (likely(ji_ptr != NULL && created_time != NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *created_time = ji_ptr->created_time;
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}

sqc_result_t
dbmgr_ji_get_queued_time(const dbmgr_job_info_t ji_ptr, sqc_chrono_t *queued_time) {
  if (likely(ji_ptr != NULL && queued_time != NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *queued_time = ji_ptr->queued_time;
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_running_time(const dbmgr_job_info_t ji_ptr, sqc_chrono_t *running_time) {
  if (likely(ji_ptr != NULL && running_time != NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *running_time = ji_ptr->running_time;
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_done_time(const dbmgr_job_info_t ji_ptr, sqc_chrono_t *done_time) {
  if (likely(ji_ptr != NULL && done_time != NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *done_time = ji_ptr->done_time;
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_cancelled_time(const dbmgr_job_info_t ji_ptr, sqc_chrono_t *cancelled_time) {
  if (likely(ji_ptr != NULL && cancelled_time != NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *cancelled_time = ji_ptr->cancelled_time;
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_error_time(const dbmgr_job_info_t ji_ptr, sqc_chrono_t *error_time) {
  if (likely(ji_ptr != NULL && error_time!= NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *error_time = ji_ptr->error_time;
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_deleted_time(const dbmgr_job_info_t ji_ptr, sqc_chrono_t *deleted_time) {
  if (likely(ji_ptr != NULL && deleted_time!= NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *deleted_time = ji_ptr->deleted_time;
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ji_get_update_time(const dbmgr_job_info_t ji_ptr, sqc_chrono_t *update_time) {
  if (likely(ji_ptr != NULL && update_time != NULL)) {
    s_dbmgr_job_info_record_rlock(ji_ptr);
    {
      *update_time = ji_ptr->update_time;
    }
    s_dbmgr_job_info_record_unlock(ji_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}

sqc_result_t
dbmgr_ji_job_arr_sort_by_created_time(dbmgr_job_info_t *ji_ptr_arr,
                                      const size_t arr_len) {
  return s_dbmgr_job_info_arr_sort_by_created_time(ji_ptr_arr, arr_len);
}

sqc_result_t
dbmgr_ji_clear_job_result(const dbmgr_job_info_t ji_ptr) {
  return s_dbmgr_job_info_clear_result(ji_ptr);
}

sqc_result_t
dbmgr_ji_clear_job_arr_result(dbmgr_job_info_t *ji_ptr_arr, const size_t arr_len) {
  return s_dbmgr_job_info_arr_clear_result(ji_ptr_arr, arr_len);
}

