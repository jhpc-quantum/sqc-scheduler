#pragma once


///
/// @file        rpc_msg_util.h
///
/// @brief       Utility functions for RPC messages.
///

#include "rpc_auth_method.h"
#include "rpc_msg_id.h"
#include "rpc_job_info.h"


__BEGIN_DECLS

//
// Validate a NUL terminated string represented as 'bytes' data of protobuf-c.
//
#define RPC_VALIDATE_PROTOC_BYTES(bytes) \
  ((bytes)->data != NULL && (bytes)->len > 0 && (bytes)->data[(bytes)->len - 1] == '\0')


///
/// @brief   Convert an RPC message ID to a string.
///
/// @param[in]     id           An RPC message ID.
///
/// @return A string that descreibes an RPC message ID.
///
/// @details The function <tt>rpc_message_id_string()</tt> converts an RPC message ID
/// to a string.
/// Since it returns a pointer to a statically allocated string,
/// the caller doesn't free or modifies it.
///
/// If \c id is out of range, the function assumes \c RPC_AUTH_METHOD_UNKNOWN is
/// given.
///
const char *
rpc_message_id_string(rpc_msg_id_t id);

///
/// @brief   Return a pointer to a string that denotes an RPC message name of the given message ID.
///
/// @param[in]     id           An RPC message ID.
///
/// @return A string that descreibes an RPC message name.
///
/// @details The function <tt>rpc_message_name_string()</tt> returns a pointer to
/// a string an RPC message name of the given message ID.
/// Since it returns a pointer to a statically allocated string,
/// the caller doesn't free or modifies it.
///
/// If \c id is out of range, the function assumes \c RPC_AUTH_METHOD_UNKNOWN is
/// given.
///
const char *
rpc_message_name_string(rpc_msg_id_t id);

///
/// @brief   Create a text string for a reply message.
///
/// @param[out]    text         A formatted text.
/// @param[in]     format       A format string.
/// @param[in]     ...          Arguments depending on `format`.
///
/// @details The function <tt>rpc_create_message_text()</tt> is equivalent with `vasprintf(3)`,
/// but its return type is `void`.  When `vasprintf(3)` fails to allocates a string, `*text`
/// is set to NULL.
///
void
rpc_create_message_text(char **text, const char *format, ...)
  __attr_format_printf__(2, 3);

///
/// @brief   Output a log message of debug level if the message is not NULL.
///
/// @param[in]    level        Debug level.
/// @param[in]    msg          A log message.
///
#define rpc_log_debug_msg(level, msg) { \
  if (msg != NULL) { \
    sqc_msg_debug((level), "%s\n", (msg)); \
  } else { \
    sqc_msg_debug((level), "(failed to create a log message)\n"); \
  } \
}

///
/// @brief   Output a log message of info level if the message is not NULL.
///
/// @param[in]    msg          A log message.
///
#define rpc_log_info_msg(msg) { \
  if (msg != NULL) { \
    sqc_msg_info("%s\n", (msg)); \
  } else { \
    sqc_msg_info("(failed to create a log message)\n"); \
  } \
}

///
/// @brief   Output a log message of notice level if the message is not NULL.
///
/// @param[in]    msg          A log message.
///
#define rpc_log_notice_msg(msg) { \
  if (msg != NULL) { \
    sqc_msg_notice("%s\n", (msg)); \
  } else { \
    sqc_msg_notice("(failed to create a log message)\n"); \
  } \
}

///
/// @brief   Output a log message of warning level if the message is not NULL.
///
/// @param[in]    msg          A log message.
///
#define rpc_log_warning_msg(msg) { \
  if (msg != NULL) { \
    sqc_msg_warning("%s\n", (msg)); \
  } else { \
    sqc_msg_warning("(failed to create a log message)\n"); \
  } \
}

///
/// @brief   Output a log message of error level if the message is not NULL.
///
/// @param[in]    msg          A log message.
///
#define rpc_log_error_msg(msg) { \
  if (msg != NULL) { \
    sqc_msg_error("%s\n", (msg)); \
  } else { \
    sqc_msg_error("(failed to create a log message)\n"); \
  } \
}

