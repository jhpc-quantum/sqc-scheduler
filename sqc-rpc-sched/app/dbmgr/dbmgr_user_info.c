#include "sqc_apis.h"

#include "dbmgr_db.h"
#include "dbmgr_user.h"
#include "dbmgr_types_internal.h"

static sqc_hashmap_t user_info_hashmap = NULL;

/*
 * user_info methods
 */


static inline void
s_dbmgr_user_info_record_rlock(dbmgr_user_info_t ui_ptr) {
  if (likely(ui_ptr != NULL)) {
    (void)sqc_rwlock_reader_lock(&(ui_ptr->rwlck_));
  }
}


static inline void
s_dbmgr_user_info_record_wlock(dbmgr_user_info_t ui_ptr) {
  if (likely(ui_ptr != NULL)) {
    (void)sqc_rwlock_writer_lock(&(ui_ptr->rwlck_));
  }
}


static inline void
s_dbmgr_user_info_record_unlock(dbmgr_user_info_t ui_ptr) {
  if (likely(ui_ptr != NULL)) {
    (void)sqc_rwlock_unlock(&(ui_ptr->rwlck_));
  }
}


static inline sqc_result_t
s_dbmgr_user_info_record_create(const char *user_id, dbmgr_user_info_t *ui_ptr) {
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

      (*ui_ptr)->role_type = SQC_RPC_SCHED_USER_ROLE_TYPE_GENERAL;

      (*ui_ptr)->status = SQC_RPC_SCHED_USER_STATUS_ENABLED;

      (*ui_ptr)->created_time = sqc_chrono_now();
      (*ui_ptr)->update_time = (*ui_ptr)->created_time;

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
s_dbmgr_user_info_record_destroy(dbmgr_user_info_t ui_ptr) {
  if (ui_ptr != NULL) {
    if (ui_ptr->rwlck_ != NULL) {
      (void)sqc_rwlock_destroy(&(ui_ptr->rwlck_));
    }
  }

  free((void *) ui_ptr);
}


static inline sqc_result_t
s_dbmgr_user_info_record_add(dbmgr_user_info_t ui_ptr) {
  if (likely(user_info_hashmap != NULL)) {
    if (likely(ui_ptr != NULL && IS_VALID_STRING(ui_ptr->user_id) == true)) {
      return sqc_hashmap_add(&user_info_hashmap,
                             (char *) (ui_ptr->user_id),
                             (void **) &ui_ptr, false);
    }

    return SQC_RESULT_INVALID_ARGS;
  } else {
    return SQC_RESULT_NOT_STARTED;
  }
}


static inline sqc_result_t
s_dbmgr_user_info_record_delete(dbmgr_user_info_t ui_ptr) {
  if (likely(user_info_hashmap != NULL)) {
    if (likely(ui_ptr != NULL && IS_VALID_STRING(ui_ptr->user_id) == true)) {
      return sqc_hashmap_delete(&user_info_hashmap,
                                (void *) ui_ptr->user_id,
                                NULL, true);
    }

    return SQC_RESULT_INVALID_ARGS;
  } else {
    return SQC_RESULT_NOT_STARTED;
  }
}


static inline sqc_result_t
s_dbmgr_user_info_record_find(const char *user_id, dbmgr_user_info_t *ui_ptr) {
  if (likely(user_info_hashmap != NULL)) {
    if (likely(ui_ptr != NULL && IS_VALID_STRING(user_id) == true)) {
      return sqc_hashmap_find(&user_info_hashmap,
                              (void *) user_id, (void **) ui_ptr);
    }

    return SQC_RESULT_INVALID_ARGS;
  } else {
    return SQC_RESULT_NOT_STARTED;
  }
}


static void
s_dbmgr_user_info_record_freeup(void *ui_ptr) {
  s_dbmgr_user_info_record_destroy((dbmgr_user_info_t) ui_ptr);
}


static sqc_result_t
s_dbmgr_user_info_initialize(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  if ((rc = sqc_hashmap_create(&user_info_hashmap,
                               SQC_HASHMAP_TYPE_STRING,
                               s_dbmgr_user_info_record_freeup)) != SQC_RESULT_OK) {
    sqc_perror(rc);
  }

  if ((rc = dbmgr_db_user_info_deserialize_all(&user_info_hashmap)) == SQC_RESULT_OK) {
    sqc_msg_info("User info deserialized successfully\n");
  } else {
    sqc_msg_error("Failed to deserialize the user info: %s\n", sqc_error_get_string(rc));
  }

  return rc;
}


static void
s_dbmgr_user_info_finalize(void) {
  sqc_hashmap_destroy(&user_info_hashmap, true);
  user_info_hashmap = NULL;
}


/*
 * export
 */


sqc_result_t
dbmgr_ui_create_user(const char *user_id, dbmgr_user_info_t *ui_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  if (likely(ui_ptr != NULL && user_id != NULL && IS_VALID_STRING(user_id) == true)) {
    rc = s_dbmgr_user_info_record_create(user_id, ui_ptr);
    if (rc == SQC_RESULT_OK) {
      rc = s_dbmgr_user_info_record_add(*ui_ptr);
      if (rc == SQC_RESULT_OK) {
        rc = dbmgr_db_user_info_insert_record((*ui_ptr)->user_id, (*ui_ptr)->role_type, (*ui_ptr)->status,
                                              (*ui_ptr)->created_time, (*ui_ptr)->update_time);
        if (rc != SQC_RESULT_OK) {
          sqc_msg_error("Failed to Insert User record: %s\n", sqc_error_get_string(rc));
        }
      } else {
        sqc_msg_error("Failed to add User record: %s\n", sqc_error_get_string(rc));
      }
    } else {
      sqc_msg_error("Failed to create User record: %s\n", sqc_error_get_string(rc));
    }
  } else {
    rc = SQC_RESULT_INVALID_ARGS;
  }

  return rc;
}


sqc_result_t
dbmgr_ui_delete_user(const char *user_id) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  dbmgr_user_info_t ui_ptr = NULL;

  rc = s_dbmgr_user_info_record_find(user_id, &ui_ptr);
  if (rc == SQC_RESULT_OK) {
    rc = s_dbmgr_user_info_record_delete(ui_ptr);
  }

  return rc;
}


