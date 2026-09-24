#include <sqlite3.h>

#include "sqc_apis.h"

#define SQC_RPC_SCHED_UT_DB_FILE "/tmp/unit_test_sqc_rpc_sched.db"

/*
 * Create table
 */
#define SQC_RPC_SCHED_UT_DB_USER_CREATE_TABLE \
  "CREATE TABLE IF NOT EXISTS user_info (" \
  "    user_id TEXT PRIMARY KEY NOT NULL," \
  "    role_type INTEGER NOT NULL DEFAULT 0," \
  "    status INTEGER NOT NULL DEFAULT 0," \
  "    created_time INTEGER NOT NULL DEFAULT 0," \
  "    update_time INTEGER NOT NULL DEFAULT 0" \
  ");"
#define SQC_RPC_SCHED_UT_DB_GROUP_CREATE_TABLE \
  "CREATE TABLE IF NOT EXISTS group_info (" \
  "    group_id TEXT PRIMARY KEY NOT NULL," \
  "    exec_time_limit_msec INTEGER NOT NULL DEFAULT 0 CHECK (exec_time_limit_msec >= 0)," \
  "    exec_time_total_msec INTEGER NOT NULL DEFAULT 0 CHECK (exec_time_total_msec >= 0)," \
  "    created_time INTEGER NOT NULL DEFAULT 0," \
  "    update_time INTEGER NOT NULL DEFAULT 0"\
  ");"
#define SQC_RPC_SCHED_UT_DB_USER_GROUP_CREATE_TABLE \
  "CREATE TABLE IF NOT EXISTS user_group_info (" \
  "    user_id TEXT NOT NULL," \
  "    group_id TEXT NOT NULL," \
  "    status INTEGER NOT NULL DEFAULT 0," \
  "    created_time INTEGER NOT NULL DEFAULT 0," \
  "    update_time INTEGER NOT NULL DEFAULT 0," \
  "    PRIMARY KEY (user_id, group_id)" \
  ");"
#define SQC_RPC_SCHED_UT_DB_JOB_CREATE_TABLE \
  "CREATE TABLE IF NOT EXISTS job_info (" \
  "    job_id TEXT PRIMARY KEY NOT NULL," \
  "    user_id TEXT NOT NULL," \
  "    group_id TEXT NOT NULL," \
  "    priority INTEGER NOT NULL DEFAULT 0," \
  "    status INTEGER NOT NULL DEFAULT 0," \
  "    qc_job_id TEXT," \
  "    qprogram TEXT," \
  "    circuit_fmt INTEGER," \
  "    shots INTEGER," \
  "    qc_type INTEGER NOT NULL DEFAULT 0," \
  "    transpiler INTEGER," \
  "    remark TEXT," \
  "    result TEXT," \
  "    exec_time_estimate_msec INTEGER NOT NULL DEFAULT 0 CHECK (exec_time_estimate_msec >= 0)," \
  "    exec_time_msec INTEGER NOT NULL DEFAULT 0 CHECK (exec_time_msec >= 0)," \
  "    created_time INTEGER NOT NULL DEFAULT 0," \
  "    queued_time INTEGER NOT NULL DEFAULT 0," \
  "    running_time INTEGER NOT NULL DEFAULT 0," \
  "    done_time INTEGER NOT NULL DEFAULT 0," \
  "    cancelled_time INTEGER NOT NULL DEFAULT 0," \
  "    error_time INTEGER NOT NULL DEFAULT 0," \
  "    deleted_time INTEGER NOT NULL DEFAULT 0," \
  "    update_time INTEGER NOT NULL DEFAULT 0" \
  ");"
#define SQC_RPC_SCHED_UT_DB_WEIGHT_CREATE_TABLE \
  "CREATE TABLE IF NOT EXISTS weight_info (" \
  "    priority INTEGER PRIMARY KEY NOT NULL, " \
  "    weight INTEGER NOT NULL DEFAULT 1000 CHECK (weight >= 0), " \
  "    created_time INTEGER NOT NULL DEFAULT 0, " \
  "    update_time INTEGER NOT NULL DEFAULT 0" \
  ");"

/*
 * Insert default records
 */
#define SQC_RPC_SCHED_UT_DB_WEIGHT_DEFAULT_RECORD \
  "INSERT OR IGNORE INTO weight_info " \
  "  (priority, weight, created_time, update_time) " \
  "VALUES " \
  "  (?, ?, ?, ?);"

#define SQC_RPC_SCHED_UT_DB_WEIGHT_DEFAULT_CREATED_TIME 1736474012
#define SQC_RPC_SCHED_UT_DB_WEIGHT_DEFAULT_UPDATE_TIME 1736474012