///
/// @brief   Output a log message of fatal level if the message is not NULL.
///
/// @param[in]    msg          A log message.
///
#define rpc_log_fatal_msg(msg) { \
  if (msg != NULL) { \
    sqc_msg_fatal("%s\n", (msg)); \
  } else { \
    sqc_msg_fatal("(failed to create a log message)\n"); \
  } \
}

///
/// @brief   Output a log message of error level with an RPC-named if the message is not NULL.
///
/// @param[in]    level        Debug level.
/// @param[in]    id           An RPC message ID.
/// @param[in]    msg          A log message.
///
#define rpc_log_debug_msg_with_name(level, id, msg) { \
  if (msg != NULL) { \
    sqc_msg_debug((level), "RPC-%s: %s\n", rpc_message_name_string((id)), (msg)); \
  } else { \
    sqc_msg_debug((level), "RPC-%s: (failed to create a log message)\n", rpc_message_name_string((id))); \
  } \
}

///
/// @brief   Output a log message of error level with an RPC-named if the message is not NULL.
///
/// @param[in]    id           An RPC message ID.
/// @param[in]    msg          A log message.
///
#define rpc_log_info_msg_with_name(id, msg) { \
  if (msg != NULL) { \
    sqc_msg_info("RPC-%s: %s\n", rpc_message_name_string((id)), (msg)); \
  } else { \
    sqc_msg_info("RPC-%s: (failed to create a log message)\n", rpc_message_name_string((id))); \
  } \
}

///
/// @brief   Output a log message of error level with an RPC-named if the message is not NULL.
///
/// @param[in]    id           An RPC message ID.
/// @param[in]    msg          A log message.
///
#define rpc_log_notice_msg_with_name(id, msg) { \
  if (msg != NULL) { \
    sqc_msg_notice("RPC-%s: %s\n", rpc_message_name_string((id)), (msg)); \
  } else { \
    sqc_msg_notice("RPC-%s: (failed to create a log message)\n", rpc_message_name_string((id))); \
  } \
}

///
/// @brief   Output a log message of error level with an RPC-named if the message is not NULL.
///
/// @param[in]    id           An RPC message ID.
/// @param[in]    msg          A log message.
///
#define rpc_log_warning_msg_with_name(id, msg) { \
  if (msg != NULL) { \
    sqc_msg_warning("RPC-%s: %s\n", rpc_message_name_string((id)), (msg)); \
  } else { \
    sqc_msg_warning("RPC-%s: (failed to create a log message)\n", rpc_message_name_string((id))); \
  } \
}

///
/// @brief   Output a log message of error level with an RPC-named if the message is not NULL.
///
/// @param[in]    id           An RPC message ID.
/// @param[in]    msg          A log message.
///
#define rpc_log_error_msg_with_name(id, msg) { \
  if (msg != NULL) { \
    sqc_msg_error("RPC-%s: %s\n", rpc_message_name_string((id)), (msg)); \
  } else { \
    sqc_msg_error("RPC-%s: (failed to create a log message)\n", rpc_message_name_string((id))); \
  } \
}

///
/// @brief   Output a log message of error level with an RPC-named if the message is not NULL.
///
/// @param[in]    id           An RPC message ID.
/// @param[in]    msg          A log message.
///
#define rpc_log_fatal_msg_with_name(id, msg) { \
  if (msg != NULL) { \
    sqc_msg_fatal("RPC-%s: %s\n", rpc_message_name_string((id)), (msg)); \
  } else { \
    sqc_msg_fatal("RPC-%s: (failed to create a log message)\n", rpc_message_name_string((id))); \
  } \
}

///
/// @brief   Pack message body of OPEN_NEW_SESSION_REQUEST.
///
/// @param[in]     auth_method  Authentication method.
/// @param[out]    body         The message body.
/// @param[out]    len          Length of \c body.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_pack_open_new_session_request()</tt> builds message body of
/// an RPC message \c OPEN_NEW_SESSION_REQUEST.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c body and \c len.
/// The caller needs to free \c body.
///
sqc_result_t
rpc_pack_open_new_session_request(rpc_auth_method_t auth_method, char **body, size_t *len);

