#!/usr/bin/env bash
#
# build-common.sh - Common path definitions and helper functions shared by
#                   all build scripts in this directory.
#
# Every build script sources this file.  All locations can be overridden
# by exporting the corresponding environment variable before running a
# script, e.g.:
#
#   $ SRC_DIR=/data/src DEPS_PREFIX=/opt/deps ./build-all.sh
#

# Fail fast: exit on error, undefined variable, or failed pipe element.
set -euo pipefail

# ---------------------------------------------------------------------------
# Path definitions
# ---------------------------------------------------------------------------

# Directory containing these build scripts.
BUILD_SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Root directory that contains the local application source trees.
PROJECT_ROOT="${PROJECT_ROOT:-$(cd "${BUILD_SCRIPT_DIR}/.." && pwd)}"

# Root directory for external sources and libraries.
EXTERNAL_ROOT="${EXTERNAL_ROOT:-${HOME}/.local}"

# Root directory for external source trees cloned from GitHub.
EXTERNAL_SRC_ROOT="${EXTERNAL_SRC_ROOT:-${EXTERNAL_ROOT}/src}"

# Root directory for the vcpkg source tree cloned from GitHub.
VCPKG_ROOT="${VCPKG_SRC_ROOT:-${EXTERNAL_SRC_ROOT}/vcpkg}"

# Local source directory for rex-apis.
REX_APIS_SRC_DIR="${REX_APIS_SRC_DIR:-${PROJECT_ROOT}/rex-apis}"

# Local source directory for grpc-job-broker.
GRPC_JOB_BROKER_SRC_DIR="${GRPC_JOB_BROKER_SRC_DIR:-${PROJECT_ROOT}/grpc-job-broker}"

# Local source directory for sqc-rpc-sched.
SQC_RPC_SCHED_SRC_DIR="${SQC_RPC_SCHED_SRC_DIR:-${PROJECT_ROOT}/sqc-rpc-sched}"

# Installation prefix for Microsoft C++ REST SDK.
CPPRESTSDK_INSTALL_PREFIX="${CPPRESTSDK_INSTALL_PREFIX:-${EXTERNAL_ROOT}/cpprestsdk}"

# Installation prefix for gRPC and its matching protobuf.
GRPC_INSTALL_PREFIX="${GRPC_INSTALL_PREFIX:-${EXTERNAL_ROOT}/grpc}"

# Installation prefix for protobuf used by protobuf-c.
PROTOBUF_INSTALL_PREFIX="${PROTOBUF_INSTALL_PREFIX:-${EXTERNAL_ROOT}/protobuf}"

# Installation prefix for protobuf-c.
PROTOBUF_C_INSTALL_PREFIX="${PROTOBUF_C_INSTALL_PREFIX:-${EXTERNAL_ROOT}/protobuf-c}"

# Installation prefix for MUNGE.
MUNGE_INSTALL_PREFIX="${MUNGE_INSTALL_PREFIX:-${EXTERNAL_ROOT}/munge}"

# Installation prefix for the header-only jwt-cpp dependency.
JWT_CPP_INSTALL_PREFIX="${JWT_CPP_INSTALL_PREFIX:-${EXTERNAL_ROOT}/jwt-cpp}"

# Installation prefix for rex-apis outputs.
REX_APIS_INSTALL_PREFIX="${REX_APIS_INSTALL_PREFIX:-${EXTERNAL_ROOT}/rex-apis}"

# Installation prefix for grpc-job-broker outputs.
GRPC_JOB_BROKER_INSTALL_PREFIX="${GRPC_JOB_BROKER_INSTALL_PREFIX:-${EXTERNAL_ROOT}/grpc-job-broker}"

# Installation prefix for sqc-rpc-sched outputs.
SQC_RPC_SCHED_INSTALL_PREFIX="${SQC_RPC_SCHED_INSTALL_PREFIX:-${EXTERNAL_ROOT}}"

# Installation prefix for simple-sqc outputs.
SIMPLE_SQC_INSTALL_PREFIX="${SIMPLE_SQC_INSTALL_PREFIX:-${EXTERNAL_ROOT}/simple-sqc}"

# Number of parallel make jobs.
MAKE_JOBS="${MAKE_JOBS:-4}"

# vcpkg target triplet used when installing minio-cpp / libzippp.
VCPKG_DEFAULT_TRIPLET="${VCPKG_DEFAULT_TRIPLET:-x64-linux}"
export VCPKG_DEFAULT_TRIPLET

# ===========================================================================
# common function
# ===========================================================================

# log_info <message>
# Print an informational message to stdout.
log_info() {
    echo "[INFO ] $*"
}

# die <message>
# Print an error message to stderr and terminate the script.
die() {
    echo "[ERROR] $*" >&2
    exit 1
}

# Print an error message with the failing location whenever any command
# exits with a non-zero status (works together with `set -e`).
on_error() {
    echo "[ERROR] command failed at ${BASH_SOURCE[1]}:${BASH_LINENO[0]}" >&2
}
trap on_error ERR

# ensure_dir <dir>
# Create the directory if it does not exist yet.
ensure_dir() {
    local dir="$1"
    if [ ! -d "${dir}" ]; then
        mkdir -p "${dir}" || die "failed to create directory: ${dir}"
    fi
}

# clone_repo <url> <dest-dir> [extra git-clone options...]
# Clone a Git repository into SRC_DIR unless it is already present.
clone_repo() {
    local url="$1"
    local dest="$2"
    shift 2
    ensure_dir "${EXTERNAL_SRC_ROOT}"
    if [ -d "${dest}/.git" ]; then
        log_info "already cloned: ${dest} (skip)"
    else
        log_info "cloning ${url} into ${dest}"
        git clone "$@" "${url}" "${dest}" || die "failed to clone ${url}"
    fi
}

# install_jwt_cpp
# Install the header-only jwt-cpp library (used by both grpc-c-wrapper and
# sqc-rpc-sched).  The headers are copied to ${JWT_CPP_PREFIX}/include.
install_jwt_cpp() {
    local src="${EXTERNAL_SRC_ROOT}/jwt-cpp"
    clone_repo "https://github.com/Thalhammer/jwt-cpp.git" "${src}"
    if [ -f "${JWT_CPP_INSTALL_PREFIX}/jwt.h" ]; then
        log_info "jwt-cpp already installed in ${JWT_CPP_INSTALL_PREFIX} (skip)"
        return 0
    fi
    log_info "installing jwt-cpp headers into ${JWT_CPP_INSTALL_PREFIX}"
    ensure_dir "${JWT_CPP_INSTALL_PREFIX}"
    cp -r "${src}/include" "${JWT_CPP_INSTALL_PREFIX}" \
        || die "failed to install jwt-cpp headers"
}

# log_step <message>
# Output a message separated by steps
log_step() {
    echo ""
    echo "=========================================="
    echo "[STEP]  $*"
    echo "=========================================="
}

