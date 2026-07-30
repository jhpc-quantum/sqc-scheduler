#!/usr/bin/env bash
#
# build-simple-sqc.sh - Build and install simple-sqc itself.
#
# This script builds only the grpc-job-broker source tree.
# The dependency libraries must already be installed by build-grpc-job-broker-deps.sh,
# so this script alone is sufficient to rebuild grpc-job-broker after
# modifying its source code.
#
# Usage:
#   ./build-simple-sqc.sh
#

# Load common path definitions and helper functions.
. "$(cd "$(dirname "$0")" && pwd)/build-common.sh"

[ -d "${GRPC_JOB_BROKER_SRC_DIR}" ] \
    || die "grpc-job-broker source not found: ${GRPC_JOB_BROKER_SRC_DIR}"
[ -x "${GRPC_INSTALL_PREFIX}/bin/protoc" ] \
    || die "gRPC not found in ${GRPC_INSTALL_PREFIX}: run build-grpc-job-broker-deps.sh first"

export PATH="${GRPC_INSTALL_PREFIX}/bin:${PATH}"

BUILD_DIR="${GRPC_JOB_BROKER_SRC_DIR}/build"
if [ -d "${BUILD_DIR}" ]; then
    log_info "Removing existing build directory: ${BUILD_DIR}"
    rm -rf "${BUILD_DIR}"
fi

ensure_dir "${BUILD_DIR}"
cd "${BUILD_DIR}" || die "failed to cd into build directory: ${BUILD_DIR}"

cmake -DCMAKE_INSTALL_PREFIX="${SIMPLE_SQC_INSTALL_PREFIX}" \
      -DCMAKE_PREFIX_PATH="${GRPC_INSTALL_PREFIX}" \
      -DJWT_CPP_PREFIX="${JWT_CPP_INSTALL_PREFIX}" \
      -DBUILD_SIMPLE_SQC_SERVER=1 \
      .. || die "cmake configuration of simple-sqc failed"

make -j "${MAKE_JOBS}" || die "build of simple-sqc failed"

make install || die "installation of simple-sqc failed"

log_info "simple-sqc was built and installed into ${SIMPLE_SQC_INSTALL_PREFIX}"
