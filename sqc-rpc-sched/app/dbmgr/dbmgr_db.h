#pragma once

#include "sqc_apis.h"
#include "sqc_rpc_sched.h"

#include "dbmgr_types.h"

__BEGIN_DECLS

sqc_result_t
dbmgr_db_user_info_deserialize_all(sqc_hashmap_t *user_info_hashmap);

sqc_result_t
dbmgr_db_group_info_deserialize_all(sqc_hashmap_t *group_info_hashmap);

sqc_result_t
dbmgr_db_user_group_info_deserialize_all(sqc_hashmap_t *user_group_info_hashmap);

sqc_result_t
dbmgr_db_job_info_deserialize_all(sqc_hashmap_t *job_info_hashmap);

sqc_result_t
dbmgr_db_weight_info_deserialize_all(sqc_hashmap_t *weight_info_hashmap);

sqc_result_t
dbmgr_db_initialize(void);

void
dbmgr_db_finalize(void);

/*
 * user_info
 */
sqc_result_t
dbmgr_db_user_info_insert_record(const char *user_id, const uint8_t role_type, const uint8_t status,
                                 const int64_t created_time, const int64_t update_time);

sqc_result_t
dbmgr_db_user_info_update_role_type(const char *user_id, const uint8_t role_type,
                                    const int64_t update_time);

sqc_result_t
dbmgr_db_user_info_update_status(const char *user_id, const uint8_t status,
                                 const int64_t update_time);

/*
 * group_info
 */
sqc_result_t
dbmgr_db_group_info_insert_record(const char *group_id, const uint64_t exec_time_limit_msec,
                                  const uint64_t exec_time_total_msec,
                                  const int64_t created_time, const int64_t update_time);

sqc_result_t
dbmgr_db_group_info_update_exec_time_limit_msec(const char *group_id,
                                                const uint64_t exec_time_limit_msec,
                                                const int64_t update_time);

sqc_result_t
dbmgr_db_group_info_update_exec_time_total_msec(const char *group_id,
                                                const uint64_t exec_time_total_msec,
                                                const int64_t update_time);

/*
 * user_group_info
 */
sqc_result_t
dbmgr_db_user_group_info_insert_record(const char *user_id, const char *group_id,
                                       const uint8_t status, const int64_t created_time,
                                       const int64_t update_time);

sqc_result_t
dbmgr_db_user_group_info_update_status(const char *user_id, const char *group_id,
                                       const uint8_t status, const int64_t update_time);

/*
 * job_info
 */

sqc_result_t
dbmgr_db_job_info_insert_record(const char *job_id, const char *user_id, const char *group_id,
                                const uint8_t priority, const uint8_t status,
                                const char *qprogram, const int circuit_fmt, const size_t shots,
                                const int qc_type, const int transpiler,
                                const char *remark, const int64_t created_time,
                                const int64_t update_time);

sqc_result_t
dbmgr_db_job_info_update_queued_status(const char *job_id, const int64_t queued_time,
                                       const int64_t update_time);

sqc_result_t
dbmgr_db_job_info_update_running_status(const char *job_id, const int64_t running_time,
                                        const int64_t update_time);

sqc_result_t
dbmgr_db_job_info_update_done_status(const char *job_id, const int64_t done_time,
                                     const int64_t update_time);

sqc_result_t
dbmgr_db_job_info_update_cancelled_status(const char *job_id, const int64_t cancelled_time,
                                          const int64_t update_time);

sqc_result_t
dbmgr_db_job_info_update_error_status(const char *job_id, const int64_t error_time,
                                      const int64_t update_time);

sqc_result_t
dbmgr_db_job_info_update_deleted_status(const char *job_id, const int64_t deleted_time,
                                        const int64_t update_time);

sqc_result_t
dbmgr_db_job_info_update_qc_job_id(const char *job_id, const char *qc_job_id,
                                   const int64_t update_time);

sqc_result_t
dbmgr_db_job_info_update_result(const char *job_id, const char *result,
                                const int64_t update_time);

sqc_result_t
dbmgr_db_job_info_update_exec_time_estimate_msec(const char *job_id,
                                                 const uint64_t exec_time_estimate_msec,
                                                 const int64_t update_time);

sqc_result_t
dbmgr_db_job_info_update_exec_time_msec(const char *job_id,
                                        const uint64_t exec_time_msec,
                                        const int64_t update_time);

sqc_result_t
dbmgr_db_job_info_clear_result(const char *job_id, const int64_t deleted_time,
                               const int64_t update_time);

sqc_result_t
dbmgr_db_job_info_arr_clear_result(const char *const *job_id_arr, const size_t arr_len,
                                   const int64_t deleted_time, const int64_t update_time);

/*
 * weight_info
 */
sqc_result_t
dbmgr_db_weight_info_insert_record(const uint8_t priority, const uint64_t weight,
                                   const int64_t created_time, const int64_t update_time);

sqc_result_t
dbmgr_db_weight_info_update_weight(const uint8_t priority, const uint64_t weight,
                                   const int64_t update_time);

__END_DECLS

