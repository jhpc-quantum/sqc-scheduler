#include <sqlite3.h>

#include <uuid/uuid.h>

#include "dbmgr_util.h"
#include "dbmgr_db.h"
#include "dbmgr_types_internal.h"

// user_info SQL template
#define SQC_RPC_SCHED_DB_USER_INFO_SELECT_ALL \
  "SELECT * FROM user_info;"
#define SQC_RPC_SCHED_DB_USER_INFO_SELECT \
  "SELECT * FROM user_info WHERE user_id=?;"
#define SQC_RPC_SCHED_DB_USER_INFO_SELECT_COUNT \
  "SELECT COUNT(*) FROM user_info;"
#define SQC_RPC_SCHED_DB_USER_INFO_INSERT \
  "INSERT INTO " \
  "  user_info (user_id, role_type, status, " \
  "             created_time, update_time)" \
  "VALUES" \
  "  (?, ?, ?, ?, ?);"
#define SQC_RPC_SCHED_DB_USER_INFO_UPDATE_ROLE_TYPE \
  "UPDATE" \
  "  user_info " \
  "SET" \
  "  role_type=?, " \
  "  update_time=? " \
  "WHERE" \
  "  user_id=? "
#define SQC_RPC_SCHED_DB_USER_INFO_UPDATE_STATUS \
  "UPDATE" \
  "  user_info " \
  "SET" \
  "  status=?, " \
  "  update_time=? " \
  "WHERE" \
  "  user_id=? "

// group_info SQL template
#define SQC_RPC_SCHED_DB_GROUP_INFO_SELECT_ALL \
  "SELECT * FROM group_info;"
#define SQC_RPC_SCHED_DB_GROUP_INFO_SELECT \
  "SELECT * FROM group_info WHERE group_id=?;"
#define SQC_RPC_SCHED_DB_GROUP_INFO_SELECT_COUNT \
  "SELECT COUNT(*) FROM group_info;"
#define SQC_RPC_SCHED_DB_GROUP_INFO_INSERT \
  "INSERT INTO " \
  "  group_info (group_id, exec_time_limit_msec, " \
  "              exec_time_total_msec, " \
  "              created_time, update_time)" \
  "VALUES" \
  "  (?, ?, ?, ?, ?);"
#define SQC_RPC_SCHED_DB_GROUP_INFO_UPDATE_LIMIT \
  "UPDATE" \
  "  group_info " \
  "SET" \
  "  exec_time_limit_msec=?, " \
  "  update_time=? "\
  "WHERE " \
  "  group_id=?;"
#define SQC_RPC_SCHED_DB_GROUP_INFO_UPDATE_TOTAL \
  "UPDATE" \
  "  group_info " \
  "SET" \
  "  exec_time_total_msec=?, " \
  "  update_time=? "\
  "WHERE " \
  "  group_id=?;"

// user_group_info SQL template
#define SQC_RPC_SCHED_DB_USER_GROUP_INFO_SELECT_ALL \
  "SELECT * FROM user_group_info;"
#define SQC_RPC_SCHED_DB_USER_GROUP_INFO_SELECT \
  "SELECT * FROM user_group_info WHERE user_id=? AND group_id=?;"
#define SQC_RPC_SCHED_DB_USER_GROUP_INFO_SELECT_COUNT \
  "SELECT COUNT(*) FROM user_group_info;"
#define SQC_RPC_SCHED_DB_USER_GROUP_INFO_INSERT \
  "INSERT INTO " \
  "  user_group_info (user_id, group_id, status, " \
  "                   created_time, update_time)" \
  "VALUES" \
  "  (?, ?, ?, ?, ?);"
#define SQC_RPC_SCHED_DB_USER_GROUP_INFO_UPDATE_STATUS \
  "UPDATE" \
  "  user_group_info " \
  "SET" \
  "  status=?, " \
  "  update_time=? " \
  "WHERE" \
  "  user_id=? AND group_id=?"

// job_info SQL template
#define SQC_RPC_SCHED_DB_JOB_INFO_SELECT_ALL \
  "SELECT * FROM job_info;"
#define SQC_RPC_SCHED_DB_JOB_INFO_SELECT \
  "SELECT * FROM job_info WHERE job_id=?;"
#define SQC_RPC_SCHED_DB_JOB_INFO_SELECT_COUNT \
  "SELECT COUNT(*) FROM job_info;"
#define SQC_RPC_SCHED_DB_JOB_INFO_INSERT \
  "INSERT INTO" \
  "  job_info (job_id, user_id, group_id, " \
  "            priority, status, qprogram, circuit_fmt, " \
  "            shots, qc_type, transpiler, " \
  "            remark, created_time, update_time)" \
  "VALUES " \
  "  (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"
#define SQC_RPC_SCHED_DB_JOB_INFO_UPDATE_QUEUED_STATUS \
  "UPDATE" \
  "  job_info " \
  "SET" \
  "  status=?, " \
  "  queued_time=?, " \
  "  update_time=? " \
  "WHERE" \
  "  job_id=? "
#define SQC_RPC_SCHED_DB_JOB_INFO_UPDATE_RUNNING_STATUS \
  "UPDATE" \
  "  job_info " \
  "SET" \
  "  status=?, " \
  "  running_time=?, " \
  "  update_time=? " \
  "WHERE" \
  "  job_id=? "
#define SQC_RPC_SCHED_DB_JOB_INFO_UPDATE_DONE_STATUS \
  "UPDATE" \
  "  job_info " \
  "SET" \
  "  status=?, " \
  "  done_time=?, " \
  "  update_time=? " \
  "WHERE" \
  "  job_id=? "
#define SQC_RPC_SCHED_DB_JOB_INFO_UPDATE_CANCELLED_STATUS \
  "UPDATE" \
  "  job_info " \
  "SET" \
  "  status=?, " \
  "  cancelled_time=?, " \
  "  update_time=? " \
  "WHERE" \
  "  job_id=? "
#define SQC_RPC_SCHED_DB_JOB_INFO_UPDATE_ERROR_STATUS \
  "UPDATE" \
  "  job_info " \
  "SET" \
  "  status=?, " \
  "  error_time=?, " \
  "  update_time=? " \
  "WHERE" \
  "  job_id=? "
#define SQC_RPC_SCHED_DB_JOB_INFO_UPDATE_DELETED_STATUS \
  "UPDATE" \
  "  job_info " \
  "SET" \
  "  status=?, " \
  "  deleted_time=?, " \
  "  update_time=? " \
  "WHERE" \
  "  job_id=? "
#define SQC_RPC_SCHED_DB_JOB_INFO_UPDATE_QC_JOB_ID \
  "UPDATE" \
  "  job_info " \
  "SET" \
  "  qc_job_id=?, " \
  "  update_time=? " \
  "WHERE" \
  "  job_id=? "
#define SQC_RPC_SCHED_DB_JOB_INFO_UPDATE_QC_JOB_RESULT \
  "UPDATE" \
  "  job_info " \
  "SET" \
  "  result=?, " \
  "  update_time=? " \
  "WHERE" \
  "  job_id=? "
#define SQC_RPC_SCHED_DB_JOB_INFO_UPDATE_QC_JOB_EXEC_TIME_ESTIMATE_MSEC \
  "UPDATE" \
  "  job_info " \
  "SET" \
  "  exec_time_estimate_msec=?, " \
  "  update_time=? " \
  "WHERE" \
  "  job_id=? "
#define SQC_RPC_SCHED_DB_JOB_INFO_UPDATE_QC_JOB_EXEC_TIME_MSEC \
  "UPDATE" \
  "  job_info " \
  "SET" \
  "  exec_time_msec=?, " \
  "  update_time=? " \
  "WHERE" \
  "  job_id=? "
#define SQC_RPC_SCHED_DB_JOB_INFO_CLEAR_RESULT \
  "UPDATE" \
  "  job_info " \
  "SET" \
  "  status=?, " \
  "  qprogram=NULL, " \
  "  circuit_fmt=NULL, " \
  "  result=NULL, " \
  "  deleted_time=?, " \
  "  update_time=? " \
  "WHERE" \
  "  job_id=? "

// weight_info SQL template
#define SQC_RPC_SCHED_DB_WEIGHT_INFO_SELECT_ALL \
  "SELECT * FROM weight_info;"
#define SQC_RPC_SCHED_DB_WEIGHT_INFO_SELECT \
  "SELECT * FROM weight_info WHERE priority=?;"
#define SQC_RPC_SCHED_DB_WEIGHT_INFO_SELECT_COUNT \
  "SELECT COUNT(*) FROM weight_info;"
#define SQC_RPC_SCHED_DB_WEIGHT_INFO_INSERT \
  "INSERT INTO " \
  "  weight_info (priority, weight, created_time, update_time) " \
  "VALUES "\
    "(?, ?, ?, ?);"
#define SQC_RPC_SCHED_DB_WEIGHT_INFO_UPDATE \
  "UPDATE " \
  "  weight_info " \
  "SET " \
  "  weight=?, " \
  "  update_time=? " \
  "WHERE " \
  "  priority=?;"

static sqlite3 *sqc_rpc_sched_db = NULL;
static sqc_mutex_t db_lck = NULL;


static inline sqc_result_t
s_dbmgr_user_info_create_from_db(const char *user_id,
                                 const sqc_rpc_sched_user_role_type_t role_type,
                                 const sqc_rpc_sched_user_status_t status,
                                 const sqc_chrono_t created_time,
                                 const sqc_chrono_t update_time,
                                 dbmgr_user_info_t *ui_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  if (likely(ui_ptr != NULL && IS_VALID_STRING(user_id) == true)) {
    size_t len = strlen(user_id);
    if (len > (SQC_RPC_SCHED_USER_ID_MAX_SIZE)) {
      return SQC_RESULT_TOO_LONG;
    }

    *ui_ptr = (dbmgr_user_info_t) malloc(sizeof(dbmgr_user_info_record));
    if (*ui_ptr != NULL) {
      (*ui_ptr)->rwlck_ = NULL;
      if ((rc = sqc_rwlock_create(&((*ui_ptr)->rwlck_))) != SQC_RESULT_OK) {
        goto error;
      }

      memcpy((*ui_ptr)->user_id, user_id, len);
      (*ui_ptr)->user_id[len] = '\0';

      (*ui_ptr)->user_id_len = len;

      (*ui_ptr)->role_type = role_type;

      (*ui_ptr)->status = status;

      (*ui_ptr)->created_time = created_time;

      (*ui_ptr)->update_time = update_time;

      return SQC_RESULT_OK;
    } else {
      rc = SQC_RESULT_NO_MEMORY;
      goto error;
    }
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }

error:
  if (*ui_ptr != NULL) {
    if ((*ui_ptr)->rwlck_ != NULL) {
      sqc_rwlock_destroy(&(*ui_ptr)->rwlck_);
    }
    free((void *) *ui_ptr);
    *ui_ptr = NULL;
  }
  return rc;
}


