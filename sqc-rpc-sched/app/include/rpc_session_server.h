#pragma once


///
/// @file        rpc_session_server.h
///

#include "rpc_auth_method.h"
#include "rpc_jwt_server.h"
#include "rpc_msg_id.h"
#include "dbmgr.h"


__BEGIN_DECLS


struct rpc_session_server;
typedef struct rpc_session_server *rpc_session_server_t;


///
/// @brief   Issue a connection ID for logging.
///
/// @retval a connection ID.
///
/// @details The function <tt>rpc_session_server_issue_connection_id()</tt> isses
/// a connection ID.  This ID is used to distinguish a TCP connection in log messages.
///
uint64_t
rpc_session_server_issue_connection_id(void);

///
/// @brief   Create an RPC server.
///
/// @param[out]    rpc_session       An RPC session object.
/// @param[in]     fd                A TCP listen socket.
/// @param[in]     addr              A socket-address of the connected server.
/// @param[in]     addrlen           Size of \c addr.
/// @param[in]     tls_conf          TLS configuration object.
/// @param[in]     jwt_ctx           JWT context.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR  Failed, communication error.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_UNSUPPORTED      Failed, unsupported server type.
/// @retval SQC_RESULT_EOF              Failed, the RPC connection has closed.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_session_server_create()</tt> creates \c sqc_session_server_t
/// object.
/// It needs a TCP socket \c fd, connected with a client.
/// In general, \c fd is issued by <tt>sqc_endpoint_accept()</tt>.
///
/// If argument \c tls_conf is NULL, the server doesn't support TLS.
/// If \c jwt_ctx is NULL, JWT authentication is disabled.
/// Note that JWT authentication requires TLS.
///
sqc_result_t
rpc_session_server_create(rpc_session_server_t* rpc_session, int fd,
                          struct sockaddr_storage *addr, socklen_t addrlen,
                          const sqc_tls_conf_t tls_conf, rpc_jwt_server_ctx_t* jwt_ctx);

///
/// @brief   Create an RPC server.
///
/// @param[out]    rpc_session       An RPC session object.
/// @param[in]     fd                A TCP listen socket.
/// @param[in]     addr              A socket-address of the connected server.
/// @param[in]     addrlen           Size of \c addr.
/// @param[in]     tls_conf          TLS configuration object.
/// @param[in]     jwt_ctx           JWT context.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR  Failed, communication error.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_UNSUPPORTED      Failed, unsupported server type.
/// @retval SQC_RESULT_EOF              Failed, the RPC connection has closed.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_session_sever_create_from_conf_dir()</tt> is the same
/// as <tt>rpc_session_server_create()</tt>, but it internally creates a TLS configuration
/// object and a JSON Web Token context from files in \c dir.
///
sqc_result_t
rpc_session_server_create_from_conf_dir(rpc_session_server_t* rpc_session, int fd,
                                        struct sockaddr_storage *addr, socklen_t addrlen,
                                        const char *dir);

///
/// @brief   Destroy an RPC session.
///
/// @param[in]     rpc_session   An RPC session for server.
///
/// @details The function <tt>rpc_session_server_destroy()</tt> destroys
/// \c sqc_session_server_t object.
///
void
rpc_session_server_destroy(rpc_session_server_t *rpc_session);

///
/// @brief   Establish a TLS session.
///
/// @param[in]     rpc_session      An RPC session for server
/// @param[in]     do_mutual_auth   Whether or not to do mutual authentication.
/// @param[in]     use_proxy_cert   Whether or not to use proxy certificates.
///
/// @retval SQC_RESULT_OK                     Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR        Failed, an error occurred in POSIX API.
/// @retval SQC_RESULT_NO_MEMORY              Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS           Failed, invalid arguments.
/// @retval SQC_RESULT_AUTHENTICATION_ERROR   Failed, not valid user.
/// @retval SQC_RESULT_INVALID_CREDENTIAL     Failed, invalid credential.
/// @retval SQC_RESULT_CERTIFICATE_VERIFY_FAILURE
////                                          Certificate verification failure.
/// @retval SQC_RESULT_ANY_FAILURES           Failed, any other reason.
///
/// @details The function <tt>rpc_session_server_establish_tls()</tt> establishes a TLS session
/// on the current TCP connection.
///
sqc_result_t
rpc_session_server_establish_tls(rpc_session_server_t *rpc_session, bool do_mutual_auth,
                                 bool use_proxy_cert);

///
/// @brief   Return a bound address and a port of the RPC session socket.
///
/// @param[in]     rpc_session   An RPC session for server.
/// @param[out]    name          A bound address.
/// @param[out]    port          A bound port.
///
/// @retval SQC_RESULT_OK                    Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR       Failed, an error occurred in POSIX API.
/// @retval SQC_RESULT_NO_MEMORY             Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS          Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES          Failed, any other reason.
///
/// @details The function <tt>rpc_session_server_get_local_name()</tt> returns a bound address
/// and a port of the socket that \c rpc_session uses.
/// The caller is responsible for freeing \c name.
///
sqc_result_t
rpc_session_server_get_local_name(rpc_session_server_t *rpc_session, char **name, int *port);

///
/// @brief   Return a bound address and a port of the RPC session socket.
///
/// @param[in]     rpc_session   An RPC session for server.
/// @param[out]    name          A connected address.
/// @param[out]    port          A connected port.
///
/// @retval SQC_RESULT_OK                    Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR       Failed, an error occurred in POSIX API.
/// @retval SQC_RESULT_NO_MEMORY             Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS          Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES          Failed, any other reason.
///
/// @details The function <tt>rpc_session_server_get_peer_name()</tt> returns a connected
/// address and a port of the socket that \c rpc_session uses.
/// The caller is responsible for freeing \c name.
///
sqc_result_t
rpc_session_server_get_peer_name(rpc_session_server_t *rpc_session, char **name, int *port);

///
/// @brief   The server receives an RPC request from a client and handle it.
///
/// @param[in]     rpc_session  An RPC session for server.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR  Failed, communication error.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_NOT_ALLOWED      Failed, invalid file access permission.
/// @retval SQC_RESULT_ANY_RUNTIME_ERROR
///                                     Failed, any runtime error occurred.
/// @retval SQC_RESULT_NOT_A_DIRECTORY  Failed, the given path doesn't point to a directory.
/// @retval SQC_RESULT_IS_A_DIRECTORY   Failed, the given path doesn't point to a file.
/// @retval SQC_RESULT_INVALID_CIPHER   Failed, the given cipher suite is invalid.
/// @retval SQC_RESULT_INVALID_OBJECT   Failed, received an unexpected reply from the scheduler.
/// @retval SQC_RESULT_EOF              Failed, the RPC session has closed.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_session_server_process_request</tt> receives
/// an RPC request from a client and handles it.  Usually it sends a reply
/// to the client.
///
sqc_result_t
rpc_session_server_process_request(rpc_session_server_t *rpc_session);


__END_DECLS
