#include "sqc_apis.h"
#include "rpc_jwt_server.h"
#include "rpc_file_util.h"

#define JWT_PUBLIC_KEY_FILE "jwt_pub.key"
#define JWT_ISSUER_FILE "jwt_iss.txt"

//
// Header files for C++.
//
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wshadow"
#include <jwt-cpp/jwt.h>
#pragma GCC diagnostic pop

#include <string>
#include <exception>


//
// JWT authentication context for server.
//
struct rpc_jwt_server_ctx {
  char *pub_key_;  ///< ES256 Public key for signature.
  char *issuer_;   ///< Expected issuer.
};


//
// Create a JWT context for RPC server.
//
static inline sqc_result_t
s_create_ctx(rpc_jwt_server_ctx_t* ctx, const char* pub_key_file, const char* issuer_file) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  rpc_jwt_server_ctx_t tmp_ctx = nullptr;

  if (likely(ctx != nullptr && IS_VALID_STRING(pub_key_file) && IS_VALID_STRING(issuer_file))) {
    tmp_ctx = static_cast<rpc_jwt_server_ctx_t>(malloc(sizeof(rpc_jwt_server_ctx)));
    if (likely(tmp_ctx != nullptr)) {
      tmp_ctx->pub_key_ = nullptr;
      tmp_ctx->issuer_ = nullptr;

      if (likely(rpc_read_text_file(pub_key_file, &tmp_ctx->pub_key_, nullptr) == SQC_RESULT_OK)) {
        ; // nothing to do.
      } else {
        sqc_msg_debug(5, "Failed to read a public key from the file, %s: %s\n",
                      sqc_error_get_string(ret), pub_key_file);
      }
      if (likely(rpc_read_text_file(issuer_file, &tmp_ctx->issuer_, nullptr) == SQC_RESULT_OK)) {
        rpc_strip_text(tmp_ctx->issuer_);
      } else {
        sqc_msg_debug(5, "Failed to read an expected issuer from the file, %s: %s\n",
                      sqc_error_get_string(ret), issuer_file);
      }
      ret = SQC_RESULT_OK;
      *ctx = tmp_ctx;
    } else {
      ret = SQC_RESULT_NO_MEMORY;
      sqc_msg_debug(5, "Failed to create a JWT server context, %s\n", sqc_error_get_string(ret));
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  if (unlikely(ret != SQC_RESULT_OK)) {
    free(tmp_ctx);
  }

  return ret;
}


//
// Create a JWT context for RPC server, from files under the directory.
//
static inline sqc_result_t
s_create_ctx_from_conf_dir(rpc_jwt_server_ctx_t* ctx, const char* dir) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char* pub_key_file = nullptr;
  char* issuer_file = nullptr;

  if (likely(ctx != nullptr && IS_VALID_STRING(dir))) {
    size_t dir_len = strlen(dir);
    size_t pub_key_file_size = dir_len + 1u + strlen(JWT_PUBLIC_KEY_FILE) + 1u;
    size_t issuer_file_size = dir_len + 1u + strlen(JWT_ISSUER_FILE) + 1u;

    pub_key_file = static_cast<char*>(malloc(pub_key_file_size));
    issuer_file = static_cast<char*>(malloc(issuer_file_size));
    if (likely(pub_key_file != nullptr && issuer_file != nullptr)) {
      static_cast<void>(snprintf(pub_key_file, pub_key_file_size, "%s/%s",
                                   dir, JWT_PUBLIC_KEY_FILE));
      static_cast<void>(snprintf(issuer_file, issuer_file_size, "%s/%s",
                                   dir, JWT_ISSUER_FILE));
      ret = s_create_ctx(ctx, pub_key_file, issuer_file);
    } else {
      ret = SQC_RESULT_NO_MEMORY;
      sqc_msg_debug(5, "%s\n", sqc_error_get_string(ret));
    }
    free(pub_key_file);
    free(issuer_file);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


//
// Copy a JWT context.
//
static inline sqc_result_t
s_copy_ctx(rpc_jwt_server_ctx_t* ctx, const rpc_jwt_server_ctx_t* other) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  rpc_jwt_server_ctx_t tmp_ctx = nullptr;

  if (likely(ctx != nullptr && other != nullptr && *other != nullptr)) {
    tmp_ctx = static_cast<rpc_jwt_server_ctx_t>(malloc(sizeof(rpc_jwt_server_ctx)));
    if (likely(tmp_ctx != NULL)) {
      tmp_ctx->pub_key_ = nullptr;
      tmp_ctx->issuer_ = nullptr;

      do {
        if ((*other)->pub_key_ != nullptr) {
          tmp_ctx->pub_key_ = strdup((*other)->pub_key_);
          if (unlikely(tmp_ctx->pub_key_ == nullptr)) {
            ret = SQC_RESULT_NO_MEMORY;
            break;
          }
        }

        if ((*other)->issuer_ != nullptr) {
          tmp_ctx->issuer_ = strdup((*other)->issuer_);
          if (unlikely(tmp_ctx->issuer_ == nullptr)) {
            ret = SQC_RESULT_NO_MEMORY;
            break;
          }
        }

        ret = SQC_RESULT_OK;
        *ctx = tmp_ctx;
      } while (0);
    } else {
      ret = SQC_RESULT_NO_MEMORY;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_debug(5, "%s\n", sqc_error_get_string(ret));
  }

  if (unlikely(ret != SQC_RESULT_OK)) {
    rpc_jwt_server_destroy_ctx(&tmp_ctx);
  }

  return ret;
}


//
// Create a JWT context.
//
static inline void
s_destroy_ctx(rpc_jwt_server_ctx_t* ctx) {
  if (likely(ctx != nullptr && *ctx != nullptr)) {
    free((*ctx)->pub_key_);
    free((*ctx)->issuer_);
    free((*ctx));
    *ctx = nullptr;
  }
}


//
// Validate a JWT and returns subject upon success.
//
static inline sqc_result_t
s_validate_token(rpc_jwt_server_ctx_t* ctx, const char* token, uint64_t conn_id, char** subject) {
  if (unlikely(ctx == nullptr || *ctx == nullptr || IS_VALID_STRING((*ctx)->pub_key_) == false ||
               IS_VALID_STRING((*ctx)->issuer_) == false || IS_VALID_STRING(token) == false ||
               subject == nullptr)) {
    sqc_msg_error("%s: conn-id=%llu\n", sqc_error_get_string(SQC_RESULT_INVALID_ARGS),
                  (unsigned long long) conn_id);
    return SQC_RESULT_INVALID_ARGS;
  }

  //
  // Decode the JWT.
  //
  try {
    auto decoded_jwt = jwt::decode(token);

    //
    // Get "sub" value in the JWT.
    //
    try {
      auto jwt_subject = decoded_jwt.get_subject();

      //
      // Get "exp" value in the JWT.
      //
      try {
        static_cast<void>(decoded_jwt.get_expires_at());
      } catch (const std::exception& exc) {
        sqc_msg_error("Failed to get 'exp' in a JWT token, %s: conn-id=%llu\n",
                      exc.what(), (unsigned long long) conn_id);
        return SQC_RESULT_INVALID_JWT_TOKEN;
      }

      //
      // Verify the JWT.
      // If failed, an exception is thrown.
      //
      try {
        auto verifier = jwt::verify()
          .with_issuer((*ctx)->issuer_)
          .allow_algorithm(jwt::algorithm::es256((*ctx)->pub_key_));
        verifier.verify(decoded_jwt);
      } catch (const std::exception& exc) {
        sqc_msg_error("Failed to validate a JWT token, %s: conn-id=%llu\n", exc.what(),
                      (unsigned long long) conn_id);
        return SQC_RESULT_AUTHENTICATION_ERROR;
      }

      //
      // Copy subject of the JWT to '*subject'.
      //
      *subject = strdup(jwt_subject.c_str());
      if (unlikely(*subject == nullptr)) {
        sqc_msg_error("%s: conn-id=%llu\n", sqc_error_get_string(SQC_RESULT_NO_MEMORY),
                      (unsigned long long) conn_id);
        return SQC_RESULT_NO_MEMORY;
      }

    } catch (const std::exception& exc) {
      sqc_msg_error("Failed to get 'sub' claim in a JWT token, %s: conn-id=%llu\n", exc.what(),
                    (unsigned long long) conn_id);
      return SQC_RESULT_INVALID_JWT_TOKEN;
    }

  } catch (const std::exception& exc) {
    sqc_msg_error("Failed to decode a JWT token, %s: conn-id=%llu\n", exc.what(),
                  (unsigned long long) conn_id);
    return SQC_RESULT_INVALID_JWT_TOKEN;
  }

  sqc_msg_info("Succeeded to verify a JWT token, subject=%s: conn-id=%llu\n",
               *subject, (unsigned long long) conn_id);
  return SQC_RESULT_OK;
}


//
// Validate a JWT and returns subject upon success.
//
static inline void
s_dump_ctx(rpc_jwt_server_ctx_t* ctx) {
  if (likely(ctx != nullptr && *ctx != nullptr)) {
    sqc_msg_debug(5, "jwt_server_ctx.public_key = %s\n",
                  (*ctx)->pub_key_ != nullptr ? (*ctx)->pub_key_ : "(null)");
    sqc_msg_debug(5, "jwt_server_ctx.issuer = %s\n",
                  (*ctx)->issuer_ != nullptr ? (*ctx)->issuer_ : "(null)");
  } else {
    sqc_msg_debug(5, "jwt_server_ctx = (null)\n");
  }
}


/*
 * Exported APIs
 */

sqc_result_t
rpc_jwt_server_create_ctx(rpc_jwt_server_ctx_t* ctx, const char* pub_key_file,
                          const char* issuer_file) {
  return s_create_ctx(ctx, pub_key_file, issuer_file);
}


sqc_result_t
rpc_jwt_server_create_ctx_from_conf_dir(rpc_jwt_server_ctx_t* ctx, const char* dir) {
  return s_create_ctx_from_conf_dir(ctx, dir);
}


sqc_result_t
rpc_jwt_server_copy_ctx(rpc_jwt_server_ctx_t* ctx, const rpc_jwt_server_ctx_t* other) {
  return s_copy_ctx(ctx, other);
}


void
rpc_jwt_server_destroy_ctx(rpc_jwt_server_ctx_t* ctx) {
  s_destroy_ctx(ctx);
}


sqc_result_t
rpc_jwt_server_validate_token(rpc_jwt_server_ctx_t* ctx, const char *token, uint64_t conn_id,
                              char **subject) {
  return s_validate_token(ctx, token, conn_id, subject);
}


void
rpc_jwt_server_dump_ctx(rpc_jwt_server_ctx_t* ctx) {
  s_dump_ctx(ctx);
}