static inline void
s_dbmgr_user_info_destroy(dbmgr_user_info_t ui_ptr) {
  if (ui_ptr != NULL) {
    if (ui_ptr->rwlck_ != NULL) {
      (void)sqc_rwlock_destroy(&(ui_ptr->rwlck_));
    }
  }

  free((void *) ui_ptr);
}


static inline sqc_result_t
s_dbmgr_group_info_create_from_db(const char *group_id,
                                  const uint64_t exec_time_limit_msec,
                                  const uint64_t exec_time_total_msec,
                                  const sqc_chrono_t created_time,
                                  const sqc_chrono_t update_time,
                                  dbmgr_group_info_t *gi_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  if (likely(gi_ptr != NULL && IS_VALID_STRING(group_id) == true)) {
    size_t len = strlen(group_id);
    if (len > (SQC_RPC_SCHED_GROUP_ID_MAX_SIZE)) {
      return SQC_RESULT_TOO_LONG;
    }

    *gi_ptr = (dbmgr_group_info_t) malloc(sizeof(dbmgr_group_info_record));
    if (*gi_ptr != NULL) {
      (*gi_ptr)->rwlck_ = NULL;
      if ((rc = sqc_rwlock_create(&((*gi_ptr)->rwlck_))) != SQC_RESULT_OK) {
        goto error;
      }

      memcpy((*gi_ptr)->group_id, group_id, len);
      (*gi_ptr)->group_id[len] = '\0';

      (*gi_ptr)->group_id_len = len;

      (*gi_ptr)->exec_time_limit_msec = exec_time_limit_msec;

      (*gi_ptr)->exec_time_total_msec = exec_time_total_msec;

      (*gi_ptr)->created_time = created_time;

      (*gi_ptr)->update_time = update_time;

      return SQC_RESULT_OK;
    } else {
      rc = SQC_RESULT_NO_MEMORY;
      goto error;
    }
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }

error:
  if (*gi_ptr != NULL) {
    if ((*gi_ptr)->rwlck_ != NULL) {
      sqc_rwlock_destroy(&(*gi_ptr)->rwlck_);
    }
    free((void *) *gi_ptr);
    *gi_ptr = NULL;
  }
  return rc;
}


static inline void
s_dbmgr_group_info_destroy(dbmgr_group_info_t gi_ptr) {
  if (gi_ptr != NULL) {
    if (gi_ptr->rwlck_ != NULL) {
      (void)sqc_rwlock_destroy(&(gi_ptr->rwlck_));
    }
  }

  free((void *) gi_ptr);
}


static inline sqc_result_t
s_dbmgr_user_group_info_create_from_db(const char *user_id, const char *group_id,
                                       const sqc_rpc_sched_user_group_status_t status,
                                       const sqc_chrono_t created_time,
                                       const sqc_chrono_t update_time,
                                       dbmgr_user_group_info_t *ugi_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  size_t user_id_len, group_id_len;

  if (likely(ugi_ptr != NULL && IS_VALID_STRING(user_id) == true && IS_VALID_STRING(group_id)) == true) {
    user_id_len = strlen(user_id);
    if (user_id_len > (SQC_RPC_SCHED_USER_ID_MAX_SIZE)) {
      return SQC_RESULT_TOO_LONG;
    }

    group_id_len = strlen(group_id);
    if (group_id_len > (SQC_RPC_SCHED_GROUP_ID_MAX_SIZE)) {
      return SQC_RESULT_TOO_LONG;
    }

    *ugi_ptr = (dbmgr_user_group_info_t) malloc(sizeof(dbmgr_user_group_info_record));
    if (*ugi_ptr != NULL) {
      (*ugi_ptr)->rwlck_ = NULL;
      if ((rc = sqc_rwlock_create(&((*ugi_ptr)->rwlck_))) != SQC_RESULT_OK) {
        goto error;
      }

      rc = dbmgr_util_create_user_group_key((*ugi_ptr)->user_group_key,
                                            sizeof((*ugi_ptr)->user_group_key),
                                            user_id, group_id,
                                            &(*ugi_ptr)->user_group_key_len);
      if (rc != SQC_RESULT_OK) {
        goto error;
      }

      memcpy((*ugi_ptr)->user_id, user_id, user_id_len);
      (*ugi_ptr)->user_id[user_id_len] = '\0';

      (*ugi_ptr)->user_id_len = user_id_len;

      memcpy((*ugi_ptr)->group_id, group_id, group_id_len);
      (*ugi_ptr)->group_id[group_id_len] = '\0';

      (*ugi_ptr)->group_id_len = group_id_len;

      (*ugi_ptr)->status = status;

      (*ugi_ptr)->created_time = created_time;

      (*ugi_ptr)->update_time = update_time;

      return SQC_RESULT_OK;
    } else {
      rc = SQC_RESULT_NO_MEMORY;
      goto error;
    }
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }

error:
  if (*ugi_ptr != NULL) {
    if ((*ugi_ptr)->rwlck_ != NULL) {
      sqc_rwlock_destroy(&(*ugi_ptr)->rwlck_);
    }
    free((void *) *ugi_ptr);
    *ugi_ptr = NULL;
  }
  return rc;
}


static inline void
s_dbmgr_user_group_info_destroy(dbmgr_user_group_info_t ugi_ptr) {
  if (ugi_ptr != NULL) {
    if (ugi_ptr->rwlck_ != NULL) {
      (void)sqc_rwlock_destroy(&(ugi_ptr->rwlck_));
    }
  }

  free((void *) ugi_ptr);
}


