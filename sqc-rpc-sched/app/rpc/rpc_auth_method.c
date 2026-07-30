#include "sqc_apis.h"
#include "rpc_auth_method.h"


//
// Return a string describing an RPC method passed in the argument 'method'.
//
static inline const char *
s_auth_method_string(rpc_auth_method_t method) {
  const char *ret = NULL;

  switch (method) {
  case RPC_AUTH_METHOD_NONE:
    ret = "none";
    break;
  case RPC_AUTH_METHOD_MUTUAL_TLS:
    ret = "mutual-tls";
    break;
  case RPC_AUTH_METHOD_MUNGE:
    ret = "munge";
    break;
  case RPC_AUTH_METHOD_JWT:
    ret = "jwt";
    break;
  default:
    ret = "unknown";
    break;
  }

  return ret;
}


//
// Return true if the authentication method requires TLS.
//
static inline bool
s_auth_method_uses_tls(rpc_auth_method_t method) {
  return (method == RPC_AUTH_METHOD_MUTUAL_TLS || method == RPC_AUTH_METHOD_JWT);
}


//
// Return a string describing an RPC method passed in the argument 'method'.
//
static inline rpc_auth_method_t
s_auth_method_value(const char *method_str) {
  rpc_auth_method_t ret = RPC_AUTH_METHOD_UNKNOWN;

  if (likely(method_str != NULL)) {
    if (strcmp(method_str, "none") == 0) {
      ret = RPC_AUTH_METHOD_NONE;
    } else if (strcmp(method_str, "mutual-tls") == 0) {
      ret = RPC_AUTH_METHOD_MUTUAL_TLS;
    } else if (strcmp(method_str, "munge") == 0) {
      ret = RPC_AUTH_METHOD_MUNGE;
    } else if (strcmp(method_str, "jwt") == 0) {
      ret = RPC_AUTH_METHOD_JWT;
    }
  }

  return ret;
}


/*
 * Exported APIs
 */

bool
rpc_auth_method_uses_tls(rpc_auth_method_t auth) {
  return s_auth_method_uses_tls(auth);
}


const char *
rpc_auth_method_string(rpc_auth_method_t auth) {
  return s_auth_method_string(auth);
}


rpc_auth_method_t
rpc_auth_method_value(const char *auth_str) {
  return s_auth_method_value(auth_str);
}
