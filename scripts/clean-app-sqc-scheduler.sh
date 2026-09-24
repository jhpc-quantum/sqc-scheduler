#!/usr/bin/env bash
#
# clean-app-sqc-scheduler.sh - Remove all installed artifacts of rex-apis,
#                              grpc-job-broker, sqc-rpc-sched, AND their
#                              dependency libraries. Full reset back to a
#                              pre-first-build state.
#
# Usage:
#   ./clean-app-sqc-scheduler.sh
#

# Load common path definitions and helper functions.
. "$(cd "$(dirname "$0")" && pwd)/build-common.sh"

clean_rex_apis
clean_grpc_job_broker
clean_sqc_rpc_sched

for prefix in \
    "${CPPRESTSDK_INSTALL_PREFIX}" \
    "${GRPC_INSTALL_PREFIX}" \
    "${PROTOBUF_INSTALL_PREFIX}" \
    "${PROTOBUF_C_INSTALL_PREFIX}" \
    "${MUNGE_INSTALL_PREFIX}" \
    "${JWT_CPP_INSTALL_PREFIX}"; do
    log_step "Cleaning installed dependency: ${prefix}"
    rm -rf "${prefix}"
done

log_info "Clean of rex-apis, grpc-job-broker, sqc-rpc-sched, and their dependencies completed."
