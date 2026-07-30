# Super Quantum Computer RPC Scheduler

## Build in Ubuntu 22.04/24.04
### Install the necessary packages for the build
```
$ sudo apt install build-essential cmake libtool pkg-config libboost-all-dev libssl-dev libcurl4-openssl-dev zip unzip
```

### Build and Install Microsoft C++ REST SDK
1.Get source code for C++ REST SDK

```
$ git clone https://github.com/microsoft/cpprestsdk.git
$ cd cpprestsdk
$ git checkout 9c654889efb6f5bda
$ git submodule update --init
```

2.Apply the following patch
```
$ diff --git a/Release/src/http/common/http_helpers.cpp b/Release/src/http/common/http_helpers.cpp
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

3.Build & Install
```
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=/usr/local/cpprestsdk -DBUILD_TESTS=OFF -DBUILD_SAMPLES=OFF ..
$ make
$ sudo make install
```

### Build and Install REX APIS
1.MinIO C++ Client SDK Build & Install
```
$ cd rex-apis/rqc
$ git clone https://github.com/microsoft/vcpkg.git
$ cd vcpkg
$ ./bootstrap-vcpkg.sh
$ export VCPKG_ROOT=$(pwd)
$ export PATH=$VCPKG_ROOT:$PATH
$ export VCPKG_DEFAULT_TRIPLET=x64-linux
$ vcpkg install minio-cpp libzippp
```

2.REX APIS Build & Install
```
$ cd ..
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local/rex-apis \
        -DCMAKE_PREFIX_PATH=/usr/local/cpprestsdk \
        -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake ..
$ make
$ sudo make install
```

### Build and Install gRPC Job Broker
1.Build & Install gRPC
```
$ git clone --recurse-submodules -b v1.66.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc
$ cd grpc
$ mkdir -p cmake/build
$ cd cmake/build
$ cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local/grpc ../..
$ make
$ sudo make install
$ cd ../../..
```

2.Build & Install
```
$ PATH=/usr/local/grpc/bin:$PATH
$ cd grpc-job-broker
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=/usr/local/grpc-job-broker ..
$ make
$ sudo make install
```

### Build Super Quantum Computer RPC Scheduler
1.Install the necessary packages for the build
```
$ sudo apt install libgmp-dev uuid-dev libsqlite3-dev sqlite3 nkf munge libmunge-dev openssl
```

2.Build and Install protobuf
```
$ git clone https://github.com/protocolbuffers/protobuf.git
$ cd protobuf
$ git checkout v22.5
$ git submodule update --init --recursive
$ vi third_party/abseil-cpp/CMakeLists.txt <- Delete lines 71, 72, 73
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=/usr/local/protobuf ..
$ make
$ sudo make install
$ cd ../..
```

3.Build and Install protobuf-c
```
$ git clone https://github.com/protobuf-c/protobuf-c.git
$ cd protobuf-c
$ git checkout v1.5.0
$ export PKG_CONFIG=/usr/bin/pkg-config
$ export PKG_CONFIG_PATH=/usr/local/protobuf/lib/pkgconfig
$ ./autogen.sh
$ ./configure --prefix=/usr/local/protobuf-c
$ make
$ make check
$ sudo make install
$ cd ..
```

4.Install jwt-cpp
```
$ git clone https://github.com/Thalhammer/jwt-cpp.git
$ sudo mkdir -p /usr/local/jwt-cpp
$ sudo cp -r jwt-cpp/include /usr/local/jwt-cpp
```

5.Set the URL and token to use QC
```
$ cd sqc-rpc-sched
$ vi app/req_invoker/rest_invoker.c
```
```
#define RQC_REST_BASE_URL "rqc-dummy-url"
#define RQC_REST_TOKE "rqc-dummy-token"
```
```
#define IBM_REST_BASE_URL "ibm-dummy-url"
#define IBM_REST_TOKE "ibm-dummy-token"
```
```
#define SLURM_REST_BASE_URL "slurm-dummy-url"
#define SLURM_REST_TOKE "slurm-dummy-token"
```
```
#define OQTOPUS_REST_BASE_URL "oqtopus-dummy-url"
#define OQTOPUS_REST_TOKEN "oqtopus-dummy-token"
```

6.Set the DB file
```
$ vi app/include/sqc_rpc_sched_macros.h
```
```
#define SQC_RPC_SCHED_DB_FILE "<DB_FILE_PATH>"
```

7.Delete old libraries
```
$ sudo rm -f /usr/local/lib/libsqc_dbmgr.* \
             /usr/local/lib/libsqc_modtmpl.* \
             /usr/local/lib/libsqc_util.*