static inline sqc_result_t
s_dbmgr_job_info_create_from_db(const char *job_id, const char *user_id, const char *group_id,
                                const uint8_t priority, const sqc_rpc_sched_job_status_t status,
                                const char *qc_job_id,
                                const char *qprogram, sqc_rpc_sched_circuit_fmt_t circuit_fmt,
                                size_t shots, sqc_rpc_sched_qc_type_t qc_type,
                                sqc_rpc_sched_transpiler_t transpiler, const char *remark,
                                const char *result,
                                uint64_t exec_time_estimate_msec, uint64_t exec_time_msec,
                                const sqc_chrono_t created_time,
                                const sqc_chrono_t queued_time, const sqc_chrono_t running_time,
                                const sqc_chrono_t done_time, const sqc_chrono_t cancelled_time,
                                const sqc_chrono_t error_time, const sqc_chrono_t deleted_time,
                                const sqc_chrono_t update_time, dbmgr_job_info_t *ji_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  size_t job_id_len, user_id_len, group_id_len, qc_job_id_len, qprogram_len, remark_len, result_len;

  if (likely(ji_ptr != NULL &&
             job_id != NULL && IS_VALID_STRING(job_id) == true &&
             user_id != NULL && IS_VALID_STRING(user_id) == true &&
             group_id != NULL && IS_VALID_STRING(group_id) == true &&
             priority <= SQC_RPC_SCHED_MAX_PRIORITY &&
             remark != NULL && IS_VALID_STRING(remark) == true)) {
    *ji_ptr = (dbmgr_job_info_t) malloc(sizeof(dbmgr_job_info_record));
    if (*ji_ptr != NULL) {
      (*ji_ptr)->rwlck_ = NULL;
      if ((rc = sqc_rwlock_create(&((*ji_ptr)->rwlck_))) != SQC_RESULT_OK) {
        goto error;
      }

      job_id_len = strlen(job_id);
      if (job_id_len > (SQC_RPC_SCHED_JOB_ID_MAX_SIZE)) {
        rc = SQC_RESULT_TOO_LONG;
        goto error;
      }
      memcpy((*ji_ptr)->job_id, job_id, job_id_len);
      (*ji_ptr)->job_id[job_id_len] = '\0';
      (*ji_ptr)->job_id_len = job_id_len;

      user_id_len = strlen(user_id);
      if (user_id_len > (SQC_RPC_SCHED_USER_ID_MAX_SIZE)) {
        rc = SQC_RESULT_TOO_LONG;
        goto error;
      }
      memcpy((*ji_ptr)->user_id, user_id, user_id_len);
      (*ji_ptr)->user_id[user_id_len] = '\0';
      (*ji_ptr)->user_id_len = user_id_len;

      group_id_len = strlen(group_id);
      if (group_id_len > (SQC_RPC_SCHED_GROUP_ID_MAX_SIZE)) {
        rc = SQC_RESULT_TOO_LONG;
        goto error;
      }
      memcpy((*ji_ptr)->group_id, group_id, group_id_len);
      (*ji_ptr)->group_id[group_id_len] = '\0';
      (*ji_ptr)->group_id_len = group_id_len;

      (*ji_ptr)->priority = priority;

      (*ji_ptr)->status = status;

      if (qc_job_id == NULL) {
        (*ji_ptr)->qc_job_id[0] = '\0';
        (*ji_ptr)->qc_job_id_len = 0;
      } else {
        qc_job_id_len = strlen(qc_job_id);
        if (qc_job_id_len > (SQC_RPC_SCHED_QC_JOB_ID_MAX_SIZE)) {
          rc = SQC_RESULT_TOO_LONG;
          goto error;
        }
        memcpy((*ji_ptr)->qc_job_id, qc_job_id, qc_job_id_len);
        (*ji_ptr)->qc_job_id[qc_job_id_len] = '\0';
        (*ji_ptr)->qc_job_id_len = qc_job_id_len;
      }

      if (qprogram == NULL) {
        (*ji_ptr)->qprogram[0] = '\0';
        (*ji_ptr)->qprogram_len = 0;
      } else {
        qprogram_len = strlen(qprogram);
        if (qprogram_len > (SQC_RPC_SCHED_QPROGRAM_MAX_SIZE)) {
          rc = SQC_RESULT_TOO_LONG;
          goto error;
        }
        memcpy((*ji_ptr)->qprogram, qprogram, qprogram_len);
        (*ji_ptr)->qprogram[qprogram_len] = '\0';
        (*ji_ptr)->qprogram_len = qprogram_len;
      }

      if (circuit_fmt == 0) {
        (*ji_ptr)->circuit_fmt = SQC_RPC_SCHED_CIRCUIT_FMT_UNKNOWN;
      } else {
        (*ji_ptr)->circuit_fmt = circuit_fmt;
      }

      (*ji_ptr)->shots = shots;

      (*ji_ptr)->qc_type = qc_type;

      (*ji_ptr)->transpiler = transpiler;

      remark_len = strlen(remark);
      if (remark_len > (SQC_RPC_SCHED_REMARK_MAX_SIZE)) {
        rc = SQC_RESULT_TOO_LONG;
        goto error;
      }
      memcpy((*ji_ptr)->remark, remark, remark_len);
      (*ji_ptr)->remark[remark_len] = '\0';
      (*ji_ptr)->remark_len = remark_len;

      if (result == NULL) {
        (*ji_ptr)->result[0] = '\0';
        (*ji_ptr)->result_len = 0;
      } else {
        result_len = strlen(result);
        if (result_len > (SQC_RPC_SCHED_RESULT_MAX_SIZE)) {
          rc = SQC_RESULT_TOO_LONG;
          goto error;
        }
        memcpy((*ji_ptr)->result, result, result_len);
        (*ji_ptr)->result[result_len] = '\0';
        (*ji_ptr)->result_len = result_len;
      }

      // user_token is not persistent
      (*ji_ptr)->user_token[0] = '\0';
      (*ji_ptr)->user_token_len = 0;

      (*ji_ptr)->exec_time_estimate_msec = exec_time_estimate_msec;
      (*ji_ptr)->exec_time_msec = exec_time_msec;

      (*ji_ptr)->created_time = created_time;

      if (queued_time == 0) {
        (*ji_ptr)->queued_time = 0;
      } else {
        (*ji_ptr)->queued_time = queued_time;
      }

      if (running_time == 0) {
        (*ji_ptr)->running_time = 0;
      } else {
        (*ji_ptr)->running_time = running_time;
      }

      if (done_time == 0) {
        (*ji_ptr)->done_time = 0;
      } else {
        (*ji_ptr)->done_time = done_time;
      }

      if (cancelled_time == 0) {
        (*ji_ptr)->cancelled_time = 0;
      } else {
        (*ji_ptr)->cancelled_time = cancelled_time;
      }

      if (error_time == 0) {
        (*ji_ptr)->error_time = 0;
      } else {
        (*ji_ptr)->error_time = error_time;
      }

      if (deleted_time == 0) {
        (*ji_ptr)->deleted_time = 0;
      } else {
        (*ji_ptr)->deleted_time = deleted_time;
      }

      (*ji_ptr)->update_time = update_time;

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
s_dbmgr_job_info_destroy(dbmgr_job_info_t ji_ptr) {
  if (ji_ptr != NULL) {
    if (ji_ptr->rwlck_ != NULL) {
      (void)sqc_rwlock_destroy(&(ji_ptr->rwlck_));
    }
  }

  free((void *) ji_ptr);
}


static inline sqc_result_t
s_dbmgr_weight_info_create_from_db(const uint8_t priority, const uint64_t weight,
                                   const sqc_chrono_t created_time, const sqc_chrono_t update_time,
                                   dbmgr_weight_info_t *wi_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  if (likely(wi_ptr != NULL)) {
    *wi_ptr = (dbmgr_weight_info_t) malloc(sizeof(dbmgr_weight_info_record));
    if (*wi_ptr != NULL) {
      (*wi_ptr)->rwlck_ = NULL;
      if ((rc = sqc_rwlock_create(&((*wi_ptr)->rwlck_))) != SQC_RESULT_OK) {
        goto error;
      }

      (*wi_ptr)->priority = (uint8_t)priority;

      (*wi_ptr)->weight = (uint64_t)weight;

      (*wi_ptr)->created_time = created_time;
      (*wi_ptr)->update_time = update_time;

      return SQC_RESULT_OK;
    } else {
      rc = SQC_RESULT_NO_MEMORY;
      goto error;
    }
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }

error:
  if (*wi_ptr != NULL) {
    if ((*wi_ptr)->rwlck_ != NULL) {
      sqc_rwlock_destroy(&(*wi_ptr)->rwlck_);
    }
    free((void *) *wi_ptr);
    *wi_ptr = NULL;
  }
  return rc;
}

static inline void
s_dbmgr_weight_info_destroy(dbmgr_weight_info_t wi_ptr) {
  if (wi_ptr != NULL) {
    if (wi_ptr->rwlck_ != NULL) {
      (void)sqc_rwlock_destroy(&(wi_ptr->rwlck_));
    }
  }

  free((void *) wi_ptr);
}


static inline void
s_lock_db(void) {
  if (likely(db_lck != NULL)) {
    (void)sqc_mutex_lock(&db_lck);
  }
}


static inline void
s_unlock_db(void) {
  if (likely(db_lck != NULL)) {
    (void)sqc_mutex_unlock(&db_lck);
  }
}


/*
 * callback function for busy handler
 */
static int
s_dbmgr_db_busy_handler(void *ptr, int count) {
  (void) ptr;

  if (count >= 10) {
    printf("DB is busy and timed out\n");
    return 0; // SQLITE_BUSY error
  }
  printf("DB is busy. retrying: %d\n", count);
  return 1; // continue retry
}


static inline sqc_result_t
s_dbmgr_db_initialize(const char *db_file) {
  sqc_result_t rc;

  if (access(db_file, F_OK) != 0) {
    sqc_msg_error("DB file does not exist: %s\n", db_file);
    return SQC_RESULT_NOT_FOUND;
  }

  rc = sqc_mutex_create(&db_lck);
  if (rc != SQC_RESULT_OK) {
    sqc_msg_error("Failed to create lock: %s\n", sqc_error_get_string(rc));
    return rc;
  }

  if (sqlite3_open(db_file, &sqc_rpc_sched_db) != SQLITE_OK) {
    sqc_msg_error("Failed to open db: %s\n", sqlite3_errmsg(sqc_rpc_sched_db));
    sqlite3_close(sqc_rpc_sched_db);
    sqc_rpc_sched_db = NULL;
    sqc_mutex_destroy(&db_lck);
    db_lck = NULL;
    return SQC_RESULT_ANY_RUNTIME_ERROR;
  }

  sqlite3_busy_handler(sqc_rpc_sched_db, s_dbmgr_db_busy_handler, NULL);
  sqlite3_busy_timeout(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_TIMEOUT);
  return SQC_RESULT_OK;
}


static inline void
s_dbmgr_db_finalize(void) {
  int rc = sqlite3_close(sqc_rpc_sched_db);
  if (rc != SQLITE_OK) {
    sqc_msg_warning("Failed to close db: %s\n", sqlite3_errmsg(sqc_rpc_sched_db));
  }

  sqc_mutex_destroy(&db_lck);

  sqc_rpc_sched_db = NULL;
  db_lck = NULL;
}


/*
 * user information
 */
// For Unit Test Use Only
static inline sqc_result_t
s_dbmgr_db_user_info_select_all(dbmgr_user_info_t **ui_arr_ptr, int *count, size_t capacity) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    *count = 0;
    *ui_arr_ptr = (dbmgr_user_info_t *) malloc(sizeof(dbmgr_user_info_t) * capacity);
    if (*ui_arr_ptr == NULL) {
      rc = SQC_RESULT_NO_MEMORY;
      goto unlock;
    }

    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_USER_INFO_SELECT_ALL, -1, &stmt, NULL);
    while ((db_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
      const unsigned char *user_id = sqlite3_column_text(stmt, 0);
      const int role_type = sqlite3_column_int(stmt, 1);
      const int status = sqlite3_column_int(stmt, 2);
      const int64_t created_time = sqlite3_column_int64(stmt, 3);
      const int64_t update_time = sqlite3_column_int64(stmt, 4);
      rc = s_dbmgr_user_info_create_from_db((const char *)user_id, role_type, status,
                                            created_time, update_time, &(*ui_arr_ptr)[*count]);
      if (rc != SQC_RESULT_OK) {
        sqc_msg_error("Failed to create user_info record: user_id=%s, err_msg=%s\n",
                      user_id, sqc_error_get_string(rc));
        rc = SQC_RESULT_ANY_RUNTIME_ERROR;
        break;
      }

      (*count)++;
    }

    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to SELECT user_info record: %s\n", sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
unlock:
  s_unlock_db();

  return rc;
}


// For Unit Test Use Only
static inline sqc_result_t
s_dbmgr_db_user_info_select(const char *target_user_id, dbmgr_user_info_t *ui_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_USER_INFO_SELECT, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, target_user_id, -1, SQLITE_STATIC);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_ROW) {
      // Only one record can be retrieved
      // because the primary key is specified in the condition
      const unsigned char *user_id = sqlite3_column_text(stmt, 0);
      const int role_type = sqlite3_column_int(stmt, 1);
      const int status = sqlite3_column_int(stmt, 2);
      const int64_t created_time = sqlite3_column_int64(stmt, 3);
      const int64_t update_time = sqlite3_column_int64(stmt, 4);
      rc = s_dbmgr_user_info_create_from_db((const char *)user_id, role_type, status,
                                            created_time, update_time, ui_ptr);
      if (rc != SQC_RESULT_OK) {
        sqc_msg_error("Failed to create user_info record: user_id=%s, err_msg=%s\n",
                      user_id, sqc_error_get_string(rc));
      }
    } else if (db_rc == SQLITE_DONE) {
      sqc_msg_info("user_info record not found: %s\n", target_user_id);
      rc = SQC_RESULT_NOT_FOUND;
    } else {
      sqc_msg_error("Failed to SELECT user_info record: %s\n", sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


// For Unit Test Use Only
static inline sqc_result_t
s_dbmgr_db_user_info_select_count(int *count) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_USER_INFO_SELECT_COUNT, -1, &stmt, NULL);
    db_rc = sqlite3_step(stmt);

    if (db_rc == SQLITE_ROW) {
      rc = SQC_RESULT_OK;
      *count = sqlite3_column_int(stmt, 0);
    } else {
      sqc_msg_error("Failed to SELECT user_info record(count): %s\n", sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_user_info_deserialize_all(sqc_hashmap_t *user_info_hashmap) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_USER_INFO_SELECT_ALL, -1, &stmt, NULL);
    while ((db_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
      dbmgr_user_info_t ui_ptr;
      const unsigned char *user_id = sqlite3_column_text(stmt, 0);
      const int role_type = sqlite3_column_int(stmt, 1);
      const int status = sqlite3_column_int(stmt, 2);
      const int64_t created_time = sqlite3_column_int64(stmt, 3);
      const int64_t update_time = sqlite3_column_int64(stmt, 4);
      rc = s_dbmgr_user_info_create_from_db((const char *)user_id, role_type, status,
                                            created_time, update_time, &ui_ptr);
      if (rc == SQC_RESULT_OK) {
        sqc_msg_debug(5, "DB(user_info) -> Hashmap user_id=%s\n", ui_ptr->user_id);
      } else {
        sqc_msg_error("Failed to create user_info record: user_id=%s, err_msg=%s\n",
                      user_id, sqc_error_get_string(rc));
        rc = SQC_RESULT_ANY_RUNTIME_ERROR;
        break;
      }

      rc = sqc_hashmap_add(user_info_hashmap, (char *) (ui_ptr->user_id),
                                              (void **) &ui_ptr, false);
      if (rc != SQC_RESULT_OK) {
        sqc_msg_error("Failed to add to user Hashmap: user_id=%s\n", ui_ptr->user_id);
        s_dbmgr_user_info_destroy(ui_ptr);
        ui_ptr = NULL;
        rc = SQC_RESULT_ANY_RUNTIME_ERROR;
        break;
      }
    }

    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to SELECT user_info record: %s\n", sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_user_info_insert_record(const char *user_id, const uint8_t role_type, const uint8_t status,
                                   const int64_t created_time, const int64_t update_time) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_USER_INFO_INSERT, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, user_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, role_type);
    sqlite3_bind_int(stmt, 3, status);
    sqlite3_bind_int64(stmt, 4, created_time);
    sqlite3_bind_int64(stmt, 5, update_time);
    db_rc = sqlite3_step(stmt);

    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to INSERT user_info record: user_id=%s, err_msg=%s\n",
                    user_id, sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_user_info_update_role_type(const char *user_id, const uint8_t role_type,
                                      const int64_t update_time) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_USER_INFO_UPDATE_ROLE_TYPE, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, role_type);
    sqlite3_bind_int64(stmt, 2, update_time);
    sqlite3_bind_text(stmt, 3, user_id, -1, SQLITE_STATIC);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to UPDATE user role type: user_id=%s, err_msg=%s\n",
                    user_id, sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_user_info_update_status(const char *user_id, const uint8_t status,
                                   const int64_t update_time) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_USER_INFO_UPDATE_STATUS, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, status);
    sqlite3_bind_int64(stmt, 2, update_time);
    sqlite3_bind_text(stmt, 3, user_id, -1, SQLITE_STATIC);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to UPDATE user status: user_id=%s, err_msg=%s\n",
                    user_id, sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


