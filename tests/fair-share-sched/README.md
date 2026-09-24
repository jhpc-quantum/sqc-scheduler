# Fair-Share Scheduling Integration Test

## What this verifies

Fair-share job scheduling (strict priority + per-priority round-robin
across groups) dispatches jobs in the correct order, submitted via a real
`sqc_rpc_sched` process over both RPC and gRPC.

20 jobs are submitted across 5 groups and 4 priority levels, in this
pattern:

| Priority | default | grpA | grpB | grpC | grpD | Subtotal |
|---------:|--------:|-----:|-----:|-----:|-----:|---------:|
|        9 |       3 |    1 |    3 |    1 |    0 |        8 |
|        6 |       1 |    0 |    0 |    2 |    1 |        4 |
|        3 |       0 |    2 |    1 |    0 |    0 |        3 |
|        0 |       1 |    1 |    1 |    1 |    1 |        5 |

The test checks the following, both derived from the `job_info.running_time`
order of all 20 jobs after completion (see `verify_order.sh`):

### Check 1: Dispatch order

| Field           | Detail                                                                               |
|-----------------|--------------------------------------------------------------------------------------|
| What it checks  | Jobs dispatch in round-robin group order [1] combined with strict priority order [2] |
| Pass condition  | The `running_time`-ordered job sequence exactly matches the expected 20-item order   |

[1]: round-robin group order: `default` -> `grpA` -> `grpB` -> `grpC` -> `grpD`
[2]: priority dispatch order, highest first: 9 -> 6 -> 3 -> 0

Expected order, in ascending `running_time` (1 = dispatched first):

| Order | Group   | Priority | Occurrence |
|------:|:--------|---------:|-----------:|
|     1 | default |        9 |          1 |
|     2 | grpA    |        9 |          1 |
|     3 | grpB    |        9 |          1 |
|     4 | grpC    |        9 |          1 |
|     5 | default |        9 |          2 |
|     6 | grpB    |        9 |          2 |
|     7 | default |        9 |          3 |
|     8 | grpB    |        9 |          3 |
|     9 | default |        6 |          1 |
|    10 | grpC    |        6 |          1 |
|    11 | grpD    |        6 |          1 |
|    12 | grpC    |        6 |          2 |
|    13 | grpA    |        3 |          1 |
|    14 | grpB    |        3 |          1 |
|    15 | grpA    |        3 |          2 |
|    16 | default |        0 |          1 |
|    17 | grpA    |        0 |          1 |
|    18 | grpB    |        0 |          1 |
|    19 | grpC    |        0 |          1 |
|    20 | grpD    |        0 |          1 |

`Occurrence` counts jobs of the same group and priority: e.g. rows 1, 5
and 7 are the 1st, 2nd and 3rd `default`-group priority-9 job to run.

### Check 2: Lowest priority is not starved

| Field           | Detail                                                                                         |
|-----------------|------------------------------------------------------------------------------------------------|
| What it checks  | Priority 0 is the lowest of the 4 levels used; it must still eventually run, not stall forever |
| Pass condition  | All 5 priority-0 jobs run strictly after every priority-9/6/3 job                              |

## Prerequisites

### Server side

- **Run this test in a dedicated test environment (a separate host or user
  account), not on top of an existing/production install.** `sqc_rpc_sched`'s
  DB path is fixed at build time, and this test recreates the DB from
  scratch, so it must not share an install with production.
- In that test environment, install `sqc_rpc_sched` the normal way:
  ```
  $ ./scripts/build-sqc-rpc-sched.sh
  ```
  (see `docs/INSTALL_SERVER.md` for full prerequisites)
- Prepare the server's JWT/TLS conf-dir manually beforehand (not done by
  these scripts).

### Client side

- Prepare the client's conf-dir (JWT token / TLS certs) manually
  beforehand, for whichever of `sqc_rpc_client` / `grpc_client` you will
  use (not done by these scripts).

## Steps

Run once per protocol (RPC, then gRPC). These steps assume the server and
client run on **separate hosts** (the normal case for this test, since
`sqc_rpc_sched`'s DB is fixed to the server host and `verify_order.sh` reads
it directly -- no RPC/gRPC call exposes `running_time`/`priority`/`group_id`).
If you happen to run both on the same host, step 4 (the copy) is
unnecessary: `client_job_ids.csv` is already at the default path.

1. **[Server]** Seed the DB (from this directory):
   ```
   $ ./seed_db.sh
   ```
2. **[Server]** Start the server, listening on all interfaces so a remote
   client can reach it:
   ```
   $ sqc_rpc_sched 0.0.0.0:30001 0.0.0.0:30002 dummy \
       --conf-dir=<path to the server's conf-dir>
   ```
3. **[Client]** Submit the 20-job pattern (from this directory, pointing
   `RPC_SERVER`/`GRPC_SERVER` at the server's address):
   ```
   $ RPC_CLIENT_CONF_DIR=<path to the client's conf-dir> \
       RPC_SERVER=<server-host>:30001 ./submit_jobs.sh rpc
   $ GRPC_CLIENT_CONF_DIR=<path to the client's conf-dir> \
       GRPC_SERVER=<server-host>:30002 ./submit_jobs.sh grpc
   ```
   This writes `${WORK_DIR}/client_job_ids.csv` (i.e.
   `tests/fair-share-sched/work/client_job_ids.csv`) on the client host --
   the name marks it as the client's output, since it must be copied over
   for step 5 below.
4. **[Client -> Server]** Copy `client_job_ids.csv` into the server's own
   `work/` directory, keeping the same filename so `verify_order.sh` finds
   it at its default path:
   ```
   $ scp work/client_job_ids.csv <server-host>:<path to repo on server>/tests/fair-share-sched/work/
   ```
   The comma-separated format survives ordinary text transfers, but a
   byte-preserving transfer (`scp`/`rsync`) is still recommended as good
   practice.
5. **[Server]** Verify dispatch order (reads the DB directly, so run where
   the DB file is):
   ```
   $ ./verify_order.sh
   ```
6. Stop the server, then repeat steps 1-5 for the other protocol.

## Overriding defaults

Every path/setting in `env.sh` can be overridden by exporting the matching
environment variable before running a script, e.g.:

```
$ RPC_SERVER=10.0.0.1:30001 ./submit_jobs.sh rpc
```
