#! /bin/sh
#
# NAME
#   submit_too_large_qasm.sh - submits a job with too large QASM
#
# SYNOPSIS
#   submit_too_large_qasm.sh
#
# DESCRIPTION
#   The script tries submitting a job with too large QASM.
#   It will output the following messages to standard error.
#
#     failed to submit a job
#     an error occurred
#
#   and the message below will be recorded in 'submit_err.log':
#
#     the qasm file is too large: test.qasm
#

. ./common.sh

SUBMIT_QASM_FILE="too_long.qasm"
SUBMIT_QASM_SIZE=$(expr 64 '*' 1024 + 1)
trap "rm -f $SUBMIT_QASM_FILE; exit 1" 1 2 3 15
remove_file $SUBMIT_QASM_FILE || exit 1

head -c $SUBMIT_QASM_SIZE /dev/zero | sed -e 's/./Q/g' > $SUBMIT_QASM_FILE || {
  echo "failed to create the file: $SUBMIT_QASM_FILE" >&2
  rm -f $SUBMIT_QASM_FILE
  exit 1
}

do_submit
RET=$?
rm -f $SUBMIT_QASM_FILE
exit $RET
