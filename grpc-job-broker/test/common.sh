#
# This file is intended to be included from other files.
#

unset http_proxy https_proxy

#
# Core settings.
#
# Path to 'job_broker_client'.
JOB_BROKER_CLIENT=${JOB_BROKER_CLIENT:-"/usr/local/job_broker/bin/job_broker_client"}

# Path to 'sqc_rpc_sched'.
SQC_RPC_SCHED=${SQC_RPC_SCHED:-"app/sqc_rpc_sched/.libs/sqc_rpc_sched"}

# An address or URL that the scheduler listens on.
SQC_RPC_SCHED_ADDRESS=${JOB_BROKER_ADDRESS:-"localhost:10080"}

############################################################################
# Utility functions.

#
# Removes a file.
#
remove_file() {
  local FILE="$1"
  rm -f $FILE || {
    echo "failed to remove the file: $FILE" >&2
    return 1
  }
  return 0
}

############################################################################
# Submits a job.

#
# 1. Input parameters.
#
# User ID to submit a job.
SUBMIT_USER_ID=${SUBMIT_USER_ID:-"test_user"}

# Priority of a job.
SUBMIT_PRIORITY=${SUBMIT_PRIORITY:-"1"}

# Size of QASM file (in bytes).
SUBMIT_QASM_SIZE=${SUBMIT_QASM_SIZE:-"100"}

# The number of shots.
SUBMIT_SHOTS=${SUBMIT_SHOTS:-"1000"}

# QC type ("rqc" or blank).
SUBMIT_QC_TYPE=${SUBMIT_QC_TYPE:-""}

# Transpiler ("none", "pass", "normal" or blank).
SUBMIT_TRANSPILER=${SUBMIT_TRANSPILER:-""}

# Remark
SUBMIT_REMARK=${SUBMIT_REMARK:-"remark_text"}

#
# 2. Other variables used by functions.
#
# Result code of 'job_broker_client submit ...'.
SUBMIT_RESULT_CODE=

# Issued job ID by execution of 'job_broker_client submit ...'.
SUBMIT_ISSUED_JOB_ID=

# Path to a dummy QASM file generated automatically.
SUBMIT_QASM_FILE=common.qasm

# Path to a log file that records stdout output by 'job_broker_client submit ...'.
SUBMIT_OUT_LOG=submit_out.log

# Path to a log file that records stderr output by 'job_broker_client submit ...'.
SUBMIT_ERR_LOG=submit_err.log

#
# Executes 'job_broker_client' command to submit a job.
#
_request_submit() {
  remove_file $SUBMIT_OUT_LOG || return 1
  remove_file $SUBMIT_ERR_LOG || return 1

  local QC_TYPE_OPTION=
  local TRANSPILER_OPTION=
  [ "$SUBMIT_QC_TYPE" != "" ] && QC_TYPE_OPTION="--qc-type=$SUBMIT_QC_TYPE"
  [ "$SUBMIT_TRANSPILER" != "" ] && TRANSPILER_OPTION="--transpiler=$SUBMIT_TRANSPILER"

  $JOB_BROKER_CLIENT submit --server="$SQC_RPC_SCHED_ADDRESS" --remark="$SUBMIT_REMARK" \
                     $QC_TYPE_OPTION $TRANSPILER_OPTION $SUBMIT_USER_ID $SUBMIT_PRIORITY \
                     "$SUBMIT_QASM_FILE" $SUBMIT_SHOTS > $SUBMIT_OUT_LOG 2> $SUBMIT_ERR_LOG
  local SUBMIT_EXIT_CODE=$?
  if [ $SUBMIT_EXIT_CODE != 0 ]; then
    echo "failed to submit a job" >&2
  fi

  return $SUBMIT_EXIT_CODE
}


#
# Gets a result code and a job ID from a log file.
#
_get_submit_result() {
  SUBMIT_RESULT_CODE=
  SUBMIT_ISSUED_JOB_ID=

  if [ -f $SUBMIT_OUT_LOG ]; then
    SUBMIT_RESULT_CODE=$(cat $SUBMIT_OUT_LOG | sed -nE -e 's|^reply:.*result_code=([^,]*).*$|\1|p')
    SUBMIT_ISSUED_JOB_ID=$(cat $SUBMIT_OUT_LOG | sed -nE -e 's|^reply:.*job_id=([^,]*).*$|\1|p')
    if [ "$SUBMIT_RESULT_CODE" = '' ]; then
      echo "result_code is not zero" >&2
      return 1
    fi
    if [ "$SUBMIT_ISSUED_JOB_ID" = '' ]; then
      echo "no job ID is found" >&2
      return 1
    fi
  else
    echo "no output from 'job_broker_client submit'" >&2
    return 1
  fi

  return 0
}

#
# Reports a error occurred while submitting a job.
#
_report_submit_error() {
  echo "an error occurred" >&2
  echo "see '$SUBMIT_OUT_LOG' and '$SUBMIT_ERR_LOG' for details" >&2
}

#
# Submit a job using 'job_broker_client submit ...'.
# Upon success, an job ID issued by `sqc_rpc_sched` is set to the variable $SUBMIT_ISSUED_JOB_ID.
#
do_submit() {
  _request_submit && _get_submit_result
  local RET=$?

  if [ $RET -eq 0 ]; then
    echo "job submitted: job_id=$SUBMIT_ISSUED_JOB_ID" >&2
  else
    _report_submit_error
  fi

  return $RET
}

############################################################################
# Retrieves job status.

#
# 1. Input parameters.
#
STATUS_JOB_ID=${STATUS_JOB_ID:-""}

#
# 2. Other variables used by functions.
#
#
# Result code of 'job_broker_client status ...'.
STATUS_RESULT_CODE=

