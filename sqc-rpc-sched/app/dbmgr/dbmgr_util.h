#pragma once

#include "sqc_apis.h"

#include "dbmgr_types.h"

__BEGIN_DECLS

sqc_result_t
dbmgr_util_create_user_group_key(char *user_group_key, size_t user_group_key_size,
                                 const char *user_id, const char *group_id,
                                 size_t *user_group_key_len);

__END_DECLS

