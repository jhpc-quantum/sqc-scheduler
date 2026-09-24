#!/usr/bin/env bash
#
# submit_jobs.sh <rpc|grpc> - Submit the job pattern defined in env.sh
# (JOB_PATTERN) via the given protocol, and record the returned job_id
# together with its group/priority/occurrence label in
# ${WORK_DIR}/client_job_ids.csv for verify_order.sh to consume.
#
# Usage:
#   ./submit_jobs.sh <rpc|grpc>
#

# Load common path definitions and helper functions.
. "$(cd "$(dirname "$0")" && pwd)/env.sh"

# A proxy in the environment can break local RPC/gRPC connections
# (see docs/INSTALL_CLIENT.md: "Unset http_proxy and https_proxy if needed").
unset -v http_proxy https_proxy HTTP_PROXY HTTPS_PROXY 2>/dev/null || true

PROTOCOL="${1:-}"
case "${PROTOCOL}" in
    rpc|grpc) ;;
    *)
        echo "Usage: $0 <rpc|grpc>" >&2
        exit 1
        ;;
esac

: > "${JOB_IDS_CSV}"

# submit_one <group> <priority> <occurrence>
# Submit one job via ${PROTOCOL} and append its job_id/label to JOB_IDS_CSV.
submit_one() {
    local group="$1" priority="$2" occurrence="$3"
    local remark="TEST01 ${group}@${priority}#${occurrence}"
    local output job_id

    # Note: the client call below must be guarded with an explicit "if !",
    # not "cmd || die"; under `set -e`, a plain `output=$(cmd)` assignment
    # silently exits the script on failure without ever reaching a
    # trailing "|| die" or a later `if [ -z ... ]` check.
    if [ "${PROTOCOL}" = "rpc" ]; then
        if ! output=$(SQC_RPC_GROUP_ID="${group}" "${SQC_RPC_CLIENT_BIN}" submit \
            --auth=jwt --server="${RPC_SERVER}" --conf-dir="${RPC_CLIENT_CONF_DIR}" \
            --remark="${remark}" "${QC_TYPE}" "${priority}" "${QPROGRAM_FILE}" "${CIRCUIT_FMT}" "${SHOTS}"); then
            echo "${output}" >&2
            die "failed to submit job ${remark} (client exited non-zero)"
        fi
    else
        if ! output=$(SQC_GRPC_GROUP_ID="${group}" "${GRPC_CLIENT_BIN}" submit \
            --server="${GRPC_SERVER}" --conf-dir="${GRPC_CLIENT_CONF_DIR}" \
            --remark="${remark}" "${QC_TYPE}" "${priority}" "${QPROGRAM_FILE}" "${CIRCUIT_FMT}" "${SHOTS}"); then
            echo "${output}" >&2
            die "failed to submit job ${remark} (client exited non-zero)"
        fi
    fi

    job_id=$(printf '%s\n' "${output}" | sed -n 's/^job_id: //p' | head -n1)
    if [ -z "${job_id}" ]; then
        echo "${output}" >&2
        die "failed to submit job ${remark}"
    fi

    printf '%s,%s,%s,%s\n' "${job_id}" "${group}" "${priority}" "${occurrence}" >> "${JOB_IDS_CSV}"
    log_info "submitted ${remark} -> job_id=${job_id}"
}

for entry in "${JOB_PATTERN[@]}"; do
    IFS=':' read -r group priority count <<< "${entry}"
    for ((n = 1; n <= count; n++)); do
        submit_one "${group}" "${priority}" "${n}"
    done
done

log_info "submitted $(wc -l < "${JOB_IDS_CSV}") jobs; recorded in ${JOB_IDS_CSV}"
