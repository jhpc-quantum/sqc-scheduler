add_executable(simple_sqc_server
  ${CMAKE_SOURCE_DIR}/common/job_broker_file_util.cc
  job.cc
  job_invoker.cc
  job_manager.cc
  jwt_verifier.cc
  simple_sqc_server_main.cc
  user_db.cc)

target_compile_definitions(simple_sqc_server
  PRIVATE
    SYSCONFDIR="${CMAKE_INSTALL_PREFIX}/etc")

target_include_directories(simple_sqc_server
  SYSTEM PRIVATE
    "${Python3_INCLUDE_DIRS}")

target_include_directories(simple_sqc_server
  PRIVATE
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/server/lib
    ${JWT_CPP_INCLUDE_DIR})

target_link_libraries(simple_sqc_server
    job_broker
    absl::check
    absl::flags
    absl::flags_parse
    absl::log
    crypto
    uuid
    ${Python3_LIBRARIES}
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/run_simple_sqc_server.sh.in
    ${CMAKE_CURRENT_BINARY_DIR}/run_simple_sqc_server.sh
    @ONLY
)

install(
  TARGETS simple_sqc_server RUNTIME
)

install(
  PROGRAMS
    qiskit_qasm2.py qiskit_qasm3.py
 DESTINATION
    "${CMAKE_INSTALL_PREFIX}/lib/python"
)

install(
  PROGRAMS
    ${CMAKE_CURRENT_BINARY_DIR}/run_simple_sqc_server.sh
 DESTINATION
    "${CMAKE_INSTALL_PREFIX}/bin"
)
