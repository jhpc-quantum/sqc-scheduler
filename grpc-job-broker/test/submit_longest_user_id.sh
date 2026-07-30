#! /bin/sh
#
# NAME
#   submit_longest_user_id.sh - submits a job with longest user ID
#
# SYNOPSIS
#   submit_longest_user_id.sh
#
# DESCRIPTION
#   The script tries submitting a job with longest user ID.
#   The submitted job is expected to be succeeded as long as the configuration of
#   the test is correct.
#

. ./common.sh

USER_ID_SIZE=256
SUBMIT_USER_ID=$(head -c $USER_ID_SIZE /dev/zero | sed -e 's/./U/g')

do_submit
