#include "sqc_apis.h"

#include "dbmgr_db.h"
#include "dbmgr_group.h"
#include "dbmgr_types_internal.h"

static sqc_hashmap_t group_info_hashmap = NULL;

/*
 * group_info methods
 */


static inline void
s_dbmgr_group_info_record_rlock(dbmgr_group_info_t gi_ptr) {
  if (likely(gi_ptr != NULL)) {
    (void)sqc_rwlock_reader_lock(&(gi_ptr->rwlck_));
  }
}


static inline void
s_dbmgr_group_info_record_wlock(dbmgr_group_info_t gi_ptr) {
  if (likely(gi_ptr != NULL)) {
    (void)sqc_rwlock_writer_lock(&(gi_ptr->rwlck_));
  }
}


static inline void
s_dbmgr_group_info_record_unlock(dbmgr_group_info_t gi_ptr) {
  if (likely(gi_ptr != NULL)) {
    (void)sqc_rwlock_unlock(&(gi_ptr->rwlck_));
  }
}


static inline sqc_result_t
s_dbmgr_group_info_record_create(const char *group_id,
                                 const uint64_t exec_time_limit_msec,
                                 const uint64_t exec_time_total_msec,
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

      (*gi_ptr)->created_time = sqc_chrono_now();
      (*gi_ptr)->update_time = (*gi_ptr)->created_time;

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
s_dbmgr_group_info_record_destroy(dbmgr_group_info_t gi_ptr) {
  if (gi_ptr != NULL) {
    if (gi_ptr->rwlck_ != NULL) {
      (void)sqc_rwlock_destroy(&(gi_ptr->rwlck_));
    }
  }

  free((void *) gi_ptr);
}


static inline sqc_result_t
s_dbmgr_group_info_record_add(dbmgr_group_info_t gi_ptr) {
  if (likely(group_info_hashmap != NULL)) {
    if (likely(gi_ptr != NULL && IS_VALID_STRING(gi_ptr->group_id) == true)) {
      return sqc_hashmap_add(&group_info_hashmap,
                             (char *) (gi_ptr->group_id),
                             (void **) &gi_ptr, false);
    }

    return SQC_RESULT_INVALID_ARGS;
  } else {
    return SQC_RESULT_NOT_STARTED;
  }
}


static inline sqc_result_t
s_dbmgr_group_info_record_delete(dbmgr_group_info_t gi_ptr) {
  if (likely(group_info_hashmap != NULL)) {
    if (likely(gi_ptr != NULL && IS_VALID_STRING(gi_ptr->group_id) == true)) {
      return sqc_hashmap_delete(&group_info_hashmap,
                                (void *) gi_ptr->group_id,
                                NULL, true);
    }

    return SQC_RESULT_INVALID_ARGS;
  } else {
    return SQC_RESULT_NOT_STARTED;
  }
}


static inline sqc_result_t
s_dbmgr_group_info_record_find(const char *group_id, dbmgr_group_info_t *gi_ptr) {
  if (likely(group_info_hashmap != NULL)) {
    if (likely(gi_ptr != NULL && IS_VALID_STRING(group_id) == true)) {
      return sqc_hashmap_find(&group_info_hashmap,
                              (void *) group_id, (void **) gi_ptr);
    }

    return SQC_RESULT_INVALID_ARGS;
  } else {
    return SQC_RESULT_NOT_STARTED;
  }
}


typedef struct {
  dbmgr_group_info_t *results;
  size_t count;
  size_t capacity;
} group_info_remaining_time_ctx_t;


static bool
s_dbmgr_group_info_collect_record(const void *key, void *val,
                                  sqc_hashentry_t he, void *arg) {
  group_info_remaining_time_ctx_t *ctx = (group_info_remaining_time_ctx_t *)arg;

  (void)key;
  (void)he;

  if (ctx->count >= ctx->capacity) {
    return false;
  }

  ctx->results[ctx->count] = (dbmgr_group_info_t)val;
  ctx->count++;

  return true;
}


static uint64_t
s_dbmgr_group_info_get_remaining_time(const dbmgr_group_info_t gi_ptr) {
  uint64_t exec_time_limit_msec;
  uint64_t exec_time_total_msec;

  s_dbmgr_group_info_record_rlock(gi_ptr);
  {
    exec_time_limit_msec = gi_ptr->exec_time_limit_msec;
    exec_time_total_msec = gi_ptr->exec_time_total_msec;
  }
  s_dbmgr_group_info_record_unlock(gi_ptr);

  if (exec_time_limit_msec > exec_time_total_msec) {
    return exec_time_limit_msec - exec_time_total_msec;
  } else {
    return 0;
  }
}


static int
s_dbmgr_group_info_compare_by_remaining_time(const void *lhs, const void *rhs) {
  const dbmgr_group_info_t left = *(const dbmgr_group_info_t *)lhs;
  const dbmgr_group_info_t right = *(const dbmgr_group_info_t *)rhs;
  const uint64_t left_remaining = s_dbmgr_group_info_get_remaining_time(left);
  const uint64_t right_remaining = s_dbmgr_group_info_get_remaining_time(right);

  if (left_remaining < right_remaining) {
    return 1;
  }

  if (left_remaining > right_remaining) {
    return -1;
  }

  /* Hashmap iteration order is unspecified. */
  return strcmp(left->group_id, right->group_id);
}


static inline sqc_result_t
s_dbmgr_group_info_record_find_all_sorted_by_remaining_time(dbmgr_group_info_t **gi_ptr_arr,
                                                            size_t *arr_len) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  group_info_remaining_time_ctx_t ctx;

  if (group_info_hashmap == NULL) {
    return SQC_RESULT_NOT_STARTED;
  }

  if (gi_ptr_arr == NULL || arr_len == NULL) {
    return SQC_RESULT_INVALID_ARGS;
  }

  rc = sqc_hashmap_size(&group_info_hashmap);
  if (rc < 0) {
    sqc_msg_error("Failed to get hash size: %s\n", sqc_error_get_string(rc));
    return rc;
  }

  ctx.count = 0;
  ctx.capacity = (size_t)rc;
  ctx.results = (dbmgr_group_info_t *)malloc(sizeof(dbmgr_group_info_t) * ctx.capacity);
  if (ctx.results == NULL) {
    rc = SQC_RESULT_NO_MEMORY;
    sqc_msg_error("Failed to malloc Group record array.\n");
    return rc;
  }

  rc = sqc_hashmap_iterate(&group_info_hashmap,
                           s_dbmgr_group_info_collect_record, &ctx);
  if (rc != SQC_RESULT_OK) {
    sqc_msg_error("Failed to hash iterate: %s\n", sqc_error_get_string(rc));
    free(ctx.results);
    return rc;
  }

  qsort(ctx.results, ctx.count, sizeof(dbmgr_group_info_t),
        s_dbmgr_group_info_compare_by_remaining_time);

  *gi_ptr_arr = ctx.results;
  *arr_len = ctx.count;

  return SQC_RESULT_OK;
}


