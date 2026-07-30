find_package(GTest REQUIRED)
message(STATUS "  GTEST_INCLUDE_DIRS    = ${GTEST_INCLUDE_DIRS}")
message(STATUS "  GTEST_LIBRARIES       = ${GTEST_LIBRARIES}")
message(STATUS "  GTEST_MAIN_LIBRARIES  = ${GTEST_MAIN_LIBRARIES}")
message(STATUS "  GTEST_BOTH_LIBRARIES  = ${GTEST_BOTH_LIBRARIES}")

set(REXAPIS_UTIL_TEST_SOURCE_FILES
    rexapis_util_test.cpp
)
add_executable(rexapis-util-test ${REXAPIS_UTIL_TEST_SOURCE_FILES})
add_dependencies(rexapis-util-test GTest::gtest)
target_include_directories(rexapis-util-test PRIVATE
                           ${PROJECT_INCLUDE_DIR}
)
target_link_libraries(rexapis-util-test PRIVATE
                      GTest::gtest_main
                      cpprestsdk::cpprest
                      libzippp::libzippp
                      rexapis
)

set(RQC_REST_TEST_SOURCE_FILES
    rqc_rest_test.cpp
)
add_executable(rqc-rest-test ${RQC_REST_TEST_SOURCE_FILES})
add_dependencies(rqc-rest-test GTest::gtest)
target_include_directories(rqc-rest-test PRIVATE
                           ${PROJECT_INCLUDE_DIR}
)
target_link_libraries(rqc-rest-test PRIVATE
                      GTest::gtest_main
                      cpprestsdk::cpprest
                      libzippp::libzippp
                      rexapis
)

set(RQC_JSON_TEST_SOURCE_FILES
    rqc_json_test.cpp
)
add_executable(rqc-json-test ${RQC_JSON_TEST_SOURCE_FILES})
add_dependencies(rqc-json-test GTest::gtest)
target_include_directories(rqc-json-test PRIVATE
                           ${PROJECT_INCLUDE_DIR}
)
target_link_libraries(rqc-json-test PRIVATE
                      GTest::gtest_main
                      cpprestsdk::cpprest
                      libzippp::libzippp
                      rexapis
)

set(IBMQ_REST_TEST_SOURCE_FILES
    ibmq_rest_test.cpp
)
add_executable(ibmq-rest-test ${IBMQ_REST_TEST_SOURCE_FILES})
add_dependencies(ibmq-rest-test GTest::gtest)
target_include_directories(ibmq-rest-test PRIVATE
                           ${PROJECT_INCLUDE_DIR}
)
target_link_libraries(ibmq-rest-test PRIVATE
                      GTest::gtest_main
                      cpprestsdk::cpprest
                      libzippp::libzippp
                      rexapis
)

set(IBMQ_JSON_TEST_SOURCE_FILES
    ibmq_json_test.cpp
)
add_executable(ibmq-json-test ${IBMQ_JSON_TEST_SOURCE_FILES})
add_dependencies(ibmq-json-test GTest::gtest)
target_include_directories(ibmq-json-test PRIVATE
                           ${PROJECT_INCLUDE_DIR}
)
target_link_libraries(ibmq-json-test PRIVATE
                      GTest::gtest_main
                      cpprestsdk::cpprest
                      libzippp::libzippp
                      rexapis
)

set(SLURM_REST_TEST_SOURCE_FILES
    slurm_rest_test.cpp
)
add_executable(slurm-rest-test ${SLURM_REST_TEST_SOURCE_FILES})
add_dependencies(slurm-rest-test GTest::gtest)
target_include_directories(slurm-rest-test PRIVATE
                           ${PROJECT_INCLUDE_DIR}
)
target_link_libraries(slurm-rest-test PRIVATE
                      GTest::gtest_main
                      cpprestsdk::cpprest
                      libzippp::libzippp
                      rexapis
)

set(SLURM_JSON_TEST_SOURCE_FILES
    slurm_json_test.cpp
)
add_executable(slurm-json-test ${SLURM_JSON_TEST_SOURCE_FILES})
add_dependencies(slurm-json-test GTest::gtest)
target_include_directories(slurm-json-test PRIVATE
                           ${PROJECT_INCLUDE_DIR}
)
target_link_libraries(slurm-json-test PRIVATE
                      GTest::gtest_main
                      cpprestsdk::cpprest
                      libzippp::libzippp
                      rexapis
)

set(MINIO_CLI_TEST_SOURCE_FILES
    minio_cli_test.cpp
)
add_executable(minio-cli-test ${MINIO_CLI_TEST_SOURCE_FILES})
add_dependencies(slurm-json-test GTest::gtest)
target_include_directories(minio-cli-test PRIVATE
                           ${CMAKE_SOURCE_DIR}/vcpkg/installed/${VCPKG_TARGET_TRIPLET}/include
                           ${PROJECT_INCLUDE_DIR}
)
target_link_libraries(minio-cli-test PRIVATE
                      GTest::gtest_main
                      cpprestsdk::cpprest
                      libzippp::libzippp
                      rexapis
)

set(OQTOPUS_REST_TEST_SOURCE_FILES
    oqtopus_rest_test.cpp
)
add_executable(oqtopus-rest-test ${OQTOPUS_REST_TEST_SOURCE_FILES})
add_dependencies(oqtopus-rest-test GTest::gtest)
target_include_directories(oqtopus-rest-test PRIVATE
                           ${PROJECT_INCLUDE_DIR}
)
target_link_libraries(oqtopus-rest-test PRIVATE
                      GTest::gtest_main
                      cpprestsdk::cpprest
                      libzippp::libzippp
                      rexapis
)

set(OQTOPUS_JSON_TEST_SOURCE_FILES
    oqtopus_json_test.cpp
)
add_executable(oqtopus-json-test ${OQTOPUS_JSON_TEST_SOURCE_FILES})
add_dependencies(oqtopus-json-test GTest::gtest)
target_include_directories(oqtopus-json-test PRIVATE
                           ${PROJECT_INCLUDE_DIR}
)
target_link_libraries(oqtopus-json-test PRIVATE
                      GTest::gtest_main
                      cpprestsdk::cpprest
                      libzippp::libzippp
                      rexapis
)