```

8.Build `sqc_rpc_sched`
```
$ ./configure --with-protobuf_c=/usr/local/protobuf-c \
              --with-rexapis=/usr/local/rex-apis \
              --with-jobbroker=/usr/local/grpc-job-broker \
              --with-jwt-cpp=/usr/local/jwt-cpp
$ make
```

### Unit Test
1.Install Unity
```
$ git clone https://github.com/ThrowTheSwitch/Unity.git
$ cd Unity
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=/usr/local/Unity ..
$ make -j 4
$ sudo make install
```

2.Build
```
$ ./configure --enable-developer --with-unity=/usr/local/Unity \
                                 --with-protobuf_c=/usr/local/protobuf-c \
                                 --with-rexapis=/usr/local/rex-apis \
                                 --with-jobbroker=/usr/local/grpc-job-broker \
                                 --with-jwt-cpp=/usr/local/jwt-cpp
$ make
```

3.Run Test
```
$ make ut-run
```

## Rocky Linux 10.2
### Install the necessary packages for the build
```
$ sudo dnf install epel-release
$ sudo dnf config-manager --set-enabled crb
$ sudo dnf groupinstall "Development Tools"
$ sudo dnf install cmake openssl-devel boost-devel python3-devel perl perl-devel nkf munge-devel gmp-devel libsqlite3x-devel libuuid-devel
```

### Build and Install Microsoft C++ REST SDK
The build procedure is the same as in the Ubuntu environment, so please refer to that.

### Build and Install REX APIS
The build procedure is the same as in the Ubuntu environment, so please refer to that.

### Build and Install gRPC C Wrapper
The build procedure is the same as in the Ubuntu environment, so please refer to that.

### Build Super Quantum Computer RPC Scheduler
Only the following commands for building `protobuf-c` must be changed from the build procedure in the Ubuntu environment.
Otherwise, the build procedure is the same as in the Ubuntu environment.
```
$ export PKG_CONFIG_PATH=/usr/local/protobuf/lib64/pkgconfig
```

## Fugaku
### Load the packages needed for the build
```
$ . /vol0004/apps/oss/spack/share/spack/setup-env.sh
$ spack load gcc
$ spack load gcc@14.3.0/x55ohwd
$ spack load boost
$ spack load boost@1.86.0/7tdr7ix
$ spack load python
$ spack load python@3.13.5/xwl6x7i
```

### Set environment variables
```
$ mkdir /data/<gid>/<uid>/sqc_extern_lib
$ mkdir /data/<gid>/<uid>/sqc
$ export SQC_EXTERN_LIB_HOME=/data/<gid>/<uid>/sqc_extern_lib
$ export SQC_HOME=/data/<gid>/<uid>/sqc
```

### Build and Install Microsoft C++ REST SDK
1.Get source code for C++ REST SDK

```
$ git clone https://github.com/microsoft/cpprestsdk.git ${SQC_HOME}/cpprestsdk
$ cd ${SQC_HOME}/cpprestsdk
$ git checkout 9c654889efb6f5bda
$ git submodule update --init
```

2.Apply the following patch
```
$ diff --git a/Release/src/http/common/http_helpers.cpp b/Release/src/http/common/http_helpers.cpp
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

3.Build & Install
```
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=${SQC_EXTERN_LIB_HOME}/cpprestsdk -DBUILD_TESTS=OFF -DBUILD_SAMPLES=OFF ..
$ make
$ make install
```

