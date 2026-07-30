CREATE TABLE IF NOT EXISTS user_info (
    user_id TEXT PRIMARY KEY,
    role_type INTEGER,
    status INTEGER,
    created_time INTEGER,
    update_time INTEGER
);

CREATE TABLE IF NOT EXISTS job_info (
    job_id TEXT PRIMARY KEY,
    user_id TEXT,
    priority INTEGER,
    status INTEGER,
    qc_job_id TEXT,
    qprogram TEXT,
    circuit_fmt INTEGER,
    shots INTEGER,
    qc_type INTEGER,
    transpiler INTEGER,
    remark TEXT,
    result TEXT,
    created_time INTEGER,
    queued_time INTEGER,
    running_time INTEGER,
    done_time INTEGER,
    cancelled_time INTEGER,
    error_time INTEGER,
    deleted_time INTEGER,
    update_time INTEGER
);

