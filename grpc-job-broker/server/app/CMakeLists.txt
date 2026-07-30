add_executable(grpc_server
  ${CMAKE_SOURCE_DIR}/common/job_broker_ecode.h
  ${CMAKE_SOURCE_DIR}/common/job_broker_file_util.cc
  ${CMAKE_SOURCE_DIR}/common/job_broker_file_util.h
  job_broker_server_main.c)

target_compile_definitions(grpc_server
  PRIVATE
    SYSCONFDIR="${CMAKE_INSTALL_PREFIX}/etc")

target_include_directories(grpc_server
  PRIVATE
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/server/lib)

target_link_libraries(grpc_server
    job_broker
    absl::check
    absl::flags
    absl::flags_parse
    absl::log
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})

install(
  TARGETS grpc_server RUNTIME)
