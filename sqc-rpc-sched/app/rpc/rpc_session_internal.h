#pragma once

#include "rpc_jwt_client.h"
#include "rpc_jwt_server.h"
#include "rpc_session_client.h"
#include "rpc_session_server.h"


/**
 * @file    rpc_session_internal.h
 */


__BEGIN_DECLS


/**
 * @brief    RPC session for client.
 */
struct rpc_session_client {
  sqc_session_t session_;             ///< Underlying session object
  struct tls_conf_struct *tls_conf_;  ///< TLS configuration
  rpc_jwt_client_ctx_t jwt_ctx_;      ///< JWT context
};


/**
 * @brief    RPC session for server.
 */
struct rpc_session_server {
  sqc_session_t session_;             ///< Underlying session object
  rpc_auth_method_t auth_method_;     ///< Authentication method requested by a client
  uint64_t conn_id_;                  ///< A connection ID.
  dbmgr_user_info_t user_info_;       ///< A pointer to an user entry
  struct tls_conf_struct *tls_conf_;  ///< TLS configuration
  rpc_jwt_server_ctx_t jwt_ctx_;      ///< JWT context
};


//
// Structure of an RPC message header.
//
#define SQC_PRC_HEADER_LEN 8
#define SQC_PRC_HEADER_ID_OFFSET  0
#define SQC_PRC_HEADER_LEN_OFFSET 4


///
/// @brief   Send a request to the server.
///
/// @param[in]     rpc_session   An RPC session for client.
/// @param[in]     id            An RPC message ID.
/// @param[in]     body          AN RPC message body.
/// @param[in]     len           Length of \c body.
///
/// @retval SQC_RESULT_OK                    Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR       Failed, an error occurred in POSIX API.
/// @retval SQC_RESULT_NO_MEMORY             Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS          Failed, invalid arguments.
/// @retval SQC_RESULT_AUTHENTICATION_ERROR  Failed, not valid user.
/// @retval SQC_RESULT_ANY_FAILURES          Failed, any other reason.
///
/// @details The function <tt>rpc_session_client_send_msg()</tt> sends a request to the server.
///
sqc_result_t
rpc_session_client_send_msg(rpc_session_client_t *rpc_session, rpc_msg_id_t id, const char *body,
                            size_t len);

///
/// @brief   Send a reply to the client.
///
/// @param[in]     rpc_session   An RPC session for server.
/// @param[in]     id            An RPC message ID.
/// @param[in]     body          AN RPC message body.
/// @param[in]     len           Length of \c body.
///
/// @retval SQC_RESULT_OK                    Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR       Failed, an error occurred in POSIX API.
/// @retval SQC_RESULT_NO_MEMORY             Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS          Failed, invalid arguments.
/// @retval SQC_RESULT_AUTHENTICATION_ERROR  Failed, not valid user.
/// @retval SQC_RESULT_ANY_FAILURES          Failed, any other reason.
///
/// @details The function <tt>rpc_session_server_send_msg()</tt> sends a reply to the client.
///
sqc_result_t
rpc_session_server_send_msg(rpc_session_server_t *rpc_session, rpc_msg_id_t id, const char *body,
                            size_t len);

///
/// @brief   Receives a reply from the server.
///
/// @param[in]     rpc_session   An RPC session for client.
/// @param[out]    id            An RPC message ID.
/// @param[out]    body          AN RPC message body.
/// @param[out]    len           Length of \c body.
///
/// @retval SQC_RESULT_OK                    Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR       Failed, an error occurred in POSIX API.
/// @retval SQC_RESULT_NO_MEMORY             Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS          Failed, invalid arguments.
/// @retval SQC_RESULT_AUTHENTICATION_ERROR  Failed, not valid user.
/// @retval SQC_RESULT_ANY_FAILURES          Failed, any other reason.
///
/// @details The function <tt>rpc_session_recv_msg()</tt> receives a reply from the server.
/// Upon success, the function sets \c id, \c body and \c len.
/// The caller is responsible for freeing the memory referenced by \c body.
///
sqc_result_t
rpc_session_client_recv_msg(rpc_session_client_t *rpc_session, rpc_msg_id_t *id, char **body,
                            size_t *len);

///
/// @brief   Receives a request from the client.
///
/// @param[in]     rpc_session   An RPC session for server.
/// @param[out]    id            An RPC message ID.
/// @param[out]    body          AN RPC message body.
/// @param[out]    len           Length of \c body.
///
/// @retval SQC_RESULT_OK                    Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR       Failed, an error occurred in POSIX API.
/// @retval SQC_RESULT_NO_MEMORY             Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS          Failed, invalid arguments.
/// @retval SQC_RESULT_AUTHENTICATION_ERROR  Failed, not valid user.
/// @retval SQC_RESULT_ANY_FAILURES          Failed, any other reason.
///
/// @details The function <tt>rpc_session_server_recv_msg()</tt> receives a request from
/// the client.
/// Upon success, the function sets \c id, \c body and \c len.
/// The caller is responsible for freeing the memory referenced by \c body.
///
sqc_result_t
rpc_session_server_recv_msg(rpc_session_server_t *rpc_session, rpc_msg_id_t *id, char **body,
                            size_t *len);


__END_DECLS
