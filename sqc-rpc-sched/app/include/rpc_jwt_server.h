#pragma once

/**
 * @once        rpc_jwt_server.h
 */


__BEGIN_DECLS


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


///
/// @brief    JWT authentication context for server.
///
/// @details JWT authentication context for RPC server.
///
typedef struct rpc_jwt_server_ctx *rpc_jwt_server_ctx_t;


///
/// @brief    Create a JWT context for a server.
///
/// @param[out]    ctx            A JWT authentication context for server.
/// @param[in]     pub_key_file   A path to a public key file.
/// @param[in]     issuer_file    An expected issuer of JWT.
///
/// @retval SQC_RESULT_OK                        Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR           Failed, an error occurred in POSIX API.
/// @retval SQC_RESULT_NO_MEMORY                 Failed, no memory.
/// @retval SQC_RESULT_PUBLIC_KEY_READ_FAILURE   Failed, public key read error.
/// @retval SQC_RESULT_INVALID_ARGS              Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES              Failed, any other reason.
///
/// @details The function <tt>rpc_jwt_server_create_ctx()</tt> creates a JWT context
/// for RPC server.  It loads a public key in PEM format from \c pub_key_file and
/// loads an expected issuer ("iss" claim value) from \c issuer_file.
///
sqc_result_t
rpc_jwt_server_create_ctx(rpc_jwt_server_ctx_t* ctx, const char* pub_key_file,
                          const char* issuer_file);

///
/// @brief    Create a JWT context for server using files under the specified directory.
///
/// @param[out]    ctx            A JWT context for server.
/// @param[in]     dir            A directory where files related to JWT are located.
///
/// @retval SQC_RESULT_OK                        Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR           Failed, an error occurred in POSIX API.
/// @retval SQC_RESULT_NO_MEMORY                 Failed, no memory.
/// @retval SQC_RESULT_PUBLIC_KEY_READ_FAILURE   Failed, public key read error.
/// @retval SQC_RESULT_INVALID_ARGS              Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES              Failed, any other reason.
///
/// @details The function <tt>rpc_jwt_create_server_ctx_from_conf_dir()</tt> creates
/// a JWT context for RPC server.  It loads a public key in PEM format and
/// an expected issuer ("iss" claim value) from files in \c dir.
///
sqc_result_t
rpc_jwt_server_create_ctx_from_conf_dir(rpc_jwt_server_ctx_t* ctx, const char* dir);

///
/// @brief    Copy a JWT context.
///
/// @param[out]    ctx            A JWT context for server.
/// @param[in]     other          Another JWT context for server.
///
/// @retval SQC_RESULT_OK                        Succeeded.
/// @retval SQC_RESULT_NO_MEMORY                 Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS              Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES              Failed, any other reason.
///
/// @details The function <tt>rpc_jwt_server_copy_ctx()</tt> creates a JWT context
/// for server by copying \c other.
///
sqc_result_t
rpc_jwt_server_copy_ctx(rpc_jwt_server_ctx_t* ctx, const rpc_jwt_server_ctx_t* other);

///
/// @brief    Destroy a JWT context.
///
/// @param[in.out]    ctx         A JWT context for server.
///
/// @details The function <tt>rpc_jwt_server_destroy_ctx()</tt> destroys a JWT context
/// for server.
///
void
rpc_jwt_server_destroy_ctx(rpc_jwt_server_ctx_t* ctx);


///
/// @brief    Validate a JWT (JSON Web Token).
///
/// @param[in]     ctx            A JWT context for server.
/// @param[in]     token          An encoded JWT.
/// @param[in]     conn_id        A connection ID for logging.
/// @param[out]    subject        Subject described in the given toen.
///
/// @retval SQC_RESULT_OK                        Succeeded.
/// @retval SQC_RESULT_NO_MEMORY                 Failed, no memory.
/// @retval SQC_RESULT_AUTHENTICATION_ERROR      Failed, authentication error.
/// @retval SQC_RESULT_INVALID_ARGS              Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES              Failed, any other reason.
///
/// @details The function <tt>rpc_jwt_server_validate_token()</tt> velidates \c token.
/// Upon success, subject described in \c token (i.e. value of "iss" claim) is set to
/// \c subject.  The caller is responsible for freeing \c subject.
///
sqc_result_t
rpc_jwt_server_validate_token(rpc_jwt_server_ctx_t* ctx, const char *token, uint64_t conn_id,
                              char **subject);

///
/// @brief    Dump a JWT context.
///
/// @param[in]        ctx         A JWT context for server.
///
/// @details The function <tt>rpc_jwt_server_dump_ctx()</tt> dumps \c ctx to the log.
///
void
rpc_jwt_server_dump_ctx(rpc_jwt_server_ctx_t* ctx);

#ifdef __cplusplus
}
#endif // __cplusplus


__END_DECLS