static void
s_dbmgr_group_info_record_freeup(void *gi_ptr) {
  s_dbmgr_group_info_record_destroy((dbmgr_group_info_t) gi_ptr);
}


static sqc_result_t
s_dbmgr_group_info_initialize(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  if ((rc = sqc_hashmap_create(&group_info_hashmap,
                               SQC_HASHMAP_TYPE_STRING,
                               s_dbmgr_group_info_record_freeup)) != SQC_RESULT_OK) {
    sqc_perror(rc);
  }

  if ((rc = dbmgr_db_group_info_deserialize_all(&group_info_hashmap)) == SQC_RESULT_OK) {
    sqc_msg_info("Group info deserialized successfully\n");
  } else {
    sqc_msg_error("Failed to deserialize the group info: %s\n", sqc_error_get_string(rc));
  }

  return rc;
}


static void
s_dbmgr_group_info_finalize(void) {
  sqc_hashmap_destroy(&group_info_hashmap, true);
  group_info_hashmap = NULL;
}


/*
 * export
 */


sqc_result_t
dbmgr_gi_create_group(const char *group_id,
                      const uint64_t exec_time_limit_msec,
                      const uint64_t exec_time_total_msec,
                      dbmgr_group_info_t *gi_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  if (likely(gi_ptr != NULL && group_id != NULL && IS_VALID_STRING(group_id) == true)) {
    rc = s_dbmgr_group_info_record_create(group_id, exec_time_limit_msec, exec_time_total_msec, gi_ptr);
    if (rc == SQC_RESULT_OK) {
      rc = s_dbmgr_group_info_record_add(*gi_ptr);
      if (rc == SQC_RESULT_OK) {
        rc = dbmgr_db_group_info_insert_record((*gi_ptr)->group_id,
                                               (*gi_ptr)->exec_time_limit_msec, (*gi_ptr)->exec_time_total_msec,
                                               (*gi_ptr)->created_time, (*gi_ptr)->update_time);
        if (rc != SQC_RESULT_OK) {
          sqc_msg_error("Failed to Insert Group record: %s\n", sqc_error_get_string(rc));
        }
      } else {
        sqc_msg_error("Failed to add Group record: %s\n", sqc_error_get_string(rc));
      }
    } else {
      sqc_msg_error("Failed to create Group record: %s\n", sqc_error_get_string(rc));
    }
  } else {
    rc = SQC_RESULT_INVALID_ARGS;
  }

  return rc;
}


