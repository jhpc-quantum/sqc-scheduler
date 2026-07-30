#!/usr/bin/env bash
#
# db-create.sh - Create DB tables.
#
# Usage:
#   ./db-create.sh [--no-wal]
#
# Options:
#   --no-wal  Skip setting WAL journal mode after table creation.
#

# Load DB-related path definitions and helper functions.
. "$(cd "$(dirname "$0")" && pwd)/db-common.sh"

# Parse arguments.
ENABLE_WAL=1
for arg in "$@"; do
    case "${arg}" in
        --no-wal) ENABLE_WAL=0 ;;
        *) die "unknown option: ${arg}" ;;
    esac
done

CREATE_TABLE_SQL="${SQL_FILE_DIR}/create_table.sql"
[ -f "${CREATE_TABLE_SQL}" ] || die "SQL file not found: ${CREATE_TABLE_SQL}"

[ -e "${DB_FILE}" ] && die "DB file already exists: ${DB_FILE}"

DB_DIR="$(dirname "${DB_FILE}")"
mkdir -p "${DB_DIR}" || die "failed to create DB directory: ${DB_DIR}"

log_info "creating tables in ${DB_FILE} using ${CREATE_TABLE_SQL}"
"${SQLITE}" "${SQLITE_OPTIONS:-}" "${DB_FILE}" < "${CREATE_TABLE_SQL}" \
    || die "failed to create DB tables"

if [ "${ENABLE_WAL}" -eq 1 ]; then
    log_info "setting WAL journal mode on ${DB_FILE}"
    "${SQLITE}" "${SQLITE_OPTIONS:-}" "${DB_FILE}" "PRAGMA journal_mode=WAL;" \
        || die "failed to set WAL journal mode"
fi

log_info "DB tables were created successfully"
