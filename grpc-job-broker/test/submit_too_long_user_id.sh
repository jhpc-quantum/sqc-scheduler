#! /bin/sh
#
# NAME
#   submit_too_long_user_id.sh - submits a job with too long user ID
#
# SYNOPSIS
#   submit_too_long_user_id.sh
#
# DESCRIPTION
#   The script tries submitting a job with too long user ID.
#   It will output the following messages to standard error.
#
#     failed to submit a job
#     an error occurred
#
#   and the message below will be recorded in 'submit_err.log':
#
#     user_id too long
#

. ./common.sh

USER_ID_SIZE=$(expr 256 + 1)
SUBMIT_USER_ID=$(head -c $USER_ID_SIZE /dev/zero | sed -e 's/./U/g')

do_submit
