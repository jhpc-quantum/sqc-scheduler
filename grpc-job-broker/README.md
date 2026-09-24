# gRPC Job Broker

## Build

### 1. Install the necessary packages

Ubuntu 24.04/26.04
```
$ sudo apt install build-essential cmake libtool pkg-config libssl-dev autoconf
```

Rocky Linux 10.2
```
$ sudo dnf install epel-release
$ sudo dnf config-manager --set-enabled crb
$ sudo dnf groupinstall "Development Tools"
$ sudo dnf install cmake openssl-devel boost-devel python3-devel libuuid-devel
```

### 2. Build and Install gRPC

```
$ git clone --recurse-submodules -b v1.66.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc
$ cd grpc
$ mkdir -p cmake/build
$ cd cmake/build
$ cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local/grpc ../..
$ make -j 4
$ sudo make install
$ cd ../..
```

protobuf (Protocol Buffers) is also installed under `/usr/local/grpc`.
Please note that gRPC doesn't work correctly in combination with other
versions of protobuf.  Use `/usr/local/grpc/bin/protoc` to compile
a message definition file (`*.proto`) of gRPC.

### 3. Build gRPC Job Broker

Change the current working directory to the top directory of gRPC
Job Broker source, then execute the following.

```
$ PATH=/usr/local/grpc/bin:$PATH
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=/usr/local/grpc-job-broker ..
$ make
$ sudo make install
```

## Run Test

### 1.Put Files for Server

For establishing HTTPS session and performing JWT authentication,
the following files are required:

* `grpc_server.crt`
   * server certificate
* `grpc_server.key`
   * server private key
* `grpc_ca.crt`
   * CA certificate
* `jwt_pub.key`
   * public key for JWT tokens
* `jwt_iss.txt`
   * expected issuer of JWT tokens

The private key file must be owned by the user who runs `grpc_server`
(denoted as `<RUN_USER>`) and its file mode must be 0600.

```
$ sudo mkdir -p /usr/local/grpc-job-broker/etc/ca
$ sudo cp grpc_server.crt /usr/local/grpc-job-broker/etc/grpc_server.crt
$ sudo cp grpc_server.key /usr/local/grpc-job-broker/etc/grpc_server.key
$ sudo chmod 0600 /usr/local/grpc-job-broker/etc/grpc_server.key
```

Put a CA certificate.
```
$ sudo cp grpc_ca.crt /usr/local/grpc-job-broker/etc/ca/grpc_ca.crt
```

For JWT authentication, put a public key file (key for varifying JWT's
signature) and an issuer file (a text file in which an expected issuer
of JWT is written).

```
$ sudo cp jwt_pub.key /usr/local/grpc-job-broker/etc/jwt_pub.key
$ sudo cp jwt_iss.txt /usr/local/grpc-job-broker/etc/jwt_iss.txt
```

Change owners of files and directories.
```
$ sudo chown -R <RUN_USER> /usr/local/grpc-job-broker/etc
```

### 2.Run Server
Run the server.
```
$ /usr/local/grpc-job-broker/bin/grpc_server 0.0.0.0:30002
```

The argument `0.0.0.0:30002` is an address and a port that the server
listens on.

### 3.Put Files for User

For establishing HTTPS session and performing JWT authentication,
the following files are required:

* `grpc_server_root.crt`
   * server or CA certificate
* `jwt.token`
   * JWT token

Put the server or CA certificate.

```
$ mkdir ~/.sqc-scheduler
$ cp grpc_server_root.crt ~/.sqc-scheduler/grpc_server_root.crt
```

Put a JWT token.
```
$ cp jwt.token ~/.sqc-scheduler/jwt.token
```

Create a QASM file.
```
$ vi /tmp/sample.qasm
```
```
OPENQASM 3;
include "stdgates.inc";

qreg q[4];
creg c[4];
h q[0];
h q[1];

ccx q[0],q[1],q[2];
cx q[0],q[3];
cx q[1],q[3];

measure q[3] -> c[0];
measure q[2] -> c[1];
measure q[1] -> c[2];
measure q[0] -> c[3];
```

### 4.Run Client

Set environment variable `SQC_GRPC_SERVER` to specify the location of
the server as the form `<DOMAIN NAME>:<PORT>`.
```
$ export SQC_GRPC_SERVER=server.example.com:30002
```

Note that the domain name of the server should be equivalent with
the common name (CN) of the subject in the server certificate and it
should be resolvable with DNS.

