# Test scripts

This directory contains shell scripts for testing job broker and client.

## Run Test

Install `job_broker_server` and `job_broker_client` following
the instruction written in `../README.md`.

Run `job_broker_server`.

```
$ /usr/local/job_broker/bin/job_broker_server 0.0.0.0:10080
```

Open another terminal and execute a test script in this directory.
For example:

```
$ ./submit_and_status.sh
job submitted: job_id=2254e7fb-c9f9-4d8a-9110-89f8ed8a18bd
retrieved job status job_id=2254e7fb-c9f9-4d8a-9110-89f8ed8a18bd: 3 (JOB_STATUS_RUNNING)
```

See the comments in each test script to see what output is expected.

## Test Scripts

The outline of each test script is as follows:

* `submit_and_status.sh`
  - submits a job and retrieve status of the job.
* `submit_and_status_pri_jobs.sh`
  - submits prioritized jobs and retrieve status.
* `submit_and_status_until.sh`
  - submits a job and retrieves status repeatedly.
* `submit_largest_qasm.sh`
  - submits QASM of largest size.
* `submit_longest_remark.sh`
  - submits a job with longest remark.
* `submit_longest_user_id.sh`
  - submits a job with longest user ID.
* `submit_too_large_qasm.sh`
   - submits too large QASM.
* `submit_too_long_remark.sh`
   - submits too long remark.
* `submit_too_long_user_id.sh`
   - submits too long user ID.

Read the comments described in each script for details about what
it tests.
