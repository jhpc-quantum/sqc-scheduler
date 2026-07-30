#!/usr/bin/env bash
#
# build-rex-apis-deps.sh - Build and install the third-party libraries
#                          required by rex-apis.
#
# Installed dependencies:
#   * C++ REST SDK v2.10.19 (with the chunked-encoding buffer patch)
#   * vcpkg + minio-cpp + libzippp
#
# Usage:
#   ./build-rex-apis-deps.sh
#

# Load common path definitions and helper functions.
. "$(cd "$(dirname "$0")" && pwd)/build-common.sh"

CPPRESTSDK_VERSION="v2.10.19"

# ---------------------------------------------------------------------------
# C++ REST SDK
# ---------------------------------------------------------------------------
build_cpprestsdk() {
    log_step "Building cpprestsdk (${CPPRESTSDK_VERSION})"

    local src="${EXTERNAL_SRC_ROOT}/cpprestsdk"
    local target="${src}/Release/src/http/common/http_helpers.cpp"

    clone_repo "https://github.com/microsoft/cpprestsdk.git" "${src}"

    (
        cd "${src}"
        git checkout "${CPPRESTSDK_VERSION}"
        git submodule update --init
    ) || die "failed to checkout cpprestsdk v2.10.19"

    # Apply the buffer-size patch described in rex-apis/README.md.
    # The sed edit is idempotent: it does nothing when already applied.
    if grep -q 'char buffer\[9\];' "${target}"; then
        log_info "patching ${target} (char buffer[9] -> char buffer[17])"
        sed -i 's/char buffer\[9\];/char buffer[17];/' "${target}" \
            || die "failed to patch cpprestsdk"
    else
        log_info "cpprestsdk patch already applied (skip)"
    fi

    log_info "building cpprestsdk"
    mkdir -p "${src}/build"
    cd "${src}/build" || die "failed to cd into build directory: ${src}/build"
    cmake -DCMAKE_INSTALL_PREFIX="${CPPRESTSDK_INSTALL_PREFIX}" \
          -DBUILD_TESTS=OFF \
          -DBUILD_SAMPLES=OFF \
          -DCPPREST_EXCLUDE_WEBSOCKETS=ON \
          .. || die "cmake configuration of cpprestsdk failed"
    make -j "${MAKE_JOBS}" || die "build of cpprestsdk failed"
    make install || die "installation of cpprestsdk failed"
}

# ---------------------------------------------------------------------------
# vcpkg + minio-cpp + libzippp
# ---------------------------------------------------------------------------
build_vcpkg_packages() {
    log_step "Installing minio-cpp and libzippp via vcpkg"

    clone_repo "https://github.com/microsoft/vcpkg.git" "${VCPKG_ROOT}"

    # Bootstrap vcpkg only when the executable does not exist yet.
    if [ ! -x "${VCPKG_ROOT}/vcpkg" ]; then
        log_info "bootstrapping vcpkg"
        (cd "${VCPKG_ROOT}" && ./bootstrap-vcpkg.sh) \
            || die "failed to bootstrap vcpkg"
    else
        log_info "vcpkg already bootstrapped (skip)"
    fi

    log_info "installing minio-cpp and libzippp via vcpkg"
    (
        export VCPKG_ROOT
        export PATH="${VCPKG_ROOT}:${PATH}"
        vcpkg install minio-cpp libzippp
    ) || die "vcpkg install of minio-cpp/libzippp failed"
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
build_cpprestsdk
build_vcpkg_packages

log_info "all rex-apis dependencies were built and installed successfully"
