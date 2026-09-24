#!/usr/bin/env bash
#
# env.sh - Common path definitions and settings shared by the
#          TEST01-fair-share-sched integration test scripts in this
#          directory.
#
# Every script in this directory sources this file. Most locations/settings
# can be overridden by exporting the corresponding environment variable
# before running a script, e.g.:
#
#   $ RPC_SERVER=10.0.0.1:30001 ./submit_jobs.sh rpc
#

# Fail fast: exit on error, undefined variable, or failed pipe element.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
DB_SCRIPTS_DIR="${REPO_ROOT}/scripts"

# Working directory for this test's own generated artifacts (currently just
# the job_id -> label map produced by submit_jobs.sh). Not checked into git;
# see .gitignore in this directory.
WORK_DIR="${SCRIPT_DIR}/work"

# ---------------------------------------------------------------------------
# Server addresses (adjust to match how sqc_rpc_sched is actually started)
# ---------------------------------------------------------------------------
RPC_SERVER="${RPC_SERVER:-127.0.0.1:30001}"
GRPC_SERVER="${GRPC_SERVER:-127.0.0.1:30002}"

# Auth conf-dir(s), prepared manually beforehand (see README.md).
RPC_CLIENT_CONF_DIR="${RPC_CLIENT_CONF_DIR:-${HOME}/.sqc-scheduler}"
GRPC_CLIENT_CONF_DIR="${GRPC_CLIENT_CONF_DIR:-${HOME}/.sqc-scheduler}"

# DB user_id to seed and associate with each group in SCHED_GROUPS. Must
# match the "sub" claim of the JWT token already placed in the conf-dirs
# above.
TEST_USER="${TEST_USER:-test01-fair-share-sched-user}"

# Groups. RR initial order is determined by ascending group_id (strcmp)
# when every group's exec_time_limit_msec/exec_time_total_msec is
# identical, so keep these two values equal across all groups below.
SCHED_GROUPS=(default grpA grpB grpC grpD)
COMMON_EXEC_TIME_LIMIT_MSEC=1000000000
COMMON_EXEC_TIME_TOTAL_MSEC=0

# Job submission pattern: "group:priority:count", in submission order.
# Matches the table in specs/TEST01-fair-share-sched.md / README.md.
JOB_PATTERN=(
    "default:9:3" "grpA:9:1" "grpB:9:3" "grpC:9:1" "grpD:9:0"
    "default:6:1" "grpA:6:0" "grpB:6:0" "grpC:6:2" "grpD:6:1"
    "default:3:0" "grpA:3:2" "grpB:3:1" "grpC:3:0" "grpD:3:0"
    "default:0:1" "grpA:0:1" "grpB:0:1" "grpC:0:1" "grpD:0:1"
)

# Expected dispatch order: "group:priority:occurrence" tuples, one per job
# in JOB_PATTERN, in ascending job_info.running_time. See README.md for the
# derivation.
EXPECTED_ORDER=(
    "default:9:1" "grpA:9:1" "grpB:9:1" "grpC:9:1"
    "default:9:2" "grpB:9:2" "default:9:3" "grpB:9:3"
    "default:6:1" "grpC:6:1" "grpD:6:1" "grpC:6:2"
    "grpA:3:1" "grpB:3:1" "grpA:3:2"
    "default:0:1" "grpA:0:1" "grpB:0:1" "grpC:0:1" "grpD:0:1"
)

# Dummy job program (qc_type=dummy needs no real backend).
QPROGRAM_FILE="${SCRIPT_DIR}/dummy.qasm"
QC_TYPE="dummy"
CIRCUIT_FMT="qasm"
SHOTS=1

# Client binaries. Override via env if these auto-detected defaults are wrong.
SQC_RPC_CLIENT_BIN="${SQC_RPC_CLIENT_BIN:-sqc_rpc_client}"
if [ -z "${GRPC_CLIENT_BIN:-}" ]; then
    if command -v grpc_client >/dev/null 2>&1; then
        GRPC_CLIENT_BIN="grpc_client"
    elif [ -x "${HOME}/.local/grpc-job-broker/bin/grpc_client" ]; then
        # Not installed under bin/ alongside sqc_rpc_client; see
        # scripts/build-grpc-job-broker.sh install layout.
        GRPC_CLIENT_BIN="${HOME}/.local/grpc-job-broker/bin/grpc_client"
    else
        GRPC_CLIENT_BIN="grpc_client"
    fi
fi

# sqc_rpc_sched's DB path is fixed at build time (see README.md
# Prerequisites); DB_FILE below matches a normal install's default location.
# verify_order.sh reads DB_FILE directly, so it must run on the server host
# (the normal setup here, since server and client are separate hosts). Copy
# the client's client_job_ids.csv into the server's own WORK_DIR (work/,
# same filename) so it's found at the default JOB_IDS_CSV path below without
# an override (see README.md "Steps").
SQLITE="${SQLITE:-sqlite3}"
EXTERNAL_ROOT="${EXTERNAL_ROOT:-${HOME}/.local}"
export DB_FILE="${DB_FILE:-${EXTERNAL_ROOT}/var/sqc-scheduler/sqc-scheduler.db}"

# job_id -> label map produced by submit_jobs.sh, consumed by verify_order.sh.
JOB_IDS_CSV="${JOB_IDS_CSV:-${WORK_DIR}/client_job_ids.csv}"

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

mkdir -p "${WORK_DIR}" || die "failed to create work directory: ${WORK_DIR}"