/*
 * group information
 */
// For Unit Test Use Only
static inline sqc_result_t
s_dbmgr_db_group_info_select_all(dbmgr_group_info_t **gi_arr_ptr, int *count, size_t capacity) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    *count = 0;
    *gi_arr_ptr = (dbmgr_group_info_t *) malloc(sizeof(dbmgr_group_info_t) * capacity);
    if (*gi_arr_ptr == NULL) {
      rc = SQC_RESULT_NO_MEMORY;
      goto unlock;
    }

    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_GROUP_INFO_SELECT_ALL, -1, &stmt, NULL);
    while ((db_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
      const unsigned char *group_id = sqlite3_column_text(stmt, 0);
      const int64_t exec_time_limit_msec = sqlite3_column_int64(stmt, 1);
      const int64_t exec_time_total_msec = sqlite3_column_int64(stmt, 2);
      const int64_t created_time = sqlite3_column_int64(stmt, 3);
      const int64_t update_time = sqlite3_column_int64(stmt, 4);
      rc = s_dbmgr_group_info_create_from_db((const char *)group_id,
                                             (uint64_t)exec_time_limit_msec, (uint64_t)exec_time_total_msec,
                                             created_time, update_time, &(*gi_arr_ptr)[*count]);
      if (rc != SQC_RESULT_OK) {
        sqc_msg_error("Failed to create group_info record: group_id=%s, err_msg=%s\n",
                      group_id, sqc_error_get_string(rc));
        rc = SQC_RESULT_ANY_RUNTIME_ERROR;
        break;
      }

      (*count)++;
    }

    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to SELECT group_info record: %s\n", sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
unlock:
  s_unlock_db();

  return rc;
}


// For Unit Test Use Only
static inline sqc_result_t
s_dbmgr_db_group_info_select(const char *target_group_id, dbmgr_group_info_t *gi_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_GROUP_INFO_SELECT, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, target_group_id, -1, SQLITE_STATIC);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_ROW) {
      // Only one record can be retrieved
      // because the primary key is specified in the condition
      const unsigned char *group_id = sqlite3_column_text(stmt, 0);
      const int64_t exec_time_limit_msec = sqlite3_column_int64(stmt, 1);
      const int64_t exec_time_total_msec = sqlite3_column_int64(stmt, 2);
      const int64_t created_time = sqlite3_column_int64(stmt, 3);
      const int64_t update_time = sqlite3_column_int64(stmt, 4);
      rc = s_dbmgr_group_info_create_from_db((const char *)group_id,
                                             (uint64_t)exec_time_limit_msec, (uint64_t)exec_time_total_msec,
                                             created_time, update_time, gi_ptr);
      if (rc != SQC_RESULT_OK) {
        sqc_msg_error("Failed to create group_info record: group_id=%s, err_msg=%s\n",
                      group_id, sqc_error_get_string(rc));
      }
    } else if (db_rc == SQLITE_DONE) {
      sqc_msg_info("group_info record not found: %s\n", target_group_id);
      rc = SQC_RESULT_NOT_FOUND;
    } else {
      sqc_msg_error("Failed to SELECT group_info record: %s\n", sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


// For Unit Test Use Only
static inline sqc_result_t
s_dbmgr_db_group_info_select_count(int *count) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_GROUP_INFO_SELECT_COUNT, -1, &stmt, NULL);
    db_rc = sqlite3_step(stmt);

    if (db_rc == SQLITE_ROW) {
      rc = SQC_RESULT_OK;
      *count = sqlite3_column_int(stmt, 0);
    } else {
      sqc_msg_error("Failed to SELECT group_info record(count): %s\n", sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_group_info_deserialize_all(sqc_hashmap_t *group_info_hashmap) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_GROUP_INFO_SELECT_ALL, -1, &stmt, NULL);
    while ((db_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
      dbmgr_group_info_t gi_ptr;
      const unsigned char *group_id = sqlite3_column_text(stmt, 0);
      const int64_t exec_time_limit_msec = sqlite3_column_int64(stmt, 1);
      const int64_t exec_time_total_msec = sqlite3_column_int64(stmt, 2);
      const int64_t created_time = sqlite3_column_int64(stmt, 3);
      const int64_t update_time = sqlite3_column_int64(stmt, 4);
      rc = s_dbmgr_group_info_create_from_db((const char *)group_id,
                                             (uint64_t)exec_time_limit_msec, (uint64_t)exec_time_total_msec,
                                             created_time, update_time, &gi_ptr);
      if (rc == SQC_RESULT_OK) {
        sqc_msg_debug(5, "DB(group_info) -> Hashmap group_id=%s\n", gi_ptr->group_id);
      } else {
        sqc_msg_error("Failed to create group_info record: group_id=%s, err_msg=%s\n",
                      group_id, sqc_error_get_string(rc));
        rc = SQC_RESULT_ANY_RUNTIME_ERROR;
        break;
      }

      rc = sqc_hashmap_add(group_info_hashmap, (char *) (gi_ptr->group_id),
                                               (void **) &gi_ptr, false);
      if (rc != SQC_RESULT_OK) {
        sqc_msg_error("Failed to add to group Hashmap: group_id=%s\n", gi_ptr->group_id);
        s_dbmgr_group_info_destroy(gi_ptr);
        gi_ptr = NULL;
        rc = SQC_RESULT_ANY_RUNTIME_ERROR;
        break;
      }
    }

    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to SELECT group_info record: %s\n", sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_group_info_insert_record(const char *group_id, const uint64_t exec_time_limit_msec,
                                    const uint64_t exec_time_total_msec,
                                    const int64_t created_time, const int64_t update_time) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_GROUP_INFO_INSERT, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, group_id, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, (int64_t)exec_time_limit_msec);
    sqlite3_bind_int64(stmt, 3, (int64_t)exec_time_total_msec);
    sqlite3_bind_int64(stmt, 4, created_time);
    sqlite3_bind_int64(stmt, 5, update_time);
    db_rc = sqlite3_step(stmt);

    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to INSERT group_id record: group_id=%s, err_msg=%s\n",
                    group_id, sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_group_info_update_exec_time_limit_msec(const char *group_id,
                                                  const uint64_t exec_time_limit_msec,
                                                  const int64_t update_time) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_GROUP_INFO_UPDATE_LIMIT, -1, &stmt, NULL);
    sqlite3_bind_int64(stmt, 1, (int64_t)exec_time_limit_msec);
    sqlite3_bind_int64(stmt, 2, update_time);
    sqlite3_bind_text(stmt, 3, group_id, -1, SQLITE_STATIC);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to UPDATE group exec_time_limit_msec: group_id=%s, err_msg=%s\n",
                    group_id, sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_group_info_update_exec_time_total_msec(const char *group_id,
                                                  const uint64_t exec_time_total_msec,
                                                  const int64_t update_time) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_GROUP_INFO_UPDATE_TOTAL, -1, &stmt, NULL);
    sqlite3_bind_int64(stmt, 1, (int64_t)exec_time_total_msec);
    sqlite3_bind_int64(stmt, 2, update_time);
    sqlite3_bind_text(stmt, 3, group_id, -1, SQLITE_STATIC);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to UPDATE group exec_time_total_msec: group_id=%s, err_msg=%s\n",
                    group_id, sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


