#!/usr/bin/env bash
#
# db-common.sh - Common path definitions and helper functions shared by
#                DB utility scripts in this directory.
#
# Every DB script sources this file. All locations can be overridden
# by exporting the corresponding environment variable before running a
# script, e.g.:
#
#   $ DB_FILE=/tmp/sqc-scheduler.db ./db-create.sh
#

# Fail fast: exit on error, undefined variable, or failed pipe element.
set -euo pipefail

# ---------------------------------------------------------------------------
# Path definitions
# ---------------------------------------------------------------------------

# SQLite executable path.
SQLITE="/usr/bin/sqlite3"

# Directory containing these DB scripts.
BUILD_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Root directory that contains the local application source trees.
PROJECT_ROOT="${PROJECT_ROOT:-$(cd "${BUILD_SCRIPT_DIR}/.." && pwd)}"

# Directory that contains SQL files for DB operations.
SQL_FILE_DIR=${SQL_FILE_DIR:-"${PROJECT_ROOT}/sqc-rpc-sched/sql"}

# Root directory for external sources and libraries.
EXTERNAL_ROOT="${EXTERNAL_ROOT:-${HOME}/.local}"

# SQLite database file path.
DB_FILE=${DB_FILE:-"${EXTERNAL_ROOT}/var/sqc-scheduler/sqc-scheduler.db"}

# Output mode option for SQLite (empty means default mode).
SQLITE_MODE=${SQLITE_MODE:-""}

# Header output option for SQLite.
SQLITE_HEADER=${SQLITE_HEADER:-"noheader"}

# Aggregated SQLite option flags used by caller scripts.
SQLITE_OPTIONS=""

# ===========================================================================
# common function
# ===========================================================================

# log_info <message>
# Print an informational message to stdout.
log_info() {
    echo "[INFO ] $*"
}

# die <message>
# Print an error message to stderr and terminate the script.
die() {
    echo "[ERROR] $*" >&2
    exit 1
}
