#include <sqlite3.h>

#include "sqc_apis.h"

#define SQC_RPC_SCHED_UT_DB_FILE "/tmp/unit_test_sqc_rpc_sched.db"
#define SQC_RPC_SCHED_UT_DB_USER_CREATE_TABLE \
  "CREATE TABLE IF NOT EXISTS user_info (" \
  "    user_id TEXT PRIMARY KEY," \
  "    role_type INTEGER," \
  "    status INTEGER," \
  "    created_time INTEGER," \
  "    update_time INTEGER" \
  ");"
#define SQC_RPC_SCHED_UT_DB_JOB_CREATE_TABLE \
  "CREATE TABLE IF NOT EXISTS job_info (" \
  "    job_id TEXT PRIMARY KEY," \
  "    user_id TEXT," \
  "    priority INTEGER," \
  "    status INTEGER," \
  "    qc_job_id TEXT," \
  "    qprogram TEXT," \
  "    circuit_fmt INTEGER," \
  "    shots INTEGER," \
  "    qc_type INTEGER," \
  "    transpiler INTEGER," \
  "    remark TEXT," \
  "    result TEXT," \
  "    created_time INTEGER," \
  "    queued_time INTEGER," \
  "    running_time INTEGER," \
  "    done_time INTEGER," \
  "    cancelled_time INTEGER," \
  "    error_time INTEGER," \
  "    deleted_time INTEGER," \
  "    update_time INTEGER" \
  ");"

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

    // create user_info table
    {
      sqlite3_prepare_v2(ut_sqc_rpc_sched_db, SQC_RPC_SCHED_UT_DB_USER_CREATE_TABLE, -1, &stmt, NULL);
      rc = sqlite3_step(stmt);
      sqlite3_finalize(stmt);

      if (rc != SQLITE_DONE) {
        printf("[%s] Failed to create user_info table: %s\n", label, sqlite3_errmsg(ut_sqc_rpc_sched_db));
        return;
      }
    }

    // create job_info table
    {
      sqlite3_prepare_v2(ut_sqc_rpc_sched_db, SQC_RPC_SCHED_UT_DB_JOB_CREATE_TABLE, -1, &stmt, NULL);
      rc = sqlite3_step(stmt);
      sqlite3_finalize(stmt);

      if (rc != SQLITE_DONE) {
        printf("[%s] Failed to create job_info table: %s\n", label, sqlite3_errmsg(ut_sqc_rpc_sched_db));
        return;
      }
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