/*
 * user-group information
 */
// For Unit Test Use Only
static inline sqc_result_t
s_dbmgr_db_user_group_info_select_all(dbmgr_user_group_info_t **ugi_arr_ptr, int *count, size_t capacity) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    *count = 0;
    *ugi_arr_ptr = (dbmgr_user_group_info_t *) malloc(sizeof(dbmgr_user_group_info_t) * capacity);
    if (*ugi_arr_ptr == NULL) {
      rc = SQC_RESULT_NO_MEMORY;
      goto unlock;
    }

    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_USER_GROUP_INFO_SELECT_ALL, -1, &stmt, NULL);
    while ((db_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
      const unsigned char *user_id = sqlite3_column_text(stmt, 0);
      const unsigned char *group_id = sqlite3_column_text(stmt, 1);
      const int status = sqlite3_column_int(stmt, 2);
      const int64_t created_time = sqlite3_column_int64(stmt, 3);
      const int64_t update_time = sqlite3_column_int64(stmt, 4);
      rc = s_dbmgr_user_group_info_create_from_db((const char *)user_id, (const char *)group_id,
                                                  status,
                                                  created_time, update_time, &(*ugi_arr_ptr)[*count]);
      if (rc != SQC_RESULT_OK) {
        sqc_msg_error("Failed to create user_group_info record: user_id=%s, group_id=%s, err_msg=%s\n",
                      user_id, group_id, sqc_error_get_string(rc));
        rc = SQC_RESULT_ANY_RUNTIME_ERROR;
        break;
      }

      (*count)++;
    }

    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to SELECT user_group_info record: %s\n", sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
unlock:
  s_unlock_db();

  return rc;
}


// For Unit Test Use Only
static inline sqc_result_t
s_dbmgr_db_user_group_info_select(const char *target_user_id, const char *target_group_id,
                                  dbmgr_user_group_info_t *ugi_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_USER_GROUP_INFO_SELECT, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, target_user_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, target_group_id, -1, SQLITE_STATIC);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_ROW) {
      // Only one record can be retrieved
      // because the primary key is specified in the condition
      const unsigned char *user_id = sqlite3_column_text(stmt, 0);
      const unsigned char *group_id = sqlite3_column_text(stmt, 1);
      const int status = sqlite3_column_int(stmt, 2);
      const int64_t created_time = sqlite3_column_int64(stmt, 3);
      const int64_t update_time = sqlite3_column_int64(stmt, 4);
      rc = s_dbmgr_user_group_info_create_from_db((const char *)user_id, (const char *)group_id,
                                                  status,
                                                  created_time, update_time, ugi_ptr);
      if (rc != SQC_RESULT_OK) {
        sqc_msg_error("Failed to create user_group_info record: user_id=%s, group_id=%s, err_msg=%s\n",
                      user_id, group_id, sqc_error_get_string(rc));
      }
    } else if (db_rc == SQLITE_DONE) {
      sqc_msg_info("user_group_info record not found: %s\n", target_user_id);
      rc = SQC_RESULT_NOT_FOUND;
    } else {
      sqc_msg_error("Failed to SELECT user_group_info record: %s\n", sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


// For Unit Test Use Only
static inline sqc_result_t
s_dbmgr_db_user_group_info_select_count(int *count) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_USER_GROUP_INFO_SELECT_COUNT, -1, &stmt, NULL);
    db_rc = sqlite3_step(stmt);

    if (db_rc == SQLITE_ROW) {
      rc = SQC_RESULT_OK;
      *count = sqlite3_column_int(stmt, 0);
    } else {
      sqc_msg_error("Failed to SELECT user_group_info record(count): %s\n", sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_user_group_info_deserialize_all(sqc_hashmap_t *user_group_info_hashmap) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_USER_GROUP_INFO_SELECT_ALL, -1, &stmt, NULL);
    while ((db_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
      dbmgr_user_group_info_t ugi_ptr;
      const unsigned char *user_id = sqlite3_column_text(stmt, 0);
      const unsigned char *group_id = sqlite3_column_text(stmt, 1);
      const int status = sqlite3_column_int(stmt, 2);
      const int64_t created_time = sqlite3_column_int64(stmt, 3);
      const int64_t update_time = sqlite3_column_int64(stmt, 4);
      rc = s_dbmgr_user_group_info_create_from_db((const char *)user_id, (const char *)group_id, status,
                                                  created_time, update_time, &ugi_ptr);
      if (rc == SQC_RESULT_OK) {
        sqc_msg_debug(5, "DB(user_group_info) -> Hashmap user_id=%s, group_id=%s\n",
                      ugi_ptr->user_id, ugi_ptr->group_id);
      } else {
        sqc_msg_error("Failed to create user_group_info record: user_id=%s, group_id=%s, err_msg=%s\n",
                      user_id, group_id, sqc_error_get_string(rc));
        rc = SQC_RESULT_ANY_RUNTIME_ERROR;
        break;
      }

      rc = sqc_hashmap_add(user_group_info_hashmap, (char *) (ugi_ptr->user_group_key),
                                                    (void **) &ugi_ptr, false);
      if (rc != SQC_RESULT_OK) {
        sqc_msg_error("Failed to add to user_group Hashmap: user_id=%s, group_id=%s\n",
                      ugi_ptr->user_id, ugi_ptr->group_id);
        s_dbmgr_user_group_info_destroy(ugi_ptr);
        ugi_ptr = NULL;
        rc = SQC_RESULT_ANY_RUNTIME_ERROR;
        break;
      }
    }

    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to SELECT user_group_info record: %s\n", sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}



static inline sqc_result_t
s_dbmgr_db_user_group_info_insert_record(const char *user_id, const char *group_id,
                                         const uint8_t status, const int64_t created_time,
                                         const int64_t update_time) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_USER_GROUP_INFO_INSERT, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, user_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, group_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, status);
    sqlite3_bind_int64(stmt, 4, created_time);
    sqlite3_bind_int64(stmt, 5, update_time);
    db_rc = sqlite3_step(stmt);

    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to INSERT user_group_info record: user_id=%s, group_id=%s, err_msg=%s\n",
                    user_id, group_id, sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_user_group_info_update_status(const char *user_id, const char *group_id,
                                         const uint8_t status, const int64_t update_time) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_USER_GROUP_INFO_UPDATE_STATUS, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, status);
    sqlite3_bind_int64(stmt, 2, update_time);
    sqlite3_bind_text(stmt, 3, user_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, group_id, -1, SQLITE_STATIC);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to UPDATE user_group status: user_id=%s, group_id=%s, err_msg=%s\n",
                    user_id, group_id, sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


/**
 * Get all job information
 */
// For Unit Test Use Only
static inline sqc_result_t
s_dbmgr_db_job_info_select_all(dbmgr_job_info_t **ji_arr_ptr, int *count, size_t capacity) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    *count = 0;
    *ji_arr_ptr = (dbmgr_job_info_t *) malloc(sizeof(dbmgr_job_info_t) * capacity);
    if (*ji_arr_ptr == NULL) {
      rc = SQC_RESULT_NO_MEMORY;
      goto unlock;
    }

    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_JOB_INFO_SELECT_ALL, -1, &stmt, NULL);
    while ((db_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
      const unsigned char *job_id = sqlite3_column_text(stmt, 0);
      const unsigned char *user_id = sqlite3_column_text(stmt, 1);
      const unsigned char *group_id = sqlite3_column_text(stmt, 2);
      int priority = sqlite3_column_int(stmt, 3);
      int status = sqlite3_column_int(stmt, 4);
      const unsigned char *qc_job_id = sqlite3_column_text(stmt, 5);
      const unsigned char *qprogram = sqlite3_column_text(stmt, 6);
      int circuit_fmt = sqlite3_column_int(stmt, 7);
      int shots = sqlite3_column_int(stmt, 8);
      int qc_type = sqlite3_column_int(stmt, 9);
      int transpiler = sqlite3_column_int(stmt, 10);
      const unsigned char *remark = sqlite3_column_text(stmt, 11);
      const unsigned char *result = sqlite3_column_text(stmt, 12);
      int64_t exec_time_estimate_msec = sqlite3_column_int64(stmt, 13);
      int64_t exec_time_msec = sqlite3_column_int64(stmt, 14);
      int64_t created_time = sqlite3_column_int64(stmt, 15);
      int64_t queued_time = sqlite3_column_int64(stmt, 16);
      int64_t running_time = sqlite3_column_int64(stmt, 17);
      int64_t done_time = sqlite3_column_int64(stmt, 18);
      int64_t cancelled_time = sqlite3_column_int64(stmt, 19);
      int64_t error_time = sqlite3_column_int64(stmt, 20);
      int64_t deleted_time = sqlite3_column_int64(stmt, 21);
      int64_t update_time = sqlite3_column_int64(stmt, 22);
      rc = s_dbmgr_job_info_create_from_db((const char *)job_id, (const char *)user_id,
                                           (const char *)group_id, (uint8_t)priority, status,
                                           (const char *)qc_job_id, (const char *)qprogram,
                                           circuit_fmt, (size_t)shots, qc_type, transpiler,
                                           (const char *)remark, (const char *)result,
                                           (uint64_t)exec_time_estimate_msec, (uint64_t)exec_time_msec,
                                           created_time, queued_time, running_time,
                                           done_time, cancelled_time, error_time,
                                           deleted_time, update_time, &(*ji_arr_ptr)[*count]);
      if (rc != SQC_RESULT_OK) {
        sqc_msg_error("Failed to create job_info record: job_id=%s, err_msg=%s\n",
                      job_id, sqc_error_get_string(rc));
        rc = SQC_RESULT_ANY_RUNTIME_ERROR;
        break;
      }

      (*count)++;
    }

    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to SELECT job_info record: %s\n", sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
unlock:
  s_unlock_db();

  return rc;
}


