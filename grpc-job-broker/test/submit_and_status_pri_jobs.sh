#! /bin/sh
#
# NAME
#   submit_and_status_pri_jobs.sh - submits prioritized jobs and retrieve status
#
# SYNOPSIS
#   submit_and_status_pri_jobs.sh  [ DURATION [ INTERVAL [MAX-TIMES] ] ]
#
# DESCRIPTION
#   The script submits ten jobs successively in the following order:
#
#     a job with priority 9
#     a job with priority 8
#     a job with priority 7
#       : snip
#     a job with priority 0
#
#   Then, it repeats retrieving status of the job with priority 0 (the last job),
#   as 'submit_and_status_until.sh' does.
#   See the description of 'submit_and_status_until.sh' for details.
#   Note that other submitted jobs (jobs with priority 1..9) are ignored.
#
#   If one or more arguments to this script are omitted, it assumes DURATION is 0,
#   INTERVAL is 0.5 and MAX-NTIMES is -1, respectively.
#   The script expects 'sleep' command accepts a decimal.
#
#   The submitted job with priority 0 is expected to be succeeded as long as
#   the configuration of the test is correct.
#

. ./common.sh

DURATION=0
INTERVAL=0.5
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
# Submits jobs with priority 9, 8, 7, ... 1.
#
SUBMIT_PRIORITY=9
while [ $SUBMIT_PRIORITY -ge 1 ]; do
  do_submit || exit $?
  SUBMIT_PRIORITY=$(expr $SUBMIT_PRIORITY - 1)
done

#
# Submits a job with priority 0 and repeats retrieving status of the job.
#
SUBMIT_PRIORITY=0
do_submit_and_status_until $DURATION $INTERVAL $MAX_NTIMES
[ "$STATUS_JOB_STATUS" = '4' ] && echo "status_computation_result=$STATUS_COMPUTATION_RESULT"
