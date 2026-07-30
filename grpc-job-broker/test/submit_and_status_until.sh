#! /bin/sh
#
# NAME
#   submit_and_status_until.sh - submits a job and retrieves status repeatedly
#
# SYNOPSIS
#   submit_and_status_until.sh  [ DURATION [ INTERVAL [MAX-TIMES] ] ]
#
# DESCRIPTION
#   The script submits a job, waits for DURATION seconds and then retrieve status of
#   the job.  It exits if the job has been done (i.e. the job status becomes 'done').
#   Otherwise, it waits for INTERVAL seconds and then retrieves status of the job again.
#   It repeats the process until at least one of the following situations is met:
#
#     * The job has been done.
#     * An error occurs.
#     * It tries retrieving job status MAX-TIMES.
#
#   The exit code is 0 in case that the job is complete, 1 in case of an error and
#   2 in case that the number of repetitions reaches MAX-TIMES.
#   If MAX-TIMES is a negative number, it repeats indefinitely getting job status.
#
#   If one or more arguments to this script are omitted, it assumes DURATION is 0,
#   INTERVAL is 2 and MAX-NTIMES is -1, respectively.
#   The submitted job is expected to be succeeded as long as the configuration of
#   the test is correct.
#

. ./common.sh

DURATION=0
INTERVAL=2
MAX_NTIMES=-1

#
# Parses arguments.
#
[ $# -ge 1 ] && DURATION="$1"
[ $# -ge 2 ] && INTERVAL="$2"
[ $# -ge 3 ] && MAX_NTIMES="$3"
if [ $# -ge 4 ]; then
  echo "Usage: $0 [ DURATION [ INTERVAL [ MAX-NTIMES ]]]" >&2
  exit 1
fi

#
# Submits a job and repeats retrieving status of the job.
#
do_submit_and_status_until $DURATION $INTERVAL $MAX_NTIMES
[ "$STATUS_JOB_STATUS" = '4' ] && echo "status_computation_result=$STATUS_COMPUTATION_RESULT"