// For Unit Test Use Only
static inline sqc_result_t
s_dbmgr_db_job_info_select(const char *target_job_id, dbmgr_job_info_t *ji_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_JOB_INFO_SELECT, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, target_job_id, -1, SQLITE_STATIC);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_ROW) {
      // Only one record can be retrieved
      // because the primary key is specified in the condition
      const unsigned char *job_id = sqlite3_column_text(stmt, 0);
      const unsigned char *user_id = sqlite3_column_text(stmt, 1);
      const unsigned char *group_id = sqlite3_column_text(stmt, 2);
      int priority = sqlite3_column_int(stmt, 3);
      int status = sqlite3_column_int(stmt, 4);
      const unsigned char *qc_job_id = sqlite3_column_text(stmt, 5);
      const unsigned char *qprogram = sqlite3_column_text(stmt, 6);
      int circuit_fmt = sqlite3_column_int(stmt, 7);
      int shots = sqlite3_column_int(stmt, 8);
      int qc_type = sqlite3_column_int(stmt, 9);
      int transpiler = sqlite3_column_int(stmt, 10);
      const unsigned char *remark = sqlite3_column_text(stmt, 11);
      const unsigned char *result = sqlite3_column_text(stmt, 12);
      int64_t exec_time_estimate_msec = sqlite3_column_int64(stmt, 13);
      int64_t exec_time_msec = sqlite3_column_int64(stmt, 14);
      int64_t created_time = sqlite3_column_int64(stmt, 15);
      int64_t queued_time = sqlite3_column_int64(stmt, 16);
      int64_t running_time = sqlite3_column_int64(stmt, 17);
      int64_t done_time = sqlite3_column_int64(stmt, 18);
      int64_t cancelled_time = sqlite3_column_int64(stmt, 19);
      int64_t error_time = sqlite3_column_int64(stmt, 20);
      int64_t deleted_time = sqlite3_column_int64(stmt, 21);
      int64_t update_time = sqlite3_column_int64(stmt, 22);
      rc = s_dbmgr_job_info_create_from_db((const char *)job_id, (const char *)user_id,
                                           (const char *)group_id, (uint8_t)priority, status,
                                           (const char *)qc_job_id, (const char *)qprogram,
                                           circuit_fmt, (size_t)shots, qc_type, transpiler,
                                           (const char *)remark, (const char *)result,
                                           (uint64_t)exec_time_estimate_msec, (uint64_t)exec_time_msec,
                                           created_time, queued_time, running_time,
                                           done_time, cancelled_time, error_time,
                                           deleted_time, update_time, ji_ptr);
      if (rc != SQC_RESULT_OK) {
        sqc_msg_error("Failed to create job_info record: job_id=%s, err_msg=%s\n",
                      job_id, sqc_error_get_string(rc));
      }
    } else if (db_rc == SQLITE_DONE) {
      sqc_msg_info("job_info record not found: %s\n", target_job_id);
      rc = SQC_RESULT_NOT_FOUND;
    } else {
      sqc_msg_error("Failed to SELECT job_info record: %s\n", sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


// For Unit Test Use Only
static inline sqc_result_t
s_dbmgr_db_job_info_select_count(int *count) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_JOB_INFO_SELECT_COUNT, -1, &stmt, NULL);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_ROW) {
      rc = SQC_RESULT_OK;
      *count = sqlite3_column_int(stmt, 0);
    } else {
      sqc_msg_error("Failed to SELECT job_info record(count): %s\n", sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_job_info_deserialize_all(sqc_hashmap_t *job_info_hashmap) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_JOB_INFO_SELECT_ALL, -1, &stmt, NULL);
    while ((db_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
      dbmgr_job_info_t ji_ptr;
      const unsigned char *job_id = sqlite3_column_text(stmt, 0);
      const unsigned char *user_id = sqlite3_column_text(stmt, 1);
      const unsigned char *group_id = sqlite3_column_text(stmt, 2);
      int priority = sqlite3_column_int(stmt, 3);
      int status = sqlite3_column_int(stmt, 4);
      const unsigned char *qc_job_id = sqlite3_column_text(stmt, 5);
      const unsigned char *qprogram = sqlite3_column_text(stmt, 6);
      int circuit_fmt = sqlite3_column_int(stmt, 7);
      int shots = sqlite3_column_int(stmt, 8);
      int qc_type = sqlite3_column_int(stmt, 9);
      int transpiler = sqlite3_column_int(stmt, 10);
      const unsigned char *remark = sqlite3_column_text(stmt, 11);
      const unsigned char *result = sqlite3_column_text(stmt, 12);
      int64_t exec_time_estimate_msec = sqlite3_column_int64(stmt, 13);
      int64_t exec_time_msec = sqlite3_column_int64(stmt, 14);
      int64_t created_time = sqlite3_column_int64(stmt, 15);
      int64_t queued_time = sqlite3_column_int64(stmt, 16);
      int64_t running_time = sqlite3_column_int64(stmt, 17);
      int64_t done_time = sqlite3_column_int64(stmt, 18);
      int64_t cancelled_time = sqlite3_column_int64(stmt, 19);
      int64_t error_time = sqlite3_column_int64(stmt, 20);
      int64_t deleted_time = sqlite3_column_int64(stmt, 21);
      int64_t update_time = sqlite3_column_int64(stmt, 22);
      rc = s_dbmgr_job_info_create_from_db((const char *)job_id, (const char *)user_id,
                                           (const char *)group_id, (uint8_t)priority, status,
                                           (const char *)qc_job_id, (const char *)qprogram,
                                           circuit_fmt, (size_t)shots, qc_type, transpiler,
                                           (const char *)remark, (const char *)result,
                                           (uint64_t)exec_time_estimate_msec, (uint64_t)exec_time_msec,
                                           created_time, queued_time, running_time,
                                           done_time, cancelled_time, error_time,
                                           deleted_time, update_time, &ji_ptr);
      if (rc == SQC_RESULT_OK) {
        sqc_msg_debug(5, "DB(job_info) -> Hashmap: job_id=%s, user_id=%s\n",
                      ji_ptr->job_id, ji_ptr->user_id);
      } else {
        sqc_msg_error("Failed to create job_info record: job_id=%s, err_msg=%s\n",
                      job_id, sqc_error_get_string(rc));
        rc = SQC_RESULT_ANY_RUNTIME_ERROR;
        break;
      }

      rc = sqc_hashmap_add(job_info_hashmap, (char *) (ji_ptr->job_id),
                                             (void **) &ji_ptr, false);
      if (rc != SQC_RESULT_OK) {
        sqc_msg_error("Failed to add to job Hashmap: job_id=%s\n", ji_ptr->job_id);
        s_dbmgr_job_info_destroy(ji_ptr);
        ji_ptr = NULL;
        rc = SQC_RESULT_ANY_RUNTIME_ERROR;
        break;
      }
    }

    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to SELECT job_info record: %s\n", sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_job_info_insert_record(const char *job_id, const char *user_id, const char *group_id,
                                  const uint8_t priority, const uint8_t status,
                                  const char *qprogram, const int circuit_fmt, const size_t shots,
                                  const int qc_type, const int transpiler,
                                  const char *remark, const int64_t created_time,
                                  const int64_t update_time) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_JOB_INFO_INSERT, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, job_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, group_id, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, priority);
    sqlite3_bind_int(stmt, 5, status);
    sqlite3_bind_text(stmt, 6, qprogram, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 7, circuit_fmt);
    sqlite3_bind_int(stmt, 8, (int)shots);
    sqlite3_bind_int(stmt, 9, qc_type);
    sqlite3_bind_int(stmt, 10, transpiler);
    sqlite3_bind_text(stmt, 11, remark, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 12, created_time);
    sqlite3_bind_int64(stmt, 13, update_time);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to INSERT job_info record: job_id=%s, err_msg=%s\n",
                    job_id, sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_job_info_update_queued_status(const char *job_id, const int64_t queued_time,
                                         const int64_t update_time) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_JOB_INFO_UPDATE_QUEUED_STATUS, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, SQC_RPC_SCHED_JOB_STATUS_QUEUED);
    sqlite3_bind_int64(stmt, 2, queued_time);
    sqlite3_bind_int64(stmt, 3, update_time);
    sqlite3_bind_text(stmt, 4, job_id, -1, SQLITE_STATIC);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to UPDATE job status(QUEUED): job_id=%s, err_msg=%s\n",
                    job_id, sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_job_info_update_running_status(const char *job_id, const int64_t running_time,
                                          const int64_t update_time) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_JOB_INFO_UPDATE_RUNNING_STATUS, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, SQC_RPC_SCHED_JOB_STATUS_RUNNING);
    sqlite3_bind_int64(stmt, 2, running_time);
    sqlite3_bind_int64(stmt, 3, update_time);
    sqlite3_bind_text(stmt, 4, job_id, -1, SQLITE_STATIC);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to UPDATE job status(RUNNING): job_id=%s, err_msg=%s\n",
                    job_id, sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_job_info_update_done_status(const char *job_id, const int64_t done_time,
                                       const int64_t update_time) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_JOB_INFO_UPDATE_DONE_STATUS, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, SQC_RPC_SCHED_JOB_STATUS_DONE);
    sqlite3_bind_int64(stmt, 2, done_time);
    sqlite3_bind_int64(stmt, 3, update_time);
    sqlite3_bind_text(stmt, 4, job_id, -1, SQLITE_STATIC);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to UPDATE job status(DONE): job_id=%s, err_msg=%s\n",
                    job_id, sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_job_info_update_cancelled_status(const char *job_id, const int64_t cancelled_time,
                                            const int64_t update_time) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_JOB_INFO_UPDATE_CANCELLED_STATUS, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, SQC_RPC_SCHED_JOB_STATUS_CANCELLED);
    sqlite3_bind_int64(stmt, 2, cancelled_time);
    sqlite3_bind_int64(stmt, 3, update_time);
    sqlite3_bind_text(stmt, 4, job_id, -1, SQLITE_STATIC);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to UPDATE job status(CANCELLED): job_id=%s, err_msg=%s\n",
                    job_id, sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_job_info_update_error_status(const char *job_id, const int64_t error_time,
                                        const int64_t update_time) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_JOB_INFO_UPDATE_ERROR_STATUS, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, SQC_RPC_SCHED_JOB_STATUS_ERROR);
    sqlite3_bind_int64(stmt, 2, error_time);
    sqlite3_bind_int64(stmt, 3, update_time);
    sqlite3_bind_text(stmt, 4, job_id, -1, SQLITE_STATIC);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to UPDATE job status(ERROR): job_id=%s, err_msg=%s\n",
                    job_id, sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_job_info_update_deleted_status(const char *job_id, const int64_t deleted_time,
                                          const int64_t update_time) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_JOB_INFO_UPDATE_DELETED_STATUS, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, SQC_RPC_SCHED_JOB_STATUS_DELETED);
    sqlite3_bind_int64(stmt, 2, deleted_time);
    sqlite3_bind_int64(stmt, 3, update_time);
    sqlite3_bind_text(stmt, 4, job_id, -1, SQLITE_STATIC);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to UPDATE job status(DELETED): job_id=%s, err_msg=%s\n",
                    job_id, sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_job_info_update_qc_job_id(const char *job_id, const char *qc_job_id,
                                     const int64_t update_time) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_JOB_INFO_UPDATE_QC_JOB_ID, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, qc_job_id, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, update_time);
    sqlite3_bind_text(stmt, 3, job_id, -1, SQLITE_STATIC);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to UPDATE job qc_job_id: job_id=%s, err_msg=%s\n",
                    job_id, sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_job_info_update_result(const char *job_id, const char *result,
                                  const int64_t update_time) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_JOB_INFO_UPDATE_QC_JOB_RESULT, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, result, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 2, update_time);
    sqlite3_bind_text(stmt, 3, job_id, -1, SQLITE_STATIC);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to UPDATE job result: job_id=%s, err_msg=%s\n",
                    job_id, sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_job_info_update_exec_time_estimate_msec(const char *job_id,
                                                   const uint64_t exec_time_estimate_msec,
                                                   const int64_t update_time) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_JOB_INFO_UPDATE_QC_JOB_EXEC_TIME_ESTIMATE_MSEC, -1, &stmt, NULL);
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)exec_time_estimate_msec);
    sqlite3_bind_int64(stmt, 2, update_time);
    sqlite3_bind_text(stmt, 3, job_id, -1, SQLITE_STATIC);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to UPDATE job exec_time_estimate_msec: job_id=%s, err_msg=%s\n",
                    job_id, sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_job_info_update_exec_time_msec(const char *job_id,
                                          const uint64_t exec_time_msec,
                                          const int64_t update_time) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_JOB_INFO_UPDATE_QC_JOB_EXEC_TIME_MSEC, -1, &stmt, NULL);
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)exec_time_msec);
    sqlite3_bind_int64(stmt, 2, update_time);
    sqlite3_bind_text(stmt, 3, job_id, -1, SQLITE_STATIC);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to UPDATE job exec_time_msec: job_id=%s, err_msg=%s\n",
                    job_id, sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_job_info_clear_result(const char *job_id, const int64_t deleted_time,
                                 const int64_t update_time) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_JOB_INFO_CLEAR_RESULT, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, SQC_RPC_SCHED_JOB_STATUS_DELETED);
    sqlite3_bind_int64(stmt, 2, deleted_time);
    sqlite3_bind_int64(stmt, 3, update_time);
    sqlite3_bind_text(stmt, 4, job_id, -1, SQLITE_STATIC);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to UPDATE job result(clear): job_id=%s, err_msg=%s\n",
                    job_id, sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_job_info_arr_clear_result(const char *const *job_id_arr, const size_t arr_len,
                                     const int64_t deleted_time, const int64_t update_time) {
  // TODO: Consider using transactions to perform batch processing.
  for (size_t i = 0; i < arr_len; i++) {
    (void)s_dbmgr_db_job_info_clear_result(job_id_arr[i], deleted_time, update_time);
  }

  return SQC_RESULT_OK;
}