sqc_result_t
dbmgr_gi_delete_group(const char *group_id) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  dbmgr_group_info_t gi_ptr = NULL;

  rc = s_dbmgr_group_info_record_find(group_id, &gi_ptr);
  if (rc == SQC_RESULT_OK) {
    rc = s_dbmgr_group_info_record_delete(gi_ptr);
  }

  return rc;
}


bool
dbmgr_gi_group_exists(const char *group_id) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  dbmgr_group_info_t gi_ptr = NULL;

  rc = s_dbmgr_group_info_record_find(group_id, &gi_ptr);
  if (rc == SQC_RESULT_OK && gi_ptr != NULL) {
    return true;
  }
  return false;
}


sqc_result_t
dbmgr_gi_group_find(const char *group_id, dbmgr_group_info_t *gi_ptr) {
  return s_dbmgr_group_info_record_find(group_id, gi_ptr);
}

sqc_result_t
dbmgr_gi_group_find_all_sorted_by_remaining_time(dbmgr_group_info_t **gi_ptr_arr,
                                                 size_t *arr_len) {
  return s_dbmgr_group_info_record_find_all_sorted_by_remaining_time(gi_ptr_arr, arr_len);
}

sqc_result_t
dbmgr_gi_get_group_id(const dbmgr_group_info_t gi_ptr, char **group_id) {
  if (likely(gi_ptr != NULL && group_id != NULL && *group_id == NULL)) {
    s_dbmgr_group_info_record_rlock(gi_ptr);
    {
      *group_id = strdup(gi_ptr->group_id);
    }
    s_dbmgr_group_info_record_unlock(gi_ptr);

    if (*group_id == NULL) {
      return SQC_RESULT_NO_MEMORY;
    }

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_gi_get_group_id_len(const dbmgr_group_info_t gi_ptr, size_t *group_id_len) {
  if (likely(gi_ptr != NULL && group_id_len != NULL)) {
    s_dbmgr_group_info_record_rlock(gi_ptr);
    {
      *group_id_len = gi_ptr->group_id_len;
    }
    s_dbmgr_group_info_record_unlock(gi_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_gi_set_exec_time_limit_msec(const dbmgr_group_info_t gi_ptr,
                                  uint64_t exec_time_limit_msec) {
  if (likely(gi_ptr != NULL)) {
    s_dbmgr_group_info_record_wlock(gi_ptr);
    {
      gi_ptr->exec_time_limit_msec = exec_time_limit_msec;
      gi_ptr->update_time = sqc_chrono_now();
    }
    s_dbmgr_group_info_record_unlock(gi_ptr);

    // Update DB
    (void)dbmgr_db_group_info_update_exec_time_limit_msec(gi_ptr->group_id,
                                                          gi_ptr->exec_time_limit_msec,
                                                          gi_ptr->update_time);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_gi_get_exec_time_limit_msec(const dbmgr_group_info_t gi_ptr,
                                  uint64_t *exec_time_limit_msec) {
  if (likely(gi_ptr != NULL && exec_time_limit_msec != NULL)) {
    s_dbmgr_group_info_record_rlock(gi_ptr);
    {
      *exec_time_limit_msec = gi_ptr->exec_time_limit_msec;
    }
    s_dbmgr_group_info_record_unlock(gi_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_gi_get_exec_time_total_msec(const dbmgr_group_info_t gi_ptr,
                                  uint64_t *exec_time_total_msec) {
  if (likely(gi_ptr != NULL && exec_time_total_msec != NULL)) {
    s_dbmgr_group_info_record_rlock(gi_ptr);
    {
      *exec_time_total_msec = gi_ptr->exec_time_total_msec;
    }
    s_dbmgr_group_info_record_unlock(gi_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_gi_add_exec_time_total_msec(const dbmgr_group_info_t gi_ptr,
                                  uint64_t exec_time_msec) {
  if (likely(gi_ptr != NULL)) {
    s_dbmgr_group_info_record_wlock(gi_ptr);
    {
      if (exec_time_msec <= (UINT64_MAX - gi_ptr->exec_time_total_msec)) {
        gi_ptr->exec_time_total_msec += exec_time_msec;
      } else {
        gi_ptr->exec_time_total_msec = UINT64_MAX;
      }
      gi_ptr->update_time = sqc_chrono_now();
    }
    s_dbmgr_group_info_record_unlock(gi_ptr);

    // Update DB
    (void)dbmgr_db_group_info_update_exec_time_total_msec(gi_ptr->group_id,
                                                          gi_ptr->exec_time_total_msec,
                                                          gi_ptr->update_time);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}

sqc_result_t
dbmgr_gi_subtract_exec_time_total_msec(const dbmgr_group_info_t gi_ptr,
                                       uint64_t exec_time_msec) {
  if (likely(gi_ptr != NULL)) {
    s_dbmgr_group_info_record_wlock(gi_ptr);
    {
      if (exec_time_msec < gi_ptr->exec_time_total_msec) {
        gi_ptr->exec_time_total_msec -= exec_time_msec;
      } else {
        gi_ptr->exec_time_total_msec = 0;
      }
      gi_ptr->update_time = sqc_chrono_now();
    }
    s_dbmgr_group_info_record_unlock(gi_ptr);

    // Update DB
    (void)dbmgr_db_group_info_update_exec_time_total_msec(gi_ptr->group_id,
                                                          gi_ptr->exec_time_total_msec,
                                                          gi_ptr->update_time);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}

sqc_result_t
dbmgr_gi_get_created_time(const dbmgr_group_info_t gi_ptr, sqc_chrono_t *created_time) {
  if (likely(gi_ptr != NULL && created_time != NULL)) {
    s_dbmgr_group_info_record_rlock(gi_ptr);
    {
      *created_time = gi_ptr->created_time;
    }
    s_dbmgr_group_info_record_unlock(gi_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_gi_get_update_time(const dbmgr_group_info_t gi_ptr, sqc_chrono_t *update_time) {
  if (likely(gi_ptr != NULL && update_time != NULL)) {
    s_dbmgr_group_info_record_rlock(gi_ptr);
    {
      *update_time = gi_ptr->update_time;
    }
    s_dbmgr_group_info_record_unlock(gi_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}

