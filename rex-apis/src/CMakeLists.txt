set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(SRC_SOURCE_FILES
    rexapis.cpp
    rexapis_util.cpp
    rqc_rest.cpp
    rqc_json.cpp
    ibmq_rest.cpp
    ibmq_json.cpp
    slurm_rest.cpp
    slurm_json.cpp
    minio_cli.cpp
    oqtopus_rest.cpp
    oqtopus_json.cpp
)

add_library(rexapis SHARED ${SRC_SOURCE_FILES})

target_include_directories(rexapis PRIVATE
                           ${CMAKE_SOURCE_DIR}/vcpkg/installed/${VCPKG_TARGET_TRIPLET}/include
                           ${PROJECT_INCLUDE_DIR}
)

target_link_libraries(rexapis PRIVATE
                      cpprestsdk::cpprest
                      miniocpp::miniocpp
                      libzippp::libzippp
)

install(DIRECTORY
          ${PROJECT_INCLUDE_DIR}/
        DESTINATION
          include
        )

install(TARGETS
          rexapis
        DESTINATION
          lib
)

