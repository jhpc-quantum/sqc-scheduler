# Build Simple SQC Scheduler

## Prerequisites
### Ubuntu 24.04/26.04

```
$ sudo apt install build-essential cmake libtool pkg-config libssl-dev autoconf
```

### Rocky Linux 10.2

```
$ sudo dnf install epel-release
$ sudo dnf config-manager --set-enabled crb
$ sudo dnf groupinstall "Development Tools"
$ sudo dnf install cmake openssl-devel boost-devel python3-devel libuuid-devel
```

## Build

```
$ ./scripts/build-app-simple-sqc.sh
```

