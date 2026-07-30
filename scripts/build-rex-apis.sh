#!/usr/bin/env bash
#
# build-rex-apis.sh - Build and install rex-apis (REX APIS) itself.
#
# This script builds only the rex-apis source tree.
# The dependency libraries must already be installed by build-rex-apis-deps.sh,
# so this script alone is sufficient to rebuild rex-apis after modifying its source code.
#
# Usage:
#   ./build-rex-apis.sh
#

# Load common path definitions and helper functions.
. "$(cd "$(dirname "$0")" && pwd)/build-common.sh"

# Build type given as the first argument (default: Release).
BUILD_TYPE="${1:-Release}"

[ -d "${REX_APIS_SRC_DIR}" ] || die "rex-apis source not found: ${REX_APIS_SRC_DIR}"
[ -f "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" ] \
    || die "vcpkg toolchain not found: run build-rex-apis-deps.sh first"

# Search path for the pre-installed dependency libraries.
PREFIX_PATH="${CPPRESTSDK_INSTALL_PREFIX}"

BUILD_DIR="${REX_APIS_SRC_DIR}/build"
if [ -d "${BUILD_DIR}" ]; then
    log_info "Removing existing build directory: ${BUILD_DIR}"
    rm -rf "${BUILD_DIR}"
fi

ensure_dir "${BUILD_DIR}"
cd "${BUILD_DIR}" || die "failed to cd into build directory: ${BUILD_DIR}"

cmake -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
      -DCMAKE_INSTALL_PREFIX="${REX_APIS_INSTALL_PREFIX}" \
      -DCMAKE_PREFIX_PATH="${PREFIX_PATH}" \
      -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" \
      .. || die "cmake configuration of rex-apis failed"

make -j "${MAKE_JOBS}" || die "build of rex-apis failed"

# Install so that sqc-rpc-sched can link against the rex-apis library.
make install || die "installation of rex-apis failed"

log_info "rex-apis (${BUILD_TYPE}) was built and installed into ${REX_APIS_INSTALL_PREFIX}"
