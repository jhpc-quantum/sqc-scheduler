#!/usr/bin/env bash
#
# build-app-sqc-scheduler.sh - Build all three applications
#                              (rex-apis, grpc-job-broker, sqc-rpc-sched)
#                              including their dependency libraries.
#
# The build order follows the dependency chain:
#   1. rex-apis dependencies, rex-apis
#   2. grpc-job-broker dependencies, grpc-job-broker
#   3. sqc-rpc-sched dependencies, sqc-rpc-sched
#      (sqc-rpc-sched links against the rex-apis and grpc-job-broker installations,
#       so it must be built last)
#
# Usage:
#   ./build-app-sqc-scheduler.sh
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

# 1. rex-apis
run_step build-rex-apis-deps.sh
run_step build-rex-apis.sh

# 2. grpc-job-broker
run_step build-grpc-job-broker-deps.sh
run_step build-grpc-job-broker.sh

# 3. sqc-rpc-sched (depends on the two installations above)
run_step build-sqc-rpc-sched-deps.sh
run_step build-sqc-rpc-sched.sh

log_info "sqc-scheduler were built successfully"