///
/// @brief   Pack message body of OPEN_NEW_SESSION_REPLY.
///
/// @param[in]     code         The result code of the request.
/// @param[in]     msg          The result message.
/// @param[out]    body         The message body.
/// @param[out]    len          Length of \c body.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_pack_open_new_session_reply()</tt> builds message body of
/// an RPC message \c OPEN_NEW_SESSION_REPLY.
/// \c msg must be a NUL-terminated text or NULL.
/// In case of NULL, it is treated as an empty string.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c body and \c len.
/// The caller needs to free \c body.
///
sqc_result_t
rpc_pack_open_new_session_reply(sqc_result_t code, const char *msg, char **body, size_t *len);

///
/// @brief   Unpack message body of OPEN_NEW_SESSION_REPLY.
///
/// @param[in]     body         The message body.
/// @param[in]     len          Length of \c body.
/// @param[out]    code         The result code of the request.
/// @param[out]    msg          The result message.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_unpack_open_new_session_reply</tt> extracts message body of
/// an RPC message \c OPEN_NEW_SESSION_REPLY.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c code, \c msg, \c status and \c job_id.
/// \c msg is a NUL-terminated string.
/// The caller needs to free it.
///
sqc_result_t
rpc_unpack_open_new_session_reply(char *body, size_t len, sqc_result_t *code, char **msg);

///
/// @brief   Pack message body of AUTH_REQUEST.
///
/// @param[in]     auth_data    authentication data.
/// @param[out]    body         The message body.
/// @param[out]    len          Length of \c body.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_pack_auth_request()</tt> builds message body of
/// an RPC message \c AUTH_REQUEST.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c body and \c len.
/// The caller needs to free \c body.
///
sqc_result_t
rpc_pack_auth_request(const uint8_t *data, size_t datalen, char **body, size_t *len);

///
/// @brief   Pack message body of AUTH_REPLY.
///
/// @param[in]     code         The result code of the request.
/// @param[in]     msg          The result message.
/// @param[out]    body         The message body.
/// @param[out]    len          Length of \c body.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_pack_auth_reply()</tt> builds message body of
/// an RPC message \c AUTH_REPLY.
/// \c msg must be a NUL-terminated text or NULL.
/// In case of NULL, it is treated as an empty string.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c body and \c len.
/// The caller needs to free \c body.
///
sqc_result_t
rpc_pack_auth_reply(sqc_result_t code, const char *msg, char **body, size_t *len);

///
/// @brief   Unpack message body of AUTH_REPLY.
///
/// @param[in]     body         The message body.
/// @param[in]     len          Length of \c body.
/// @param[out]    code         The result code of the request.
/// @param[out]    msg          The result message.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_unpack_auth_reply</tt> extracts message body of
/// an RPC message \c AUTH_REPLY.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c code, \c msg, \c status and \c job_id.
/// \c msg is a NUL-terminated string.
/// The caller needs to free it.
///
sqc_result_t
rpc_unpack_auth_reply(char *body, size_t len, sqc_result_t *code, char **msg);

///
/// @brief   Pack message body of SUBMIT_JOB_REQUEST.
///
/// @param[in]     proiority    Job priority.
/// @param[in]     qprogram     Program text.
/// @param[in]     circuit_fmt  Format of qprogram.
/// @param[in]     shots        The number of shots.
/// @param[in]     qc_type      QC type of the job.
/// @param[in]     transpiler   Transpiler of the job.
/// @param[in]     remark       Remark comment of the job.
/// @param[in]     user_token   User token of the job.
/// @param[out]    body         The message body.
/// @param[out]    len          Length of \c body.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_pack_submit_job_request</tt> builds message body of
/// an RPC message \c SUBMIT_JOB_REQUEST.
/// \c qprogram must be a NUL-terminated text.
/// In case of NULL, it is treated as an empty string.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c body and \c len.
/// The caller needs to free \c body.
///
sqc_result_t
rpc_pack_submit_job_request(uint8_t priority, const char *qprogram, sqc_rpc_sched_circuit_fmt_t circuit_fmt,
                            size_t shots, sqc_rpc_sched_qc_type_t qc_type,
                            sqc_rpc_sched_transpiler_t transpiler,
                            const char *remark, const char *user_token,
                            char **body, size_t *len);

