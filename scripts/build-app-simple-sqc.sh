#!/usr/bin/env bash
#
# build-app-simple-sqc.sh - Build Simple SQC applications
#                           including their dependency libraries.
#
# The build order follows the dependency chain:
#   1. grpc-job-broker dependencies, simple-sqc
#
# Usage:
#   ./build-app-simple-sqc.sh
#

# Load common path definitions and helper functions.
. "$(cd "$(dirname "$0")" && pwd)/build-common.sh"

# run_step <script> [args...]
# Execute one build script and abort the whole build on failure.
run_step() {
    local script="${BUILD_SCRIPT_DIR}/$1"
    shift
    log_info "==== Running ${script} $* ===="
    bash "${script}" "$@" || die "step failed: ${script} $*"
}

run_step build-grpc-job-broker-deps.sh
run_step build-simple-sqc.sh

log_info "simple-sqc were built successfully"
