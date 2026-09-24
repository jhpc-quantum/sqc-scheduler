#include "sqc_apis.h"

#include "dbmgr_util.h"
#include "dbmgr_db.h"
#include "dbmgr_user_group.h"
#include "dbmgr_types_internal.h"

static sqc_hashmap_t user_group_info_hashmap = NULL;

/*
 * user_group_info methods
 */

static inline void
s_dbmgr_user_group_info_record_rlock(dbmgr_user_group_info_t ugi_ptr) {
  if (likely(ugi_ptr != NULL)) {
    (void)sqc_rwlock_reader_lock(&(ugi_ptr->rwlck_));
  }
}


static inline void
s_dbmgr_user_group_info_record_wlock(dbmgr_user_group_info_t ugi_ptr) {
  if (likely(ugi_ptr != NULL)) {
    (void)sqc_rwlock_writer_lock(&(ugi_ptr->rwlck_));
  }
}


static inline void
s_dbmgr_user_group_info_record_unlock(dbmgr_user_group_info_t ugi_ptr) {
  if (likely(ugi_ptr != NULL)) {
    (void)sqc_rwlock_unlock(&(ugi_ptr->rwlck_));
  }
}


static inline sqc_result_t
s_dbmgr_user_group_info_record_create(const char *user_id, const char *group_id,
                                      dbmgr_user_group_info_t *ugi_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  size_t user_id_len, group_id_len;

  if (likely(ugi_ptr != NULL &&
             IS_VALID_STRING(user_id) == true &&
             IS_VALID_STRING(group_id) == true)) {
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

      (*ugi_ptr)->status = SQC_RPC_SCHED_USER_GROUP_STATUS_ENABLED;

      (*ugi_ptr)->created_time = sqc_chrono_now();
      (*ugi_ptr)->update_time = (*ugi_ptr)->created_time;

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
s_dbmgr_user_group_info_record_destroy(dbmgr_user_group_info_t ugi_ptr) {
  if (ugi_ptr != NULL) {
    if (ugi_ptr->rwlck_ != NULL) {
      (void)sqc_rwlock_destroy(&(ugi_ptr->rwlck_));
    }
  }

  free((void *) ugi_ptr);
}


static inline sqc_result_t
s_dbmgr_user_group_info_record_add(dbmgr_user_group_info_t ugi_ptr) {
  if (likely(user_group_info_hashmap != NULL)) {
    if (likely(ugi_ptr != NULL && IS_VALID_STRING(ugi_ptr->user_group_key) == true)) {
      return sqc_hashmap_add(&user_group_info_hashmap,
                             (char *) (ugi_ptr->user_group_key),
                             (void **) &ugi_ptr, false);
    }

    return SQC_RESULT_INVALID_ARGS;
  } else {
    return SQC_RESULT_NOT_STARTED;
  }
}


static inline sqc_result_t
s_dbmgr_user_group_info_record_delete(dbmgr_user_group_info_t ugi_ptr) {
  if (likely(user_group_info_hashmap != NULL)) {
    if (likely(ugi_ptr != NULL && IS_VALID_STRING(ugi_ptr->user_group_key) == true)) {
      return sqc_hashmap_delete(&user_group_info_hashmap,
                                (void *) ugi_ptr->user_group_key,
                                NULL, true);
    }

    return SQC_RESULT_INVALID_ARGS;
  } else {
    return SQC_RESULT_NOT_STARTED;
  }
}


static inline sqc_result_t
s_dbmgr_user_group_info_record_find(const char *user_id, const char *group_id,
                                    dbmgr_user_group_info_t *ugi_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  char user_group_key[SQC_RPC_SCHED_USER_GROUP_KEY_MAX_SIZE + 1];
  size_t user_group_key_len;

  if (likely(user_group_info_hashmap != NULL)) {
    if (likely(ugi_ptr != NULL &&
               IS_VALID_STRING(user_id) == true &&
               IS_VALID_STRING(group_id) == true)) {

      rc = dbmgr_util_create_user_group_key(user_group_key,
                                            sizeof(user_group_key),
                                            user_id, group_id,
                                            &user_group_key_len);
      if (rc == SQC_RESULT_OK) {
        rc = sqc_hashmap_find(&user_group_info_hashmap,
                              (void *) user_group_key, (void **) ugi_ptr);
      }

      return rc;
    } else {
      return SQC_RESULT_INVALID_ARGS;
    }
  } else {
    return SQC_RESULT_NOT_STARTED;
  }
}


static void
s_dbmgr_user_group_info_record_freeup(void *ugi_ptr) {
  s_dbmgr_user_group_info_record_destroy((dbmgr_user_group_info_t) ugi_ptr);
}


static sqc_result_t
s_dbmgr_user_group_info_initialize(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  if ((rc = sqc_hashmap_create(&user_group_info_hashmap,
                               SQC_HASHMAP_TYPE_STRING,
                               s_dbmgr_user_group_info_record_freeup)) != SQC_RESULT_OK) {
    sqc_perror(rc);
  }

  if ((rc = dbmgr_db_user_group_info_deserialize_all(&user_group_info_hashmap)) == SQC_RESULT_OK) {
    sqc_msg_info("User_group info deserialized successfully\n");
  } else {
    sqc_msg_error("Failed to deserialize the user_group info: %s\n", sqc_error_get_string(rc));
  }

  return rc;
}


static void
s_dbmgr_user_group_info_finalize(void) {
  sqc_hashmap_destroy(&user_group_info_hashmap, true);
  user_group_info_hashmap = NULL;
}


/*
 * export
 */

sqc_result_t
dbmgr_ugi_add_user_to_group(const char *user_id, const char *group_id) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  dbmgr_user_group_info_t ugi_ptr = NULL;

  if (likely(user_group_info_hashmap != NULL)) {
    if (likely(IS_VALID_STRING(user_id) == true && IS_VALID_STRING(group_id) == true)) {
      rc = s_dbmgr_user_group_info_record_create(user_id, group_id, &ugi_ptr);
      if (rc == SQC_RESULT_OK) {
        rc = s_dbmgr_user_group_info_record_add(ugi_ptr);
        if (rc == SQC_RESULT_OK) {
          rc = dbmgr_db_user_group_info_insert_record(ugi_ptr->user_id, ugi_ptr->group_id, ugi_ptr->status,
                                                      ugi_ptr->created_time, ugi_ptr->update_time);
          if (rc != SQC_RESULT_OK) {
            sqc_msg_error("Failed to INSERT user_group_info record: %s\n", sqc_error_get_string(rc));
          }
        } else {
          sqc_msg_error("Failed to add user_group_info record: %s\n", sqc_error_get_string(rc));
        }
      } else {
        sqc_msg_error("Failed to create user_group_info record: %s\n", sqc_error_get_string(rc));
      }
    } else {
      return SQC_RESULT_INVALID_ARGS;
    }
  } else {
    return SQC_RESULT_NOT_STARTED;
  }

  return rc;
}

sqc_result_t
dbmgr_ugi_delete_user_from_group(const char *user_id, const char *group_id) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  dbmgr_user_group_info_t ugi_ptr = NULL;

  rc = s_dbmgr_user_group_info_record_find(user_id, group_id, &ugi_ptr);
  if (rc == SQC_RESULT_OK) {
    rc = s_dbmgr_user_group_info_record_delete(ugi_ptr);
  }

  return rc;
}


