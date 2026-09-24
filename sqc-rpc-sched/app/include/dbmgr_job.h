#pragma once

#include "sqc_rpc_sched.h"

#include "dbmgr_types.h"

__BEGIN_DECLS

sqc_result_t
dbmgr_ji_create_job(const char *user_id, const char *group_id, const uint8_t priority,
                    const char *qprogram, sqc_rpc_sched_circuit_fmt_t circuit_fmt, size_t shots,
                    sqc_rpc_sched_qc_type_t qc_type, sqc_rpc_sched_transpiler_t transpiler,
                    const char *remark, const char *user_token, dbmgr_job_info_t *ji_ptr);

sqc_result_t
dbmgr_ji_delete_job(const char *job_id);

bool
dbmgr_ji_job_exists(const char *job_id);

bool
dbmgr_ji_is_deletable(const dbmgr_job_info_t ji_ptr);

sqc_result_t
dbmgr_ji_job_find(const char *job_id, dbmgr_job_info_t *ji_ptr);

sqc_result_t
dbmgr_ji_job_find_by_user_id(const char *target_user_id,
                             dbmgr_job_info_t **ji_ptr_arr, size_t *arr_len);

sqc_result_t
dbmgr_ji_job_find_by_job_status(const sqc_rpc_sched_job_status_t target_status,
                                dbmgr_job_info_t **ji_ptr_arr, size_t *arr_len);

sqc_result_t
dbmgr_ji_job_find_by_created_time(const sqc_chrono_t from_time, const sqc_chrono_t to_time,
                                  dbmgr_job_info_t **ji_ptr_arr, size_t *arr_len);

sqc_result_t
dbmgr_ji_job_find_by_delete_target(const char *target_user_id,
                                   const sqc_chrono_t from_time, const sqc_chrono_t to_time,
                                   dbmgr_job_info_t **ji_ptr_arr, size_t *arr_len);

sqc_result_t
dbmgr_ji_get_job_id(const dbmgr_job_info_t ji_ptr, char **job_id);

sqc_result_t
dbmgr_ji_get_job_id_len(const dbmgr_job_info_t ji_ptr, size_t *job_id_len);

sqc_result_t
dbmgr_ji_get_user_id(const dbmgr_job_info_t ji_ptr, char **user_id);

sqc_result_t
dbmgr_ji_get_user_id_len(const dbmgr_job_info_t ji_ptr, size_t *user_id_len);

sqc_result_t
dbmgr_ji_get_group_id(const dbmgr_job_info_t ji_ptr, char **group_id);

sqc_result_t
dbmgr_ji_get_group_id_len(const dbmgr_job_info_t ji_ptr, size_t *group_id_len);

sqc_result_t
dbmgr_ji_get_priority(const dbmgr_job_info_t ji_ptr, uint8_t *priority);

sqc_result_t
dbmgr_ji_get_status(const dbmgr_job_info_t ji_ptr, sqc_rpc_sched_job_status_t *status);

sqc_result_t
dbmgr_set_job_status_queued(const dbmgr_job_info_t ji_ptr);

sqc_result_t
dbmgr_set_job_status_running(const dbmgr_job_info_t ji_ptr);

sqc_result_t
dbmgr_set_job_status_done(const dbmgr_job_info_t ji_ptr);

sqc_result_t
dbmgr_set_job_status_cancelled(const dbmgr_job_info_t ji_ptr);

sqc_result_t
dbmgr_set_job_status_error(const dbmgr_job_info_t ji_ptr);

sqc_result_t
dbmgr_set_job_status_deleted(const dbmgr_job_info_t ji_ptr);

sqc_result_t
dbmgr_ji_set_qc_job_id(dbmgr_job_info_t ji_ptr, const char *qc_job_id);

sqc_result_t
dbmgr_ji_get_qc_job_id(const dbmgr_job_info_t ji_ptr, char **qc_job_id);

sqc_result_t
dbmgr_ji_get_qc_job_id_len(const dbmgr_job_info_t ji_ptr, size_t *qc_job_id_len);

