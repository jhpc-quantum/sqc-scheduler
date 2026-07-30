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

add_executable(grpc_client
  ${grpc_source}
  ${grpc_header}
  ${proto_source}
  ${proto_header}
  ${CMAKE_SOURCE_DIR}/common/job_broker_ecode.h
  ${CMAKE_SOURCE_DIR}/common/job_broker_file_util.cc
  ${CMAKE_SOURCE_DIR}/common/job_broker_file_util.h
  job_broker_client.cc
  job_broker_client.h
  app_adm_del_jobs.cc
  app_adm_add_user.cc
  app_adm_set_user_status.cc
  app_cancel.cc
  app_common.cc
  app_delete.cc
  app_list.cc
  app_main.cc
  app_status.cc
  app_submit.cc
  app.h)

target_compile_definitions(grpc_client
  PRIVATE
    _GNU_SOURCE=1)

target_compile_options(grpc_client
  PRIVATE
    -Wall -Wextra)

target_include_directories(grpc_client
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/common)

target_link_libraries(grpc_client
    job_broker
    absl::check
    absl::flags
    absl::flags_parse
    absl::log
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})

install(
  TARGETS grpc_client RUNTIME)
