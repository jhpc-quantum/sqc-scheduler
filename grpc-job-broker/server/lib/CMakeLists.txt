get_filename_component(job_broker_proto "../../proto/job_broker.proto" ABSOLUTE)
get_filename_component(job_broker_proto_path "${job_broker_proto}" PATH)

set(proto_source "${CMAKE_CURRENT_BINARY_DIR}/job_broker.pb.cc")
set(proto_header "${CMAKE_CURRENT_BINARY_DIR}/job_broker.pb.h")
set(grpc_source "${CMAKE_CURRENT_BINARY_DIR}/job_broker.grpc.pb.cc")
set(grpc_header "${CMAKE_CURRENT_BINARY_DIR}/job_broker.grpc.pb.h")

add_custom_command(
  OUTPUT
    "${grpc_source}"
    "${grpc_header}"
    "${proto_source}"
    "${proto_header}"
  COMMAND
     ${_PROTOBUF_PROTOC}
  ARGS
    --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
    --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
    -I "${job_broker_proto_path}"
    --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
    "${job_broker_proto}"
  DEPENDS
    "${job_broker_proto}")

add_library(job_broker
  SHARED
    ${grpc_source}
    ${grpc_header}
    ${proto_source}
    ${proto_header}
    rpc_job_broker_service.cc
    rpc_job_broker_service.h
    job_broker_server.cc
    job_broker_server.h
    ${CMAKE_SOURCE_DIR}/common/job_broker_ecode.h
    ${CMAKE_SOURCE_DIR}/common/job_broker_file_util.cc
    ${CMAKE_SOURCE_DIR}/common/job_broker_file_util.h
    ${CMAKE_SOURCE_DIR}/common/job_broker_logger.cc
    ${CMAKE_SOURCE_DIR}/common/job_broker_logger.h
    ${CMAKE_SOURCE_DIR}/common/job_broker_logger_internal.h)

target_include_directories(job_broker
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/common)

target_link_libraries(job_broker
  absl::check
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

install(
  TARGETS job_broker LIBRARY)

install(
  FILES
    job_broker_server.h
    ${CMAKE_SOURCE_DIR}/common/job_broker_logger.h
  DESTINATION
    include)
