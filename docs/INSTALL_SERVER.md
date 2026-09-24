# Install and Configure SQC Scheduler(Server)

This document consolidates server-side installation and configuration for all supported authentication methods.

> [!CAUTION]
> Operating this software as a server on the Fugaku environment is out of scope.

## Application Home Directory

`SQC_HOME` is the application home directory. The default is `${HOME}/.local`.

```
$ export SQC_HOME=${SQC_HOME:-${HOME}/.local}
```

> [!NOTE]
> Running sqc-scheduler does not require root privileges. The commands in this document
> avoid `sudo` on the assumption that `SQC_HOME` points to a location writable by the
> current user (e.g. the default `${HOME}/.local`). If you set `SQC_HOME` to a location
> that requires root privileges to create directories or files in, use `sudo` as needed.

## DB Setting

1.Create DB files and tables

By default, WAL (Write-Ahead Logging) journal mode is enabled. To skip WAL mode, use the `--no-wal` option.

```
$ ./scripts/db-create.sh
```

To create the DB without WAL mode:

```
$ ./scripts/db-create.sh --no-wal
```

2.Add user information to DB

Add the user name required by your authentication method to the scheduler DB.
```
$ ./scripts/db-util.sh user add <USER_ID> general enable
```

### User names to add by authentication method

#### MUNGE
Add the user names registered on the local host, such as entries in `/etc/passwd`, to the scheduler DB.

#### JWT / gRPC
Add the subject in the JWT token (`sub` claim) as the user name. Use the same user name for both JWT on TCP and gRPC on HTTPS.

Example:
```
    "sub": "dcaf1157-954c-442b-b26f-5597d98c6621",
```

Add the user name to the scheduler DB using `db-util.sh`.
```
$ ./scripts/db-util.sh user add "dcaf1157-954c-442b-b26f-5597d98c6621" general enable
```

#### Mutual TLS
Add the subject in the client certificate as the user name.

Example:
```
$ openssl x509 -text -in client.crt | grep 'Subject:'
        Subject: C = JP, ST = Tokyo, O = Example, CN = Alice
```

Add the user name to the scheduler DB using `db-util.sh`.
```
$ ./scripts/db-util.sh user add "C = JP, ST = Tokyo, O = Example, CN = Alice" general enable
```

## Authentication-specific Settings

### TCP with MUNGE Authentication

#### Run MUNGE Server
1.Create a MUNGE key file (/etc/munge/munge.key), if missing
```
$ sudo mkdir -p /etc/munge
$ sudo chown munge:munge /etc/munge
$ sudo chmod 700 /etc/munge
$ sudo -u munge /usr/sbin/mungekey
```

2.Create a Directory To Place a PID File, if Missing
```
$ sudo mkdir -p /run/munge
$ sudo chown munge:munge /run/munge
$ sudo chmod 755 /run/munge
```

3.Run MUNGE Server
```
$ sudo -u munge /usr/sbin/munged
```

### TLS on TCP with JWT Authentication

For establishing TLS session and performing JWT authentication, the following files are required.

For the RPC scheduler:

* `server.crt`
   * server certificate
* `server.key`
   * server private key
* `ca.crt`
   * CA certificate
* `jwt_pub.key`
   * a public key of the issuer of valid JWT tokens
* `jwt_iss.txt`
   * a text file which contains value of an "ISS" claim of a valid JWT token

Put a server certificate and a private key.
```
$ mkdir -p ${SQC_HOME}/etc/sqc-scheduler
$ cp server.crt ${SQC_HOME}/etc/sqc-scheduler/server.crt
$ cp server.key ${SQC_HOME}/etc/sqc-scheduler/server.key
$ chmod 0600 ${SQC_HOME}/etc/sqc-scheduler/server.key
```

Put a CA certificate.
```
$ mkdir -p ${SQC_HOME}/etc/sqc-scheduler/ca
$ cp ca.crt ${SQC_HOME}/etc/sqc-scheduler/ca/ca.crt
$ ln -s ca.crt ${SQC_HOME}/etc/sqc-scheduler/ca/$(openssl x509 -hash -noout -in ${SQC_HOME}/etc/sqc-scheduler/ca/ca.crt).0
```