/*
 * weight information
 */
// For Unit Test Use Only
static inline sqc_result_t
s_dbmgr_db_weight_info_select_all(dbmgr_weight_info_t **wi_arr_ptr, int *count, size_t capacity) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    *count = 0;
    *wi_arr_ptr = (dbmgr_weight_info_t *) malloc(sizeof(dbmgr_weight_info_t) * capacity);
    if (*wi_arr_ptr == NULL) {
      rc = SQC_RESULT_NO_MEMORY;
      goto unlock;
    }

    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_WEIGHT_INFO_SELECT_ALL, -1, &stmt, NULL);
    while ((db_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
      const int priority = sqlite3_column_int(stmt, 0);
      const int64_t weight = sqlite3_column_int(stmt, 1);
      const int64_t created_time = sqlite3_column_int64(stmt, 2);
      const int64_t update_time = sqlite3_column_int64(stmt, 3);
      rc = s_dbmgr_weight_info_create_from_db((uint8_t)priority, (uint64_t)weight,
                                              created_time, update_time, &(*wi_arr_ptr)[*count]);
      if (rc != SQC_RESULT_OK) {
        sqc_msg_error("Failed to create weight_info record: priority=%d, err_msg=%s\n",
                      priority, sqc_error_get_string(rc));
        rc = SQC_RESULT_ANY_RUNTIME_ERROR;
        break;
      }

      (*count)++;
    }

    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to SELECT weight_info record: %s\n", sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
unlock:
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_weight_info_select(const uint8_t target_priority, dbmgr_weight_info_t *wi_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_WEIGHT_INFO_SELECT, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, target_priority);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_ROW) {
      // Only one record can be retrieved
      // because the primary key is specified in the condition
      const int priority = sqlite3_column_int(stmt, 0);
      const int64_t weight = sqlite3_column_int(stmt, 1);
      const int64_t created_time = sqlite3_column_int64(stmt, 2);
      const int64_t update_time = sqlite3_column_int64(stmt, 3);
      rc = s_dbmgr_weight_info_create_from_db((uint8_t)priority, (uint64_t)weight,
                                              created_time, update_time, wi_ptr);
      if (rc != SQC_RESULT_OK) {
        sqc_msg_error("Failed to create weight_info record: priority=%d, err_msg=%s\n",
                      priority, sqc_error_get_string(rc));
      }
    } else if (db_rc == SQLITE_DONE) {
      sqc_msg_info("weight_info record not found: %d\n", target_priority);
      rc = SQC_RESULT_NOT_FOUND;
    } else {
      sqc_msg_error("Failed to SELECT weight_info record: %s\n", sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_weight_info_select_count(int *count) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int db_rc;
  sqlite3_stmt *stmt;

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_WEIGHT_INFO_SELECT_COUNT, -1, &stmt, NULL);
    db_rc = sqlite3_step(stmt);

    if (db_rc == SQLITE_ROW) {
      rc = SQC_RESULT_OK;
      *count = sqlite3_column_int(stmt, 0);
    } else {
      sqc_msg_error("Failed to SELECT weight_info record(count): %s\n", sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_weight_info_deserialize_all(sqc_hashmap_t *weight_info_hashmap) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  sqlite3_stmt *stmt = NULL;
  int db_rc;
  int deserialized_count = 0;

  if (weight_info_hashmap == NULL) {
    return SQC_RESULT_INVALID_ARGS;
  }

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_WEIGHT_INFO_SELECT_ALL, -1, &stmt, NULL);
    while ((db_rc = sqlite3_step(stmt)) == SQLITE_ROW) {
      dbmgr_weight_info_t wi_ptr = NULL;
      const int priority = sqlite3_column_int(stmt, 0);
      const int64_t weight = sqlite3_column_int64(stmt, 1);
      const int64_t created_time = sqlite3_column_int64(stmt, 2);
      const int64_t update_time = sqlite3_column_int64(stmt, 3);

      // Check invalid priority
      if (unlikely(priority < 0 || priority > SQC_RPC_SCHED_MAX_PRIORITY)) {
        sqc_msg_error("Invalid weight_info priority from DB: priority=%d, max=%d\n",
                      priority, SQC_RPC_SCHED_MAX_PRIORITY);
        rc = SQC_RESULT_ANY_RUNTIME_ERROR;
        goto finalize;
      }
      deserialized_count++;

      rc = s_dbmgr_weight_info_create_from_db((uint8_t)priority, (uint64_t)weight,
                                              created_time, update_time, &wi_ptr);
      if (rc == SQC_RESULT_OK) {
        sqc_msg_debug(5, "DB(weight_info) -> Hashmap priority=%d\n", wi_ptr->priority);
      } else {
        sqc_msg_error("Failed to create priority_info record: priority=%d, err_msg=%s\n",
                      priority, sqc_error_get_string(rc));
        rc = SQC_RESULT_ANY_RUNTIME_ERROR;
        break;
      }

      rc = sqc_hashmap_add(weight_info_hashmap,
                           (void *)(uintptr_t)wi_ptr->priority,
                           (void **)&wi_ptr, false);
      if (rc != SQC_RESULT_OK) {
        sqc_msg_error("Failed to add to weight Hashmap: priority=%d\n", wi_ptr->priority);
        s_dbmgr_weight_info_destroy(wi_ptr);
        break;
      }
    }

    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
      if (deserialized_count == SQC_RPC_SCHED_PQ_NUM) {
        rc = SQC_RESULT_OK;
      } else {
        sqc_msg_error("Invalid weight_info record count: actual=%d, expected=%d\n",
                      deserialized_count, SQC_RPC_SCHED_PQ_NUM);
        rc = SQC_RESULT_ANY_RUNTIME_ERROR;
      }
    } else {
      sqc_msg_error("Failed to SELECT weight_info record: %s\n", sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

finalize:
    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_weight_info_insert_record(const uint8_t priority, const uint64_t weight,
                                     const int64_t created_time, const int64_t update_time) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  sqlite3_stmt *stmt = NULL;
  int db_rc;

  if (priority > SQC_RPC_SCHED_MAX_PRIORITY || weight > INT64_MAX) {
    return SQC_RESULT_INVALID_ARGS;
  }

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_WEIGHT_INFO_INSERT, -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, priority);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)weight);
    sqlite3_bind_int64(stmt, 3, created_time);
    sqlite3_bind_int64(stmt, 4, update_time);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to INSERT weight_info: priority=%u, err_msg=%s\n",
                    priority, sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


