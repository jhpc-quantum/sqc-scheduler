CREATE TABLE IF NOT EXISTS user_info (
    user_id TEXT PRIMARY KEY NOT NULL,
    role_type INTEGER NOT NULL DEFAULT 0,
    status INTEGER NOT NULL DEFAULT 0,
    created_time INTEGER NOT NULL DEFAULT 0,
    update_time INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS group_info (
    group_id TEXT PRIMARY KEY NOT NULL,
    exec_time_limit_msec INTEGER NOT NULL DEFAULT 0 CHECK (exec_time_limit_msec >= 0),
    exec_time_total_msec INTEGER NOT NULL DEFAULT 0 CHECK (exec_time_total_msec >= 0),
    created_time INTEGER NOT NULL DEFAULT 0,
    update_time INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS user_group_info (
    user_id TEXT NOT NULL,
    group_id TEXT NOT NULL,
    status INTEGER NOT NULL DEFAULT 0,
    created_time INTEGER NOT NULL DEFAULT 0,
    update_time INTEGER NOT NULL DEFAULT 0,
    PRIMARY KEY (user_id, group_id)
);

CREATE TABLE IF NOT EXISTS job_info (
    job_id TEXT PRIMARY KEY NOT NULL,
    user_id TEXT NOT NULL,
    group_id TEXT NOT NULL,
    priority INTEGER NOT NULL DEFAULT 0,
    status INTEGER NOT NULL DEFAULT 0,
    qc_job_id TEXT,
    qprogram TEXT,
    circuit_fmt INTEGER,
    shots INTEGER,
    qc_type INTEGER NOT NULL DEFAULT 0,
    transpiler INTEGER,
    remark TEXT,
    result TEXT,
    exec_time_estimate_msec INTEGER NOT NULL DEFAULT 0 CHECK (exec_time_estimate_msec >= 0),
    exec_time_msec INTEGER NOT NULL DEFAULT 0 CHECK (exec_time_msec >= 0),
    created_time INTEGER NOT NULL DEFAULT 0,
    queued_time INTEGER NOT NULL DEFAULT 0,
    running_time INTEGER NOT NULL DEFAULT 0,
    done_time INTEGER NOT NULL DEFAULT 0,
    cancelled_time INTEGER NOT NULL DEFAULT 0,
    error_time INTEGER NOT NULL DEFAULT 0,
    deleted_time INTEGER NOT NULL DEFAULT 0,
    update_time INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE IF NOT EXISTS weight_info (
    priority INTEGER PRIMARY KEY NOT NULL,
    weight INTEGER NOT NULL DEFAULT 1000 CHECK (weight >= 0),
    created_time INTEGER NOT NULL DEFAULT 0,
    update_time INTEGER NOT NULL DEFAULT 0
);

