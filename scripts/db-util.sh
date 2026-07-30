#!/usr/bin/env bash
#
# db-util.sh - Manage sqc scheduler DB records (user/job) via sqlite3.
#
# This script provides sub-commands for listing, showing, adding, deleting,
# and updating records in user_info and job_info tables.
#
# Usage:
#   ./db-util.sh [OPTION...] user ARG ...
#   ./db-util.sh [OPTION...] job ARG ...
#

# Load DB-related path definitions and helper functions.
. "$(cd "$(dirname "$0")" && pwd)/db-common.sh"

show_user() {
  printf "SELECT * FROM user_info WHERE user_id = '%s';\n" "$1" | "$SQLITE" "$SQLITE_OPTIONS" "$DB_FILE"
}

show_all_users() {
  printf 'SELECT * FROM user_info;\n' | "$SQLITE" "$SQLITE_OPTIONS" "$DB_FILE"
}

add_user() {
  CREATED_TIME=$(date +"%s")'000000000'
  UPDATE_TIME=$CREATED_TIME
  printf "INSERT INTO user_info VALUES('%s', %d, %d, %d, %d);\n" \
         "$1" "$2" "$3" "$CREATED_TIME" "$UPDATE_TIME" | "$SQLITE" "$SQLITE_OPTIONS" "$DB_FILE"
}

delete_user() {
  printf "DELETE FROM user_info WHERE user_id='%s';\n" "$1" | "$SQLITE" "$SQLITE_OPTIONS" "$DB_FILE"
}

show_job() {
  printf "SELECT * FROM job_info WHERE job_id='%s';\n" "$1" | "$SQLITE" "$SQLITE_OPTIONS" "$DB_FILE"
}

show_all_jobs() {
  printf 'SELECT * FROM job_info;\n' | "$SQLITE" "$SQLITE_OPTIONS" "$DB_FILE"
}

delete_job() {
  printf "DELETE FROM job_info WHERE job_id='%s';\n" "$1" | "$SQLITE" "$SQLITE_OPTIONS" "$DB_FILE"
}

set_job_status() {
  printf "UPDATE job_info SET status=%d WHERE job_id='%s';\n" "$2" "$1" | "$SQLITE" "$SQLITE_OPTIONS" "$DB_FILE"
}

set_job_created_time() {
  printf "UPDATE job_info SET created_time=%d WHERE job_id='%s';\n" "$2" "$1" | "$SQLITE" "$SQLITE_OPTIONS" "$DB_FILE"
}

set_job_user_id() {
  printf "UPDATE job_info SET user_id='%s' WHERE job_id='%s';\n" "$2" "$1" | "$SQLITE" "$SQLITE_OPTIONS" "$DB_FILE"
}

set_user_status() {
  printf "UPDATE user_info SET status=%d WHERE user_id='%s';\n" "$2" "$1" | "$SQLITE" "$SQLITE_OPTIONS" "$DB_FILE"
}

set_user_role_type() {
  printf "UPDATE user_info SET role_type=%d WHERE user_id='%s';\n" "$2" "$1" | "$SQLITE" "$SQLITE_OPTIONS" "$DB_FILE"
}

#
# Print command line options.
#
print_options() {
  echo "Options:"
  echo "  -f FILE  specify the DB file (default: ${DB_FILE})"
  echo "  -h       also display RDB headers"
  echo "  -m MODE  specify the output mode of sqlite3"
}

#
# Parse user status
#
parse_user_status() {
  case "$1" in
  unknown)
    echo 0
    ;;
  disable)
    echo 1
    ;;
  enable)
    echo 2
    ;;
  0|1|2)
    echo "$1"
    ;;
  *)
    echo "invalid user status: $1" >&2
    exit 1
    ;;
  esac
}

#
# Parse user role type
#
parse_user_role_type() {
  case "$1" in
  unknown)
    echo 0
    ;;
  general)
    echo 1
    ;;
  admin)
    echo 2
    ;;
  0|1|2)
    echo "$1"
    ;;
  *)
    echo "invalid user role: $1" >&2
    exit 1
    ;;
  esac
}

#
# Parse job status
#
parse_job_status() {
  case "$1" in
  unknown)
    echo 0
    ;;
  created)
    echo 1
    ;;
  queued)
    echo 2
    ;;
  running)
    echo 3
    ;;
  done)
    echo 4
    ;;
  cancelled)
    echo 5
    ;;
  error)
    echo 6
    ;;
  0|1|2|3|4|5|6)
    echo "$1"
    ;;
  *)
    echo "invalid job status: $1" >&2
    exit 1
    ;;
  esac
}

#
# Parse output mode of sqlite.
#
parse_sqlite_mode() {
  case "$1" in
  ascii|box|column|csv|html|json|line|list|markdown|quote|table|tabs)
      true
      ;;
  *)
    echo "invalid output mode of sqlite command: $1" >&2
    exit 1
    ;;
  esac
}

