BEGIN IMMEDIATE;

INSERT OR IGNORE INTO group_info (
  group_id, exec_time_limit_msec, exec_time_total_msec, created_time, update_time
) VALUES
  ('default', 15552000000, 0, CAST(strftime('%s','now') AS INTEGER) * 1000000000, CAST(strftime('%s','now') AS INTEGER) * 1000000000),
  ('group1', 15552000000, 0, CAST(strftime('%s','now') AS INTEGER) * 1000000000, CAST(strftime('%s','now') AS INTEGER) * 1000000000),
  ('group2', 15552000000, 0, CAST(strftime('%s','now') AS INTEGER) * 1000000000, CAST(strftime('%s','now') AS INTEGER) * 1000000000),
  ('group3', 15552000000, 0, CAST(strftime('%s','now') AS INTEGER) * 1000000000, CAST(strftime('%s','now') AS INTEGER) * 1000000000),
  ('group4', 15552000000, 0, CAST(strftime('%s','now') AS INTEGER) * 1000000000, CAST(strftime('%s','now') AS INTEGER) * 1000000000);

INSERT OR IGNORE INTO weight_info (
  priority, weight, created_time, update_time
) VALUES
  (0, 1000, CAST(strftime('%s','now') AS INTEGER) * 1000000000, CAST(strftime('%s','now') AS INTEGER) * 1000000000),
  (1, 1000, CAST(strftime('%s','now') AS INTEGER) * 1000000000, CAST(strftime('%s','now') AS INTEGER) * 1000000000),
  (2, 1000, CAST(strftime('%s','now') AS INTEGER) * 1000000000, CAST(strftime('%s','now') AS INTEGER) * 1000000000),
  (3, 1000, CAST(strftime('%s','now') AS INTEGER) * 1000000000, CAST(strftime('%s','now') AS INTEGER) * 1000000000),
  (4, 1000, CAST(strftime('%s','now') AS INTEGER) * 1000000000, CAST(strftime('%s','now') AS INTEGER) * 1000000000),
  (5, 1000, CAST(strftime('%s','now') AS INTEGER) * 1000000000, CAST(strftime('%s','now') AS INTEGER) * 1000000000),
  (6, 1000, CAST(strftime('%s','now') AS INTEGER) * 1000000000, CAST(strftime('%s','now') AS INTEGER) * 1000000000),
  (7, 1000, CAST(strftime('%s','now') AS INTEGER) * 1000000000, CAST(strftime('%s','now') AS INTEGER) * 1000000000),
  (8, 1000, CAST(strftime('%s','now') AS INTEGER) * 1000000000, CAST(strftime('%s','now') AS INTEGER) * 1000000000),
  (9, 1000, CAST(strftime('%s','now') AS INTEGER) * 1000000000, CAST(strftime('%s','now') AS INTEGER) * 1000000000);

COMMIT;

