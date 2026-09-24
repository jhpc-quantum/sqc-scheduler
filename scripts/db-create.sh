#!/usr/bin/env bash
#
# db-create.sh - Create DB tables.
#
# Usage:
#   ./db-create.sh [--no-wal] [--force]
#
# Options:
#   --no-wal  Skip setting WAL journal mode after table creation.
#   --force   Remove an existing DB file before creating a new one.
#

# Load DB-related path definitions and helper functions.
. "$(cd "$(dirname "$0")" && pwd)/db-common.sh"

# Parse arguments.
ENABLE_WAL=1
ENABLE_FORCE=0
for arg in "$@"; do
    case "${arg}" in
        --no-wal) ENABLE_WAL=0 ;;
        --force) ENABLE_FORCE=1 ;;
        *) die "unknown option: ${arg}" ;;
    esac
done

CREATE_TABLE_SQL="${SQL_FILE_DIR}/create_table.sql"
[ -f "${CREATE_TABLE_SQL}" ] || die "SQL file not found: ${CREATE_TABLE_SQL}"

SEED_DEFAULT_SQL="${SQL_FILE_DIR}/seed_default_records.sql"
[ -f "${SEED_DEFAULT_SQL}" ] || die "SQL file not found: ${SEED_DEFAULT_SQL}"

if [ -e "${DB_FILE}" ]; then
    if [ "${ENABLE_FORCE}" -eq 1 ]; then
        log_info "removing existing DB file: ${DB_FILE}"
        rm -f "${DB_FILE}" "${DB_FILE}-wal" "${DB_FILE}-shm" \
            || die "failed to remove existing DB file: ${DB_FILE}"
    else
        die "DB file already exists: ${DB_FILE}"
    fi
fi

DB_DIR="$(dirname "${DB_FILE}")"
mkdir -p "${DB_DIR}" || die "failed to create DB directory: ${DB_DIR}"

log_info "creating tables in ${DB_FILE} using ${CREATE_TABLE_SQL}"
"${SQLITE}" "${SQLITE_OPTIONS[@]}" "${DB_FILE}" < "${CREATE_TABLE_SQL}" \
    || die "failed to create DB tables"
log_info "DB tables were created successfully"

log_info "seeding default records in ${DB_FILE} using ${SEED_DEFAULT_SQL}"
"${SQLITE}" "${SQLITE_OPTIONS[@]}" "${DB_FILE}" < "${SEED_DEFAULT_SQL}" \
    || die "failed to seed default records"
log_info "Default records were seeded successfully"

if [ "${ENABLE_WAL}" -eq 1 ]; then
    log_info "setting WAL journal mode on ${DB_FILE}"
    "${SQLITE}" "${SQLITE_OPTIONS[@]}" "${DB_FILE}" "PRAGMA journal_mode=WAL;" \
        || die "failed to set WAL journal mode"
    log_info "WAL journal mode was set successfully"
fi

log_info "DB creation process completed successfully"
