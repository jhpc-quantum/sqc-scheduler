#!/usr/bin/env bash
#
# clean-rebuild-app-sqc-scheduler.sh - Remove installed artifacts of
#                                      rex-apis/grpc-job-broker/sqc-rpc-sched
#                                      (not their dependencies), then rebuild
#                                      and reinstall via the existing
#                                      build-*.sh scripts.
#
# Usage:
#   ./clean-rebuild-app-sqc-scheduler.sh [--clean-only] [rex-apis|grpc-job-broker|sqc-rpc-sched|all] [build_type]
#
#   component defaults to "all".
#   --clean-only: only remove installed artifacts, skip the rebuild.
#

# Load common path definitions and helper functions.
. "$(cd "$(dirname "$0")" && pwd)/build-common.sh"

SCRIPT_DIR="${BUILD_SCRIPT_DIR}"

usage() {
    cat <<EOF
Usage: $(basename "$0") [--clean-only] [rex-apis|grpc-job-broker|sqc-rpc-sched|all] [build_type]
EOF
    exit 1
}

run_rex_apis() {
    clean_rex_apis
    [ "${CLEAN_ONLY}" -eq 1 ] \
        || "${SCRIPT_DIR}/build-rex-apis.sh" "${BUILD_TYPE}" \
        || die "rebuild of rex-apis failed"
}

run_grpc_job_broker() {
    clean_grpc_job_broker
    [ "${CLEAN_ONLY}" -eq 1 ] \
        || "${SCRIPT_DIR}/build-grpc-job-broker.sh" \
        || die "rebuild of grpc-job-broker failed"
}

run_sqc_rpc_sched() {
    clean_sqc_rpc_sched
    [ "${CLEAN_ONLY}" -eq 1 ] \
        || "${SCRIPT_DIR}/build-sqc-rpc-sched.sh" \
        || die "rebuild of sqc-rpc-sched failed"
}

CLEAN_ONLY=0
ARGS=()
for arg in "$@"; do
    case "${arg}" in
        --clean-only)
            CLEAN_ONLY=1
            ;;
        *)
            ARGS+=("${arg}")
            ;;
    esac
done

COMPONENT="${ARGS[0]:-all}"
BUILD_TYPE="${ARGS[1]:-Release}"

case "${COMPONENT}" in
    rex-apis)
        run_rex_apis
        ;;
    grpc-job-broker)
        run_grpc_job_broker
        ;;
    sqc-rpc-sched)
        run_sqc_rpc_sched
        ;;
    all)
        run_rex_apis
        run_grpc_job_broker
        run_sqc_rpc_sched
        ;;
    *)
        usage
        ;;
esac

if [ "${CLEAN_ONLY}" -eq 1 ]; then
    log_info "Clean of '${COMPONENT}' completed."
else
    log_info "Clean rebuild of '${COMPONENT}' completed."
fi