# Current job status code.
STATUS_JOB_STAUTS=

# Current job status message.
STATUS_JOB_STAUTS_MESSAGE=

# Computation result.
COMPUTATION_RESULT=

# Path to a log file that records stdout output by 'job_broker_client status ...'.
STATUS_OUT_LOG=status_out.log

# Path to a log file that records stderr output by 'job_broker_client status ...'.
STATUS_ERR_LOG=status_err.log

#
# Executes 'job_broker_client' command to retrieve job status.
#
_request_status() {
  remove_file $STATUS_OUT_LOG || return 1
  remove_file $STATUS_ERR_LOG || return 1

  $JOB_BROKER_CLIENT status --server="$SQC_RPC_SCHED_ADDRESS" "$STATUS_JOB_ID" \
                     > $STATUS_OUT_LOG 2> $STATUS_ERR_LOG
  local STATUS_EXIT_CODE=$?
  [ $STATUS_EXIT_CODE -ne 0 ] && echo "failed to retrieve job status: job_id=$STATUS_JOB_ID" >&2

  return $STATUS_EXIT_CODE
}

#
# Gets a result code, job status, and computation results from a log file.
#
_get_status_result() {
  STATUS_RESULT_CODE=
  STATUS_JOB_STAUTS=
  STATUS_JOB_STAUTS_MESSAGE=

  if [ -f $STATUS_OUT_LOG ]; then
    STATUS_RESULT_CODE=$(cat $STATUS_OUT_LOG | sed -nE -e 's|^reply:.*result_code=([^,]*).*$|\1|p')
    STATUS_JOB_STATUS=$(cat $STATUS_OUT_LOG | sed -nE -e 's|^reply:.*job_status=([^,]*).*$|\1|p')
    STATUS_JOB_STATUS_MESSAGE=$(cat $STATUS_OUT_LOG | sed -nE -e 's|^reply:.*job_status_message=([^,]*).*$|\1|p')
    STATUS_COMPUTATION_RESULT=$(cat $STATUS_OUT_LOG | sed -nE -e 's|^reply:.*computation_result=||p')
    if [ "$STATUS_RESULT_CODE" != 0 ]; then
      echo "result_code is not zero: $STATUS_RESULT_CODE" >&2
      return 1
    elif [ "$STATUS_JOB_STATUS" = '' ]; then
      echo "no job_status is found" >&2
      return 1
    elif [ "$STATUS_JOB_STATUS_MESSAGE" = '' ]; then
      echo "no job_status_message is found" >&2
      return 1
    fi
    [ "$STATUS_JOB_STATUS" = '1' ] && return 2   # created (1)
    [ "$STATUS_JOB_STATUS" = '2' ] && return 2   # queued (2)
    [ "$STATUS_JOB_STATUS" = '3' ] && return 2   # running (3)
    [ "$STATUS_JOB_STATUS" = '4' ] && return 0   # done (4)
    echo "unexpected job status job_id=$STATUS_JOB_ID: $STATUS_JOB_STATUS ($STATUS_JOB_STATUS_MESSAGE)" >&2
    return 1
  else
    echo "no output from 'job_broker_client status'" >&2
    return 1
  fi
}

#
# Reports a error occurred while getting the job status.
#
_report_status_error() {
  echo "an error occurred: job_id=$STATUS_JOB_ID" >&2
  echo "see '$STATUS_OUT_LOG' and '$STATUS_ERR_LOG' for details" >&2
}

#
# Retrieves job status using 'job_broker_client status ...'.
#
do_status() {
  _request_status && _get_status_result
  local RET=$?

  if [ $RET -eq 1 ]; then
    _report_status_error
  else
    echo "retrieved job status job_id=$STATUS_JOB_ID: $STATUS_JOB_STATUS ($STATUS_JOB_STATUS_MESSAGE)" >&2
    [ $RET -eq 0 ] && echo "done: job_id=$STATUS_JOB_ID" >&2
  fi
  return $RET
}

############################################################################
# Complex processing.

#
# Submits a job and retrieves status of the job.
# See 'submit_and_status.sh' for defails about the function.
#
do_submit_and_status() {
  local DURATION="$1"

  do_submit
  local DO_SUBMIT_RET=$?
  if [ $DO_SUBMIT_RET -ne 0 ]; then
    return 1
  fi

  STATUS_JOB_ID=$SUBMIT_ISSUED_JOB_ID

  [ $DURATION -ne 0 ] && sleep "$1"
  do_status
  local DO_STAUTS_RET=$?
  if [ $DO_STAUTS_RET -eq 1 ]; then
    return 1
  fi
  return 0
}

#
# Submits a job and retrieves status of the job until the job is complete.
# See 'submit_and_status_until.sh' for defails about the function.
#
do_submit_and_status_until() {
  local DURATION="$1"
  local INTERVAL="$2"
  local MAX_TIMES="$3"

  do_submit
  local DO_SUBMIT_RET=$?
  if [ $DO_SUBMIT_RET -ne 0 ]; then
    return 1
  fi

  STATUS_JOB_ID=$SUBMIT_ISSUED_JOB_ID

  [ $DURATION -ne 0 ] && sleep "$1"
  local I=0
  while [ $I -ne $MAX_TIMES ]; do
    [ $I -gt 0 ] && sleep $INTERVAL
    do_status
    local DO_STAUTS_RET=$?
    if [ $DO_STAUTS_RET -eq 0 ]; then
      return 0
    elif [ $DO_STAUTS_RET -eq 1 ]; then
      return 1
    fi
    I=$(expr $I + 1)
  done

  echo "give up waiting for job completion" >&2
  return 2
}
