#pragma once

#include "sqc_rpc_sched.h"

#include "dbmgr_types.h"

__BEGIN_DECLS

sqc_result_t
dbmgr_gi_create_group(const char *group_id,
                      const uint64_t exec_time_limit_msec,
                      const uint64_t exec_time_total_msec,
                      dbmgr_group_info_t *gi_ptr);

sqc_result_t
dbmgr_gi_delete_group(const char *group_id);

bool
dbmgr_gi_group_exists(const char *group_id);

sqc_result_t
dbmgr_gi_group_find(const char *group_id, dbmgr_group_info_t *gi_ptr);

sqc_result_t
dbmgr_gi_group_find_all_sorted_by_remaining_time(dbmgr_group_info_t **gi_ptr_arr,
                                                 size_t *arr_len);

sqc_result_t
dbmgr_gi_get_group_id(const dbmgr_group_info_t gi_ptr, char **group_id);

sqc_result_t
dbmgr_gi_get_group_id_len(const dbmgr_group_info_t gi_ptr, size_t *group_id_len);

sqc_result_t
dbmgr_gi_set_exec_time_limit_msec(const dbmgr_group_info_t gi_ptr,
                                  uint64_t exec_time_limit_msec);

sqc_result_t
dbmgr_gi_get_exec_time_limit_msec(const dbmgr_group_info_t gi_ptr,
                                  uint64_t *exec_time_limit_msec);

sqc_result_t
dbmgr_gi_get_exec_time_total_msec(const dbmgr_group_info_t gi_ptr,
                                  uint64_t *exec_time_total_msec);

sqc_result_t
dbmgr_gi_add_exec_time_total_msec(const dbmgr_group_info_t gi_ptr,
                                  uint64_t exec_time_msec);

sqc_result_t
dbmgr_gi_subtract_exec_time_total_msec(const dbmgr_group_info_t gi_ptr,
                                       uint64_t exec_time_msec);

sqc_result_t
dbmgr_gi_get_created_time(const dbmgr_group_info_t gi_ptr, sqc_chrono_t *created_time);

sqc_result_t
dbmgr_gi_get_update_time(const dbmgr_group_info_t gi_ptr, sqc_chrono_t *update_time);

__END_DECLS