Put a public key for JWT (key for verifying JWT's signature) and an issuer (a text file in which an expected issuer of JWT is written).
```
$ cp jwt_pub.key ${SQC_HOME}/etc/sqc-scheduler/jwt_pub.key
$ cp jwt_iss.txt ${SQC_HOME}/etc/sqc-scheduler/jwt_iss.txt
```

### TLS on TCP with Mutual TLS Authentication

For establishing TLS session and performing mutual-TLS authentication, the following files are required.

For the RPC scheduler:

* `server.crt`
   * server certificate
* `server.key`
   * server private key
* `ca.crt`
   * CA certificate

Put a server certificate and a private key.
```
$ mkdir -p ${SQC_HOME}/etc/sqc-scheduler
$ cp server.crt ${SQC_HOME}/etc/sqc-scheduler/server.crt
$ cp server.key ${SQC_HOME}/etc/sqc-scheduler/server.key
$ chmod 0600 ${SQC_HOME}/etc/sqc-scheduler/server.key
```

Put a CA certificate.
```
$ mkdir -p ${SQC_HOME}/etc/sqc-scheduler/ca
$ cp ca.crt ${SQC_HOME}/etc/sqc-scheduler/ca/ca.crt
$ ln -s ca.crt ${SQC_HOME}/etc/sqc-scheduler/ca/$(openssl x509 -hash -noout -in ${SQC_HOME}/etc/sqc-scheduler/ca/ca.crt).0
```

### gRPC on HTTPS with JWT Authentication

For establishing HTTPS session and performing JWT authentication, the following files are required.

For the RPC scheduler:

* `grpc_server.crt`
   * server certificate
* `grpc_server.key`
   * server private key
* `grpc_ca.crt`
   * CA certificate
* `jwt_pub.key`
   * a public key of the issuer of valid JWT tokens
* `jwt_iss.txt`
   * a text file which contains value of an "ISS" claim of a valid JWT token

Put a server certificate and a private key.
```
$ mkdir -p ${SQC_HOME}/etc/sqc-scheduler
$ cp grpc_server.crt ${SQC_HOME}/etc/sqc-scheduler/grpc_server.crt
$ cp grpc_server.key ${SQC_HOME}/etc/sqc-scheduler/grpc_server.key
$ chmod 0600 ${SQC_HOME}/etc/sqc-scheduler/grpc_server.key
```

Put a CA certificate.
```
$ mkdir -p ${SQC_HOME}/etc/sqc-scheduler/ca
$ cp grpc_ca.crt ${SQC_HOME}/etc/sqc-scheduler/ca/grpc_ca.crt
```

Put a public key for JWT (key for verifying JWT's signature) and an issuer (a text file in which an expected issuer of JWT is written).
```
$ cp jwt_pub.key ${SQC_HOME}/etc/sqc-scheduler/jwt_pub.key
$ cp jwt_iss.txt ${SQC_HOME}/etc/sqc-scheduler/jwt_iss.txt
```

## Log

- Log file path(default: ./sqc-scheduler.log)

Example:
```
$ export SQC_LOG_FILE=/var/log/sqc-scheduler/sqc-scheduler.log
```

- Log level(default: 1)
Logs at the specified level or higher are output.
(DEBUG=1, INFO=2, NOTICE=3, WARNING=4, ERROR=5, FATAL=6)

Example:
```
$ export SQC_LOG_LEVEL=1
```

- Log debug level(default: 0)
Logs with values smaller than the specified value are output.

Example:
```
$ export SQC_LOG_DEBUGLEVEL=5
```

- Log rotation(default: 5M)
Log rotation size.
Specify a number or a number followed by 'G', 'M', or 'K'.

Example:
```
$ export SQC_LOG_ROTATESIZE=10M
```

## Start SQC Scheduler

```
$ ${SQC_HOME}/bin/sqc_rpc_sched 0.0.0.0:30001 0.0.0.0:30002 rqc-rest
```

where `<RUN_USER>` is a user for running the SQC scheduler.

`0.0.0.0:30001` and `0.0.0.0:30002` are addresses and ports that the RPC scheduler listens on for simple TCP RPC and for gRPC.
Even if you don't plan to use one protocol, you still need to specify a listen address and a port for both protocols.

`rqc-rest` is type of QC the SQC scheduler services for.
