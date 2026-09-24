#include "sqc_apis.h"

#include "dbmgr_db.h"
#include "dbmgr_weight.h"
#include "dbmgr_types_internal.h"

static sqc_hashmap_t weight_info_hashmap = NULL;

static inline void
s_dbmgr_weight_info_record_rlock(dbmgr_weight_info_t wi_ptr) {
  if (wi_ptr != NULL) {
    (void)sqc_rwlock_reader_lock(&wi_ptr->rwlck_);
  }
}


static inline void
s_dbmgr_weight_info_record_wlock(dbmgr_weight_info_t wi_ptr) {
  if (wi_ptr != NULL) {
    (void)sqc_rwlock_writer_lock(&wi_ptr->rwlck_);
  }
}


static inline void
s_dbmgr_weight_info_record_unlock(dbmgr_weight_info_t wi_ptr) {
  if (wi_ptr != NULL) {
    (void)sqc_rwlock_unlock(&wi_ptr->rwlck_);
  }
}


static inline sqc_result_t
s_dbmgr_weight_info_record_create(uint8_t priority, uint64_t weight,
                                  dbmgr_weight_info_t *wi_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  if (likely(wi_ptr != NULL && priority <= SQC_RPC_SCHED_MAX_PRIORITY)) {
    *wi_ptr = malloc(sizeof(**wi_ptr));
    if (*wi_ptr == NULL) {
      return SQC_RESULT_NO_MEMORY;
    }

    (*wi_ptr)->rwlck_ = NULL;
    if ((rc = sqc_rwlock_create(&((*wi_ptr)->rwlck_))) != SQC_RESULT_OK) {
      goto error;
    }

    (*wi_ptr)->priority = priority;

    (*wi_ptr)->weight = weight;

    (*wi_ptr)->created_time = sqc_chrono_now();
    (*wi_ptr)->update_time = (*wi_ptr)->created_time;

    return SQC_RESULT_OK;
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
s_dbmgr_weight_info_record_destroy(dbmgr_weight_info_t wi_ptr) {
  if (wi_ptr != NULL) {
    if (wi_ptr->rwlck_ != NULL) {
      (void)sqc_rwlock_destroy(&(wi_ptr->rwlck_));
    }
  }

  free(wi_ptr);
}


static inline sqc_result_t
s_dbmgr_weight_info_record_find(uint8_t priority, dbmgr_weight_info_t *wi_ptr) {
  if (likely(weight_info_hashmap != NULL)) {
    if (likely(wi_ptr != NULL)) {
      return sqc_hashmap_find(&weight_info_hashmap,
                              (void *)(uintptr_t)priority, (void **)wi_ptr);
    }

    return SQC_RESULT_INVALID_ARGS;
  } else {
    return SQC_RESULT_NOT_STARTED;
  }
}


static void
s_dbmgr_weight_info_record_freeup(void *wi_ptr) {
  s_dbmgr_weight_info_record_destroy((dbmgr_weight_info_t)wi_ptr);
}


static sqc_result_t
s_dbmgr_weight_info_initialize(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  if ((rc = sqc_hashmap_create(&weight_info_hashmap,
                               SQC_HASHMAP_TYPE_ONE_WORD,
                               s_dbmgr_weight_info_record_freeup)) != SQC_RESULT_OK) {
    sqc_perror(rc);
  }

  if ((rc = dbmgr_db_weight_info_deserialize_all(&weight_info_hashmap)) == SQC_RESULT_OK) {
    sqc_msg_info("Weight info deserialized successfully\n");
  } else {
    sqc_msg_error("Failed to deserialize the weight info: %s\n", sqc_error_get_string(rc));
  }

  return rc;
}


static void
s_dbmgr_weight_info_finalize(void) {
  sqc_hashmap_destroy(&weight_info_hashmap, true);
  weight_info_hashmap = NULL;
}


/*
 * export
 */


sqc_result_t
dbmgr_wi_get_weight(const uint8_t priority, uint64_t *weight) {
  dbmgr_weight_info_t wi_ptr = NULL;
  sqc_result_t rc;

  if (priority <= SQC_RPC_SCHED_MAX_PRIORITY && weight != NULL) {
    rc = s_dbmgr_weight_info_record_find(priority, &wi_ptr);
    if (rc != SQC_RESULT_OK) {
      return rc;
    }

    s_dbmgr_weight_info_record_rlock(wi_ptr);
    {
      *weight = wi_ptr->weight;
    }
    s_dbmgr_weight_info_record_unlock(wi_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_wi_get_created_time(const uint8_t priority, sqc_chrono_t *created_time) {
  dbmgr_weight_info_t wi_ptr = NULL;
  sqc_result_t rc;

  if (priority <= SQC_RPC_SCHED_MAX_PRIORITY && created_time != NULL) {
    rc = s_dbmgr_weight_info_record_find(priority, &wi_ptr);
    if (rc != SQC_RESULT_OK) {
      return rc;
    }

    s_dbmgr_weight_info_record_rlock(wi_ptr);
    {
      *created_time = wi_ptr->created_time;
    }
    s_dbmgr_weight_info_record_unlock(wi_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}


sqc_result_t
dbmgr_wi_get_update_time(const uint8_t priority, sqc_chrono_t *update_time) {
  dbmgr_weight_info_t wi_ptr = NULL;
  sqc_result_t rc;

  if (priority <= SQC_RPC_SCHED_MAX_PRIORITY && update_time != NULL) {
    rc = s_dbmgr_weight_info_record_find(priority, &wi_ptr);
    if (rc != SQC_RESULT_OK) {
      return rc;
    }

    s_dbmgr_weight_info_record_rlock(wi_ptr);
    {
      *update_time = wi_ptr->update_time;
    }
    s_dbmgr_weight_info_record_unlock(wi_ptr);

    return SQC_RESULT_OK;
  } else {
    return SQC_RESULT_INVALID_ARGS;
  }
}