///
/// @brief   Pack message body of SUBMIT_JOB_REPLY.
///
/// @param[in]     code         The result code of the request.
/// @param[in]     msg          The result message.
/// @param[in]     job_id       Job ID.
/// @param[out]    body         The message body.
/// @param[out]    len          Length of \c body.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_pack_submit_job_reply()</tt> builds message body of
/// an RPC message \c SUBMIT_JOB_REPLY.
/// \c job_id must be a NUL-terminated text.
/// \c msg must be a NUL-terminated text or NULL.
/// In case of NULL, it is treated as an empty string.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c body and \c len.
/// The caller needs to free \c body.
///
sqc_result_t
rpc_pack_submit_job_reply(sqc_result_t code, const char *msg, const char *job_id, char **body,
                          size_t *len);

///
/// @brief   Unpack message body of SUBMIT_JOB_REPLY.
///
/// @param[in]     body         The message body.
/// @param[in]     len          Length of \c body.
/// @param[out]    code         The result code of the request.
/// @param[out]    msg          The result message.
/// @param[out]    job_id       Job ID.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_unpack_submit_job_reply()</tt> extracts message body of
/// an RPC message \c SUBMIT_JOB_REPLY.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c code, \c msg and \c job_id.
/// \c msg and \c job_id are NUL-terminated strings.
/// The caller needs to free them.
///
sqc_result_t
rpc_unpack_submit_job_reply(char *body, size_t len, sqc_result_t *code, char **msg, char **job_id);

///
/// @brief   Pack message body of JOB_STATUS_REQUEST.
///
/// @param[in]     job_id       Job ID.
/// @param[out]    body         The message body.
/// @param[out]    len          Length of \c body.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_pack_job_status_request()</tt> builds message body of
/// an RPC message JOB_STATUS_REQUEST.
/// \c job_id must be a NUL-terminated text.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c body and \c len.
/// \c job_id is a NUL-terminated string.
/// The caller needs to free it.
///
sqc_result_t
rpc_pack_job_status_request(const char *job_id, char **body, size_t *len);

///
/// @brief   Pack message body of JOB_STATUS_REPLY.
///
/// @param[in]     code         The result code of the request.
/// @param[in]     msg          The result message.
/// @param[in]     status       Status of the job.
/// @param[in]     qc_job_id    QC Job ID.
/// @param[in]     result       Calclation result.
/// @param[out]    body         The message body.
/// @param[out]    len          Length of \c body.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_pack_job_status_reply()</tt> builds message body of
/// an RPC message \c JOB_STATUS_REPLY.
/// \c msg must be a NUL-terminated text or NULL.
/// In case of NULL, it is treated as an empty string.
/// Since \c result is an optional field, it may be NULLL but it must be a NUL-terminated
/// text if non-NULL value is set.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c body and \c len.
/// The caller needs to free \c body.
///
sqc_result_t
rpc_pack_job_status_reply(sqc_result_t code, const char *msg, sqc_rpc_sched_job_status_t status,
                          const char *qc_job_id, const char *result, char **body, size_t *len);

///
/// @brief   Unpack message body of JOB_STATUS_REPLY.
///
/// @param[in]     body         The message body.
/// @param[in]     len          Length of \c body.
/// @param[out]    code         The result code of the request.
/// @param[out]    msg          The result message.
/// @param[out]    status       Job ID.
/// @param[out]    qc_job_id    QC Job ID.
/// @param[out]    result       Calculation result.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_unpack_submit_job_reply()</tt> extracts message body of
/// an RPC message \c STATUS_JOB_REPLY.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c code, \c msg and \c status and
/// it may also set \c qc_job_id and \c result according with the current job status.
/// \c msg, \c qc_job_id and \c result are NUL-terminated strings.
/// The caller needs to free them.
///
sqc_result_t
rpc_unpack_job_status_reply(char *body, size_t len, sqc_result_t *code, char **msg,
                            sqc_rpc_sched_job_status_t *status, char **qc_job_id, char **result);

