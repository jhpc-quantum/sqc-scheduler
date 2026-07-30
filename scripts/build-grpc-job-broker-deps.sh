#!/usr/bin/env bash
#
# build-grpc-c-wrapper-deps.sh - Build and install the third-party
#                                libraries required by grpc-c-wrapper.
#
# Installed dependencies:
#   * gRPC v1.66.0 (protobuf is bundled and installed together)
#   * jwt-cpp (header-only, required when building the Simple SQC server)
#
# Usage:
#   ./build-grpc-c-wrapper-deps.sh
#

# Load common path definitions and helper functions.
. "$(cd "$(dirname "$0")" && pwd)/build-common.sh"

GRPC_VERSION="v1.66.0"

# ---------------------------------------------------------------------------
# gRPC
# ---------------------------------------------------------------------------
build_grpc() {
    log_step "Building gRPC (${GRPC_VERSION})"

    local src="${EXTERNAL_SRC_ROOT}/grpc"

    # gRPC must be cloned with its submodules for an in-tree build.
    clone_repo "https://github.com/grpc/grpc" "${src}" \
        --recurse-submodules -b "${GRPC_VERSION}" --depth 1 --shallow-submodules

    mkdir -p "${src}/cmake/build"
    cd "${src}/cmake/build" || die "failed to cd into build directory: ${src}/cmake/build"
    cmake -DgRPC_INSTALL=ON \
          -DgRPC_BUILD_TESTS=OFF \
          -DCMAKE_INSTALL_PREFIX="${GRPC_INSTALL_PREFIX}" \
          ../.. || die "cmake configuration of gRPC failed"
    make -j "${MAKE_JOBS}" || die "build of gRPC failed"
    make install || die "installation of gRPC failed"
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
build_grpc

# jwt-cpp is required by the Simple SQC server build.
# It is header-only and cheap to install, so it is always installed here.
install_jwt_cpp

log_info "all grpc-job-broker dependencies were built and installed successfully"
