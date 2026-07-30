#!/usr/bin/env bash
#
# build-sqc-rpc-sched.sh - Build sqc-rpc-sched itself.
#
# This script builds only the sqc-rpc-sched source tree.
# The dependency libraries must already be installed beforehand:
#   * protobuf / protobuf-c / jwt-cpp   ... build-sqc-rpc-sched-deps.sh
#   * REX APIS (rex-apis)               ... build-rex-apis-deps.sh + build-rex-apis.sh
#   * gRPC Job Broker (grpc-job-broker) ... build-grpc-job-broker-deps.sh + build-grpc-job-broker.sh
#
# Therefore this script alone is sufficient to rebuild sqc-rpc-sched after
# modifying its source code.
#
# Usage:
#   ./build-sqc-rpc-sched.sh
#

# Load common path definitions and helper functions.
. "$(cd "$(dirname "$0")" && pwd)/build-common.sh"

[ -d "${SQC_RPC_SCHED_SRC_DIR}" ] \
    || die "sqc-rpc-sched source not found: ${SQC_RPC_SCHED_SRC_DIR}"

# Verify that all required installations exist before running configure.
[ -x "${PROTOBUF_C_INSTALL_PREFIX}/bin/protoc-c" ] \
    || die "protobuf-c not found in ${PROTOBUF_C_INSTALL_PREFIX}: run build-sqc-rpc-sched-deps.sh first"
[ -f "${JWT_CPP_INSTALL_PREFIX}/include/jwt-cpp/jwt.h" ] \
    || die "jwt-cpp not found in ${JWT_CPP_INSTALL_PREFIX}: run build-sqc-rpc-sched-deps.sh first"
[ -d "${REX_APIS_INSTALL_PREFIX}" ] \
    || die "REX APIS not found in ${REX_APIS_INSTALL_PREFIX}: run build-rex-apis.sh first"
[ -d "${GRPC_JOB_BROKER_INSTALL_PREFIX}" ] \
    || die "gRPC Job Broker not found in ${GRPC_JOB_BROKER_INSTALL_PREFIX}: run build-grpc-job-broker.sh first"

log_info "building sqc-rpc-sched in ${SQC_RPC_SCHED_SRC_DIR}"
cd "${SQC_RPC_SCHED_SRC_DIR}" || die "failed to cd into source directory: ${SQC_RPC_SCHED_SRC_DIR}"

# sqc-rpc-sched uses an in-tree autotools build (configure + make).
./configure --prefix="${SQC_RPC_SCHED_INSTALL_PREFIX}" \
            --with-protobuf_c="${PROTOBUF_C_INSTALL_PREFIX}" \
            --with-jwt-cpp="${JWT_CPP_INSTALL_PREFIX}" \
            --with-rexapis="${REX_APIS_INSTALL_PREFIX}" \
            --with-jobbroker="${GRPC_JOB_BROKER_INSTALL_PREFIX}" \
    || die "configure of sqc-rpc-sched failed"

make -j "${MAKE_JOBS}" || die "build of sqc-rpc-sched failed"

make install || die "installation of sqc-rpc-sched failed"

log_info "sqc-rpc-sched was built successfully"
