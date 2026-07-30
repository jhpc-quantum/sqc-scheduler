#!/usr/bin/env python3

import argparse
import os
import sqlite3
import sys
import time

USER_STATUS_DISABLED = 1
USER_STATUS_ENABLED = 2

SQL_COUNT_USER_NUM = 'SELECT COUNT(*) FROM user_info WHERE user_id=?'
SQL_UPDATE_USER_STATUS = 'UPDATE user_info SET status=?, update_time=? WHERE user_id=?'

def update_user_status(db_path, user_id, status_str):
    if not os.path.exists(db_path):
        print(f"Database file was not found: '{db_path}'", file=sys.stderr)
        sys.exit(1)

    status = USER_STATUS_ENABLED if status_str.lower() == 'enable' else USER_STATUS_DISABLED

    conn = None
    try:
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()

        cursor.execute(SQL_COUNT_USER_NUM, (user_id,))
        user_exists = cursor.fetchone()[0]

        if user_exists == 1:
            current_unix_time = int(time.time())
            cursor.execute(SQL_UPDATE_USER_STATUS, (status, current_unix_time, user_id))

            conn.commit()

            print(f"Update '{user_id}' was successful: status='{status_str}', update_time='{current_unix_time}'")

        else:
            print(f"User not found: '{user_id}'", file=sys.stderr)
            sys.exit(1)

    except sqlite3.Error as e:
        print(f"Database error occurred: '{e}'", file=sys.stderr)
        if conn:
            conn.rollback()
        sys.exit(1)
    finally:
        if conn:
            conn.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Switche user enabled/disabled status.')
    parser.add_argument('db_path', help='DB path')
    parser.add_argument('user_id', help='Target user_id')
    parser.add_argument('status', help="Status(enable or disable)",
                        choices=['enable', 'disable'])

    args = parser.parse_args()

    update_user_status(args.db_path, args.user_id, args.status)

