# Build SQC Scheduler

## Prerequisites

### Ubuntu 24.04/26.04

```
$ sudo apt install build-essential cmake libtool pkg-config libboost-all-dev libssl-dev libcurl4-openssl-dev zip unzip
```

### Rocky Linux 10.2

```
$ sudo dnf install epel-release
$ sudo dnf config-manager --set-enabled crb
$ sudo dnf groupinstall "Development Tools"
$ sudo dnf install cmake openssl-devel boost-devel python3-devel perl perl-devel nkf munge-devel gmp-devel libsqlite3x-devel libuuid-devel
```

## Build

1.Set the URL and token to use QC
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

2.Build
```
$ ./scripts/build-app-sqc-scheduler.sh
```

