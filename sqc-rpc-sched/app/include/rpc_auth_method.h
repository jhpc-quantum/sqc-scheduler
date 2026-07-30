#pragma once

/**
 * @once        rpc_auth_method.h
 */


__BEGIN_DECLS


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


///
/// @brief    Authentication method.
///
/// @details Authentication method for a session.
/// Note that \c AUTH_METHOD_MUTUAL_TLS and AUTH_METHOD_JWT must use TLS.
///
typedef enum rpc_auth_method {
  RPC_AUTH_METHOD_UNKNOWN = 0,
  RPC_AUTH_METHOD_NONE = 1,
  RPC_AUTH_METHOD_MUTUAL_TLS = 2,
  RPC_AUTH_METHOD_MUNGE = 3,
  RPC_AUTH_METHOD_JWT = 4,
} rpc_auth_method_t;


///
/// @brief    Inspect authentication method requires TLS.
///
/// @param[in]     method         An authentication method ID
///
/// @retval true           The method requires TLS.
/// @retval false          Otherwise.
///
/// @details The function <tt>rpc_auth_method_uses_tls()</tt> returns true
/// if the specified authentication method requires its underlying session
/// is TLS.
///
bool
rpc_auth_method_uses_tls(rpc_auth_method_t auth);

///
/// @brief    Convert an authentication method ID to a string.
///
/// @param[in]     method         An authentication method ID
///
/// @retval "none"         \c auth is \c RPC_AUTH_METHOD_NONE.
/// @retval "mutual-tls"   \c auth is \c RPC_AUTH_METHOD_MUTUAL_TLS.
/// @retval "munge"        \c auth is \c RPC_AUTH_METHOD_MUNGE.
/// @retval "jwt"          \c auth is \c RPC_AUTH_METHOD_JWT.
/// @retval "unknown"      Any other value is given.
///
/// @details The function <tt>rpc_auth_method_string()</tt> converts
/// an authentication method ID to a NUL-terminated constant text string.
/// The string must not be freed or modified by the caller.
///
const char *
rpc_auth_method_string(rpc_auth_method_t method);

///
/// @brief    Convert a string to an authentication method ID.
///
/// @param[in]     method_str     A string
///
/// @retval RPC_AUTH_METHOD_NONE          \c auth_str is "none".
/// @retval RPC_AUTH_METHOD_MUTUAL_TLS    \c auth_str is "mutual-tls".
/// @retval RPC_AUTH_METHOD_MUNGE         \c auth_str is "munge".
/// @retval RPC_AUTH_METHOD_JWT           \c auth_str is "jwt".
/// @retval RPC_AUTH_METHOD_UNKNOWN       Any other string is given.
///
/// @details The function <tt>rpc_auth_method_value()</tt> converts
/// a NUL-terminated text string to a corresponding authentication method ID.
///
rpc_auth_method_t
rpc_auth_method_value(const char *method_str);


#ifdef __cplusplus
}
#endif // __cplusplus


__END_DECLS
