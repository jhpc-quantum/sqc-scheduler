#! /bin/sh
#
# NAME
#   submit_longest_remark.sh - submits a job with longest remark
#
# SYNOPSIS
#   submit_longest_remark.sh
#
# DESCRIPTION
#   The script tries submitting a job with longest remark.
#   The submitted job is expected to be succeeded as long as the configuration of
#   the test is correct.
#

. ./common.sh

REMARK_SIZE=1024
SUBMIT_REMARK=$(head -c $REMARK_SIZE /dev/zero | sed -e 's/./R/g')

do_submit