bool
dbmgr_ui_user_exists(const char *user_id) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  dbmgr_user_info_t ui_ptr = NULL;

  rc = s_dbmgr_user_info_record_find(user_id, &ui_ptr);
  if (rc == SQC_RESULT_OK && ui_ptr != NULL) {
    return true;
  }
  return false;
}


sqc_result_t
dbmgr_ui_user_find(const char *user_id, dbmgr_user_info_t *ui_ptr) {
  return s_dbmgr_user_info_record_find(user_id, ui_ptr);
}


// TODO: deprecated
sqc_result_t
dbmgr_ui_set_user_id(dbmgr_user_info_t ui_ptr, const char *user_id) {
  if (likely(ui_ptr != NULL && user_id != NULL && IS_VALID_STRING(user_id) == true)) {
    size_t len = strlen(user_id);
    if (len > SQC_RPC_SCHED_USER_ID_MAX_SIZE) {
      return SQC_RESULT_TOO_LONG;
    }

    s_dbmgr_user_info_record_wlock(ui_ptr);
    {
      strncpy(ui_ptr->user_id, user_id, len);
      ui_ptr->user_id[len] = '\0';

      ui_ptr->user_id_len = len;
    }
    s_dbmgr_user_info_record_unlock(ui_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ui_get_user_id(const dbmgr_user_info_t ui_ptr, char **user_id) {
  if (likely(ui_ptr != NULL && user_id != NULL && *user_id == NULL)) {
    s_dbmgr_user_info_record_rlock(ui_ptr);
    {
      *user_id = strdup(ui_ptr->user_id);
    }
    s_dbmgr_user_info_record_unlock(ui_ptr);

    if (*user_id == NULL) {
      return SQC_RESULT_NO_MEMORY;
    }

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ui_get_user_id_len(const dbmgr_user_info_t ui_ptr, size_t *user_id_len) {
  if (likely(ui_ptr != NULL && user_id_len != NULL)) {
    s_dbmgr_user_info_record_rlock(ui_ptr);
    {
      *user_id_len = ui_ptr->user_id_len;
    }
    s_dbmgr_user_info_record_unlock(ui_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ui_set_user_role_type(const dbmgr_user_info_t ui_ptr, const sqc_rpc_sched_user_role_type_t role_type) {
  if (likely(ui_ptr != NULL)) {
    s_dbmgr_user_info_record_wlock(ui_ptr);
    {
      ui_ptr->role_type = role_type;
      ui_ptr->update_time = sqc_chrono_now();
    }
    s_dbmgr_user_info_record_unlock(ui_ptr);

    // Update DB
    (void)dbmgr_db_user_info_update_role_type(ui_ptr->user_id, ui_ptr->role_type,
                                              ui_ptr->update_time);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ui_get_user_role_type(const dbmgr_user_info_t ui_ptr, sqc_rpc_sched_user_role_type_t *role_type) {
  if (likely(ui_ptr != NULL && role_type != NULL)) {
    s_dbmgr_user_info_record_rlock(ui_ptr);
    {
      *role_type = ui_ptr->role_type;
    }
    s_dbmgr_user_info_record_unlock(ui_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


bool
dbmgr_ui_is_user_admin(const dbmgr_user_info_t ui_ptr) {
  sqc_rpc_sched_user_role_type_t role_type = SQC_RPC_SCHED_USER_ROLE_TYPE_UNKNOWN;
  if (likely(ui_ptr != NULL)) {
    s_dbmgr_user_info_record_rlock(ui_ptr);
    {
      role_type = ui_ptr->role_type;
    }
    s_dbmgr_user_info_record_unlock(ui_ptr);

    if (role_type == SQC_RPC_SCHED_USER_ROLE_TYPE_ADMIN) {
      return true;
    }
  }
  return false;
}


sqc_result_t
dbmgr_ui_set_user_enabled(const dbmgr_user_info_t ui_ptr) {
  if (likely(ui_ptr != NULL)) {
    s_dbmgr_user_info_record_wlock(ui_ptr);
    {
      ui_ptr->status = SQC_RPC_SCHED_USER_STATUS_ENABLED;
      ui_ptr->update_time = sqc_chrono_now();
    }
    s_dbmgr_user_info_record_unlock(ui_ptr);

    // Update DB
    (void)dbmgr_db_user_info_update_status(ui_ptr->user_id, ui_ptr->status,
                                           ui_ptr->update_time);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ui_set_user_disabled(const dbmgr_user_info_t ui_ptr) {
  if (likely(ui_ptr != NULL)) {
    s_dbmgr_user_info_record_wlock(ui_ptr);
    {
      ui_ptr->status = SQC_RPC_SCHED_USER_STATUS_DISABLED;
      ui_ptr->update_time = sqc_chrono_now();
    }
    s_dbmgr_user_info_record_unlock(ui_ptr);

    // Update DB
    (void)dbmgr_db_user_info_update_status(ui_ptr->user_id, ui_ptr->status,
                                           ui_ptr->update_time);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


bool
dbmgr_ui_is_user_enabled(const dbmgr_user_info_t ui_ptr) {
  sqc_rpc_sched_user_status_t status = SQC_RPC_SCHED_USER_STATUS_UNKNOWN;
  if (likely(ui_ptr != NULL)) {
    s_dbmgr_user_info_record_rlock(ui_ptr);
    {
      status = ui_ptr->status;
    }
    s_dbmgr_user_info_record_unlock(ui_ptr);

    if (status == SQC_RPC_SCHED_USER_STATUS_ENABLED) {
      return true;
    }
  }
  return false;
}


sqc_result_t
dbmgr_ui_get_created_time(const dbmgr_user_info_t ui_ptr, sqc_chrono_t *created_time) {
  if (likely(ui_ptr != NULL && created_time != NULL)) {
    s_dbmgr_user_info_record_rlock(ui_ptr);
    {
      *created_time = ui_ptr->created_time;
    }
    s_dbmgr_user_info_record_unlock(ui_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ui_get_update_time(const dbmgr_user_info_t ui_ptr, sqc_chrono_t *update_time) {
  if (likely(ui_ptr != NULL && update_time != NULL)) {
    s_dbmgr_user_info_record_rlock(ui_ptr);
    {
      *update_time = ui_ptr->update_time;
    }
    s_dbmgr_user_info_record_unlock(ui_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}

