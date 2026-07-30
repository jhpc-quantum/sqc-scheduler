#! /bin/sh
#
# NAME
#   submit_too_long_remark.sh - submits a job with too long remark
#
# SYNOPSIS
#   submit_too_long_remark.sh
#
# DESCRIPTION
#   The script tries submitting a job with too long remark.
#   It will output the following messages to standard error.
#
#     failed to submit a job
#     an error occurred
#
#   and the message below will be recorded in 'submit_err.log':
#
#     remark too long
#

. ./common.sh

REMARK_SIZE=$(expr 1024 + 1)
SUBMIT_REMARK=$(head -c $REMARK_SIZE /dev/zero | sed -e 's/./R/g')

do_submit