#
# sub-command 'user set'.
#
subcmd_user_set() {
  if [ $# -eq 0 ]; then
    echo "missing field name to '$SUBCMD1 $SUBCMD2'" >&2
    exit 1
  elif [ $# -eq 1 ]; then
    echo "missing field value to '$SUBCMD1 $SUBCMD2'" >&2
    exit 1
  elif [ $# -eq 2 ]; then
    echo "missing user ID to '$SUBCMD1 $SUBCMD2'" >&2
    exit 1
  elif [ $# -gt 3 ]; then
    echo "too many arguments to sub-command '$SUBCMD1 $SUBCMD2'" >&2
    exit 1
  fi

  FIELD_NAME=$1
  JOB=$2
  FIELD_VALUE=$3
  shift 3

  case "$FIELD_NAME" in
  status)
    set_user_status "$JOB" "$(parse_user_status "$FIELD_VALUE")"
    exit $?
    ;;
  role_type|role)
    set_user_role_type "$JOB" "$(parse_user_role_type "$FIELD_VALUE")"
    exit $?
    ;;
  *)
    echo "invalid argument to sub-command '$SUBCMD1 $SUBCMD2'" >&2
    exit 1
    ;;
  esac
}

#
# sub-command 'job'.
#
subcmd_user() {
  TRY_HELP_MSG="try '$0 $SUBCMD1 help'"
  if [ $# -eq 0 ]; then
    show_all_users
    exit $?
  fi

  SUBCMD2=$1
  shift

  case "$SUBCMD2" in
  list)
    if [ $# -gt 0 ]; then
        echo "too many arguments to sub-command '$SUBCMD1 $SUBCMD2'" >&2
      exit 1
    fi
    show_all_users
    exit $?
    ;;
  show)
    if [ $# -lt 1 ]; then
      echo "missing USER-NAME to sub-command '$SUBCMD1 $SUBCMD2'" >&2
      echo "$TRY_HELP_MSG" >&2
    elif [ $# -gt 1 ]; then
      echo "too many arguments to sub-command '$SUBCMD1 $SUBCMD2'" >&2
      echo "$TRY_HELP_MSG" >&2
      exit 1
    fi
    show_user "$1"
    exit $?
    ;;
  add)
    if [ $# -lt 3 ]; then
      echo "missing USER-NAME to sub-command '$SUBCMD1 $SUBCMD2'" >&2
      echo "$TRY_HELP_MSG" >&2
      exit 1
    elif [ $# -gt 3 ]; then
      echo "too many arguments to sub-command '$SUBCMD1 $SUBCMD2'" >&2
      echo "$TRY_HELP_MSG" >&2
      exit 1
    fi
    add_user "$1" "$(parse_user_role_type "$2")" "$(parse_user_status "$3")"
    exit $?
    ;;
  delete|del|remove|rm)
    if [ $# -lt 1 ]; then
      echo "missing USER-NAME to sub-command '$SUBCMD1 $SUBCMD2'" >&2
      echo "$TRY_HELP_MSG" >&2
      exit 1
    elif [ $# -gt 1 ]; then
      echo "too many arguments to sub-command '$SUBCMD1 $SUBCMD2'" >&2
      echo "$TRY_HELP_MSG" >&2
      exit 1
    fi
    delete_user "$1"
    exit $?
    ;;
  set)
    if [ $# -eq 0 ]; then
      echo "missing field name to sub-command '$SUBCMD1 $SUBCMD2'" >&2
      echo "$TRY_HELP_MSG" >&2
      exit 1
    fi
    subcmd_user_set "$@"
    exit $?
    ;;
  help)
    echo "Usage: $0 [OPTION...] user list"
    echo "       $0 [OPTION...] user show USER"
    echo "       $0 [OPTION...] user add USER {general | admin} {enable | disable}"
    echo "       $0 [OPTION...] user delete USER"
    echo "       $0 [OPTION...] user set status USER VALUE"
    echo "       $0 [OPTION...] user set role_type USER VALUE"
    print_options
    exit 0
    ;;
  *)
    echo "invalid argument to sub-command '$SUBCMD1 $SUBCMD2'" >&2
    echo "$TRY_HELP_MSG" >&2
    exit 1
    ;;
  esac
}

#
# sub-command 'job set'.
#
subcmd_job_set() {
  if [ $# -eq 0 ]; then
    echo "missing field name to '$SUBCMD1 $SUBCMD2'" >&2
    exit 1
  elif [ $# -eq 1 ]; then
    echo "missing field value to '$SUBCMD1 $SUBCMD2'" >&2
    exit 1
  elif [ $# -eq 2 ]; then
    echo "missing job ID to '$SUBCMD1 $SUBCMD2'" >&2
    exit 1
  elif [ $# -gt 3 ]; then
    echo "too many arguments to sub-command '$SUBCMD1 $SUBCMD2'" >&2
    exit 1
  fi

  FIELD_NAME=$1
  JOB=$2
  FIELD_VALUE=$3
  shift 3

  case "$FIELD_NAME" in
  status)
    set_job_status "$JOB" "$(parse_job_status "$FIELD_VALUE")"
    exit $?
    ;;
  created_time|ctime)
    set_job_created_time "$JOB" "$FIELD_VALUE"
    exit $?
    ;;
  user_id)
    set_job_user_id "$JOB" "$FIELD_VALUE"
    exit $?
    ;;
  *)
    echo "invalid argument to sub-command '$SUBCMD1 $SUBCMD2'" >&2
    exit 1
    ;;
  esac
}

#
# sub-command 'job'.
#
subcmd_job() {
  TRY_HELP_MSG="try '$0 $SUBCMD1 help'"
  if [ $# -eq 0 ]; then
    show_all_jobs
    exit $?
  fi

  SUBCMD2=$1
  shift

  case "$SUBCMD2" in
  list)
    if [ $# -gt 0 ]; then
      echo "too many arguments to sub-command '$SUBCMD1 $SUBCMD2'" >&2
      echo "$TRY_HELP_MSG" >&2
      exit 1
    fi
    show_all_jobs
    exit $?
    ;;
  show)
    if [ $# -lt 1 ]; then
      echo "missing JOB-ID to sub-command '$SUBCMD1 $SUBCMD2'" >&2
      echo "$TRY_HELP_MSG" >&2
      exit 1
    elif [ $# -gt 1 ]; then
      echo "too many arguments to sub-command '$SUBCMD1 $SUBCMD2'" >&2
      exit 1
    fi
    show_job "$1"
    exit $?
    ;;
  delete|del|remove|rm)
    if [ $# -lt 1 ]; then
      echo "missing JOB-ID to sub-command '$SUBCMD1 $SUBCMD2'" >&2
      echo "$TRY_HELP_MSG" >&2
      exit 1
    elif [ $# -gt 1 ]; then
      echo "too many arguments to sub-command '$SUBCMD1 $SUBCMD2'" >&2
      echo "$TRY_HELP_MSG" >&2
      exit 1
    fi
    delete_job "$1"
    exit $?
    ;;
  set)
    if [ $# -eq 0 ]; then
      echo "missing field name to sub-command '$SUBCMD1 $SUBCMD2'" >&2
      echo "$TRY_HELP_MSG" >&2
      exit 1
    fi
    subcmd_job_set "$@"
    exit $?
    ;;
  help)
    echo "Usage: $0 [OPTION...] job [list]"
    echo "       $0 [OPTION...] job show JOB"
    echo "       $0 [OPTION...] job delete JOB"
    echo "       $0 [OPTION...] job set status JOB VALUE"
    echo "       $0 [OPTION...] job set created_time JOB TIME"
    echo "       $0 [OPTION...] job set user_id JOB USER"
    print_options
    exit 0
    ;;
  *)
    echo "invalid argument to sub-command '$SUBCMD1'" >&2
    echo "$TRY_HELP_MSG" >&2
    exit 1
    ;;
  esac
}

#
# Dispatch a handler of sub-command.
#
dispatch_subcmd() {
  TRY_HELP_MSG="try '$0 help'"
  if [ $# -eq 0 ]; then
    echo "missing sub-command" >&2
    echo "$TRY_HELP_MSG" >&2
    exit 1
  fi

  SUBCMD1=$1
  shift

  case "$SUBCMD1" in
  user)
    subcmd_user "$@"
    ;;
  job)
    subcmd_job "$@"
    ;;
  help)
    echo "Usage: $0 [OPTION...] user ARG ..."
    echo "       $0 [OPTION...] job ARG ..."
    print_options
    exit 0
    ;;
  *)
    echo "invalid sub-command: $SUBCMD1"
    echo "$TRY_HELP_MSG" >&2
    exit 1
    ;;
  esac
}

#
# Parse options.
#
while [ $# -ge 1 ]; do
  TRY_HELP_MSG="try '$0 help'"
  case "$1" in
  --)
    shift
    break
    ;;
  -)
    break
    ;;
  -f)
    if [ $# -eq 1 ]; then
      echo "missing argument to option '-f'" >&2
      echo "$TRY_HELP_MSG" >&2
      exit 1
    fi
    DB_FILE="$2"
    shift 2
    ;;
  -f*)
    DB_FILE=$(echo "X$1" | sed -e 's|^X-f||')
    shift
    ;;
  -h)
      SQLITE_HEADER=header
      shift
      ;;
  -m)
    if [ $# -eq 1 ]; then
      echo "missing argument to option '-m'" >&2
      echo "$TRY_HELP_MSG" >&2
      exit 1
    fi
    SQLITE_MODE="$2"
    shift 2
    ;;
  -m*)
    SQLITE_MODE=$(echo "X$1" | sed -e 's|^X-m||')
    shift
    ;;
  -*)
    echo "invalid option '$1'" >&2
    echo "$TRY_HELP_MSG" >&2
    exit 1
    ;;
  *)
    break
    ;;
  esac
done

if [ "$SQLITE_MODE" = '' ]; then
    SQLITE_OPTIONS="-$SQLITE_HEADER"
else
    parse_sqlite_mode "$SQLITE_MODE"
    SQLITE_OPTIONS="-$SQLITE_HEADER -$SQLITE_MODE"
fi
dispatch_subcmd "$@"
