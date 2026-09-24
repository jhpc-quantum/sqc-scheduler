#pragma once

#include "sqc_rpc_sched.h"

#include "dbmgr_types.h"

__BEGIN_DECLS

sqc_result_t
dbmgr_ugi_add_user_to_group(const char *user_id, const char *group_id);

sqc_result_t
dbmgr_ugi_delete_user_from_group(const char *user_id, const char *group_id);

sqc_result_t
dbmgr_ugi_user_find(const char *user_id, const char *group_id,
                    dbmgr_user_group_info_t *ugi_ptr);

bool
dbmgr_ugi_is_user_in_group(const char *user_id, const char *group_id);

sqc_result_t
dbmgr_ugi_get_user_id(const dbmgr_user_group_info_t ugi_ptr, char **user_id);

sqc_result_t
dbmgr_ugi_get_user_id_len(const dbmgr_user_group_info_t ugi_ptr, size_t *user_id_len);

sqc_result_t
dbmgr_ugi_get_group_id(const dbmgr_user_group_info_t ugi_ptr, char **group_id);

sqc_result_t
dbmgr_ugi_get_group_id_len(const dbmgr_user_group_info_t ugi_ptr, size_t *group_id_len);

sqc_result_t
dbmgr_ugi_set_user_group_enabled(const dbmgr_user_group_info_t ugi_ptr);

sqc_result_t
dbmgr_ugi_set_user_group_disabled(const dbmgr_user_group_info_t ugi_ptr);

bool
dbmgr_ugi_is_user_group_enabled(const dbmgr_user_group_info_t ugi_ptr);

sqc_result_t
dbmgr_ugi_get_created_time(const dbmgr_user_group_info_t ugi_ptr, sqc_chrono_t *created_time);

sqc_result_t
dbmgr_ugi_get_update_time(const dbmgr_user_group_info_t ugi_ptr, sqc_chrono_t *update_time);

__END_DECLS

