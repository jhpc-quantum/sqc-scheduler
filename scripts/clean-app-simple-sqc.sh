#!/usr/bin/env bash
#
# clean-app-simple-sqc.sh - Remove all installed artifacts of simple-sqc
#                           AND its dependency libraries. Full reset back
#                           to a pre-first-build state.
#
# Usage:
#   ./clean-app-simple-sqc.sh
#

# Load common path definitions and helper functions.
. "$(cd "$(dirname "$0")" && pwd)/build-common.sh"

clean_simple_sqc

for prefix in \
    "${GRPC_INSTALL_PREFIX}" \
    "${JWT_CPP_INSTALL_PREFIX}"; do
    log_step "Cleaning installed dependency: ${prefix}"
    rm -rf "${prefix}"
done

log_info "Clean of simple-sqc and its dependencies completed."
