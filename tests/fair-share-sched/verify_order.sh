#!/usr/bin/env bash
#
# verify_order.sh - Wait for all jobs recorded by submit_jobs.sh to reach a
# terminal status, then compare the order of job_info.running_time against
# EXPECTED_ORDER (env.sh) and report PASS/FAIL.
#
# Usage:
#   ./verify_order.sh
#

# Load common path definitions and helper functions.
. "$(cd "$(dirname "$0")" && pwd)/env.sh"

TIMEOUT_SECS="${TIMEOUT_SECS:-300}"
POLL_INTERVAL_SECS="${POLL_INTERVAL_SECS:-2}"

[ -s "${JOB_IDS_CSV}" ] || die "no job IDs recorded; run submit_jobs.sh first"

mapfile -t JOB_IDS < <(awk -F',' '{print $1}' "${JOB_IDS_CSV}")
JOB_ID_LIST=$(printf "'%s'," "${JOB_IDS[@]}")
JOB_ID_LIST=${JOB_ID_LIST%,}

# Fail fast (instead of timing out or ending up with a confusing row-count
# mismatch later) if DB_FILE doesn't actually contain these job IDs -- e.g.
# because it's a different host's DB than the one JOB_IDS_CSV was submitted
# against. See README.md "Steps".
if ! found=$("${SQLITE}" "${DB_FILE}" \
    "SELECT COUNT(*) FROM job_info WHERE job_id IN (${JOB_ID_LIST});"); then
    die "failed to query DB_FILE=${DB_FILE} (sqlite3 exited non-zero)"
fi
if [ "${found}" -ne "${#JOB_IDS[@]}" ]; then
    die "only ${found}/${#JOB_IDS[@]} job IDs from ${JOB_IDS_CSV} exist in DB_FILE=${DB_FILE}; is DB_FILE the DB these jobs were actually submitted against?"
fi

log_info "waiting for all ${#JOB_IDS[@]} jobs to reach a terminal status (timeout ${TIMEOUT_SECS}s)"
elapsed=0
while :; do
    if ! pending=$("${SQLITE}" "${DB_FILE}" \
        "SELECT COUNT(*) FROM job_info WHERE job_id IN (${JOB_ID_LIST}) AND status NOT IN (4,5,6);"); then
        die "failed to query DB_FILE=${DB_FILE} (sqlite3 exited non-zero)"
    fi
    [ "${pending}" -eq 0 ] && break
    if [ "${elapsed}" -ge "${TIMEOUT_SECS}" ]; then
        die "timed out waiting for jobs to complete (${pending} still pending)"
    fi
    sleep "${POLL_INTERVAL_SECS}"
    elapsed=$((elapsed + POLL_INTERVAL_SECS))
done
log_info "all jobs reached a terminal status"

if ! failed=$("${SQLITE}" "${DB_FILE}" \
    "SELECT COUNT(*) FROM job_info WHERE job_id IN (${JOB_ID_LIST}) AND status IN (5,6);"); then
    die "failed to query DB_FILE=${DB_FILE} (sqlite3 exited non-zero)"
fi
if [ "${failed}" -gt 0 ]; then
    echo "FAIL: ${failed} job(s) ended as cancelled/error" >&2
    "${SQLITE}" -header -column "${DB_FILE}" \
        "SELECT job_id, group_id, priority, status FROM job_info WHERE job_id IN (${JOB_ID_LIST}) AND status IN (5,6);"
    exit 1
fi

# job_id -> "group:priority:occurrence" label
declare -A LABEL_OF
while IFS=',' read -r job_id group priority occurrence; do
    LABEL_OF["${job_id}"]="${group}:${priority}:${occurrence}"
done < "${JOB_IDS_CSV}"

mapfile -t ORDERED_JOB_IDS < <("${SQLITE}" "${DB_FILE}" \
    "SELECT job_id FROM job_info WHERE job_id IN (${JOB_ID_LIST}) ORDER BY running_time ASC;")

if [ "${#ORDERED_JOB_IDS[@]}" -ne "${#EXPECTED_ORDER[@]}" ]; then
    echo "FAIL: expected ${#EXPECTED_ORDER[@]} jobs, got ${#ORDERED_JOB_IDS[@]} rows from DB" >&2
    exit 1
fi

mismatch=0
for i in "${!EXPECTED_ORDER[@]}"; do
    actual_label="${LABEL_OF[${ORDERED_JOB_IDS[$i]}]:-<unknown:${ORDERED_JOB_IDS[$i]}>}"
    expected_label="${EXPECTED_ORDER[$i]}"
    if [ "${actual_label}" = "${expected_label}" ]; then
        printf '  [%2d] OK   %s\n' "$((i + 1))" "${actual_label}"
    else
        printf '  [%2d] DIFF expected=%s actual=%s\n' "$((i + 1))" "${expected_label}" "${actual_label}"
        mismatch=$((mismatch + 1))
    fi
done

if [ "${mismatch}" -eq 0 ]; then
    log_info "PASS: dispatch order matches EXPECTED_ORDER (${#EXPECTED_ORDER[@]} jobs)"
else
    log_info "FAIL: ${mismatch} position(s) differ from EXPECTED_ORDER"
    exit 1
fi
