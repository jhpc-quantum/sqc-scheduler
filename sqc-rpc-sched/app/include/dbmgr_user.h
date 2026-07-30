#pragma once

#include "sqc_rpc_sched.h"

#include "dbmgr_types.h"

__BEGIN_DECLS

sqc_result_t
dbmgr_ui_create_user(const char *user_id, dbmgr_user_info_t *ui_ptr);

sqc_result_t
dbmgr_ui_delete_user(const char *user_id);

bool
dbmgr_ui_user_exists(const char *user_id);

sqc_result_t
dbmgr_ui_user_find(const char *user_id, dbmgr_user_info_t *ui_ptr);

sqc_result_t
dbmgr_ui_set_user_id(dbmgr_user_info_t ui_ptr, const char *user_id);

sqc_result_t
dbmgr_ui_get_user_id(const dbmgr_user_info_t ui_ptr, char **user_id);

sqc_result_t
dbmgr_ui_get_user_id_len(const dbmgr_user_info_t ui_ptr, size_t *user_id_len);

sqc_result_t
dbmgr_ui_set_user_role_type(const dbmgr_user_info_t ui_ptr, const sqc_rpc_sched_user_role_type_t user_role_type);

sqc_result_t
dbmgr_ui_get_user_role_type(const dbmgr_user_info_t ui_ptr, sqc_rpc_sched_user_role_type_t *user_role_type);

bool
dbmgr_ui_is_user_admin(const dbmgr_user_info_t ui_ptr);

sqc_result_t
dbmgr_ui_set_user_enabled(const dbmgr_user_info_t ui_ptr);

sqc_result_t
dbmgr_ui_set_user_disabled(const dbmgr_user_info_t ui_ptr);

bool
dbmgr_ui_is_user_enabled(const dbmgr_user_info_t ui_ptr);

sqc_result_t
dbmgr_ui_get_created_time(const dbmgr_user_info_t ui_ptr, sqc_chrono_t *created_time);

sqc_result_t
dbmgr_ui_get_update_time(const dbmgr_user_info_t ui_ptr, sqc_chrono_t *update_time);

__END_DECLS