### Build and Install REX APIS
1.MinIO C++ Client SDK Build & Install
```
$ cd rex-apis/rqc
$ git clone https://github.com/microsoft/vcpkg.git
$ cd vcpkg
$ ./bootstrap-vcpkg.sh
$ export VCPKG_ROOT=$(pwd)
$ export PATH=$VCPKG_ROOT:$PATH
$ export VCPKG_DEFAULT_TRIPLET=x64-linux
$ vcpkg install minio-cpp libzippp
```

3.REX REST Build & Install
```
$ cd ..
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=${SQC_EXTERN_LIB_HOME}/rex-apis \
        -DCMAKE_PREFIX_PATH=${SQC_EXTERN_LIB_HOME}/cpprestsdk \
        -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake ..
$ make
$ make install
```

### Build and Install gRPC Job Broker
1.Build & Install gRPC
```
$ git clone --recurse-submodules -b v1.66.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc ${SQC_HOME}/grpc
$ cd ${SQC_HOME}/grpc
$ mkdir -p cmake/build
$ cd cmake/build
$ cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=${SQC_EXTERN_LIB_HOME}/grpc ../..
$ make
$ make install
```

2.Build & Install
```
$ PATH=${SQC_EXTERN_LIB_HOME}/grpc/bin:$PATH
$ cd grpc-job-broker
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=${SQC_EXTERN_LIB_HOME}/grpc-job-broker ..
$ make
$ make install
```

### Build Super Quantum Computer RPC Scheduler
1.Build and Install MUNGE
```
$ git clone https://github.com/dun/munge.git ${SQC_HOME}/munge
$ cd ${SQC_HOME}/munge
$ git checkout munge-0.5.16
$ ./bootstrap
$ ./configure --prefix=${SQC_EXTERN_LIB_HOME}/munge
$ make
$ make install
```

2.Build and Install protobuf
```
$ git clone https://github.com/protocolbuffers/protobuf.git ${SQC_HOME}/protobuf
$ cd ${SQC_HOME}/protobuf
$ git checkout v22.5
$ git submodule update --init --recursive
$ vi third_party/abseil-cpp/CMakeLists.txt <- Delete lines 71, 72, 73
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=${SQC_EXTERN_LIB_HOME}/protobuf ..
$ make
$ make install
```

3.Build and Install protobuf-c
```
$ git clone https://github.com/protobuf-c/protobuf-c.git ${SQC_HOME}/protobuf-c
$ cd ${SQC_HOME}/protobuf-c
$ git checkout v1.5.0
$ export PKG_CONFIG=/usr/bin/pkg-config
$ export PKG_CONFIG_PATH=${SQC_EXTERN_LIB_HOME}/protobuf/lib64/pkgconfig
$ ./autogen.sh
$ ./configure --prefix=${SQC_EXTERN_LIB_HOME}/protobuf-c
$ make
$ make check
$ make install
```

4.Install jwt-cpp
```
$ git clone https://github.com/Thalhammer/jwt-cpp.git ${SQC_HOME}/jwt-cpp
$ mkdir -p ${SQC_EXTERN_LIB_HOME}/jwt-cpp
$ cp -r ${SQC_HOME}/jwt-cpp/include ${SQC_EXTERN_LIB_HOME}/jwt-cpp
```

5.Extracting tgz archive.
```
$ cd ${SQC_HOME}
$ tar zxvf sqc-rpc-sched.tgz
```

6.Set the URL and token to use QC
```
$ cd sqc-rpc-sched
$ vi app/req_invoker/rest_invoker.c
```
```
#define RQC_REST_BASE_URL "rqc-dummy-url"
#define RQC_REST_TOKE "rqc-dummy-token"
```
```
#define IBM_REST_BASE_URL "ibm-dummy-url"
#define IBM_REST_TOKE "ibm-dummy-token"
```
```
#define SLURM_REST_BASE_URL "slurm-dummy-url"
#define SLURM_REST_TOKE "slurm-dummy-token"

#define OQTOPUS_REST_BASE_URL "oqtopus-dummy-url"
#define OQTOPUS_REST_TOKEN "oqtopus-dummy-token"
```