sqc_result_t
dbmgr_ji_get_qprogram(const dbmgr_job_info_t ji_ptr, char **qprogram);

sqc_result_t
dbmgr_ji_get_qprogram_len(const dbmgr_job_info_t ji_ptr, size_t *qprogram_len);

sqc_result_t
dbmgr_ji_get_circuit_fmt(const dbmgr_job_info_t ji_ptr, sqc_rpc_sched_circuit_fmt_t *circuit_fmt);

sqc_result_t
dbmgr_ji_get_shots(const dbmgr_job_info_t ji_ptr, size_t *shots);

sqc_result_t
dbmgr_ji_get_qc_type(const dbmgr_job_info_t ji_ptr, sqc_rpc_sched_qc_type_t *qc_type);

sqc_result_t
dbmgr_ji_get_transpiler(const dbmgr_job_info_t ji_ptr, sqc_rpc_sched_transpiler_t *transpiler);

sqc_result_t
dbmgr_ji_get_remark(const dbmgr_job_info_t ji_ptr, char **remark);

sqc_result_t
dbmgr_ji_get_remark_len(const dbmgr_job_info_t ji_ptr, size_t *remark_len);

sqc_result_t
dbmgr_ji_set_result(dbmgr_job_info_t ji_ptr, const char *result, size_t result_len);

sqc_result_t
dbmgr_ji_get_result(const dbmgr_job_info_t ji_ptr, char **result);

sqc_result_t
dbmgr_ji_get_result_len(const dbmgr_job_info_t ji_ptr, size_t *result_len);

sqc_result_t
dbmgr_ji_get_user_token(const dbmgr_job_info_t ji_ptr, char **user_token);

sqc_result_t
dbmgr_ji_get_user_token_len(const dbmgr_job_info_t ji_ptr, size_t *user_token_len);

sqc_result_t
dbmgr_ji_get_exec_time_estimate_msec(const dbmgr_job_info_t ji_ptr,
                                     uint64_t *exec_time_estimate_msec);

sqc_result_t
dbmgr_ji_set_exec_time_estimate_msec(dbmgr_job_info_t ji_ptr,
                                     uint64_t exec_time_estimate_msec);

sqc_result_t
dbmgr_ji_get_exec_time_msec(const dbmgr_job_info_t ji_ptr,
                            uint64_t *exec_time_msec);

sqc_result_t
dbmgr_ji_set_exec_time_msec(dbmgr_job_info_t ji_ptr, uint64_t exec_time_msec);

sqc_result_t
dbmgr_ji_get_created_time(const dbmgr_job_info_t ji_ptr, sqc_chrono_t *created_time);

sqc_result_t
dbmgr_ji_get_queued_time(const dbmgr_job_info_t ji_ptr, sqc_chrono_t *queued_time);

sqc_result_t
dbmgr_ji_get_running_time(const dbmgr_job_info_t ji_ptr, sqc_chrono_t *running_time);

sqc_result_t
dbmgr_ji_get_done_time(const dbmgr_job_info_t ji_ptr, sqc_chrono_t *done_time);

sqc_result_t
dbmgr_ji_get_cancelled_time(const dbmgr_job_info_t ji_ptr, sqc_chrono_t *cancelled_time);

sqc_result_t
dbmgr_ji_get_error_time(const dbmgr_job_info_t ji_ptr, sqc_chrono_t *error_time);

sqc_result_t
dbmgr_ji_get_deleted_time(const dbmgr_job_info_t ji_ptr, sqc_chrono_t *deleted_time);

sqc_result_t
dbmgr_ji_get_update_time(const dbmgr_job_info_t ji_ptr, sqc_chrono_t *update_time);

sqc_result_t
dbmgr_ji_job_arr_sort_by_created_time(dbmgr_job_info_t *ji_ptr_arr, const size_t arr_len);

sqc_result_t
dbmgr_ji_clear_job_result(const dbmgr_job_info_t ji_ptr);

sqc_result_t
dbmgr_ji_clear_job_arr_result(dbmgr_job_info_t *ji_ptr_arr, const size_t arr_len);

__END_DECLS

