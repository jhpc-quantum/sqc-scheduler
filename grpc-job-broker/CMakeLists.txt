cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_VERBOSE_MAKEFILE 1)
set(CMAKE_FIND_DEBUG_MODE 0)

project(grpc-job-broker C CXX)

find_package(Python3 REQUIRED COMPONENTS Interpreter Development)

include (cmake/common.cmake)

add_compile_definitions(_GNU_SOURCE=1)
add_compile_options(-Wall -Wextra)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
add_compile_options(-DINSTALL_PREFIX="${CMAKE_INSTALL_PREFIX}")

option(BUILD_SIMPLE_SQC_SERVER "Build the simple SQC server" OFF)
if (BUILD_SIMPLE_SQC_SERVER)
  message(STATUS "Build the qiskit server")
  if(NOT DEFINED JWT_CPP_PREFIX)
    set(JWT_CPP_PREFIX "/usr/local/jwt-cpp")
  endif()
  find_path(
      JWT_CPP_INCLUDE_DIR
    NAMES
      "jwt-cpp/jwt.h"
    HINTS
      "${JWT_CPP_PREFIX}/include"
    NO_CMAKE_FIND_ROOT_PATH
  )
  if(NOT JWT_CPP_INCLUDE_DIR)
    message(FATAL_ERROR "jwt-cpp/jwt.h not found.")
  endif()
endif()

add_subdirectory(server/lib)
add_subdirectory(server/app)
if (BUILD_SIMPLE_SQC_SERVER)
  add_subdirectory(simple_sqc_server/app)
endif()
add_subdirectory(client/app)
add_subdirectory(perf_client/app)