void create_db_files(const char *label) {
  int rc;
  sqlite3 *ut_sqc_rpc_sched_db;
  sqlite3_stmt *stmt;
  struct stat st;

  if (stat(SQC_RPC_SCHED_UT_DB_FILE, &st) != 0) {
    rc = sqlite3_open(SQC_RPC_SCHED_UT_DB_FILE, &ut_sqc_rpc_sched_db);
    if (rc != SQLITE_OK) {
      printf("[%s] Failed to create DB file: %s\n", label, SQC_RPC_SCHED_UT_DB_FILE);
      return;
    }

    // Create user_info table
    {
      sqlite3_prepare_v2(ut_sqc_rpc_sched_db, SQC_RPC_SCHED_UT_DB_USER_CREATE_TABLE, -1, &stmt, NULL);
      rc = sqlite3_step(stmt);
      sqlite3_finalize(stmt);

      if (rc != SQLITE_DONE) {
        printf("[%s] Failed to create user_info table: %s\n", label, sqlite3_errmsg(ut_sqc_rpc_sched_db));
        return;
      }
    }

    // Create group_info table
    {
      sqlite3_prepare_v2(ut_sqc_rpc_sched_db, SQC_RPC_SCHED_UT_DB_GROUP_CREATE_TABLE, -1, &stmt, NULL);
      rc = sqlite3_step(stmt);
      sqlite3_finalize(stmt);

      if (rc != SQLITE_DONE) {
        printf("[%s] Failed to create group_info table: %s\n", label, sqlite3_errmsg(ut_sqc_rpc_sched_db));
        return;
      }
    }

    // Create user_group_info table
    {
      sqlite3_prepare_v2(ut_sqc_rpc_sched_db, SQC_RPC_SCHED_UT_DB_USER_GROUP_CREATE_TABLE, -1, &stmt, NULL);
      rc = sqlite3_step(stmt);
      sqlite3_finalize(stmt);

      if (rc != SQLITE_DONE) {
        printf("[%s] Failed to create user_group_info table: %s\n", label, sqlite3_errmsg(ut_sqc_rpc_sched_db));
        return;
      }
    }

    // Create job_info table
    {
      sqlite3_prepare_v2(ut_sqc_rpc_sched_db, SQC_RPC_SCHED_UT_DB_JOB_CREATE_TABLE, -1, &stmt, NULL);
      rc = sqlite3_step(stmt);
      sqlite3_finalize(stmt);

      if (rc != SQLITE_DONE) {
        printf("[%s] Failed to create job_info table: %s\n", label, sqlite3_errmsg(ut_sqc_rpc_sched_db));
        return;
      }
    }

    // Create weight_info table
    {
      sqlite3_prepare_v2(ut_sqc_rpc_sched_db, SQC_RPC_SCHED_UT_DB_WEIGHT_CREATE_TABLE, -1, &stmt, NULL);
      rc = sqlite3_step(stmt);
      sqlite3_finalize(stmt);
      if (rc != SQLITE_DONE) {
        printf("[%s] Failed to create weight_info table: %s\n", label, sqlite3_errmsg(ut_sqc_rpc_sched_db));
        return;
      }
    }

    // Insert default weight_info records
    {
      sqlite3_prepare_v2(ut_sqc_rpc_sched_db, SQC_RPC_SCHED_UT_DB_WEIGHT_DEFAULT_RECORD, -1, &stmt, NULL);
      for (int priority = 0; priority <= SQC_RPC_SCHED_MAX_PRIORITY; priority++) {
        sqlite3_bind_int(stmt, 1, priority);
        sqlite3_bind_int64(stmt, 2, (sqlite3_int64)priority * (sqlite3_int64)SQC_RPC_SCHED_WEIGHT_SCALE);
        sqlite3_bind_int64(stmt, 3, SQC_RPC_SCHED_UT_DB_WEIGHT_DEFAULT_CREATED_TIME);
        sqlite3_bind_int64(stmt, 4, SQC_RPC_SCHED_UT_DB_WEIGHT_DEFAULT_UPDATE_TIME);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
          sqlite3_finalize(stmt);
          printf("[%s] Failed to insert default weight_info record: %s\n", label, sqlite3_errmsg(ut_sqc_rpc_sched_db));
          return;
        }

        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
      }
      sqlite3_finalize(stmt);
    }

    rc = sqlite3_close(ut_sqc_rpc_sched_db);
    if (rc != SQLITE_OK) {
      printf("[%s] Failed to close DB file: %s\n", label, SQC_RPC_SCHED_UT_DB_FILE);
      return;
    }

    printf("[%s] create DB file: %s\n", label, SQC_RPC_SCHED_UT_DB_FILE);
    return;
  }

  printf("[%s] DB file already exists: %s\n", label, SQC_RPC_SCHED_UT_DB_FILE);
}

void remove_db_files(const char *label) {
  struct stat st;

  if (stat(SQC_RPC_SCHED_UT_DB_FILE, &st) == 0) {
    if (remove(SQC_RPC_SCHED_UT_DB_FILE) == 0) {
      printf("[%s] remove DB file: %s\n", label, SQC_RPC_SCHED_UT_DB_FILE);
    }
  }
}

