# Install and Configure SQC Scheduler(Client)

This document consolidates client-side installation and configuration for all supported authentication methods.

## Application Home Directory

`SQC_HOME` is the application home directory. The default is `${HOME}/.local`.

```
$ export SQC_HOME=${SQC_HOME:-${HOME}/.local}
```

## Common Preparation

### Create a QASM file

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

### Set environment variables to specify the RPC scheduler

For RPC:
```
$ export SQC_RPC_SERVER=<SERVER_ADDRESS>:30001
```

For gRPC:
```
$ export SQC_GRPC_SERVER=<SERVER_ADDRESS>:30002
```

Note that the domain name of the SQC Scheduler should be equivalent with
the common name (CN) of the subject in the server certificate and it
should be resolvable with DNS.
```
$ openssl x509 -in ${SQC_HOME}/etc/sqc-scheduler/grpc_server.crt -text | grep Subject:
        Subject: C = JP, ST = Tokyo, O = Example, CN = server.example.com
```

Unset `http_proxy` and `https_proxy` if needed.
```
$ unset http_proxy https_proxy
```

## Authentication-specific Settings

### TCP with MUNGE Authentication

With MUNGE authentication, the client command `sqc_rpc_client` executes
`ssh`. Configure `ssh` to login to the SQC scheduler, since `ssh` runs
`munge` command on the SQC Scheduler.

### TLS on TCP with JWT Authentication

For each user:

* `ca.crt`
   * CA certificate
* `jwt.token`
   * JWT token

Put a CA certificate.
```
$ mkdir -p ~/.sqc-scheduler/ca
$ cp ca.crt ~/.sqc-scheduler/ca/ca.crt
$ ln -s ca.crt ~/.sqc-scheduler/ca/$(openssl x509 -hash -noout -in ~/.sqc-scheduler/ca/ca.crt).0
```

Put a JWT token.
```
$ cp jwt.token ~/.sqc-scheduler/jwt.token
```

### TLS on TCP with Mutual TLS Authentication

For each user:

* `user.crt`
   * user certificate
* `user.key`
   * user private key
* `ca.crt`
   * CA certificate

Put a user certificate and a private key.
```
$ mkdir -p ~/.sqc-scheduler
$ cp user.crt ~/.sqc-scheduler/user.crt
$ cp user.key ~/.sqc-scheduler/user.key
$ chmod 0600 ~/.sqc-scheduler/user.key
```

Put a CA certificate.
```
$ mkdir -p ~/.sqc-scheduler/ca
$ cp ca.crt ~/.sqc-scheduler/ca/ca.crt
$ ln -s ca.crt ~/.sqc-scheduler/ca/$(openssl x509 -hash -noout -in ~/.sqc-scheduler/ca/ca.crt).0
```

### gRPC on HTTPS with JWT Authentication

For each user:

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

## Run Client Commands

### `sqc_rpc_client`(MUNGE/JWT/Mutual TLS)

Use `--auth=munge`, `--auth=jwt`, or `--auth=mutual-tls` based on your selected method.

Submit Job:
```
$ ${SQC_HOME}/bin/sqc_rpc_client submit --auth=<AUTH_METHOD> --remark='remark text' rqc-rest 2 /tmp/sample.qasm qasm 1000
```

where `rqc-rest` is type of QC, `/tmp/sample.qasm` is a path to a program file,
`qasm` is format type of the program file, `2` is priority of the job and `1000` is
the number of shots, respectively.

If you are using a user token for QC authentication, set environment variable.
```
$ export SQC_RPC_USER_TOKEN=<USER_TOKEN>
```

For MUNGE authentication, `ssh` may ask you for a password or passphrase like as follows:
```
user@<SERVER_ADDRESS>'s password:
```

Upon success, the super quantum computer RPC scheduler issues a job ID.
```
result: success
code: 0
message:
job_id: d4d22952-3159-4c7c-864c-89560700d552
```

Get Job Status:
```
$ ${SQC_HOME}/bin/sqc_rpc_client status --auth=<AUTH_METHOD> d4d22952-3159-4c7c-864c-89560700d552
```
```
result: success
code: 0
message:
job status:
  status: 4 (done)
  qc_job_id:
  result: {   "counts": { (snip) } }
```

Cancel Submitted Job:
```
$ ${SQC_HOME}/bin/sqc_rpc_client cancel --auth=<AUTH_METHOD> d4d22952-3159-4c7c-864c-89560700d552
```
```
result: success
code: 0
message:
```

Delete Submitted Job:
```
$ ${SQC_HOME}/bin/sqc_rpc_client delete --auth=<AUTH_METHOD> d4d22952-3159-4c7c-864c-89560700d552
```
```
result: success
code: 0
message:
```

List Information of Jobs Submitted by the Current User:
```
$ ${SQC_HOME}/bin/sqc_rpc_client list --auth=<AUTH_METHOD>
```
```
result: success
code: 0
message:
jobs:
  id=bb5ef342-4f9e-41b3-bfcb-37897b468dae, status=4 (done), qc_job_id=
  id=3c804a87-cd5a-4668-98ce-ead7c039639d, status=4 (done), qc_job_id=
  id=dd2fa67a-d4eb-4bd2-9523-379961c8cdce, status=4 (done), qc_job_id=
```

