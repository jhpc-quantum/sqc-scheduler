#pragma once


/**
 * @once        rpc_jwt_client.h
 */

__BEGIN_DECLS


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


///
/// @brief    JWT authentication context for client.
///
/// @details JWT authentication context for RPC client.
///
typedef struct rpc_jwt_client_ctx *rpc_jwt_client_ctx_t;


///
/// @brief    Create a JWT context for a client.
///
/// @param[out]    ctx            A JWT context for client.
/// @param[in]     token_file     A path to a token file.
///
/// @retval SQC_RESULT_OK                        Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR           Failed, an error occurred in POSIX API.
/// @retval SQC_RESULT_NO_MEMORY                 Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS              Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES              Failed, any other reason.
///
/// @details The function <tt>rpc_jwt_client_create_ctx()</tt> creates a JWT context for
/// RPC client.  It loads a token in JSON format from \c token_file.
///
sqc_result_t
rpc_jwt_client_create_ctx(rpc_jwt_client_ctx_t* ctx, const char* token_file);

///
/// @brief    Create a JWT context for a client using files under the specified directory.
///
/// @param[out]    ctx            A JWT context for client.
/// @param[in]     dir            A directory where files related to JWT are located.
///
/// @retval SQC_RESULT_OK                        Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR           Failed, an error occurred in POSIX API.
/// @retval SQC_RESULT_NO_MEMORY                 Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS              Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES              Failed, any other reason.
///
/// @details The function <tt>rpc_jwt_client_create_ctx_from_conf_dir()</tt> creates
/// a JWT context for RPC client.  It loads a token in JSON format file in \c dir.
///
sqc_result_t
rpc_jwt_client_create_ctx_from_conf_dir(rpc_jwt_client_ctx_t* ctx, const char* dir);

///
/// @brief    Copy a JWT context.
///
/// @param[out]    ctx            A JWT context for client.
/// @param[in]     other          Another JWT context for client.
///
/// @retval SQC_RESULT_OK                        Succeeded.
/// @retval SQC_RESULT_NO_MEMORY                 Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS              Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES              Failed, any other reason.
///
/// @details The function <tt>rpc_jwt_client_copy_ctx()</tt> creates a JWT context
/// for client, by copying \c other.
///
sqc_result_t
rpc_jwt_client_copy_ctx(rpc_jwt_client_ctx_t* ctx, const rpc_jwt_client_ctx_t* other);

///
/// @brief    Destroy a JWT context.
///
/// @param[in.out]    ctx         A JWT context for client.
///
/// @details The function <tt>rpc_jwt_client_destroy_ctx()</tt> destroys a JWT context
/// for client.
///
void
rpc_jwt_client_destroy_ctx(rpc_jwt_client_ctx_t* ctx);

///
/// @brief    Get a token loaded to the JWT context.
///
/// @param[in]        ctx         A JWT context for client.
/// @param[out]       token       A token.
///
/// @retval SQC_RESULT_OK                        Succeeded.
/// @retval SQC_RESULT_NO_MEMORY                 Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS              Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES              Failed, any other reason.
///
/// @details The function <tt>rpc_jwt_client_get_token()</tt> gets a token loaded to
/// the JWT context.
///
/// Upon success, the token is set to \c token.  The token is a NUL-terminated string.
/// The caller is responsible for freeing \c token.
///
sqc_result_t
rpc_jwt_client_get_token(rpc_jwt_client_ctx_t* ctx, char** token);


///
/// @brief    Dump a JWT context.
///
/// @param[in]        ctx         A JWT context for client.
///
/// @details The function <tt>rpc_jwt_client_dump_ctx()</tt> dumps \c ctx to the log.
///
void
rpc_jwt_client_dump_ctx(rpc_jwt_client_ctx_t* ctx);

#ifdef __cplusplus
}
#endif // __cplusplus


__END_DECLS
