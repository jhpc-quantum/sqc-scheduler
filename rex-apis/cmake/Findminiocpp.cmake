# Find MinIO
find_path(MINIOCPP_INCLUDE_DIR miniocpp/client.h
  PATHS
    ENV VCPKG_ROOT
    ${VCPKG_ROOT}
  PATH_SUFFIXES
    packages/minio-cpp_x64-linux/include
  NO_DEFAULT_PATH
)
message(STATUS "  Found MinIO CPP header directory: ${MINIOCPP_INCLUDE_DIR}")

find_library(MINIOCPP_LIBRARY
  NAMES
    miniocpp
  PATHS
    ENV VCPKG_ROOT
    ${VCPKG_ROOT}
  PATH_SUFFIXES
    packages/minio-cpp_x64-linux/lib
  NO_DEFAULT_PATH
)
message(STATUS "  Found MinIO CPP library         : ${MINIOCPP_LIBRARY}")

# Find cURLpp
find_path(CURLPP_INCLUDE_DIR curlpp/Easy.hpp
  PATHS
    ENV VCPKG_ROOT
    ${VCPKG_ROOT}
  PATH_SUFFIXES
    packages/curlpp_x64-linux/include
  NO_DEFAULT_PATH
)
message(STATUS "  Found cURLpp header directory   : ${CURLPP_INCLUDE_DIR}")

find_library(CURLPP_LIBRARY
  NAMES
    curlpp
  PATHS
    ENV VCPKG_ROOT
    ${VCPKG_ROOT}
  PATH_SUFFIXES
    packages/curlpp_x64-linux/lib
  NO_DEFAULT_PATH
)
message(STATUS "  Found cURLpp library            : ${CURLPP_LIBRARY}")

# Find nlohmann-json
find_path(NLOHMANNJSON_INCLUDE_DIR nlohmann/json_fwd.hpp
  PATHS
    ENV VCPKG_ROOT
    ${VCPKG_ROOT}
  PATH_SUFFIXES
    packages/nlohmann-json_x64-linux/include
  NO_DEFAULT_PATH
)
message(STATUS "  Found nlohmann header directory : ${NLOHMANNJSON_INCLUDE_DIR}")

# Find pugixml
find_library(PUGIXML_LIBRARY
  NAMES
    pugixml
  PATHS
    ENV VCPKG_ROOT
    ${VCPKG_ROOT}
  PATH_SUFFIXES
    packages/pugixml_x64-linux/lib
  NO_DEFAULT_PATH
)
message(STATUS "  Found pugixml library           : ${PUGIXML_LIBRARY}")

# Find curl
find_library(CURL_LIBRARY
  NAMES
    curl
  PATHS
    ENV VCPKG_ROOT
    ${VCPKG_ROOT}
  PATH_SUFFIXES
    packages/curl_x64-linux/lib
  NO_DEFAULT_PATH
)
message(STATUS "  Found curl library              : ${CURL_LIBRARY}")

# Find OpenSSL
find_library(SSL_LIBRARY
  NAMES
    ssl
  PATHS
    ENV VCPKG_ROOT
    ${VCPKG_ROOT}
  PATH_SUFFIXES
    packages/openssl_x64-linux/lib
  NO_DEFAULT_PATH
)
message(STATUS "  Found SSL library               : ${SSL_LIBRARY}")

# Find crypto
find_library(CRYPTO_LIBRARY
  NAMES
    crypto
  PATHS
    ENV VCPKG_ROOT
    ${VCPKG_ROOT}
  PATH_SUFFIXES
    packages/openssl_x64-linux/lib
  NO_DEFAULT_PATH
)
message(STATUS "  Found crypto library            : ${CRYPTO_LIBRARY}")

# Find inih
find_library(INIH_LIBRARY
  NAMES
    inih
  PATHS
    ENV VCPKG_ROOT
    ${VCPKG_ROOT}
  PATH_SUFFIXES
    packages/inih_x64-linux/lib
  NO_DEFAULT_PATH
)
message(STATUS "  Found inih library              : ${INIH_LIBRARY}")

# Find INIReader
find_library(INIREADER_LIBRARY
  NAMES
    INIReader
  PATHS
    ENV VCPKG_ROOT
    ${VCPKG_ROOT}
  PATH_SUFFIXES
    packages/inih_x64-linux/lib
  NO_DEFAULT_PATH
)
message(STATUS "  Found INIReader library         : ${INIREADER_LIBRARY}")

mark_as_advanced(
  MINIOCPP_INCLUDE_DIR
  MINIOCPP_LIBRARY
  CURLPP_INCLUDE_DIR
  CURLPP_LIBRARY
  NLOHMANNJSON_INCLUDE_DIR
  PUGIXML_LIBRARY
  CURL_LIBRARY
  SSL_LIBRARY
  CRYPTO_LIBRARY
  INIH_LIBRARY
  INIREADER_LIBRARY
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(miniocpp
  REQUIRED_VARS
    MINIOCPP_INCLUDE_DIR
    MINIOCPP_LIBRARY
    CURLPP_INCLUDE_DIR
    CURLPP_LIBRARY
    NLOHMANNJSON_INCLUDE_DIR
    PUGIXML_LIBRARY
    CURL_LIBRARY
    SSL_LIBRARY
    CRYPTO_LIBRARY
    INIH_LIBRARY
    INIREADER_LIBRARY
)

