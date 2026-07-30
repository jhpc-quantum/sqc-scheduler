#include "sqc_apis.h"
#include "rpc_jwt_client.h"
#include "rpc_file_util.h"

#define JWT_TOKEN_FILE "jwt.token"


//
// JWT authentication context for client.
//
struct rpc_jwt_client_ctx {
  char *token_;       ///< Token.
};


//
// Create a JWT context for an RPC client.
//
static inline sqc_result_t
s_create_ctx(rpc_jwt_client_ctx_t* ctx, const char* token_file) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *expanded_token_file = nullptr;
  rpc_jwt_client_ctx_t tmp_ctx = nullptr;

  if (likely(ctx != nullptr && IS_VALID_STRING(token_file))) {
    tmp_ctx = static_cast<rpc_jwt_client_ctx_t>(malloc(sizeof(rpc_jwt_client_ctx)));
    if (likely(tmp_ctx != nullptr)) {
      tmp_ctx->token_ = nullptr;
      if (likely(rpc_expand_path(token_file, &expanded_token_file) == SQC_RESULT_OK)) {
        if (likely(rpc_read_text_file(expanded_token_file, &tmp_ctx->token_, nullptr)
                   == SQC_RESULT_OK)) {
          rpc_strip_text(tmp_ctx->token_);
        } else {
          sqc_msg_debug(5, "Failed to read a token from the file, %s: %s\n",
                        sqc_error_get_string(ret), token_file);
        }
      }
      *ctx = tmp_ctx;
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_NO_MEMORY;
      sqc_msg_debug(5, "Failed to create a JWT client context, %s\n", sqc_error_get_string(ret));
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_debug(5, "%s\n", sqc_error_get_string(ret));
  }

  if (unlikely(ret != SQC_RESULT_OK)) {
    free(tmp_ctx);
  }

  return ret;
}


//
// Create a JWT context for an RPC server, from files under the directory.
//
static inline sqc_result_t
s_create_ctx_from_conf_dir(rpc_jwt_client_ctx_t* ctx, const char* dir) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char* token_file = nullptr;

  if (likely(ctx != nullptr && IS_VALID_STRING(dir))) {
    size_t dir_len = strlen(dir);
    size_t token_file_size = dir_len + 1u + strlen(JWT_TOKEN_FILE) + 1u;

    token_file = static_cast<char*>(malloc(token_file_size));
    if (likely(token_file != nullptr)) {
      static_cast<void>(snprintf(token_file, token_file_size, "%s/%s",
                                   dir, JWT_TOKEN_FILE));
      ret = s_create_ctx(ctx, token_file);
    }
    free(token_file);
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
s_copy_ctx(rpc_jwt_client_ctx_t* ctx, const rpc_jwt_client_ctx_t* other) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  rpc_jwt_client_ctx_t tmp_ctx = nullptr;

  if (likely(ctx != nullptr && other != nullptr && *other != nullptr)) {
    tmp_ctx = static_cast<rpc_jwt_client_ctx_t>(malloc(sizeof(rpc_jwt_client_ctx)));
    if (likely(tmp_ctx != NULL)) {
      tmp_ctx->token_ = nullptr;

      do {
        if ((*other)->token_ != nullptr) {
          tmp_ctx->token_ = strdup((*other)->token_);
          if (unlikely(tmp_ctx->token_ == nullptr)) {
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
    rpc_jwt_client_destroy_ctx(&tmp_ctx);
  }

  return ret;
}


//
// Create a JWT context.
//
static inline void
s_destroy_ctx(rpc_jwt_client_ctx_t* ctx) {
  if (likely(ctx != nullptr && *ctx != nullptr)) {
    free((*ctx)->token_);
    free((*ctx));
    *ctx = nullptr;
  }
}


//
// Get a token.
//
static inline sqc_result_t
s_get_token(rpc_jwt_client_ctx_t* ctx, char** token) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(ctx != nullptr && *ctx != nullptr && token != nullptr)) {
    if (likely((*ctx)->token_ != nullptr)) {
      char* tmp_token = strdup((*ctx)->token_);
      if (likely(tmp_token != nullptr)) {
        ret = SQC_RESULT_OK;
        *token = tmp_token;
      } else {
        ret = SQC_RESULT_NO_MEMORY;
      }
    } else {
      *token = nullptr;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


//
// Dump the context.
//
static inline void
s_dump_ctx(rpc_jwt_client_ctx_t* ctx) {
  if (likely(ctx != nullptr && *ctx != nullptr)) {
    sqc_msg_debug(5, "jwt_client_ctx.token = %s\n",
                  (*ctx)->token_ != nullptr ? (*ctx)->token_ : "(null)");
  } else {
    sqc_msg_debug(5, "jwt_client_ctx = (null)\n");
  }
}


/*
 * Exported APIs
 */

sqc_result_t
rpc_jwt_client_create_ctx(rpc_jwt_client_ctx_t* ctx, const char* token_file) {
  return s_create_ctx(ctx, token_file);
}


sqc_result_t
rpc_jwt_client_create_ctx_from_conf_dir(rpc_jwt_client_ctx_t* ctx, const char* dir) {
  return s_create_ctx_from_conf_dir(ctx, dir);
}


sqc_result_t
rpc_jwt_client_copy_ctx(rpc_jwt_client_ctx_t* ctx, const rpc_jwt_client_ctx_t* other) {
  return s_copy_ctx(ctx, other);
}


void
rpc_jwt_client_destroy_ctx(rpc_jwt_client_ctx_t* ctx) {
  s_destroy_ctx(ctx);
}


sqc_result_t
rpc_jwt_client_get_token(rpc_jwt_client_ctx_t* ctx, char** token) {
  return s_get_token(ctx, token);
}


void
rpc_jwt_client_dump_ctx(rpc_jwt_client_ctx_t* ctx) {
  s_dump_ctx(ctx);
}
