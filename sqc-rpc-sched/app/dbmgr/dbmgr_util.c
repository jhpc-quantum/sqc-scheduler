#include "dbmgr_util.h"

sqc_result_t
dbmgr_util_create_user_group_key(char *user_group_key, size_t user_group_key_size,
                                 const char *user_id, const char *group_id,
                                 size_t *user_group_key_len) {
  size_t user_id_len, group_id_len, key_len;
  int rc;

  if (likely(user_group_key == NULL || user_group_key_len == NULL ||
             IS_VALID_STRING(user_id) == false || IS_VALID_STRING(group_id) == false)) {
    return SQC_RESULT_INVALID_ARGS;
  }

  user_id_len = strlen(user_id);
  group_id_len = strlen(group_id);
  if (user_id_len > SQC_RPC_SCHED_USER_ID_MAX_SIZE ||
      group_id_len > SQC_RPC_SCHED_GROUP_ID_MAX_SIZE) {
    return SQC_RESULT_TOO_LONG;
  }

  key_len = user_id_len + SQC_RPC_SCHED_USER_GROUP_KEY_SEP_LEN + group_id_len;
  if ((key_len + 1) > user_group_key_size) {
    return SQC_RESULT_INVALID_ARGS;
  }

  rc = snprintf(user_group_key, user_group_key_size, "%s%s%s",
                user_id, SQC_RPC_SCHED_USER_GROUP_KEY_SEP, group_id);
  if (rc < 0 || (size_t)rc != key_len) {
    return SQC_RESULT_ANY_RUNTIME_ERROR;
  }

  *user_group_key_len = (size_t)rc;

  return SQC_RESULT_OK;
}

