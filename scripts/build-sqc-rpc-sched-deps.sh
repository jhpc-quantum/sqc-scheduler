#!/usr/bin/env bash
#
# build-sqc-rpc-sched-deps.sh - Build and install the third-party
#                               libraries required by sqc-rpc-sched.
#
# Installed dependencies:
#   * protobuf v22.5
#   * protobuf-c v1.5.0
#   * jwt-cpp
#
# Note:
#   sqc-rpc-sched also requires the rex-apis and grpc-job-broker installations.
#   Build them with their own build scripts before building sqc-rpc-sched.
#
# Usage:
#   ./build-sqc-rpc-sched-deps.sh
#

# Load common path definitions and helper functions.
. "$(cd "$(dirname "$0")" && pwd)/build-common.sh"

PROTOBUF_VERSION="v22.5"
PROTOBUF_C_VERSION="v1.5.0"
MUNGE_VERSION="munge-0.5.16"

# ---------------------------------------------------------------------------
# protobuf
# ---------------------------------------------------------------------------
build_protobuf() {
    log_step "Building protobuf (${PROTOBUF_VERSION})"

    local src="${EXTERNAL_SRC_ROOT}/protobuf"
    local abseil_cmake="${src}/third_party/abseil-cpp/CMakeLists.txt"
    local patch_stamp="${src}/.abseil-cmake-patched"

    clone_repo "https://github.com/protocolbuffers/protobuf.git" "${src}"

    (
        cd "${src}"
        git checkout "${PROTOBUF_VERSION}"
        git submodule update --init --recursive
    ) || die "failed to checkout protobuf ${PROTOBUF_VERSION}"

    # Workaround from docs/BUILD.md: delete lines 71-73 of the bundled
    # abseil-cpp CMakeLists.txt.  A stamp file keeps this idempotent.
    if [ ! -f "${patch_stamp}" ]; then
        log_info "patching ${abseil_cmake} (deleting lines 71-73)"
        sed -i '71,73d' "${abseil_cmake}" \
            || die "failed to patch abseil-cpp CMakeLists.txt"
        touch "${patch_stamp}"
    else
        log_info "abseil-cpp CMakeLists.txt already patched (skip)"
    fi

    mkdir -p "${src}/build"
    cd "${src}/build" || die "failed to cd into build directory: ${src}/build"
    cmake -DCMAKE_INSTALL_PREFIX="${PROTOBUF_INSTALL_PREFIX}" .. \
        || die "cmake configuration of protobuf failed"
    make -j "${MAKE_JOBS}" || die "build of protobuf failed"
    make install || die "installation of protobuf failed"
}

# ---------------------------------------------------------------------------
# protobuf-c
# ---------------------------------------------------------------------------
build_protobuf_c() {
    log_step "Building protobuf-c (${PROTOBUF_C_VERSION})"

    local src="${EXTERNAL_SRC_ROOT}/protobuf-c"
    local pkgconfig_dir

    clone_repo "https://github.com/protobuf-c/protobuf-c.git" "${src}"

    (cd "${src}" && git checkout "${PROTOBUF_C_VERSION}") \
        || die "failed to checkout protobuf-c ${PROTOBUF_C_VERSION}"

    # protobuf installs its pkg-config files into lib/ or lib64/ depending
    # on the distribution, so pick whichever exists.
    if [ -d "${PROTOBUF_INSTALL_PREFIX}/lib64/pkgconfig" ]; then
        pkgconfig_dir="${PROTOBUF_INSTALL_PREFIX}/lib64/pkgconfig"
    else
        pkgconfig_dir="${PROTOBUF_INSTALL_PREFIX}/lib/pkgconfig"
    fi

    (
        cd "${src}"
        export PKG_CONFIG="$(command -v pkg-config)"
        export PKG_CONFIG_PATH="${pkgconfig_dir}"
        ./autogen.sh
        ./configure --prefix="${PROTOBUF_C_INSTALL_PREFIX}"
        make -j "${MAKE_JOBS}"
        make install
    ) || die "build/installation of protobuf-c failed"
}

# ---------------------------------------------------------------------------
# MUNGE
# ---------------------------------------------------------------------------
build_munge() {
    log_step "Building MUNGE (${MUNGE_VERSION})"

    local src="${EXTERNAL_SRC_ROOT}/munge"

    clone_repo "https://github.com/dun/munge.git" "${src}"

    (
        cd "${src}"
        git checkout "${MUNGE_VERSION}"
    ) || die "failed to checkout MUNGE ${MUNGE_VERSION}"

    (
        cd "${src}"
        ./bootstrap
        ./configure --prefix="${MUNGE_INSTALL_PREFIX}"
        make -j "${MAKE_JOBS}"
        make install
    ) || die "build/installation of MUNGE failed"
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
build_protobuf
build_protobuf_c
build_munge
install_jwt_cpp

log_info "all sqc-rpc-sched dependencies were built and installed successfully"