bool
dbmgr_ugi_is_user_in_group(const char *user_id, const char *group_id) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  dbmgr_user_group_info_t ugi_ptr = NULL;

  rc = s_dbmgr_user_group_info_record_find(user_id, group_id, &ugi_ptr);
  if (rc == SQC_RESULT_OK && ugi_ptr != NULL) {
    return true;
  }
  return false;
}


sqc_result_t
dbmgr_ugi_user_find(const char *user_id, const char *group_id,
                    dbmgr_user_group_info_t *ugi_ptr) {
  return s_dbmgr_user_group_info_record_find(user_id, group_id, ugi_ptr);
}


sqc_result_t
dbmgr_ugi_get_user_id(const dbmgr_user_group_info_t ugi_ptr, char **user_id) {
  if (likely(ugi_ptr != NULL && user_id != NULL && *user_id == NULL)) {
    s_dbmgr_user_group_info_record_rlock(ugi_ptr);
    {
      *user_id = strdup(ugi_ptr->user_id);
    }
    s_dbmgr_user_group_info_record_unlock(ugi_ptr);

    if (*user_id == NULL) {
      return SQC_RESULT_NO_MEMORY;
    }

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ugi_get_user_id_len(const dbmgr_user_group_info_t ugi_ptr, size_t *user_id_len) {
  if (likely(ugi_ptr != NULL && user_id_len != NULL)) {
    s_dbmgr_user_group_info_record_rlock(ugi_ptr);
    {
      *user_id_len = ugi_ptr->user_id_len;
    }
    s_dbmgr_user_group_info_record_unlock(ugi_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ugi_get_group_id(const dbmgr_user_group_info_t ugi_ptr, char **group_id) {
  if (likely(ugi_ptr != NULL && group_id != NULL && *group_id == NULL)) {
    s_dbmgr_user_group_info_record_rlock(ugi_ptr);
    {
      *group_id = strdup(ugi_ptr->group_id);
    }
    s_dbmgr_user_group_info_record_unlock(ugi_ptr);

    if (*group_id == NULL) {
      return SQC_RESULT_NO_MEMORY;
    }

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ugi_get_group_id_len(const dbmgr_user_group_info_t ugi_ptr, size_t *group_id_len) {
  if (likely(ugi_ptr != NULL && group_id_len != NULL)) {
    s_dbmgr_user_group_info_record_rlock(ugi_ptr);
    {
      *group_id_len = ugi_ptr->group_id_len;
    }
    s_dbmgr_user_group_info_record_unlock(ugi_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ugi_set_user_group_enabled(const dbmgr_user_group_info_t ugi_ptr) {
  if (likely(ugi_ptr != NULL)) {
    s_dbmgr_user_group_info_record_wlock(ugi_ptr);
    {
      ugi_ptr->status = SQC_RPC_SCHED_USER_GROUP_STATUS_ENABLED;
      ugi_ptr->update_time = sqc_chrono_now();
    }
    s_dbmgr_user_group_info_record_unlock(ugi_ptr);

    // Update DB
    (void)dbmgr_db_user_group_info_update_status(ugi_ptr->user_id, ugi_ptr->group_id,
                                                 ugi_ptr->status,
                                                 ugi_ptr->update_time);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ugi_set_user_group_disabled(const dbmgr_user_group_info_t ugi_ptr) {
  if (likely(ugi_ptr != NULL)) {
    s_dbmgr_user_group_info_record_wlock(ugi_ptr);
    {
      ugi_ptr->status = SQC_RPC_SCHED_USER_GROUP_STATUS_DISABLED;
      ugi_ptr->update_time = sqc_chrono_now();
    }
    s_dbmgr_user_group_info_record_unlock(ugi_ptr);

    // Update DB
    (void)dbmgr_db_user_group_info_update_status(ugi_ptr->user_id, ugi_ptr->group_id,
                                                 ugi_ptr->status,
                                                 ugi_ptr->update_time);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


bool
dbmgr_ugi_is_user_group_enabled(const dbmgr_user_group_info_t ugi_ptr) {
  sqc_rpc_sched_user_group_status_t status = SQC_RPC_SCHED_USER_GROUP_STATUS_UNKNOWN;
  if (likely(ugi_ptr != NULL)) {
    s_dbmgr_user_group_info_record_rlock(ugi_ptr);
    {
      status = ugi_ptr->status;
    }
    s_dbmgr_user_group_info_record_unlock(ugi_ptr);

    if (status == SQC_RPC_SCHED_USER_GROUP_STATUS_ENABLED) {
      return true;
    }
  }
  return false;
}


sqc_result_t
dbmgr_ugi_get_created_time(const dbmgr_user_group_info_t ugi_ptr, sqc_chrono_t *created_time) {
  if (likely(ugi_ptr != NULL && created_time != NULL)) {
    s_dbmgr_user_group_info_record_rlock(ugi_ptr);
    {
      *created_time = ugi_ptr->created_time;
    }
    s_dbmgr_user_group_info_record_unlock(ugi_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_ugi_get_update_time(const dbmgr_user_group_info_t ugi_ptr, sqc_chrono_t *update_time) {
  if (likely(ugi_ptr != NULL && update_time != NULL)) {
    s_dbmgr_user_group_info_record_rlock(ugi_ptr);
    {
      *update_time = ugi_ptr->update_time;
    }
    s_dbmgr_user_group_info_record_unlock(ugi_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}

