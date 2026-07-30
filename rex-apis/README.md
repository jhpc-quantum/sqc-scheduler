# REX APIS

## Dependencies
- [C++ REST SDK](https://github.com/microsoft/cpprestsdk)
- [MinIO C++ Client SDK for Amazon S3 Compatible Cloud Storage](https://github.com/minio/minio-cpp)
- [LIBZIPPP](https://github.com/ctabin/libzippp)

## Preprocessing
### Ubuntu 24.04 Environment
```
$ sudo apt -y install build-essential cmake
```

### Rocky Linux 10.2 Environment
```
$ sudo dnf install epel-release
$ sudo dnf config-manager --set-enabled crb
$ sudo dnf groupinstall "Development Tools"
$ sudo dnf install cmake boost-devel
```

## Build and Install Dependency Libraries
### C++ REST SDK
```
$ git clone https://github.com/microsoft/cpprestsdk.git
$ cd cpprestsdk
$ git checkout v2.10.19
$ git submodule update --init
```
Apply the following patch
```
diff --git a/Release/src/http/common/http_helpers.cpp b/Release/src/http/common/http_helpers.cpp
index 9ffbd20d..2faceb94 100644
--- a/Release/src/http/common/http_helpers.cpp
+++ b/Release/src/http/common/http_helpers.cpp
@@ -84,7 +84,7 @@ size_t chunked_encoding::add_chunked_delimiters(_Out_writes_(buffer_size) uint8_
     }
     else
     {
-        char buffer[9];
+        char buffer[17];
 #ifdef _WIN32
         sprintf_s(buffer, sizeof(buffer), "%8IX", bytes_read);
 #else
```
```
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=<CPPRESTSDK_INSTALL_DIR> -DBUILD_TESTS=OFF -DBUILD_SAMPLES=OFF -DCPPREST_EXCLUDE_WEBSOCKETS=ON ..
$ make
$ make install
```

### MinIO C++ Client SDK, LIBZIPPP
```
$ git clone https://github.com/microsoft/vcpkg.git
$ cd vcpkg
$ ./bootstrap-vcpkg.sh
$ export VCPKG_ROOT=<VCPKG_INSTALL_DIR>
$ export PATH=$VCPKG_ROOT:$PATH
$ export VCPKG_DEFAULT_TRIPLET=x64-linux
$ vcpkg install minio-cpp libzippp
```

### GoogleTest(Development Only)
```
$ git clone https://github.com/google/googletest.git
$ cd googletest
$ git checkout v1.14.0
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=<GOOGLETEST_INSTALL_DIR> ..
$ make
$ make install
```

## Getting started
Build and install after installing the required libraries.

### Release
```
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> \
        -DCMAKE_PREFIX_PATH="<CPPRESTSDK_INSTALL_DIR>" \
        -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake ..
$ make
$ make install
```

### Develop
```
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> \
        -DCMAKE_PREFIX_PATH="<CPPRESTSDK_INSTALL_DIR>" \
        -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake ..
$ make
```