static inline sqc_result_t
s_dbmgr_db_weight_info_update_weight(const uint8_t priority, const uint64_t weight,
                                     const int64_t update_time) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  sqlite3_stmt *stmt = NULL;
  int db_rc;

  if (priority > SQC_RPC_SCHED_MAX_PRIORITY || weight > INT64_MAX) {
    return SQC_RESULT_INVALID_ARGS;
  }

  s_lock_db();
  {
    sqlite3_prepare_v2(sqc_rpc_sched_db, SQC_RPC_SCHED_DB_WEIGHT_INFO_UPDATE, -1, &stmt, NULL);
    sqlite3_bind_int64(stmt, 1, (sqlite3_int64)weight);
    sqlite3_bind_int64(stmt, 2, update_time);
    sqlite3_bind_int(stmt, 3, priority);

    db_rc = sqlite3_step(stmt);
    if (db_rc == SQLITE_DONE) {
      rc = SQC_RESULT_OK;
    } else {
      sqc_msg_error("Failed to UPDATE weight_info: priority=%u, err_msg=%s\n",
                    priority, sqlite3_errmsg(sqc_rpc_sched_db));
      rc = SQC_RESULT_ANY_RUNTIME_ERROR;
    }

    sqlite3_finalize(stmt);
  }
  s_unlock_db();

  return rc;
}


/*
 * export
 */


sqc_result_t
dbmgr_db_user_info_deserialize_all(sqc_hashmap_t *user_info_hashmap) {
  return s_dbmgr_db_user_info_deserialize_all(user_info_hashmap);
}

sqc_result_t
dbmgr_db_group_info_deserialize_all(sqc_hashmap_t *group_info_hashmap) {
  return s_dbmgr_db_group_info_deserialize_all(group_info_hashmap);
}

sqc_result_t
dbmgr_db_user_group_info_deserialize_all(sqc_hashmap_t *user_group_info_hashmap) {
  return s_dbmgr_db_user_group_info_deserialize_all(user_group_info_hashmap);
}

sqc_result_t
dbmgr_db_job_info_deserialize_all(sqc_hashmap_t *job_info_hashmap) {
  return s_dbmgr_db_job_info_deserialize_all(job_info_hashmap);
}

sqc_result_t
dbmgr_db_weight_info_deserialize_all(sqc_hashmap_t *weight_info_hashmap) {
  return s_dbmgr_db_weight_info_deserialize_all(weight_info_hashmap);
}

sqc_result_t
dbmgr_db_initialize(void) {
  return s_dbmgr_db_initialize(SQC_RPC_SCHED_DB_FILE);
}

void
dbmgr_db_finalize(void) {
  s_dbmgr_db_finalize();
}

/*
 * user_info
 */
sqc_result_t
dbmgr_db_user_info_insert_record(const char *user_id, const uint8_t role_type, const uint8_t status,
                                 const int64_t created_time, const int64_t update_time) {
  return s_dbmgr_db_user_info_insert_record(user_id, role_type, status,
                                            created_time, update_time);
}

sqc_result_t
dbmgr_db_user_info_update_role_type(const char *user_id, const uint8_t role_type,
                                    const int64_t update_time) {
  return s_dbmgr_db_user_info_update_role_type(user_id, role_type, update_time);
}

sqc_result_t
dbmgr_db_user_info_update_status(const char *user_id, const uint8_t status,
                                 const int64_t update_time) {
  return s_dbmgr_db_user_info_update_status(user_id, status, update_time);
}

sqc_result_t
dbmgr_db_group_info_insert_record(const char *group_id, const uint64_t exec_time_limit_msec,
                                  const uint64_t exec_time_total_msec,
                                  const int64_t created_time, const int64_t update_time) {
  return s_dbmgr_db_group_info_insert_record(group_id, exec_time_limit_msec, exec_time_total_msec,
                                             created_time, update_time);
}

sqc_result_t
dbmgr_db_group_info_update_exec_time_limit_msec(const char *group_id,
                                                const uint64_t exec_time_limit_msec,
                                                const int64_t update_time) {
  return s_dbmgr_db_group_info_update_exec_time_limit_msec(group_id,
                                                           exec_time_limit_msec,
                                                           update_time);
}

sqc_result_t
dbmgr_db_group_info_update_exec_time_total_msec(const char *group_id,
                                                const uint64_t exec_time_total_msec,
                                                const int64_t update_time) {
  return s_dbmgr_db_group_info_update_exec_time_total_msec(group_id,
                                                           exec_time_total_msec,
                                                           update_time);
}

/*
 * user_group_info
 */
sqc_result_t
dbmgr_db_user_group_info_insert_record(const char *user_id, const char *group_id,
                                       const uint8_t status, const int64_t created_time,
                                       const int64_t update_time) {
  return s_dbmgr_db_user_group_info_insert_record(user_id, group_id, status,
                                                  created_time, update_time);
}

sqc_result_t
dbmgr_db_user_group_info_update_status(const char *user_id, const char *group_id,
                                       const uint8_t status, const int64_t update_time) {
  return s_dbmgr_db_user_group_info_update_status(user_id, group_id,
                                                  status, update_time);
}

/*
 * job_info
 */
sqc_result_t
dbmgr_db_job_info_insert_record(const char *job_id, const char *user_id, const char *group_id,
                                const uint8_t priority, const uint8_t status,
                                const char *qprogram, const int circuit_fmt, const size_t shots,
                                const int qc_type, const int transpiler,
                                const char *remark, const int64_t created_time,
                                const int64_t update_time) {
  return s_dbmgr_db_job_info_insert_record(job_id, user_id, group_id,
                                           priority, status,
                                           qprogram, circuit_fmt, shots,
                                           qc_type, transpiler,
                                           remark, created_time,
                                           update_time);
}

sqc_result_t
dbmgr_db_job_info_update_queued_status(const char *job_id, const int64_t queued_time,
                                       const int64_t update_time) {
  return s_dbmgr_db_job_info_update_queued_status(job_id, queued_time, update_time);
}

sqc_result_t
dbmgr_db_job_info_update_running_status(const char *job_id, const int64_t running_time,
                                        const int64_t update_time) {
  return s_dbmgr_db_job_info_update_running_status(job_id, running_time, update_time);
}

sqc_result_t
dbmgr_db_job_info_update_done_status(const char *job_id, const int64_t done_time,
                                     const int64_t update_time) {
  return s_dbmgr_db_job_info_update_done_status(job_id, done_time, update_time);
}


sqc_result_t
dbmgr_db_job_info_update_cancelled_status(const char *job_id, const int64_t cancelled_time,
                                          const int64_t update_time) {
  return s_dbmgr_db_job_info_update_cancelled_status(job_id, cancelled_time, update_time);
}

sqc_result_t
dbmgr_db_job_info_update_error_status(const char *job_id, const int64_t error_time,
                                      const int64_t update_time) {
  return s_dbmgr_db_job_info_update_error_status(job_id, error_time, update_time);
}

sqc_result_t
dbmgr_db_job_info_update_deleted_status(const char *job_id, const int64_t deleted_time,
                                        const int64_t update_time) {
  return s_dbmgr_db_job_info_update_deleted_status(job_id, deleted_time, update_time);
}

sqc_result_t
dbmgr_db_job_info_update_qc_job_id(const char *job_id, const char *qc_job_id,
                                   const int64_t update_time) {
  return s_dbmgr_db_job_info_update_qc_job_id(job_id, qc_job_id, update_time);
}

sqc_result_t
dbmgr_db_job_info_update_result(const char *job_id, const char *result,
                                const int64_t update_time) {
  return s_dbmgr_db_job_info_update_result(job_id, result, update_time);
}

sqc_result_t
dbmgr_db_job_info_update_exec_time_estimate_msec(const char *job_id,
                                                 const uint64_t exec_time_estimate_msec,
                                                 const int64_t update_time) {
  return s_dbmgr_db_job_info_update_exec_time_estimate_msec(job_id,
                                                            exec_time_estimate_msec,
                                                            update_time);
}

sqc_result_t
dbmgr_db_job_info_update_exec_time_msec(const char *job_id,
                                        const uint64_t exec_time_msec,
                                        const int64_t update_time) {
  return s_dbmgr_db_job_info_update_exec_time_msec(job_id, exec_time_msec, update_time);
}

sqc_result_t
dbmgr_db_job_info_clear_result(const char *job_id, const int64_t deleted_time,
                               const int64_t update_time) {
  return s_dbmgr_db_job_info_clear_result(job_id, deleted_time, update_time);
}

sqc_result_t
dbmgr_db_job_info_arr_clear_result(const char *const *job_id_arr, const size_t arr_len,
                                   const int64_t deleted_time, const int64_t update_time) {
  return s_dbmgr_db_job_info_arr_clear_result(job_id_arr, arr_len, deleted_time, update_time);
}

/*
 * weight_info
 */
sqc_result_t
dbmgr_db_weight_info_insert_record(const uint8_t priority, const uint64_t weight,
                                   const int64_t created_time, const int64_t update_time) {
  return s_dbmgr_db_weight_info_insert_record(priority, weight, created_time, update_time);
}

sqc_result_t
dbmgr_db_weight_info_update_weight(const uint8_t priority, const uint64_t weight,
                                   const int64_t update_time) {
  return s_dbmgr_db_weight_info_update_weight(priority, weight, update_time);
}

