# Install and Configure Simple SQC Scheduler

## Application Home Directory

`SIMPLE_SQC_HOME` is the application home directory. The default is `${HOME}/.local/simple-sqc`.

```
$ export SIMPLE_SQC_HOME=${SIMPLE_SQC_HOME:-${HOME}/.local/simple-sqc}
```

## Create a directory

```
$ mkdir -p ${SIMPLE_SQC_HOME}/var/qiskit_server
```

## Install Qiskit Aer

```
$ pip3 install "qiskit[qasm3-import]==1.3.2" qiskit-aer==0.16.0
```

## Run Server

Currently, the simple SQC server supports Qiskit Aer.

1.Put Files for Server

Detailed instructions can be found under `gRPC on HTTPS with JWT Authentication` section of [Install and Configure SQC Scheduler(Server)](INSTALL_SERVER.md).

2.Create the user DB file.
Set the user ID (JWT sub) on one line per user.
```
$ mkdir -p ${SIMPLE_SQC_HOME}/etc
$ vi ${SIMPLE_SQC_HOME}/etc/users.db
```
```
fbd8f3b7-9697-4d2e-ab32-f1ad38b9e13a
0847b863-d992-4e6e-9db6-03e88632d804
.
.
.
```

3.Put Files for Server
 Detailed instructions can be found under `gRPC on HTTPS with JWT Authentication` in [Install and Configure SQC Scheduler(Server)](INSTALL_SERVER.md).

4.Run Server
```
$ ${SIMPLE_SQC_HOME}/bin/run_simple_sqc_server.sh 0.0.0.0:30002
```

The argument `0.0.0.0:30002` is an address and a port that the server listens on.


## Run Client

1.Put Files for Client

Detailed instructions can be found under `gRPC on HTTPS with JWT Authentication` section of [Install and Configure SQC Scheduler(Client)](INSTALL_CLIENT.md).

2.Run Client

Detailed instructions can be found under `grpc_client(gRPC)` section of [Install and Configure SQC Scheduler(Client)](INSTALL_CLIENT.md).

Note that the simple SQC server only accepts `submit` and `status` sub-commands
of `grpc_client`.