Delete Jobs by Administrator:
```
$ ${SQC_HOME}/bin/sqc_rpc_client adm-del-jobs --auth=<AUTH_METHOD> alice 20240101 20241231
```
```
result: success
code: 0
message:
```

Add a User:
```
$ ${SQC_HOME}/bin/sqc_rpc_client adm-add-user --auth=<AUTH_METHOD> bob
```
```
result: success
code: 0
message:
```

Set Status of a User:
```
$ ${SQC_HOME}/bin/sqc_rpc_client adm-set-user-status --auth=<AUTH_METHOD> bob disable
```
```
result: success
code: 0
message:
```

To reactivate the user:
```
$ ${SQC_HOME}/bin/sqc_rpc_client adm-set-user-status --auth=<AUTH_METHOD> bob enable
```

Set Executable Time Limit of a Group:
```
$ ${SQC_HOME}/bin/sqc_rpc_client adm-set-group-exec-time-limit --auth=<AUTH_METHOD> research 100
```
```
result: success
code: 0
message:
```

where `research` is the group ID and `100` is the executable time limit in hours.

Set Status of a User-Group Association:
```
$ ${SQC_HOME}/bin/sqc_rpc_client adm-set-user-group-status --auth=<AUTH_METHOD> bob research disable
```
```
result: success
code: 0
message:
```

where `bob` is the user ID, `research` is the group ID, and `disable` deactivates the
association between them. If the association does not exist yet, `enable` creates it;
`disable` on a non-existent association fails instead of creating one.

To reactivate the association:
```
$ ${SQC_HOME}/bin/sqc_rpc_client adm-set-user-group-status --auth=<AUTH_METHOD> bob research enable
```

### `grpc_client`(gRPC)

Unlike simple TCP RPC, we use `grpc_client` here.

Submit Job:
```
$ ${SQC_HOME}/grpc-job-broker/bin/grpc_client submit --remark='remark text' rqc-rest 2 /tmp/sample.qasm qasm 1000
```

If you are using a user token for QC authentication, set environment variable.
```
$ export SQC_GRPC_USER_TOKEN=<USER_TOKEN>
```

Upon success, the super quantum computer RPC scheduler issues a job ID.
```
result: success
code: 0
message:
job_id: d4d22952-3159-4c7c-864c-89560700d552
```

Get Job Status:
```
$ ${SQC_HOME}/grpc-job-broker/bin/grpc_client status d4d22952-3159-4c7c-864c-89560700d552
```
```
result: success
code: 0
message:
job status:
  status: 4
  qc_job_id:
  qc_result: {   "counts": { (snip) } }
```

Cancel Submitted Job:
```
$ ${SQC_HOME}/grpc-job-broker/bin/grpc_client cancel d4d22952-3159-4c7c-864c-89560700d552
```
```
result: success
code: 0
message:
```

Delete Submitted Job:
```
$ ${SQC_HOME}/grpc-job-broker/bin/grpc_client delete d4d22952-3159-4c7c-864c-89560700d552
```
```
result: success
code: 0
message:
```

List Information of Jobs Submitted by the Current User:
```
$ ${SQC_HOME}/grpc-job-broker/bin/grpc_client list
```
```
result: success
code: 0
message:
jobs:
  id=bb5ef342-4f9e-41b3-bfcb-37897b468dae, status=4, qc_job_id=
  id=3c804a87-cd5a-4668-98ce-ead7c039639d, status=4, qc_job_id=
  id=dd2fa67a-d4eb-4bd2-9523-379961c8cdce, status=4, qc_job_id=
```

Delete Jobs by Administrator:
```
$ ${SQC_HOME}/grpc-job-broker/bin/grpc_client adm-del-jobs alice 20240101 20241231
```
```
result: success
code: 0
message:
```

Add a User:
```
$ ${SQC_HOME}/grpc-job-broker/bin/grpc_client adm-add-user bob
```
```
result: success
code: 0
message:
```

Set Status of a User:
```
$ ${SQC_HOME}/grpc-job-broker/bin/grpc_client adm-set-user-status bob disable
```
```
result: success
code: 0
message:
```

To reactivate the user:
```
$ ${SQC_HOME}/grpc-job-broker/bin/grpc_client adm-set-user-status bob enable
```

Set Executable Time Limit of a Group:
```
$ ${SQC_HOME}/grpc-job-broker/bin/grpc_client adm-set-group-exec-time-limit research 100
```
```
result: success
code: 0
message:
```

where `research` is the group ID and `100` is the executable time limit in hours.

Set Status of a User-Group Association:
```
$ ${SQC_HOME}/grpc-job-broker/bin/grpc_client adm-set-user-group-status bob research disable
```
```
result: success
code: 0
message:
```

where `bob` is the user ID, `research` is the group ID, and `disable` deactivates the
association between them. If the association does not exist yet, `enable` creates it;
`disable` on a non-existent association fails instead of creating one.

To reactivate the association:
```
$ ${SQC_HOME}/grpc-job-broker/bin/grpc_client adm-set-user-group-status bob research enable
```
