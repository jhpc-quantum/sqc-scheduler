#! /bin/sh
#
# NAME
#   submit_largest_qasm.sh - submits a job with QASM of largest size
#
# SYNOPSIS
#   submit_largest_qasm.sh
#
# DESCRIPTION
#   The script tries submitting a job with QASM of largest size.
#   The submitted job is expected to be succeeded as long as the configuration of
#   the test is correct.
#

. ./common.sh

SUBMIT_QASM_FILE="largest.qasm"
SUBMIT_QASM_SIZE=$(expr 64 '*' 1024)
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