///
/// @brief   Pack message body of CANCEL_JOB_REQUEST.
///
/// @param[in]     job_id       Job ID.
/// @param[out]    body         The message body.
/// @param[out]    len          Length of \c body.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_pack_cancel_job_request()</tt> builds message body of
/// an RPC message \c CANCEL_JOB_REQUEST.
/// \c job_id must be a NUL-terminated text.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c body and \c len.
/// \c job_id is a NUL-terminated string.
/// The caller needs to free it.
///
sqc_result_t
rpc_pack_cancel_job_request(const char *job_id, char **body, size_t *len);

///
/// @brief   Pack message body of CANCEL_JOB_REPLY.
///
/// @param[in]     code         The result code of the request.
/// @param[in]     msg          The result message.
/// @param[out]    body         The message body.
/// @param[out]    len          Length of \c body.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_pack_cancel_job_reply()</tt> builds message body of
/// an RPC message \c CANCEL_JOB.
/// \c msg must be a NUL-terminated text or NULL.
/// In case of NULL, it is treated as an empty string.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c body and \c len.
/// The caller needs to free \c body.
///
sqc_result_t
rpc_pack_cancel_job_reply(sqc_result_t code, const char *msg, char **body, size_t *len);

///
/// @brief   Unpack message body of CANCEL_JOB_REPLY.
///
/// @param[in]     body         The message body.
/// @param[in]     len          Length of \c body.
/// @param[out]    code         The result code of the request.
/// @param[out]    msg          The result message.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_unpack_cancel_job_reply()</tt> extracts message body of
/// an RPC message \c CANCEL_JOB_REPLY.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c code, \c msg and \c job_id.
/// \c msg and \c job_id are NUL-terminated strings.
/// The caller needs to free them.
///
sqc_result_t
rpc_unpack_cancel_job_reply(char *body, size_t len, sqc_result_t *code, char **msg);

///
/// @brief   Pack message body of DELETE_JOB_REQUEST.
///
/// @param[in]     job_id       Job ID.
/// @param[out]    body         The message body.
/// @param[out]    len          Length of \c body.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_pack_delete_job_request()</tt> builds message body of
/// an RPC message \c DELETE_JOB_REQUEST.
/// \c job_id must be a NUL-terminated text.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c body and \c len.
/// \c job_id is a NUL-terminated string.
/// The caller needs to free it.
///
sqc_result_t
rpc_pack_delete_job_request(const char *job_id, char **body, size_t *len);

///
/// @brief   Pack message body of DELETE_JOB_REPLY.
///
/// @param[in]     code         The result code of the request.
/// @param[in]     msg          The result message.
/// @param[out]    body         The message body.
/// @param[out]    len          Length of \c body.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_pack_delete_job_reply()</tt> builds message body of
/// an RPC message \c DELETE_JOB_REPLY.
/// \c msg must be a NUL-terminated text or NULL.
/// In case of NULL, it is treated as an empty string.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c body and \c len.
/// The caller needs to free \c body.
///
sqc_result_t
rpc_pack_delete_job_reply(sqc_result_t code, const char *msg, char **body, size_t *len);

///
/// @brief   Unpack message body of DELETE_JOB_REPLY.
///
/// @param[in]     body         The message body.
/// @param[in]     len          Length of \c body.
/// @param[out]    code         The result code of the request.
/// @param[out]    msg          The result message.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_unpack_delete_job_reply()</tt> extracts message body of
/// an RPC message \c DELETE_JOB_REPLY.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c code, \c msg and \c job_id.
/// \c msg and \c job_id are NUL-terminated strings.
/// The caller needs to free them.
///
sqc_result_t
rpc_unpack_delete_job_reply(char *body, size_t len, sqc_result_t *code, char **msg);

