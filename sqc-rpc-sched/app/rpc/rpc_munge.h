#pragma once

/**
 * @once        rpc_munge.h
 */

#include "rpc_session_client.h"
#include "rpc_session_server.h"


__BEGIN_DECLS


///
/// @brief   Issue a MUNGE credential.
///
/// @param[in]     rpc_session   An RPC session object.
/// @param[out]    cred          An issued credential.
/// @param[out]    credlen       Length of \c cread.
///
/// @retval SQC_RESULT_OK                    Succeeded.
/// @retval SQC_RESULT_NO_MEMORY             Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS          Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES          Failed, any other reason.
///
/// @details The function <tt>rpc_munge_get_cred()</tt> issues a MUNGE
/// credential \c cred which is a NUL-terminated base64 string.
/// Note that \c cred is set only when the function returns \c SQC_RESULT_OK.
///
/// This API is for schedulers, not clients.
///
sqc_result_t
rpc_munge_client_get_cred(rpc_session_client_t *rpc_session, char **cred, size_t *creadlen);


///
/// @brief   Authenticate an user by validating a MUNGE credential.
///
/// @param[in]     rpc_session   An RPC session object.
///                              a MUNGE credential.
/// @param[in]     cred          An issued credential.
/// @param[in]     conn_id       A connection ID for logging.
/// @param[out]    pwname        User name corresponding with UID written in \c cred.
///
/// @retval SQC_RESULT_OK                    Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR       Failed to get an user entry.
/// @retval SQC_RESULT_NO_MEMORY             Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS          Failed, invalid arguments.
/// @retval SQC_RESULT_AUTHENTICATION_ERROR  Failed, not valid user.
/// @retval SQC_RESULT_ANY_FAILURES          Failed, any other reason.
///
/// @details The function <tt>rpc_munge_validate_cred()</tt> authenticates
/// an user by validating a MUNGE credential \cred which is a NUL-terminated
/// base64 string.
///
/// Note that \c pwname is set only when the function returns \c SQC_RESULT_OK.
/// The caller is responsible for freeing the memory referenced by \c pwname.
///
/// This API is for schedulers, not clients.
///
sqc_result_t
rpc_munge_server_validate_cred(rpc_session_server_t *rpc_session, const char *cred,
                               uint64_t conn_id, char **pwname);


__END_DECLS
