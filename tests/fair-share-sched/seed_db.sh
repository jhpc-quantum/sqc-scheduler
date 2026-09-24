#!/usr/bin/env bash
#
# seed_db.sh - Recreate the DB at DB_FILE (env.sh) and seed it with the
# groups (SCHED_GROUPS), test user and user<->group associations needed by
# specs/TEST01-fair-share-sched.md.
#
# Run this against a dedicated test environment's DB, not an
# existing/production one (this deletes and recreates the DB file). Must be
# run before each protocol run (RPC, then gRPC), with the sqc_rpc_sched
# process stopped.
#
# Usage:
#   ./seed_db.sh
#

# Load common path definitions and helper functions.
. "$(cd "$(dirname "$0")" && pwd)/env.sh"

DB_UTIL="${DB_SCRIPTS_DIR}/db-util.sh"

log_info "creating DB tables (this deletes any existing DB file)"
"${DB_SCRIPTS_DIR}/db-create.sh" --force || die "failed to create DB tables"

# "default" is seeded by seed_default_records.sql with a different
# exec_time_limit_msec than the other groups; align it so the RR initial
# order (sorted by remaining time, tie-break group_id strcmp ascending) is
# deterministic across all groups.
log_info "aligning group 'default' exec_time_limit_msec/exec_time_total_msec"
"${DB_UTIL}" group set exec_time_limit_msec default "${COMMON_EXEC_TIME_LIMIT_MSEC}" \
    || die "failed to set exec_time_limit_msec for group 'default'"
"${DB_UTIL}" group set exec_time_total_msec default "${COMMON_EXEC_TIME_TOTAL_MSEC}" \
    || die "failed to set exec_time_total_msec for group 'default'"

for g in "${SCHED_GROUPS[@]}"; do
    [ "${g}" = "default" ] && continue
    log_info "adding group ${g}"
    "${DB_UTIL}" group add "${g}" "${COMMON_EXEC_TIME_LIMIT_MSEC}" "${COMMON_EXEC_TIME_TOTAL_MSEC}" \
        || die "failed to add group ${g}"
done

# seed_default_records.sql also seeds group1..group4, which are not part of
# this test's groups (SCHED_GROUPS). Remove them so the RR ring only
# contains the groups configured above.
for g in group1 group2 group3 group4; do
    log_info "removing unrelated default-seeded group ${g}"
    "${DB_UTIL}" group delete "${g}" || die "failed to delete group ${g}"
done

log_info "adding test user ${TEST_USER}"
"${DB_UTIL}" user add "${TEST_USER}" general enable || die "failed to add user ${TEST_USER}"

for g in "${SCHED_GROUPS[@]}"; do
    log_info "adding user_group ${TEST_USER}/${g}"
    "${DB_UTIL}" user_group add "${TEST_USER}" "${g}" enable \
        || die "failed to add user_group ${TEST_USER}/${g}"
done

log_info "done."
