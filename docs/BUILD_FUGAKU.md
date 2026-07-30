# Build SQC Scheduler(Fugaku)

## Build in Fugaku

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
$ mkdir /data/<gid>/<uid>/sqc
$ export SQC_HOME=/data/<gid>/<uid>/sqc
```

The following files are placed in <SQC_HOME>:

* `sqc-scheduler.tgz`

### Build

1.Extract the .tar.gz file
```
$ cd $SQC_HOME
$ tar zxvf sqc-scheduler.tgz
$ cd sqc-scheduler
```

2.Change the path to the external library directory
```
$ vi scripts/build-common.sh
```
```
-EXTERNAL_ROOT="${EXTERNAL_ROOT:-${HOME}/.local}"
+EXTERNAL_ROOT="${EXTERNAL_ROOT:-${SQC_HOME}}"
```

3.Apply the following patch
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

4.Set the URL and token to use QC
```
$ vi sqc-rpc-sched/app/req_invoker/rest_invoker.c
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

5.Build
```
$ ./scripts/build-app-sqc-scheduler.sh
```