7.Apply the following patch
```
$ vi sqc-rpc-sched/app/sqc_rpc_sched/Makefile.in
```
```
--- a/sqc-rpc-sched/app/sqc_rpc_sched/Makefile.in
+++ b/sqc-rpc-sched/app/sqc_rpc_sched/Makefile.in
@@ -16,6 +16,8 @@ SRCS          =       main.c modules.c
 CPPFLAGS       +=      -I$(APP_INC_BUILDDIR)
 CPPFLAGS       +=      -I$(APP_INC_SRCDIR)
 
+LDFLAGS                +=      -lssl -lcrypto
+
 DEP_LIBS       =       $(DEP_SQC_UTIL_LIB)
 DEP_LIBS       +=      $(DEP_APP_MODTMPL_LIB)
 DEP_LIBS       +=      $(DEP_APP_DBMGR_LIB)
```

```
$ vi sqc-rpc-sched/app/sqc_rpc_client/Makefile.in
```
```
--- a/sqc-rpc-sched/app/sqc_rpc_client/Makefile.in
+++ b/sqc-rpc-sched/app/sqc_rpc_client/Makefile.in
@@ -16,6 +16,9 @@ SRCS          =       adm_del_jobs.c adm_add_user.c adm_set_user_status.c cancel.c \
 include $(MKRULES_BUILDDIR)/vars.mk
 
 CPPFLAGS       +=      -I$(APP_INC_BUILDDIR) -I$(APP_INC_SRCDIR)
+
+LDFLAGS                +=      -lssl -lcrypto
+
 DEP_LIBS       +=      $(DEP_SQC_UTIL_LIB)
 DEP_LIBS       +=      $(DEP_APP_RPC_LIB)
 DEP_LIBS       +=      $(DEP_APP_SQC_RPC_COMMON_LIB)
```

```
$ vi sqc-rpc-sched/app/sqc_rpc_perf_client/Makefile.in
```
```
--- a/sqc-rpc-sched/app/sqc_rpc_perf_client/Makefile.in
+++ b/sqc-rpc-sched/app/sqc_rpc_perf_client/Makefile.in
@@ -15,6 +15,9 @@ SRCS          =       common.c submit.c main.c
 include $(MKRULES_BUILDDIR)/vars.mk
 
 CPPFLAGS       +=      -I$(APP_INC_BUILDDIR) -I$(APP_INC_SRCDIR)
+
+LDFLAGS                +=      -lssl -lcrypto
+
 DEP_LIBS       +=      $(DEP_SQC_UTIL_LIB)
 DEP_LIBS       +=      $(DEP_APP_RPC_LIB)
 DEP_LIBS       +=      $(DEP_APP_SQC_RPC_COMMON_LIB)
```

```
$ vi scripts/build-sqc-rpc-sched.sh
```
```
--- a/scripts/build-sqc-rpc-sched.sh
+++ b/scripts/build-sqc-rpc-sched.sh
@@ -39,6 +39,7 @@ cd "${SQC_RPC_SCHED_SRC_DIR}"
             --with-jwt-cpp="${JWT_CPP_INSTALL_PREFIX}" \
             --with-rexapis="${REX_APIS_INSTALL_PREFIX}" \
             --with-jobbroker="${GRPC_JOB_BROKER_INSTALL_PREFIX}" \
+            --with-munge=${EXTERNAL_ROOT}/munge \
     || die "configure of sqc-rpc-sched failed"
 
 make -j "${MAKE_JOBS}" || die "build of sqc-rpc-sched failed"
```

8.Set the DB file
```
$ vi sqc-rpc-sched/app/include/sqc_rpc_sched_macros.h
```
```
#define SQC_RPC_SCHED_DB_FILE "<DB_FILE_PATH>"
```

9.Build `sqc_rpc_sched`
```
$ ./configure --with-protobuf_c=${SQC_EXTERN_LIB_HOME}/protobuf-c \
              --with-rexrest=${SQC_EXTERN_LIB_HOME}/rex-apis \
              --with-jobbroker=${SQC_EXTERN_LIB_HOME}/grpc-job-broker \
              --with-jwt-cpp=${SQC_EXTERN_LIB_HOME}/jwt-cpp \
              --with-munge=${SQC_EXTERN_LIB_HOME}/munge
$ make
```

### Unit Test
The build procedure is the same as in the Ubuntu environment, so please refer to that.

