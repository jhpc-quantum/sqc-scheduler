/* job_id, user_id, priority, status, qc_job_id,
   qprogram, circuit_fmt, shots, qc_type, transpiler, remark, result,
   created_time, queued_time, running_time, done_time,
   cancelled_time, error_time, update_time */
INSERT INTO job_info VALUES("test-job-id", "alice", 9, 1, NULL,
                            "test-qprogram", 0, 1000, 0, 0, "test-remark", NULL,
                            1736474012, NULL, NULL, NULL, NULL, NULL, NULL);

