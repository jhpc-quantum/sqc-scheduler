#! /bin/sh
#
# NAME
#   submit_and_status.sh - submits a job and retrieve status of the job
#
# SYNOPSIS
#   submit_and_status.sh  [DURATION]
#
# DESCRIPTION
#   The script submits a job and wait for DURATION seconds and then retrieves status
#   of the job.  The exit code is 0 if the job is submitted and retrieves status
#   the job successfully.  Otherwise, the exit code is 1.
#
#   The submitted job is expected to be submitted correctly as long as the configuration
#   of the test is correct.  Upon success, the job status will be `created`, `queued`,
#   `running` or `done`.
#

. ./common.sh

#
# Parses arguments.
#
DURATION=0
[ $# -ge 1 ] && DURATION="$1"
if [ $# -ge 2 ]; then
  echo "Usage: $0 [DURATION]" >&2
  exit 1
fi

#
# Submits a job 0 and repeats retrieving status of the job.
#
do_submit_and_status $DURATION