///
/// @brief   Pack message body of JOB_LIST_REPLY.
///
/// @param[in]     code         The result code of the request.
/// @param[in]     msg          The result message.
/// @param[in]     jobs         An array of jobs submitted by the current user.
//  @param[in]     n_jobs       Length of \c jobs.
/// @param[out]    body         The message body.
/// @param[out]    len          Length of \c body.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_pack_job_list_reply()</tt> builds message body of
/// an RPC message \c JOB_LIST_REPLY.
/// \c msg must be a NUL-terminated text or NULL.
/// In case of NULL, it is treated as an empty string.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c body and \c len.
/// The caller needs to free \c body.
///
sqc_result_t
rpc_pack_job_list_reply(sqc_result_t code, const char *msg,
                        const dbmgr_job_info_t *jobs, size_t n_jobs,
                        char **body, size_t *len);

///
/// @brief   Unpack message body of JOB_LIST_REPLY.
///
/// @param[in]     body         The message body.
/// @param[in]     len          Length of \c body.
/// @param[out]    code         The result code of the request.
/// @param[out]    msg          The result message.
/// @param[out]    jobs         An array of jobs submitted by the current user.
/// @param[out]    n_jobs       Length of the \c jobs.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_unpack_job_list_reply()</tt> extracts message body of
/// an RPC message \c JOB_LIST_REPLY.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c code, \c msg, \c jobs and \c n_jobs.
/// \c msg is a NUL-terminated strings.
/// The caller needs to free them.
/// \c jobs is an array of status of the jobs submitted by the user.
/// The caller needs to free it by calling <tt>rpc_job_info_destroy_array()</tt>.
///
sqc_result_t
rpc_unpack_job_list_reply(char *body, size_t len, sqc_result_t *code, char **msg,
                          rpc_job_info_t ***jobs, size_t *n_jobs);

///
/// @brief   Pack message body of ADM_DEL_JOBS_REQUEST.
///
/// @param[in]     user_id      User ID.
/// @param[in]     from_time    Start date of deletaion target.
/// @param[in]     to_time      End date of deletaion target.
/// @param[out]    body         The message body.
/// @param[out]    len          Length of \c body.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_pack_adm_del_jobs_request()</tt> builds message body of
/// an RPC message \c ADM_DEL_JOBS_REQUEST.
/// \c job_id must be a NUL-terminated text.
///
/// \c from_time and \c to_time are the number of nanoseconds since 1970-01-01 00:00:00.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c body and \c len.
/// \c job_id is a NUL-terminated string.
/// The caller needs to free it.
///
sqc_result_t
rpc_pack_adm_del_jobs_request(const char *user_id, sqc_chrono_t from_time, sqc_chrono_t to_time,
                              char **body, size_t *len);

///
/// @brief   Pack message body of ADM_DEL_JOBS_REPLY.
///
/// @param[in]     code         The result code of the request.
/// @param[in]     msg          The result message.
/// @param[out]    body         The message body.
/// @param[out]    len          Length of \c body.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_pack_adm_del_jobs_reply()</tt> builds message body of
/// an RPC message \c ADM_DEL_JOBS_REPLY.
/// \c msg must be a NUL-terminated text or NULL.
/// In case of NULL, it is treated as an empty string.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c body and \c len.
/// The caller needs to free \c body.
///
sqc_result_t
rpc_pack_adm_del_jobs_reply(sqc_result_t code, const char *msg, char **body, size_t *len);

///
/// @brief   Unpack message body of ADM_DEL_JOBS_REPLY.
///
/// @param[in]     body         The message body.
/// @param[in]     len          Length of \c body.
/// @param[out]    code         The result code of the request.
/// @param[out]    msg          The result message.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_unpack_delete_job_reply()</tt> extracts message body of
/// an RPC message \c ADM_DEL_JOBS_REPLY.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c code, \c msg and \c job_id.
/// \c msg and \c job_id are NUL-terminated strings.
/// The caller needs to free them.
///
sqc_result_t
rpc_unpack_adm_del_jobs_reply(char *body, size_t len, sqc_result_t *code, char **msg);