```
$ openssl x509 -in /usr/local/grpc-job-broker/etc/grpc_server.crt -text | grep Subject:
        Subject: C = JP, ST = Tokyo, O = Example, CN = server.example.com
```

Unset environment variable `http_proxy` and `https_proxy` if needed.
```
$ unset http_proxy https_proxy
```

Execute the client.
```
$ /usr/local/grpc-job-broker/bin/grpc_client submit --remark='remark text' rqc-rest 2 /tmp/sample.qasm qasm 1000
```

The command requests a job creation to the scheduler, where `rqc-rest` is QC type,
`2` is priority of the job, `/path/to/test.qasm` is a path to QASM file and `1000`
is the number of shots respectively.

Upon success, the the server issues a job ID.

```
[INFO ] reply: result_code=0, message=, job_id=d4d22952-3159-4c7c-864c-89560700d552
```

The following command gets status of the submitted job.

```
$ /usr/local/grpc-job-broker/bin/grpc_client status d4d22952-3159-4c7c-864c-89560700d552
```

where `d4d22952-...-89560700d552` is a job ID issued by the scheduler.
A reply from the scheduler is printed on standard out.
```
[INFO ] reply: result_code=0, message=, job_status=4, computation_result={ (snip) }
```

To cancel the submitted job, execute `grpc_client` with `cancel` sub-command.

```
$ /usr/local/grpc-job-broker/bin/grpc_client cancel d4d22952-3159-4c7c-864c-89560700d552
```

Upon success, the the server replies a message with `result_code=0`.
```
[INFO ] reply: result_code=0, message=
```

Since `grpc_client cancel` sets status of the job to *cancelled*, you can still get
status of the cancelled job.  On the other hand, `grpc_client delete` removes
the entire data record of the job from the scheduler.

```
$ /usr/local/grpc-job-broker/bin/grpc_client delete d4d22952-3159-4c7c-864c-89560700d552
```

Upon success, the the server replies a message with `result_code=0`.
```
[INFO ] reply: result_code=0, message=
```

## Build Simple SQC Server

This source distribution includes the simple SQC Server, but it is not built
by default.  To build gRPC Job Broker with the simple SQC server, follow the
procedure below:

### 1. Install the necessary packages

Same as the default "Build" procedure.

### 2. Build and Install gRPC

Same as the default "Build" procedure.

### 3. Install jwt-cpp

```
$ git clone https://github.com/Thalhammer/jwt-cpp.git
$ sudo mkdir -p /usr/local/jwt-cpp
$ sudo cp -r jwt-cpp/include /usr/local/jwt-cpp
```

### 4. Build gRPC Request Broker with Simple SQC Server

Change the current working directory to the top directory of gRPC
Request Broker source, then execute the following.

```
$ PATH=/usr/local/grpc/bin:$PATH
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX=/usr/local/grpc-job-broker \
        -DBUILD_SIMPLE_SQC_SERVER=1 ..
$ make
$ sudo make install
```

Create a directory and change its owner.
```
$ sudo mkdir -p /usr/local/grpc-job-broker/var/qiskit_server
$ sudo chown -R <RUN_USER> /usr/local/grpc-job-broker/var
```

### 5. Install Qiskit Aer

```
$ pip3 install "qiskit[qasm3-import]==1.3.2" qiskit-aer==0.16.0
```

## Run Test

Currently, the simple SQC server supports Qiskit Aer.

### 1.Put Files for Server

Same as the default "Run Test" procedure.

Create the user DB file.
Set the user ID (JWT sub) on one line per user.
```
$ vi /usr/local/grpc-job-broker/etc/users.db
```
```
fbd8f3b7-9697-4d2e-ab32-f1ad38b9e13a
0847b863-d992-4e6e-9db6-03e88632d804
.
.
.
```

Change owners of files and directories to the user who runs the RPC scheduler (denoted
as <RUN_USER>).
```
$ sudo chown -R <RUN_USER> /usr/local/grpc-job-broker/etc
```

### 2.Run Server

Run the server.
```
$ /usr/local/grpc-job-broker/bin/run_simple_sqc_server.sh 0.0.0.0:30002
```

The argument `0.0.0.0:30002` is an address and a port that the server listens on.

### 3.Put Files for User

Same as the default "Run Test" procedure.

### 4.Run Client

Same as the default "Run Test" procedure.

Note that the simple SQC server only accepts `submit` and `status` sub-commands
of `grpc_client`.