///
/// @brief   Pack message body of ADM_ADD_USER_REQUEST.
///
/// @param[in]     user_id      User ID.
/// @param[out]    body         The message body.
/// @param[out]    len          Length of \c body.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_pack_adm_add_user_request()</tt> builds message body of
/// an RPC message \c ADM_ADD_USER_REQUEST.
/// \c job_id must be a NUL-terminated text.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c body and \c len.
/// \c job_id is a NUL-terminated string.
/// The caller needs to free it.
///
sqc_result_t
rpc_pack_adm_add_user_request(const char *user_id, char **body, size_t *len);

///
/// @brief   Pack message body of ADM_ADD_USER_REPLY.
///
/// @param[in]     code         The result code of the request.
/// @param[in]     msg          The result message.
/// @param[out]    body         The message body.
/// @param[out]    len          Length of \c body.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_pack_adm_add_user_reply()</tt> builds message body of
/// an RPC message \c ADM_ADD_USER_REPLY.
/// \c msg must be a NUL-terminated text or NULL.
/// In case of NULL, it is treated as an empty string.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c body and \c len.
/// The caller needs to free \c body.
///
sqc_result_t
rpc_pack_adm_add_user_reply(sqc_result_t code, const char *msg, char **body, size_t *len);

///
/// @brief   Unpack message body of ADM_ADD_USER_REPLY.
///
/// @param[in]     body         The message body.
/// @param[in]     len          Length of \c body.
/// @param[out]    code         The result code of the request.
/// @param[out]    msg          The result message.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_unpack_add_user_reply()</tt> extracts message body of
/// an RPC message \c ADM_ADD_USER_REPLY.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c code, \c msg and \c job_id.
/// \c msg and \c job_id are NUL-terminated strings.
/// The caller needs to free them.
///
sqc_result_t
rpc_unpack_adm_add_user_reply(char *body, size_t len, sqc_result_t *code, char **msg);

///
/// @brief   Pack message body of ADM_SET_USER_STATUS_REQUEST.
///
/// @param[in]     user_id      User ID.
/// @param[in]     enabled      Validity of \c user_id.
/// @param[out]    body         The message body.
/// @param[out]    len          Length of \c body.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_pack_adm_set_user_status_request()</tt> builds message body of
/// an RPC message \c ADM_SET_USER_STATUS_REQUEST.
/// \c job_id must be a NUL-terminated text.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c body and \c len.
/// \c job_id is a NUL-terminated string.
/// The caller needs to free it.
///
sqc_result_t
rpc_pack_adm_set_user_status_request(const char *user_id, bool enabled, char **body, size_t *len);

///
/// @brief   Pack message body of ADM_SET_USER_STATUS_REPLY.
///
/// @param[in]     code         The result code of the request.
/// @param[in]     msg          The result message.
/// @param[out]    body         The message body.
/// @param[out]    len          Length of \c body.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_pack_adm_set_user_status_reply()</tt> builds message body of
/// an RPC message \c ADM_SET_USER_STATUS_REPLY.
/// \c msg must be a NUL-terminated text or NULL.
/// In case of NULL, it is treated as an empty string.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c body and \c len.
/// The caller needs to free \c body.
///
sqc_result_t
rpc_pack_adm_set_user_status_reply(sqc_result_t code, const char *msg, char **body, size_t *len);

///
/// @brief   Unpack message body of ADM_SET_USER_STATUS_REPLY.
///
/// @param[in]     body         The message body.
/// @param[in]     len          Length of \c body.
/// @param[out]    code         The result code of the request.
/// @param[out]    msg          The result message.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_unpack_adm_set_user_status_reply()</tt> extracts message body of
/// an RPC message \c ADM_SET_USER_STATUS_REPLY.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c code, \c msg and \c job_id.
/// \c msg and \c job_id are NUL-terminated strings.
/// The caller needs to free them.
///
sqc_result_t
rpc_unpack_adm_set_user_status_reply(char *body, size_t len, sqc_result_t *code, char **msg);


__END_DECLS
