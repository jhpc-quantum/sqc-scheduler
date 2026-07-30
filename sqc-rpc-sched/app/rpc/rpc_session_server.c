#include "sqc_apis.h"
#include "dbmgr.h"
#include "req_sched.h"
#include "rpc.pb-c.h"
#include "rpc_file_util.h"
#include "rpc_munge.h"
#include "rpc_msg_util.h"
#include "rpc_session_internal.h"
#include "sqc_rpc_sched_conf.h"
#include "sqc_rpc_sched_conv_enums.h"


/**
 * @brief    Flags set by a request handler.
 *
 * @details  When a request handler returns \c SQC_RESULT_OK,
 * <tt>rpc_recv_then_send()</tt>, the caller of the request handler, does
 * post-process of the RPC request according with value of \c flags set by
 * the request handler.
 *
 * If \c RPC_REQUEST_HANDLER_FLAG_TLS bit is set, it tries establishing
 * a TLS session on the current TCP connection.
 *
 * If both \c RPC_REQUEST_HANDLER_FLAG_MUTUAL_TLS_AUTH bit is set, it does
 * mutual TLS auhentication.  This flag is ignored if \c RPC_REQUEST_HANDLER_FLAG_TLS
 * bit is disabled.
 */
typedef uint32_t rpc_request_handler_flags_t;

#define RPC_REQUEST_HANDLER_FLAG_TLS              1u
#define RPC_REQUEST_HANDLER_FLAG_MUTUAL_TLS_AUTH  2u

//
// Special user name that means "all users".
//
static const char* all_users = "ALL";

//
// Issue a connection ID.
//
static inline uint64_t
s_issue_connection_id(void) {
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  static uint64_t conn_seq = 0ull;
  uint64_t ret = 0ull;

  pthread_mutex_lock(&mutex);
  ret = conn_seq++;
  pthread_mutex_unlock(&mutex);

  return ret;
}


//
// Log peer name.
//
static inline sqc_result_t
s_log_peer_name(rpc_session_server_t *rpc_session) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *name = NULL;
  int port = 0;

  if (likely(rpc_session != NULL && *rpc_session != NULL)) {
    ret = rpc_session_server_get_peer_name(rpc_session, &name, &port);
    if (likely(ret == SQC_RESULT_OK)) {
      sqc_msg_info("RPC Session: Client connected: conn-id=%llu, address=%s, port=%u\n",
                    (unsigned long long) (*rpc_session)->conn_id_, name, (unsigned int) port);
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_debug(5, "%s\n", sqc_error_get_string(ret));
  }

  free(name);
  return ret;
}


//
// Create an RPC server.
//
static inline sqc_result_t
s_create(rpc_session_server_t *rpc_session, int fd, struct sockaddr_storage *addr,
         socklen_t addrlen, const sqc_tls_conf_t tls_conf, rpc_jwt_server_ctx_t* jwt_ctx) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t create_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t copy_jwt_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t copy_tls_result = SQC_RESULT_ANY_FAILURES;
  rpc_session_server_t tmp_rpc_session = NULL;

  if (likely(rpc_session != NULL && addr != NULL && tls_conf != NULL && jwt_ctx != NULL &&
             *jwt_ctx != NULL)) {
    tmp_rpc_session = malloc(sizeof(struct rpc_session_server));
    if (likely(tmp_rpc_session != NULL)) {
      tmp_rpc_session->session_ = NULL;
      tmp_rpc_session->auth_method_ = RPC_AUTH_METHOD_UNKNOWN;
      tmp_rpc_session->conn_id_ = s_issue_connection_id();
      tmp_rpc_session->user_info_ = NULL;
      tmp_rpc_session->tls_conf_ = NULL;
      tmp_rpc_session->jwt_ctx_ = NULL;

      copy_tls_result = sqc_tls_conf_copy(&tmp_rpc_session->tls_conf_, tls_conf);
      if (likely(copy_tls_result == SQC_RESULT_OK)) {
        (void)sqc_tls_conf_set_allow_no_crl(tmp_rpc_session->tls_conf_, true);
        sqc_msg_debug(5, "Register TLS configuration to the RPC session\n");
      } else {
        ret = copy_tls_result;
        sqc_msg_debug(5, "Failed to register TLS configuration to the RPC session: %s\n",
                      sqc_error_get_string(ret));
      }

      copy_jwt_result = rpc_jwt_server_copy_ctx(&tmp_rpc_session->jwt_ctx_, jwt_ctx);
      if (likely(copy_jwt_result == SQC_RESULT_OK)) {
        sqc_msg_debug(5, "Register JWT context to the RPC session\n");
      } else {
        ret = copy_jwt_result;
        sqc_msg_debug(5, "Failed to register JWT context to the RPC session: %s\n",
                      sqc_error_get_string(ret));
      }

      if (likely(copy_tls_result == SQC_RESULT_OK && copy_jwt_result == SQC_RESULT_OK)) {
        create_result = sqc_session_create_server(&(tmp_rpc_session->session_), 0u,
                                                  fd, addr, addrlen);
        if (likely(create_result == SQC_RESULT_OK)) {
          ret = s_log_peer_name(&tmp_rpc_session);
          if (likely(ret == SQC_RESULT_OK)) {
            *rpc_session = tmp_rpc_session;
          }
        } else {
          ret = create_result;
        }
      }
    } else {
      ret = SQC_RESULT_NO_MEMORY;
      sqc_msg_debug(5, "%s\n", sqc_error_get_string(ret));
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_debug(5, "%s\n", sqc_error_get_string(ret));
  }

  if (likely(ret == SQC_RESULT_OK)) {
    sqc_tls_conf_dump((*rpc_session)->tls_conf_);
    rpc_jwt_server_dump_ctx(&(*rpc_session)->jwt_ctx_);
  } else {
    if (tmp_rpc_session != NULL) {
      rpc_session_server_destroy(&tmp_rpc_session);
    }
  }

  return ret;
}


//
// Create an RPC server by using files under the configuration directory.
//
static inline sqc_result_t
s_create_from_conf_dir(rpc_session_server_t *rpc_session, int fd, struct sockaddr_storage *addr,
         socklen_t addrlen, const char *dir) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t tls_conf_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t jwt_ctx_result = SQC_RESULT_ANY_FAILURES;
  sqc_tls_conf_t tls_conf = NULL;
  rpc_jwt_server_ctx_t jwt_ctx = NULL;

  if (likely(rpc_session != NULL && addr != NULL && dir != NULL)) {
    tls_conf_result = sqc_tls_conf_create_from_conf_dir(&tls_conf, dir, TLS_ROLE_SERVER);
    if (likely(tls_conf_result == SQC_RESULT_OK)) {
      jwt_ctx_result = rpc_jwt_server_create_ctx_from_conf_dir(&jwt_ctx, dir);
      if (likely(jwt_ctx_result == SQC_RESULT_OK)) {
        ret = rpc_session_server_create(rpc_session, fd, addr, addrlen, tls_conf, &jwt_ctx);
      } else {
        ret = jwt_ctx_result;
      }
    } else {
      ret = tls_conf_result;
    }
  } else {
    ret = SQC_RESULT_POSIX_API_ERROR;
    sqc_msg_debug(5, "%s\n", sqc_error_get_string(ret));
  }

  free(tls_conf);
  free(jwt_ctx);

  return ret;
}


//
// Destroy the RPC session.
//
static inline void
s_destroy(rpc_session_server_t *rpc_session) {
  if (likely(rpc_session != NULL && *rpc_session != NULL)) {
    sqc_tls_conf_destroy((*rpc_session)->tls_conf_);
    sqc_session_destroy(&(*rpc_session)->session_);
    rpc_jwt_server_destroy_ctx(&(*rpc_session)->jwt_ctx_);
    free(*rpc_session);
    *rpc_session = NULL;
  }
}


//
// Get a user name of the client and its status.
//
static inline sqc_result_t
s_get_session_user(rpc_session_server_t *rpc_session, char **user_name, bool *enabled, bool *admin) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(rpc_session != NULL && *rpc_session != NULL && user_name != NULL)) {
    if (likely((*rpc_session)->user_info_ != NULL)) {
      ret = dbmgr_ui_get_user_id((*rpc_session)->user_info_, user_name);
      if (likely(ret == SQC_RESULT_OK)) {
        if (enabled != NULL) {
          *enabled = dbmgr_ui_is_user_enabled((*rpc_session)->user_info_);
        }
        if (admin != NULL) {
          *admin = dbmgr_ui_is_user_admin((*rpc_session)->user_info_);
        }
      }
    } else {
      ret = SQC_RESULT_AUTHENTICATION_ERROR;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_debug(5, "%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


//
// Set a user name of the client.
//
static inline sqc_result_t
s_set_session_user(rpc_session_server_t *rpc_session, const char *user_name) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  dbmgr_user_info_t user_info = NULL;

  if (rpc_session != NULL && *rpc_session != NULL && user_name != NULL) {
    ret = dbmgr_ui_user_find(user_name, &user_info);
    if (likely(ret == SQC_RESULT_OK)) {
      (*rpc_session)->user_info_ = user_info;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


//
// Set a user name of the client authenticated with mutual TLS.
//
static inline sqc_result_t
s_set_mutual_tls_session_user(rpc_session_server_t *rpc_session) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  const char *user_name = NULL;

  if (likely(rpc_session != NULL && *rpc_session != NULL)) {
    do {
      user_name = sqc_session_peer_subjectdn_oneline(&(*rpc_session)->session_);
      if (user_name != NULL) {
        if (likely(s_set_session_user(rpc_session, user_name) == SQC_RESULT_OK)) {
          ret = SQC_RESULT_OK;
          sqc_msg_debug(5, "subjectdn-oneline '%s' is a valid user, authenticated\n", user_name);
          break;
        } else {
          sqc_msg_debug(5, "subjectdn-oneline '%s' is not a valid user\n", user_name);
        }
      }

      user_name = sqc_session_peer_subjectdn_rfc2253(&(*rpc_session)->session_);
      if (user_name != NULL) {
        if (likely(s_set_session_user(rpc_session, user_name) == SQC_RESULT_OK)) {
          ret = SQC_RESULT_OK;
          sqc_msg_debug(5, "subjectdn-rfc2553 '%s' is a valid user, authenticated\n", user_name);
          break;
        } else {
          sqc_msg_debug(5, "subjectdn-rfc2553 '%s' is not a valid user\n", user_name);
        }
      }

      user_name = sqc_session_peer_subjectdn_gsi(&(*rpc_session)->session_);
      if (user_name != NULL) {
        if (likely(s_set_session_user(rpc_session, user_name) == SQC_RESULT_OK)) {
          ret = SQC_RESULT_OK;
          sqc_msg_debug(5, "subjectdn-gsi '%s' is a valid user, authenticated\n", user_name);
          break;
        } else {
          sqc_msg_debug(5, "subjectdn-gsi '%s' is not a valid user\n", user_name);
        }
      }

      user_name = sqc_session_peer_cn(&(*rpc_session)->session_);
      if (user_name != NULL) {
        if (likely(s_set_session_user(rpc_session, user_name) == SQC_RESULT_OK)) {
          ret = SQC_RESULT_OK;
          sqc_msg_debug(5, "cn '%s' is a valid user, authenticated\n", user_name);
          break;
        } else {
          sqc_msg_debug(5, "cn '%s' is not a valid user\n", user_name);
        }
      }

      ret = SQC_RESULT_INVALID_ARGS;
      sqc_msg_debug(5, "%s\n", sqc_error_get_string(ret));
      sqc_msg_debug(5, "no valid user in the TLS client key\n");
    } while (0);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_debug(5, "%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


//
// Handle a 'open_new_session_request' message.
//
static inline sqc_result_t
s_handle_open_new_session_request(rpc_session_server_t *rpc_session, rpc_msg_id_t id, char *body,
                                  size_t len, rpc_msg_id_t *reply_id, char **reply_body,
                                  size_t *reply_len, rpc_request_handler_flags_t *reply_flags) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t reply_code = SQC_RESULT_ANY_FAILURES;
  char *reply_msg = NULL;
  sqc_result_t pack_result = SQC_RESULT_ANY_FAILURES;
  OpenNewSessionRequest *request = NULL;
  rpc_auth_method_t auth_method = RPC_AUTH_METHOD_UNKNOWN;
  bool auth_method_uses_tls = false;
  (void) body;
  (void) len;

  if (likely(rpc_session != NULL && *rpc_session != NULL && reply_id != NULL &&
             reply_body != NULL && reply_len != NULL && reply_flags != NULL)) {
    request = open_new_session_request__unpack(NULL, len, (uint8_t *) body);

    if (likely(request != NULL)) {
      auth_method = (rpc_auth_method_t)request->auth_method;
      auth_method_uses_tls = rpc_auth_method_uses_tls(auth_method);
      sqc_msg_info("RPC-%s: Received a request: auth_method=%d(%s)\n",
                   rpc_message_name_string(id),
                   auth_method, rpc_auth_method_string(auth_method));
      if (likely(auth_method == RPC_AUTH_METHOD_MUTUAL_TLS ||
                 auth_method == RPC_AUTH_METHOD_MUNGE ||
                 auth_method == RPC_AUTH_METHOD_JWT)) {
        reply_code = SQC_RESULT_OK;
        (*rpc_session)->auth_method_ = auth_method;
        (*rpc_session)->user_info_ = NULL;
      } else {
        reply_code = SQC_RESULT_UNSUPPORTED;
        rpc_create_message_text(&reply_msg,
                                "Received a request with unsupported or unknown method:auth_method=%d(%s)\n",
                                auth_method, rpc_auth_method_string(auth_method));
        rpc_log_error_msg_with_name(id, reply_msg);
      }
    } else {
      reply_code = SQC_RESULT_INVALID_ARGS;
      rpc_create_message_text(&reply_msg, "Received a request with invalid arguments");
      rpc_log_error_msg_with_name(id, reply_msg);
    }

    pack_result = rpc_pack_open_new_session_reply(reply_code, reply_msg, reply_body, reply_len);

    if (likely(pack_result == SQC_RESULT_OK)) {
      *reply_id = RPC_MSG_OPEN_NEW_SESSION_REPLY;
      if (auth_method == RPC_AUTH_METHOD_MUTUAL_TLS) {
        *reply_flags = RPC_REQUEST_HANDLER_FLAG_TLS | RPC_REQUEST_HANDLER_FLAG_MUTUAL_TLS_AUTH;
      } else if (auth_method_uses_tls == true) {
        *reply_flags = RPC_REQUEST_HANDLER_FLAG_TLS;
      } else {
        *reply_flags = 0u;
      }
      ret = SQC_RESULT_OK;
      sqc_msg_info("RPC-%s: Send a reply: code=%d(%s), message=%s\n",
                   rpc_message_name_string(id),
                   (int) reply_code, sqc_error_get_string(reply_code), reply_msg);
    } else {
      ret = pack_result;
      sqc_msg_error("RPC-%s: Failed to construct a reply, %s\n",
                    rpc_message_name_string(id), sqc_error_get_string(ret));
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("RPC-%s: Failed to handle a request, %s\n",
                  rpc_message_name_string(id), sqc_error_get_string(ret));
  }

  free(reply_msg);
  open_new_session_request__free_unpacked(request, NULL);
  return ret;
}


//
// Handle a 'auth_request' message.
//
static inline sqc_result_t
s_handle_auth_request(rpc_session_server_t *rpc_session, rpc_msg_id_t id, char *body,
                      size_t len, rpc_msg_id_t *reply_id, char **reply_body, size_t *reply_len,
                      rpc_request_handler_flags_t *reply_flags) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t reply_code = SQC_RESULT_ANY_FAILURES;
  char *reply_msg = NULL;
  sqc_result_t pack_result = SQC_RESULT_ANY_FAILURES;
  AuthRequest *request = NULL;
  char *user_name = NULL;

  if (likely(rpc_session != NULL && *rpc_session != NULL && body != NULL && reply_id != NULL &&
             reply_body != NULL && reply_len != NULL && reply_flags != NULL)) {
    request = auth_request__unpack(NULL, len, (uint8_t *) body);

    if (likely(request != NULL)) {
      sqc_msg_info("RPC-%s: Received a request: auth_data=(%u bytes) %s\n",
                   rpc_message_name_string(id),
                   (unsigned int) request->data.len, (char *)request->data.data);

      switch ((*rpc_session)->auth_method_) {
        case RPC_AUTH_METHOD_MUTUAL_TLS: {
          if (likely(request->data.len == 0)) {
            reply_code = s_set_mutual_tls_session_user(rpc_session);
            if (reply_code == SQC_RESULT_OK) {
              sqc_msg_info("RPC-%s: Valid Mutual TLS certificate\n", rpc_message_name_string(id));
            } else {
              rpc_create_message_text(&reply_msg, "Invalid Mutual TLS certificate");
              rpc_log_error_msg_with_name(id, reply_msg);
            }
          } else {
            reply_code = SQC_RESULT_INVALID_ARGS;
            rpc_create_message_text(&reply_msg, "Received Mutual TLS auth request with invalid arguments");
            rpc_log_error_msg_with_name(id, reply_msg);
          }
          break;
        }
        case RPC_AUTH_METHOD_MUNGE: {
          sqc_result_t munge_result = SQC_RESULT_ANY_FAILURES;
          if (likely(RPC_VALIDATE_PROTOC_BYTES(&request->data) == true)) {
            munge_result = rpc_munge_server_validate_cred(rpc_session,
                                                          (char *)request->data.data,
                                                          (*rpc_session)->conn_id_,
                                                          &user_name);
            if (likely(munge_result == SQC_RESULT_OK)) {
              reply_code = s_set_session_user(rpc_session, user_name);
              if (likely(reply_code == SQC_RESULT_OK)) {
                sqc_msg_info("RPC-%s: Valid MUNGE credential, user=%s\n",
                             rpc_message_name_string(id), user_name);
              } else {
                rpc_create_message_text(&reply_msg, "Invalid MUNGE credential, user=%s", user_name);
                rpc_log_error_msg_with_name(id, reply_msg);
              }
            } else {
              reply_code = munge_result;
              rpc_create_message_text(&reply_msg, "Failed to validate a MUNGE credential, %s",
                                      sqc_error_get_string(reply_code));
              rpc_log_error_msg_with_name(id, reply_msg);
            }
          } else {
            reply_code = SQC_RESULT_INVALID_ARGS;
            rpc_create_message_text(&reply_msg, "Received MUNGE auth request with invalid arguments");
            rpc_log_error_msg_with_name(id, reply_msg);
          }
          break;
        }
        case RPC_AUTH_METHOD_JWT: {
          sqc_result_t jwt_result = SQC_RESULT_ANY_FAILURES;
          if (likely(RPC_VALIDATE_PROTOC_BYTES(&request->data) == true)) {
            rpc_strip_text((char *)request->data.data);
            jwt_result = rpc_jwt_server_validate_token(&(*rpc_session)->jwt_ctx_,
                                                       (char *)request->data.data,
                                                       (*rpc_session)->conn_id_,
                                                       &user_name);
            if (likely(jwt_result == SQC_RESULT_OK)) {
              reply_code = s_set_session_user(rpc_session, user_name);
              if (likely(reply_code == SQC_RESULT_OK)) {
                sqc_msg_info("RPC-%s: Valid JWT: subject=%s\n", rpc_message_name_string(id), user_name);
              } else {
                rpc_create_message_text(&reply_msg, "Invalid JWT: subject=%s", user_name);
                rpc_log_error_msg_with_name(id, reply_msg);
              }
            } else {
              reply_code = jwt_result;
              rpc_create_message_text(&reply_msg, "Failed to validate JWT, %s",
                                      sqc_error_get_string(reply_code));
              rpc_log_error_msg_with_name(id, reply_msg);
            }
          } else {
            reply_code = SQC_RESULT_INVALID_ARGS;
            rpc_create_message_text(&reply_msg, "Received JWT auth request with invalid arguments");
            rpc_log_error_msg_with_name(id, reply_msg);
          }
          break;
        }
        default: {
          reply_code = SQC_RESULT_UNSUPPORTED;
          rpc_create_message_text(&reply_msg, "Received an auth request of an unsupported method");
          rpc_log_error_msg_with_name(id, reply_msg);
          break;
        }
      }
    } else {
      reply_code = SQC_RESULT_INVALID_ARGS;
      rpc_create_message_text(&reply_msg, "Received a request with invalid arguments");
      rpc_log_error_msg_with_name(id, reply_msg);
    }

    pack_result = rpc_pack_auth_reply(reply_code, reply_msg, reply_body, reply_len);

    if (likely(pack_result == SQC_RESULT_OK)) {
      *reply_id = RPC_MSG_AUTH_REPLY;
      ret = SQC_RESULT_OK;
      sqc_msg_info("RPC-%s: Send a reply: code=%d(%s), message=%s\n",
                   rpc_message_name_string(id), (int) reply_code,
                   sqc_error_get_string(reply_code), reply_msg);
    } else {
      ret = pack_result;
      sqc_msg_error("RPC-%s: Failed to construct a reply, %s\n",
                    rpc_message_name_string(id), sqc_error_get_string(ret));
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("RPC-%s: Failed to handle a request, %s\n",
                  rpc_message_name_string(id), sqc_error_get_string(ret));
  }

  free(reply_msg);
  free(user_name);
  auth_request__free_unpacked(request, NULL);
  return ret;
}


//
// Request DB manager to submit a job.
//
static inline sqc_result_t
s_submit_job(const char *user_id, uint8_t priority,
             const char *qprogram, sqc_rpc_sched_circuit_fmt_t circuit_fmt, size_t shots,
             sqc_rpc_sched_qc_type_t qc_type, sqc_rpc_sched_transpiler_t transpiler,
             const char *remark, const char *user_token, char **job_id, char **reply_msg) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  dbmgr_job_info_t ji_ptr = NULL;

  if (likely(qc_type == sqc_rpc_sched_conf_get_qc_type())) {
    rc = dbmgr_ji_create_job(user_id, priority, qprogram, circuit_fmt, shots,
                             qc_type, transpiler, remark, user_token, &ji_ptr);
    if (rc == SQC_RESULT_OK) {
      rc = dbmgr_ji_get_job_id(ji_ptr, job_id);
      if (rc == SQC_RESULT_OK) {
        sqc_msg_debug(5, "Created the job: job_id=%s\n", *job_id);

        rc = req_sched_enqueue(ji_ptr);
        if (rc == SQC_RESULT_OK) {
          sqc_msg_debug(5, "Enqueued the job: job_id=%s\n", *job_id);
        } else {
          rpc_create_message_text(reply_msg, "Failed to enqueue the job to scheduler, %s", sqc_error_get_string(rc));
          rpc_log_debug_msg(5, *reply_msg);
        }
      } else {
        rpc_create_message_text(reply_msg, "Failed to get a created job ID, %s", sqc_error_get_string(rc));
        rpc_log_debug_msg(5, *reply_msg);
      }
    } else {
      rpc_create_message_text(reply_msg, "Failed to create a job, job_id=%s", sqc_error_get_string(rc));
      rpc_log_debug_msg(5, *reply_msg);
    }
  } else {
    rc = SQC_RESULT_INVALID_ARGS;
    rpc_create_message_text(reply_msg, "Unexpected qc-type %d(%s)",
                            (int)qc_type, sqc_rpc_sched_qc_type_to_string(qc_type));
    rpc_log_error_msg(*reply_msg);
  }

  return rc;
}


//
// Handle an 'submit_job_request' message.
//
// The function processes the received request and builds a reply message.
//
static inline sqc_result_t
s_handle_submit_job_request(rpc_session_server_t *rpc_session, rpc_msg_id_t id, char *body,
                            size_t len, rpc_msg_id_t *reply_id, char **reply_body,
                            size_t *reply_len, rpc_request_handler_flags_t *reply_flags) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t get_user_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t reply_code = SQC_RESULT_ANY_FAILURES;
  char *reply_msg = NULL;
  sqc_result_t pack_result = SQC_RESULT_ANY_FAILURES;
  SubmitJobRequest *request = NULL;
  char *session_user_name = NULL;
  bool is_user_enabled = false;
  char *job_id = NULL;

  if (likely(rpc_session != NULL && *rpc_session != NULL && body != NULL && reply_id != NULL &&
             reply_body != NULL && reply_len != NULL && reply_flags != NULL)) {
    request = submit_job_request__unpack(NULL, len, (uint8_t *) body);

    if (likely(request != NULL &&
               RPC_VALIDATE_PROTOC_BYTES(&request->qprogram) == true &&
               RPC_VALIDATE_PROTOC_BYTES(&request->remark) == true)) {
      sqc_msg_info("RPC-%s: Received a request: priority=%u, qprogram_len=%d, circuit_fmt=%d(%s), "
                   "shots=%zu, qc_type=%d(%s), transpiler=%d(%s), remark=%s, has_user_token=%s\n",
                   rpc_message_name_string(id),
                   (unsigned int) request->priority,
                   (int) (&request->qprogram)->len,
                   (int) request->circuit_fmt,
                   sqc_rpc_sched_circuit_fmt_to_string((sqc_rpc_sched_circuit_fmt_t)request->circuit_fmt),
                   (size_t) request->shots,
                   (int) request->qc_type,
                   sqc_rpc_sched_qc_type_to_string((sqc_rpc_sched_qc_type_t)request->qc_type),
                   (int) request->transpiler,
                   sqc_rpc_sched_transpiler_to_string((sqc_rpc_sched_transpiler_t)request->transpiler),
                   (char *) request->remark.data, request->has_user_token ? "true" : "false");

      get_user_result = s_get_session_user(rpc_session, &session_user_name, &is_user_enabled, NULL);
      if (likely(get_user_result == SQC_RESULT_OK)) {
        if (likely(is_user_enabled)) {
          reply_code = s_submit_job(session_user_name, (uint8_t) request->priority,
                                    (char *) request->qprogram.data,
                                    (sqc_rpc_sched_circuit_fmt_t) request->circuit_fmt, (size_t) request->shots,
                                    (sqc_rpc_sched_qc_type_t) request->qc_type,
                                    (sqc_rpc_sched_transpiler_t) request->transpiler,
                                    (char *) request->remark.data, (char *) request->user_token.data,
                                    &job_id, &reply_msg);
          if (likely(reply_code == SQC_RESULT_OK)) {
            sqc_msg_info("RPC-%s: Submitted: job_id=%s\n", rpc_message_name_string(id), job_id);
          } else {
            rpc_log_error_msg_with_name(id, reply_msg);
          }
        } else {
          reply_code = SQC_RESULT_DISABLED_USER;
          rpc_create_message_text(&reply_msg, "Requested by the disabled user: user=%s",
                                  session_user_name);
          rpc_log_error_msg_with_name(id, reply_msg);
        }
      } else {
        rpc_create_message_text(&reply_msg, "Failed to get a user name of the session, %s",
                                sqc_error_get_string(get_user_result));
        rpc_log_error_msg_with_name(id, reply_msg);
      }
    } else {
      reply_code = SQC_RESULT_INVALID_ARGS;
      rpc_create_message_text(&reply_msg, "Received a request with invalid arguments");
      rpc_log_error_msg_with_name(id, reply_msg);
    }

    pack_result = rpc_pack_submit_job_reply(reply_code, reply_msg, job_id, reply_body, reply_len);

    if (likely(pack_result == SQC_RESULT_OK)) {
      *reply_id = RPC_MSG_SUBMIT_JOB_REPLY;
      *reply_flags = 0u;
      ret = SQC_RESULT_OK;
      if (job_id == NULL) {
        sqc_msg_info("RPC-%s: Send a reply: code=%d(%s), message=%s\n",
                     rpc_message_name_string(id),
                     (int) reply_code, sqc_error_get_string(reply_code), reply_msg);
      } else {
        sqc_msg_info("RPC-%s: Send a reply: code=%d(%s), message=%s, job_id=%s\n",
                     rpc_message_name_string(id),
                     (int) reply_code, sqc_error_get_string(reply_code), reply_msg, job_id);
      }
    } else {
      ret = pack_result;
      sqc_msg_error("RPC-%s: Faild to construct a reply, %s\n",
                    rpc_message_name_string(id), sqc_error_get_string(ret));
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("RPC-%s: Failed to handle a request, %s\n",
                  rpc_message_name_string(id), sqc_error_get_string(ret));
  }

  free(reply_msg);
  free(job_id);
  free(session_user_name);
  submit_job_request__free_unpacked(request, NULL);
  return ret;
}


//
// Get a record of the specified job.
// It also check if the user name given by the client is equivalent with that of
// the submitted job.
//
static inline sqc_result_t
s_get_submitted_job(const char *session_user_name, const char* job_id, dbmgr_job_info_t* ji_ptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  sqc_result_t find_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t get_result = SQC_RESULT_ANY_FAILURES;
  dbmgr_job_info_t tmp_ji_ptr = NULL;
  char *job_owner = NULL;

  if (likely(session_user_name != NULL && ji_ptr != NULL && job_id != NULL)) {
    find_result = dbmgr_ji_job_find(job_id, &tmp_ji_ptr);
    if (find_result == SQC_RESULT_OK) {
      get_result = dbmgr_ji_get_user_id(tmp_ji_ptr, &job_owner);
      if (likely(get_result == SQC_RESULT_OK)) {
        if (likely(strcmp(job_owner, session_user_name) == 0)) {
          *ji_ptr = tmp_ji_ptr;
          sqc_msg_debug(5, "Accessed the job: job_id=%s, job_owner=%s, requested_user=%s\n",
                        job_id, job_owner, session_user_name);
          rc = SQC_RESULT_OK;
        } else {
          sqc_msg_debug(5, "Denied to access the job: job_id=%s, job_owner=%s, requested_user=%s\n",
                        job_id, job_owner, session_user_name);
          rc = SQC_RESULT_NOT_OWNER;
        }
      } else {
        sqc_msg_debug(5, "Failed to get a user of the job, %s: job_id=%s\n",
                      sqc_error_get_string(get_result), job_id);
        rc = get_result;
      }
    } else {
      sqc_msg_debug(5, "Failed to get a record of the job, %s: job_id=%s\n",
                    sqc_error_get_string(find_result), job_id);
      rc = find_result;
    }
  } else {
    rc = SQC_RESULT_INVALID_ARGS;
    sqc_msg_debug(5, "%s\n", sqc_error_get_string(rc));
  }

  free(job_owner);
  return rc;
}


//
// Retrieve status of the specified job.
//
static inline sqc_result_t
s_get_job_status(const char *session_user_name, const char *job_id, sqc_rpc_sched_job_status_t *status,
                 char** qc_job_id, char** result, char** reply_msg) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  sqc_result_t get_job_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t get_status_result = SQC_RESULT_ANY_FAILURES;
  dbmgr_job_info_t ji_ptr = NULL;
  sqc_rpc_sched_job_status_t job_status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;

  if (likely(session_user_name != NULL && job_id != NULL && status != NULL && qc_job_id != NULL && result != NULL)) {
    get_job_result = s_get_submitted_job(session_user_name, job_id, &ji_ptr);
    if (likely(get_job_result == SQC_RESULT_OK)) {
      get_status_result = dbmgr_ji_get_status(ji_ptr, &job_status);
      if (likely(get_status_result == SQC_RESULT_OK)) {
        sqc_msg_debug(5, "Got job status: job_id=%s, status=%d\n", job_id, job_status);
        *status = job_status;

        // get QC Job ID
        if (job_status == SQC_RPC_SCHED_JOB_STATUS_RUNNING ||
            job_status == SQC_RPC_SCHED_JOB_STATUS_DONE ||
            job_status == SQC_RPC_SCHED_JOB_STATUS_CANCELLED ||
            job_status == SQC_RPC_SCHED_JOB_STATUS_ERROR) {
          rc = dbmgr_ji_get_qc_job_id(ji_ptr, qc_job_id);
          if (likely(rc == SQC_RESULT_OK)) {
            sqc_msg_debug(5, "Got qc_job_id of the job: job_id=%s, qc_job_id=%s\n",
                          job_id, *qc_job_id);
          } else {
            // when the retrieval of QC Job ID fails, only output a warning log.
            sqc_msg_warning("Failed to get qc_job_id of the job, %s: job_id=%s\n",
                            sqc_error_get_string(rc), job_id);
          }
        } else {
          rc = SQC_RESULT_OK;
        }

        // get result
        if (job_status == SQC_RPC_SCHED_JOB_STATUS_DONE) {
          rc = dbmgr_ji_get_result(ji_ptr, result);
          if (likely(rc == SQC_RESULT_OK)) {
            sqc_msg_debug(5, "Got result of the job: job_id=%s\n", job_id);
          } else {
            rpc_create_message_text(reply_msg, "Failed to get result of the job, %s: job_id=%s",
                                    sqc_error_get_string(rc), job_id);
            rpc_log_debug_msg(5, *reply_msg);
          }
        } else {
          rc = SQC_RESULT_OK;
        }
      } else {
        rc = get_status_result;
        rpc_create_message_text(reply_msg, "Failed to get status of the job, %s: job_id=%s",
                                sqc_error_get_string(rc), job_id);
        rpc_log_debug_msg(5, *reply_msg);
      }

    } else {
      rc = get_job_result;
      rpc_create_message_text(reply_msg, "Failed to get a DB record of the job, %s: job_id=%s",
                              sqc_error_get_string(rc), job_id);
      rpc_log_debug_msg(5, *reply_msg);
    }
  } else {
    rc = SQC_RESULT_INVALID_ARGS;
    rpc_create_message_text(reply_msg, "Failed to get job status, %s",
                            sqc_error_get_string(rc));
    rpc_log_debug_msg(5, *reply_msg);
  }

  return rc;
}


//
// Handle a 'job_status_request' message.
//
// The function processes the received request and builds a reply message.
//
static inline sqc_result_t
s_handle_job_status_request(rpc_session_server_t *rpc_session, rpc_msg_id_t id, char *body,
                            size_t len, rpc_msg_id_t *reply_id, char **reply_body,
                            size_t *reply_len, rpc_request_handler_flags_t *reply_flags) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t get_user_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t reply_code = SQC_RESULT_ANY_FAILURES;
  char *reply_msg = NULL;
  sqc_result_t pack_result = SQC_RESULT_ANY_FAILURES;
  sqc_rpc_sched_job_status_t status = SQC_RPC_SCHED_JOB_STATUS_UNKNOWN;
  JobStatusRequest *request = NULL;
  char *session_user_name = NULL;
  bool is_user_enabled = false;
  char *qc_job_id = NULL;
  char *qc_result = NULL;

  if (likely(rpc_session != NULL && *rpc_session != NULL && body != NULL && reply_id != NULL &&
             reply_body != NULL && reply_len != NULL && reply_flags != NULL)) {
    request = job_status_request__unpack(NULL, len, (uint8_t *) body);

    if (likely(request != NULL && RPC_VALIDATE_PROTOC_BYTES(&request->job_id) == true)) {
      sqc_msg_info("RPC-%s: Received a request: job_id=%s\n",
                   rpc_message_name_string(id), (char *) request->job_id.data);
      get_user_result = s_get_session_user(rpc_session, &session_user_name, &is_user_enabled, NULL);
      if (likely(get_user_result == SQC_RESULT_OK)) {
        if (likely(is_user_enabled)) {
          reply_code = s_get_job_status(session_user_name, (char *) request->job_id.data, &status,
                                        &qc_job_id, &qc_result, &reply_msg);
          if (likely(reply_code == SQC_RESULT_OK)) {
            sqc_msg_info("RPC-%s: Got job status: job_id=%s, status=%d(%s)\n",
                         rpc_message_name_string(id), (char *) request->job_id.data,
                         status, sqc_rpc_sched_job_status_to_string(status));
          } else {
            rpc_log_error_msg_with_name(id, reply_msg);
          }
        } else {
          reply_code = SQC_RESULT_DISABLED_USER;
          rpc_create_message_text(&reply_msg, "Requested by the disabled user: job_id=%s, user=%s",
                                  (char *) request->job_id.data, session_user_name);
          rpc_log_error_msg_with_name(id, reply_msg);
        }
      } else {
        reply_code = get_user_result;
        rpc_create_message_text(&reply_msg, "Failed to get a user name of the session: job_id=%s",
                                (char *) request->job_id.data);
        rpc_log_error_msg_with_name(id, reply_msg);
      }
    } else {
      reply_code = SQC_RESULT_INVALID_ARGS;
      rpc_create_message_text(&reply_msg, "Received a request with invalid arguments");
      rpc_log_error_msg_with_name(id, reply_msg);
    }

    pack_result = rpc_pack_job_status_reply(reply_code, reply_msg, status, qc_job_id, qc_result,
                                            reply_body, reply_len);

    if (likely(pack_result == SQC_RESULT_OK)) {
      *reply_id = RPC_MSG_JOB_STATUS_REPLY;
      *reply_flags = 0u;
      ret = SQC_RESULT_OK;
      if (qc_result == NULL) {
        sqc_msg_info("RPC-%s: Send a reply: code=%d(%s), message=%s, status=%d(%s)\n",
                     rpc_message_name_string(id),
                     (int) reply_code,
                     sqc_error_get_string(reply_code),
                     reply_msg,
                     (int) status,
                     sqc_rpc_sched_job_status_to_string(status));
      } else {
        sqc_msg_info("RPC-%s: Send a reply: code=%d(%s), message=%s, status=%d(%s), "
                     "qc_job_id=%s, result=%s\n",
                     rpc_message_name_string(id),
                     (int) reply_code,
                     sqc_error_get_string(reply_code),
                     reply_msg,
                     (int) status,
                     sqc_rpc_sched_job_status_to_string(status),
                     (char *) qc_job_id, (char *) qc_result);
      }
    } else {
      ret = pack_result;
      sqc_msg_error("RPC-%s: Failed to construct a reply, %s\n",
                    rpc_message_name_string(id),
                    sqc_error_get_string(ret));
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("RPC-%s: Failed to handle a request, %s\n",
                  rpc_message_name_string(id),
                  sqc_error_get_string(ret));
  }

  free(reply_msg);
  free(session_user_name);
  free(qc_job_id);
  free(qc_result);
  job_status_request__free_unpacked(request, NULL);
  return ret;
}


//
// Cancel the specified job.
//
static inline sqc_result_t
s_cancel_job(const char *session_user_name, const char *job_id, char** reply_msg) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  sqc_result_t get_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t cancel_result = SQC_RESULT_ANY_FAILURES;
  dbmgr_job_info_t ji_ptr = NULL;

  if (likely(session_user_name != NULL && job_id != NULL)) {
    get_result = s_get_submitted_job(session_user_name, job_id, &ji_ptr);
    if (get_result == SQC_RESULT_OK) {
      cancel_result = dbmgr_set_job_status_cancelled(ji_ptr);

      if (likely(cancel_result == SQC_RESULT_OK)) {
        sqc_msg_debug(5, "Cancelled the job: job_id=%s\n", job_id);
        rc = SQC_RESULT_OK;
      } else {
        rc = cancel_result;
        rpc_create_message_text(reply_msg, "Failed to cancel the job, %s: job_id=%s",
                                sqc_error_get_string(rc), job_id);
        rpc_log_debug_msg(5, *reply_msg);
      }
    } else {
      rc = get_result;
      rpc_create_message_text(reply_msg, "Failed to get a record of the job, %s: job_id=%s",
                              sqc_error_get_string(rc), job_id);
      rpc_log_debug_msg(5, *reply_msg);
    }
  } else {
    rc = SQC_RESULT_INVALID_ARGS;
    rpc_create_message_text(reply_msg, "Failed to cancel the job, %s: job_id=%s",
                            sqc_error_get_string(rc), job_id);
    rpc_log_debug_msg(5, *reply_msg);
  }
  return rc;
}


//
// Handle a 'cancel_job_request' message.
//
// The function processes the received request and builds a reply message.
//
static inline sqc_result_t
s_handle_cancel_job_request(rpc_session_server_t *rpc_session, rpc_msg_id_t id, char *body,
                            size_t len, rpc_msg_id_t *reply_id, char **reply_body,
                            size_t *reply_len, rpc_request_handler_flags_t *reply_flags) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t reply_code = SQC_RESULT_ANY_FAILURES;
  char *reply_msg = NULL;
  sqc_result_t get_user_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t pack_result = SQC_RESULT_ANY_FAILURES;
  CancelJobRequest *request = NULL;
  char *session_user_name = NULL;
  bool is_user_enabled = false;

  if (likely(rpc_session != NULL && *rpc_session != NULL && body != NULL && reply_id != NULL &&
             reply_body != NULL && reply_len != NULL && reply_flags != NULL)) {
    request = cancel_job_request__unpack(NULL, len, (uint8_t *) body);

    if (likely(request != NULL && RPC_VALIDATE_PROTOC_BYTES(&request->job_id) == true)) {
      sqc_msg_info("RPC-%s: Received a request: job_id=%s\n",
                   rpc_message_name_string(id),
                   (char *) request->job_id.data);
      get_user_result = s_get_session_user(rpc_session, &session_user_name, &is_user_enabled, NULL);
      if (likely(is_user_enabled)) {
        if (likely(get_user_result == SQC_RESULT_OK)) {
          reply_code = s_cancel_job(session_user_name, (char *) request->job_id.data, &reply_msg);
          if (likely(reply_code == SQC_RESULT_OK)) {
            sqc_msg_info("RPC-%s: Cancelled: job_id=%s\n",
                         rpc_message_name_string(id), (char *) request->job_id.data);
          } else {
            rpc_log_error_msg_with_name(id, reply_msg);
          }
        } else {
          reply_code = SQC_RESULT_DISABLED_USER;
          rpc_create_message_text(&reply_msg, "Requested by the disabled user: user=%s",
                                  session_user_name);
          rpc_log_error_msg_with_name(id, reply_msg);
        }
      } else {
        reply_code = get_user_result;
        rpc_create_message_text(&reply_msg, "Failed to get a user name of the session, %s",
                                sqc_error_get_string(reply_code));
        rpc_log_error_msg_with_name(id, reply_msg);
      }
    } else {
      reply_code = SQC_RESULT_INVALID_ARGS;
      rpc_create_message_text(&reply_msg, "Received a request with invalid arguments");
      rpc_log_error_msg_with_name(id, reply_msg);
    }

    pack_result = rpc_pack_cancel_job_reply(reply_code, reply_msg, reply_body, reply_len);

    if (likely(pack_result == SQC_RESULT_OK)) {
      *reply_id = RPC_MSG_CANCEL_JOB_REPLY;
      *reply_flags = 0u;
      ret = SQC_RESULT_OK;
      sqc_msg_info("RPC-%s: Send a reply: code=%d(%s), message=%s\n",
                   rpc_message_name_string(id), (int) reply_code,
                   sqc_error_get_string(reply_code), reply_msg);
    } else {
      ret = pack_result;
      sqc_msg_error("RPC-%s: Failed to construct a reply, %s\n",
                    rpc_message_name_string(id), sqc_error_get_string(ret));
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("RPC-%s: Failed to handle a request, %s\n",
                  rpc_message_name_string(id), sqc_error_get_string(ret));
  }

  free(reply_msg);
  free(session_user_name);
  cancel_job_request__free_unpacked(request, NULL);
  return ret;
}


//
// Delete the specified job.
//
static inline sqc_result_t
s_delete_job(const char *session_user_name, const char *job_id, char** reply_msg) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  dbmgr_job_info_t ji_ptr = NULL;
  sqc_rpc_sched_job_status_t status;

  do {
    if (session_user_name == NULL || job_id == NULL) {
      rc = SQC_RESULT_INVALID_ARGS;
      sqc_msg_debug(5, "%s\n", sqc_error_get_string(rc));
      break;
    }

    rc = s_get_submitted_job(session_user_name, job_id, &ji_ptr);
    if (rc != SQC_RESULT_OK) {
      rpc_create_message_text(reply_msg, "Failed to get a record of the job, %s: job_id=%s",
                              sqc_error_get_string(rc), job_id);
      rpc_log_debug_msg(5, *reply_msg);
      break;
    }

    rc = dbmgr_ji_get_status(ji_ptr, &status);
    if (rc != SQC_RESULT_OK) {
      rpc_create_message_text(reply_msg, "Failed to get status of the job, %s: job_id=%s",
                              sqc_error_get_string(rc), job_id);
      rpc_log_debug_msg(5, *reply_msg);
      break;
    }

    if (status == SQC_RPC_SCHED_JOB_STATUS_DELETED) {
      // Do nothing since the jobs has already been deleted.
      rc = SQC_RESULT_OK;
      break;
    }

    if (!dbmgr_ji_is_deletable(ji_ptr)) {
      rc = SQC_RESULT_INVALID_STATE;
      rpc_create_message_text(reply_msg, "The job is not deletable, %s: job_id=%s",
                              sqc_error_get_string(rc), job_id);
      rpc_log_debug_msg(5, *reply_msg);
      break;
    }

    rc = dbmgr_ji_clear_job_result(ji_ptr);
    if (rc != SQC_RESULT_OK) {
      rpc_create_message_text(reply_msg, "Failed to clear the job result, %s: job_id=%s",
                              sqc_error_get_string(rc), job_id);
      rpc_log_debug_msg(5, *reply_msg);
      break;
    }

    rc = SQC_RESULT_OK;
  } while (0);

  return rc;
}


//
// Handle a 'delete_job_request' message.
//
// The function processes the received request and builds a reply message.
//
static inline sqc_result_t
s_handle_delete_job_request(rpc_session_server_t *rpc_session, rpc_msg_id_t id, char *body,
                            size_t len, rpc_msg_id_t *reply_id, char **reply_body,
                            size_t *reply_len, rpc_request_handler_flags_t *reply_flags) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t reply_code = SQC_RESULT_ANY_FAILURES;
  char *reply_msg = NULL;
  sqc_result_t get_user_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t pack_result = SQC_RESULT_ANY_FAILURES;
  DeleteJobRequest *request = NULL;
  char *session_user_name = NULL;
  bool is_user_enabled = false;

  if (likely(rpc_session != NULL && *rpc_session != NULL && body != NULL && reply_id != NULL &&
             reply_body != NULL && reply_len != NULL && reply_flags != NULL)) {
    request = delete_job_request__unpack(NULL, len, (uint8_t *) body);

    if (likely(request != NULL && RPC_VALIDATE_PROTOC_BYTES(&request->job_id) == true)) {
      sqc_msg_info("RPC-%s: Received a request: job_id=%s\n",
                   rpc_message_name_string(id),
                   (char *) request->job_id.data);
      get_user_result = s_get_session_user(rpc_session, &session_user_name, &is_user_enabled, NULL);
      if (likely(get_user_result == SQC_RESULT_OK)) {
        if (likely(is_user_enabled)) {
          reply_code = s_delete_job(session_user_name, (char *) request->job_id.data, &reply_msg);
          if (likely(reply_code == SQC_RESULT_OK)) {
            sqc_msg_info("RPC-%s: Deleted: job_id=%s\n",
                         rpc_message_name_string(id), (char *) request->job_id.data);
          } else {
            rpc_log_error_msg_with_name(id, reply_msg);
          }
        } else {
          reply_code = SQC_RESULT_DISABLED_USER;
          rpc_create_message_text(&reply_msg, "Requested by the disabled user: user=%s",
                                  session_user_name);
          rpc_log_error_msg_with_name(id, reply_msg);
        }
      } else {
        reply_code = get_user_result;
        rpc_create_message_text(&reply_msg, "Failed to get a user name of the session, %s",
                                sqc_error_get_string(reply_code));
        rpc_log_error_msg_with_name(id, reply_msg);
      }
    } else {
      reply_code = SQC_RESULT_INVALID_ARGS;
      rpc_create_message_text(&reply_msg, "Received a request with invalid arguments");
      rpc_log_error_msg_with_name(id, reply_msg);
    }

    pack_result = rpc_pack_delete_job_reply(reply_code, reply_msg, reply_body, reply_len);

    if (likely(pack_result == SQC_RESULT_OK)) {
      *reply_id = RPC_MSG_DELETE_JOB_REPLY;
      *reply_flags = 0u;
      ret = SQC_RESULT_OK;
      sqc_msg_info("RPC-%s: Send a reply: code=%d(%s), message=%s\n",
                   rpc_message_name_string(id), (int) reply_code,
                   sqc_error_get_string(reply_code), reply_msg);
    } else {
      ret = pack_result;
      sqc_msg_error("RPC-%s: Failed to construct a reply, %s\n",
                    rpc_message_name_string(id), sqc_error_get_string(ret));
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("RPC-%s: Failed to handle a request, %s\n",
                  rpc_message_name_string(id), sqc_error_get_string(ret));
  }

  free(reply_msg);
  free(session_user_name);
  delete_job_request__free_unpacked(request, NULL);
  return ret;
}


//
// Handle a 'job_list_request' message.
//
// The function processes the received request and builds a reply message.
//
static inline sqc_result_t
s_handle_job_list_request(rpc_session_server_t *rpc_session, rpc_msg_id_t id, char *body,
                          size_t len, rpc_msg_id_t *reply_id, char **reply_body,
                          size_t *reply_len, rpc_request_handler_flags_t *reply_flags) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t reply_code = SQC_RESULT_ANY_FAILURES;
  char *reply_msg = NULL;
  sqc_result_t get_user_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t pack_result = SQC_RESULT_ANY_FAILURES;
  char *session_user_name = NULL;
  bool is_user_enabled = false;
  dbmgr_job_info_t *jobs = NULL;
  size_t n_jobs = 0;
  (void) body;

  if (likely(rpc_session != NULL && *rpc_session != NULL && reply_id != NULL &&
             reply_body != NULL && reply_len != NULL && reply_flags != NULL)) {

    if (likely(len == 0)) {
      sqc_msg_info("RPC-%s: Received a request\n", rpc_message_name_string(id));
      get_user_result = s_get_session_user(rpc_session, &session_user_name, &is_user_enabled, NULL);

      if (likely(get_user_result == SQC_RESULT_OK)) {
        if (likely(is_user_enabled)) {
          reply_code = dbmgr_ji_job_find_by_user_id(session_user_name, &jobs, &n_jobs);
          if (likely(reply_code == SQC_RESULT_OK)) {
            sqc_msg_info("RPC-%s: Got status of submitted %zu job(s)\n", rpc_message_name_string(id), n_jobs);
          } else {
            rpc_create_message_text(&reply_msg, "Failed to get status of submitted jobs, %s",
                                    sqc_error_get_string(reply_code));
            rpc_log_error_msg_with_name(id, reply_msg);
          }
        } else {
          reply_code = SQC_RESULT_DISABLED_USER;
          rpc_create_message_text(&reply_msg, "Requested by the disabled user: user=%s",
                                  session_user_name);
          rpc_log_error_msg_with_name(id, reply_msg);
        }
      } else {
        reply_code = get_user_result;
        rpc_create_message_text(&reply_msg, "Failed to get a user name of the session, %s",
                                sqc_error_get_string(get_user_result));
        rpc_log_error_msg_with_name(id, reply_msg);
      }
    } else {
      reply_code = SQC_RESULT_INVALID_ARGS;
      rpc_create_message_text(&reply_msg, "Received a request with invalid arguments");
      rpc_log_error_msg_with_name(id, reply_msg);
    }

    pack_result = rpc_pack_job_list_reply(reply_code, reply_msg, jobs, n_jobs, reply_body, reply_len);
    if (likely(pack_result == SQC_RESULT_OK)) {
      *reply_id = RPC_MSG_JOB_LIST_REPLY;
      *reply_flags = 0u;
      ret = SQC_RESULT_OK;
      sqc_msg_info("RPC-%s: Send a reply: code=%d(%s), message=%s\n",
                   rpc_message_name_string(id), (int) reply_code,
                   sqc_error_get_string(reply_code), reply_msg);
    } else {
      ret = pack_result;
      sqc_msg_error("RPC-%s: Failed to construct a reply, %s\n",
                    rpc_message_name_string(id), sqc_error_get_string(ret));
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("RPC-%s: Failed to handle a request, %s\n",
                  rpc_message_name_string(id), sqc_error_get_string(ret));
  }

  free(reply_msg);
  free(session_user_name);
  free(jobs);
  return ret;
}


//
// Delete jobs by administrator privilege.
//
static inline sqc_result_t
s_adm_del_jobs(const char *user_id, sqc_chrono_t from_time, sqc_chrono_t to_time,
               size_t *n_jobs, char **reply_msg) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  dbmgr_user_info_t user_info = NULL;
  dbmgr_job_info_t *ji_ptr_arr = NULL;
  size_t arr_len = 0u;
  const char *target_user_id = NULL;

  // Check arguments.
  if (user_id == NULL) {
    rc = SQC_RESULT_INVALID_ARGS;
    rpc_create_message_text(reply_msg, "Failed to delete user jobs, %s", sqc_error_get_string(rc));
    rpc_log_debug_msg(5, *reply_msg);
    return rc;
  }

  do {
    // Get a list of target jobs.
    if (strcmp(user_id, all_users) != 0) {
      rc = dbmgr_ui_user_find(user_id, &user_info);
      if (rc != SQC_RESULT_OK) {
        rpc_create_message_text(reply_msg, "Failed to get information of the user %s: user=%s",
                                sqc_error_get_string(rc), user_id);
        rpc_log_debug_msg(5, *reply_msg);
        break;
      }
      target_user_id = user_id;
    }

    rc = dbmgr_ji_job_find_by_delete_target(target_user_id, from_time, to_time, &ji_ptr_arr, &arr_len);
    if (rc != SQC_RESULT_OK) {
      rpc_create_message_text(reply_msg, "Failed to get a list of user jobs, %s",
                              sqc_error_get_string(rc));
      rpc_log_debug_msg(5, *reply_msg);
      break;
    }

    // Delete the target jobs if no serious error has occurred.
    rc = dbmgr_ji_clear_job_arr_result(ji_ptr_arr, arr_len);
    if (rc != SQC_RESULT_OK) {
      rpc_create_message_text(reply_msg, "Failed to delete user jobs, %s", sqc_error_get_string(rc));
      rpc_log_debug_msg(5, *reply_msg);
      break;
    }

    rc = SQC_RESULT_OK;
    if (n_jobs != NULL) {
      *n_jobs = arr_len;
    }
  } while (0);

  free(ji_ptr_arr);
  return rc;
}


//
// Handle a 'adm_del_jobs_request' message.
//
// The function processes the received request and builds a reply message.
//
static inline sqc_result_t
s_handle_adm_del_jobs_request(rpc_session_server_t *rpc_session, rpc_msg_id_t id, char *body,
                              size_t len, rpc_msg_id_t *reply_id, char **reply_body,
                              size_t *reply_len, rpc_request_handler_flags_t *reply_flags) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t reply_code = SQC_RESULT_ANY_FAILURES;
  char *reply_msg = NULL;
  AdmDelJobsRequest *request = NULL;
  char *session_user_name = NULL;
  bool is_user_enabled = false;
  bool is_user_admin = false;
  size_t n_jobs = 0u;

  // Check arguments.
  if (rpc_session == NULL || *rpc_session == NULL || body == NULL || reply_id == NULL ||
      reply_body == NULL || reply_len == NULL || reply_flags == NULL) {
    ret = SQC_RESULT_INVALID_ARGS;
    rpc_create_message_text(&reply_msg, "Failed to handle a request, %s", sqc_error_get_string(ret));
    rpc_log_error_msg_with_name(id, reply_msg);
    return ret;
  }

  // Unpack and validate the request message.
  do {
    request = adm_del_jobs_request__unpack(NULL, len, (uint8_t *) body);
    if (request == NULL || !RPC_VALIDATE_PROTOC_BYTES(&request->user_id)) {
      ret = SQC_RESULT_INVALID_ARGS;
      rpc_create_message_text(&reply_msg, "Received a request with invalid arguments");
      rpc_log_error_msg_with_name(id, reply_msg);
      break;
    }

    sqc_msg_info("RPC-%s: Received a request: user_id=%s, from_time=%lld, to_time=%lld\n",
                 rpc_message_name_string(id),
                 (char *) request->user_id.data, (long long) request->from_time,
                 (long long) request->to_time);

    ret = s_get_session_user(rpc_session, &session_user_name, &is_user_enabled, &is_user_admin);
    if (ret != SQC_RESULT_OK) {
      reply_code = ret;
      ret = SQC_RESULT_OK;
      rpc_create_message_text(&reply_msg, "Failed to get a user name of the session, %s",
                              sqc_error_get_string(reply_code));
      rpc_log_error_msg_with_name(id, reply_msg);
      break;
    } else if (!is_user_admin) {
      reply_code = SQC_RESULT_NOT_ADMIN_USER;
      ret = SQC_RESULT_OK;
      rpc_create_message_text(&reply_msg, "Requested by the non-administrator user: user=%s",
                              session_user_name);
      rpc_log_error_msg_with_name(id, reply_msg);
      break;
    } else if (!is_user_enabled) {
      reply_code = SQC_RESULT_DISABLED_USER;
      ret = SQC_RESULT_OK;
      rpc_create_message_text(&reply_msg, "Requested by the disabled user: user=%s",
                              session_user_name);
      rpc_log_error_msg_with_name(id, reply_msg);
      break;
    }

    ret = s_adm_del_jobs((char *) request->user_id.data, (sqc_chrono_t) request->from_time,
                         (sqc_chrono_t) request->to_time, &n_jobs, &reply_msg);
    if (ret == SQC_RESULT_OK) {
      reply_code = ret;
      ret = SQC_RESULT_OK;
      rpc_create_message_text(&reply_msg, "Deleted %zu job(s)", n_jobs);
      rpc_log_info_msg_with_name(id, reply_msg);
      break;
    } else {
      reply_code = ret;
      ret = SQC_RESULT_OK;
      rpc_create_message_text(&reply_msg, "Failed to delete jobs, %s", sqc_error_get_string(reply_code));
      rpc_log_error_msg_with_name(id, reply_msg);
      break;
    }
  } while (0);

  // Create a reply message if no serious error has occurred.
  do {
    if (ret != SQC_RESULT_OK) {
      break;
    }
    ret = rpc_pack_adm_del_jobs_reply(reply_code, reply_msg, reply_body, reply_len);
    if (ret != SQC_RESULT_OK) {
      sqc_msg_error("RPC-%s: Failed to construct a reply, %s\n",
                    rpc_message_name_string(id), sqc_error_get_string(ret));
      break;
    }
    *reply_id = RPC_MSG_ADM_DEL_JOBS_REPLY;
    *reply_flags = 0u;
    sqc_msg_info("RPC-%s: Send a reply: code=%d(%s), message=%s\n",
                 rpc_message_name_string(id),
                 (int) reply_code, sqc_error_get_string(reply_code), reply_msg);
    ret = SQC_RESULT_OK;
  } while (0);

  free(reply_msg);
  free(session_user_name);
  adm_del_jobs_request__free_unpacked(request, NULL);
  return ret;
}


//
// Add a user.
//
static inline sqc_result_t
s_adm_add_user(const char *user_id, char **reply_msg) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  // Check arguments.
  if (user_id == NULL || *user_id == '\0' || strcmp(user_id, all_users) == 0) {
    rc = SQC_RESULT_INVALID_ARGS;
    rpc_create_message_text(reply_msg, "Failed to add a user, %s", sqc_error_get_string(rc));
    rpc_log_debug_msg(5, *reply_msg);
    return rc;
  }

  // Add the specified user.
  do {
    dbmgr_user_info_t user_info = NULL;
    rc = dbmgr_ui_user_find(user_id, &user_info);
    if (rc == SQC_RESULT_OK) {
      rc = SQC_RESULT_ALREADY_EXISTS;
      rpc_create_message_text(reply_msg, "Failed to add the user, %s: user=%s",
                              sqc_error_get_string(rc), user_id);
      rpc_log_debug_msg(5, *reply_msg);
      break;
    } else if (rc != SQC_RESULT_NOT_FOUND) {
      rpc_create_message_text(reply_msg, "Failed to get user information, %s: user=%s",
                              sqc_error_get_string(rc), user_id);
      rpc_log_debug_msg(5, *reply_msg);
      break;
    }

    rc = dbmgr_ui_create_user(user_id, &user_info);
    if (rc != SQC_RESULT_OK) {
      rpc_create_message_text(reply_msg, "Failed to add the user, %s: user=%s",
                              sqc_error_get_string(rc), user_id);
      rpc_log_debug_msg(5, *reply_msg);
      break;
    }

    rc = SQC_RESULT_OK;
  } while (0);

  return rc;
}

//
// Handle a 'adm_add_user_request' message.
//
// The function processes the received request and builds a reply message.
//
static inline sqc_result_t
s_handle_adm_add_user_request(rpc_session_server_t *rpc_session, rpc_msg_id_t id, char *body,
                              size_t len, rpc_msg_id_t *reply_id, char **reply_body,
                              size_t *reply_len, rpc_request_handler_flags_t *reply_flags) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t reply_code = SQC_RESULT_ANY_FAILURES;
  char *reply_msg = NULL;
  AdmAddUserRequest *request = NULL;
  char *session_user_name = NULL;
  bool is_user_enabled = false;
  bool is_user_admin = false;

  // Check arguments.
  if (rpc_session == NULL || *rpc_session == NULL || body == NULL || reply_id == NULL ||
      reply_body == NULL || reply_len == NULL || reply_flags == NULL) {
    ret = SQC_RESULT_INVALID_ARGS;
    rpc_create_message_text(&reply_msg, "Failed to handle a request, %s", sqc_error_get_string(ret));
    rpc_log_error_msg_with_name(id, reply_msg);
    return ret;
  }

  // Unpack and validate the request message.
  do {
    request = adm_add_user_request__unpack(NULL, len, (uint8_t *) body);
    if (request == NULL || !RPC_VALIDATE_PROTOC_BYTES(&request->user_id)) {
      ret = SQC_RESULT_INVALID_ARGS;
      rpc_create_message_text(&reply_msg, "Received a request with invalid arguments");
      rpc_log_error_msg_with_name(id, reply_msg);
      break;
    }

    sqc_msg_info("RPC-%s: Received a request: user_id=%s\n",
                 rpc_message_name_string(id), (char *) request->user_id.data);

    ret = s_get_session_user(rpc_session, &session_user_name, &is_user_enabled, &is_user_admin);
    if (ret != SQC_RESULT_OK) {
      reply_code = ret;
      ret = SQC_RESULT_OK;
      rpc_create_message_text(&reply_msg, "Failed to get a user name of the session, %s",
                              sqc_error_get_string(reply_code));
      rpc_log_error_msg_with_name(id, reply_msg);
      break;
    } else if (!is_user_admin) {
      reply_code = SQC_RESULT_NOT_ADMIN_USER;
      ret = SQC_RESULT_OK;
      rpc_create_message_text(&reply_msg, "Requested by the non-administrator user: user=%s",
                              session_user_name);
      rpc_log_error_msg_with_name(id, reply_msg);
      break;
    } else if (!is_user_enabled) {
      reply_code = SQC_RESULT_DISABLED_USER;
      ret = SQC_RESULT_OK;
      rpc_create_message_text(&reply_msg, "Requested by the disabled user: user=%s",
                              session_user_name);
      rpc_log_error_msg_with_name(id, reply_msg);
      break;
    }

    ret = s_adm_add_user((char *) request->user_id.data, &reply_msg);
    if (ret == SQC_RESULT_OK) {
      reply_code = ret;
      ret = SQC_RESULT_OK;
      rpc_create_message_text(&reply_msg, "Added: user=%s", (char *) request->user_id.data);
      rpc_log_info_msg_with_name(id, reply_msg);
      break;
    } else {
      reply_code = ret;
      ret = SQC_RESULT_OK;
      rpc_log_error_msg_with_name(id, reply_msg);
      break;
    }
  } while (0);

  // Create a reply message if no serious error has occurred.
  do {
    if (ret != SQC_RESULT_OK) {
      break;
    }

    ret = rpc_pack_adm_add_user_reply(reply_code, reply_msg, reply_body, reply_len);
    if (ret != SQC_RESULT_OK) {
      sqc_msg_error("RPC-%s: Failed to construct a reply, %s\n",
                    rpc_message_name_string(id),
                    sqc_error_get_string(ret));
      break;
    }

    *reply_id = RPC_MSG_ADM_ADD_USER_REPLY;
    *reply_flags = 0u;
    sqc_msg_info("RPC-%s: Send a reply: code=%d(%s), message=%s\n",
                 rpc_message_name_string(id),
                 (int) reply_code, sqc_error_get_string(reply_code), reply_msg);
    ret = SQC_RESULT_OK;
  } while (0);

  free(reply_msg);
  free(session_user_name);
  adm_add_user_request__free_unpacked(request, NULL);
  return ret;
}


//
// Add a user.
//
static inline sqc_result_t
s_adm_set_user_status(const char *user_id, bool enabled, char **reply_msg) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  // Check arguments.
  if (user_id == NULL || *user_id == '\0' || strcmp(user_id, all_users) == 0) {
    rc = SQC_RESULT_INVALID_ARGS;
    rpc_create_message_text(reply_msg, "Failed to set uesr status, %s", sqc_error_get_string(rc));
    rpc_log_debug_msg(5, *reply_msg);
    return rc;
  }

  // Add the specified user.
  do {
    dbmgr_user_info_t user_info = NULL;
    rc = dbmgr_ui_user_find(user_id, &user_info);
    if (rc != SQC_RESULT_OK) {
      rpc_create_message_text(reply_msg, "Failed to set status of the user, %s: user=%s",
                              sqc_error_get_string(rc), user_id);
      rpc_log_debug_msg(5, *reply_msg);
      break;
    }

    if (enabled) {
      rc = dbmgr_ui_set_user_enabled(user_info);
    } else {
      rc = dbmgr_ui_set_user_disabled(user_info);
    }

    if (rc != SQC_RESULT_OK) {
      rpc_create_message_text(reply_msg, "Failed to set status of the user, %s: user=%s",
                              sqc_error_get_string(rc), user_id);
      rpc_log_debug_msg(5, *reply_msg);
      break;
    }

    rc = SQC_RESULT_OK;
  } while (0);

  return rc;
}


//
// Handle a 'adm_set_user_status_request' message.
//
// The function processes the received request and builds a reply message.
//
static inline sqc_result_t
s_handle_adm_set_user_status_request(rpc_session_server_t *rpc_session, rpc_msg_id_t id, char *body,
                                     size_t len, rpc_msg_id_t *reply_id, char **reply_body,
                                     size_t *reply_len, rpc_request_handler_flags_t *reply_flags) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t reply_code = SQC_RESULT_ANY_FAILURES;
  char *reply_msg = NULL;
  AdmSetUserStatusRequest *request = NULL;
  char *session_user_name = NULL;
  bool is_user_enabled = false;
  bool is_user_admin = false;

  // Check arguments.
  if (rpc_session == NULL || *rpc_session == NULL || body == NULL || reply_id == NULL ||
      reply_body == NULL || reply_len == NULL || reply_flags == NULL) {
    ret = SQC_RESULT_INVALID_ARGS;
    rpc_create_message_text(&reply_msg, "Received a request with invalid arguments");
    rpc_log_error_msg_with_name(id, reply_msg);
    return ret;
  }

  // Unpack and validate the request message.
  do {
    request = adm_set_user_status_request__unpack(NULL, len, (uint8_t *) body);
    if (request == NULL || !RPC_VALIDATE_PROTOC_BYTES(&request->user_id)) {
      ret = SQC_RESULT_INVALID_ARGS;
      rpc_create_message_text(&reply_msg, "Failed to handle a request, %s", sqc_error_get_string(ret));
      rpc_log_error_msg_with_name(id, reply_msg);
      break;
    }

    sqc_msg_info("RPC-%s: Received a request: user_id=%s, status=%s\n",
                 rpc_message_name_string(id), (char *) request->user_id.data,
                 request->enabled ? "enable" : "disable");

    ret = s_get_session_user(rpc_session, &session_user_name, &is_user_enabled, &is_user_admin);
    if (ret != SQC_RESULT_OK) {
      reply_code = ret;
      ret = SQC_RESULT_OK;
      rpc_create_message_text(&reply_msg, "Failed to get a user name of the session, %s",
                              sqc_error_get_string(reply_code));
      rpc_log_error_msg_with_name(id, reply_msg);
      break;
    } else if (!is_user_admin) {
      reply_code = SQC_RESULT_NOT_ADMIN_USER;
      ret = SQC_RESULT_OK;
      rpc_create_message_text(&reply_msg, "Requested by the non-administrator user: user=%s",
                              session_user_name);
      rpc_log_error_msg_with_name(id, reply_msg);
      break;
    } else if (!is_user_enabled) {
      reply_code = SQC_RESULT_DISABLED_USER;
      ret = SQC_RESULT_OK;
      rpc_create_message_text(&reply_msg, "Requested by the disabled user: user=%s",
                              session_user_name);
      rpc_log_error_msg_with_name(id, reply_msg);
      break;
    }

    ret = s_adm_set_user_status((char *) request->user_id.data, request->enabled, &reply_msg);
    if (ret == SQC_RESULT_OK) {
      reply_code = ret;
      ret = SQC_RESULT_OK;
      rpc_create_message_text(&reply_msg, "Set status: user=%s, status=%s",
                              (char *) request->user_id.data,
                              request->enabled ? "enable" : "disable");
      rpc_log_info_msg_with_name(id, reply_msg);
      break;
    } else {
      reply_code = ret;
      ret = SQC_RESULT_OK;
      rpc_log_error_msg_with_name(id, reply_msg);
      break;
    }
  } while (0);

  // Create a reply message if no serious error has occurred.
  do {
    if (ret != SQC_RESULT_OK) {
      break;
    }

    ret = rpc_pack_adm_set_user_status_reply(reply_code, reply_msg, reply_body, reply_len);
    if (ret != SQC_RESULT_OK) {
      sqc_msg_error("RPC-%s: Failed to construct a reply, %s\n",
                    rpc_message_name_string(id), sqc_error_get_string(ret));
      break;
    }

    *reply_id = RPC_MSG_ADM_SET_USER_STATUS_REPLY;
    *reply_flags = 0u;
    sqc_msg_info("RPC-%s: Send a reply: code=%d(%s), message=%s\n",
                 rpc_message_name_string(id),
                 (int) reply_code, sqc_error_get_string(reply_code), reply_msg);
    ret = SQC_RESULT_OK;
  } while (0);

  free(reply_msg);
  free(session_user_name);
  adm_set_user_status_request__free_unpacked(request, NULL);
  return ret;
}


//
// Dispatch the received message to a handler.
//
static inline sqc_result_t
s_dispatch_request(rpc_session_server_t *rpc_session, rpc_msg_id_t id, char *body, size_t len,
                   rpc_msg_id_t *reply_id, char **reply_body, size_t *reply_len,
                   rpc_request_handler_flags_t *reply_flags) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_msg_debug(5, "RPC Session: Received a request from client, msg_id=%u (%s)\n",
                (unsigned int) id, rpc_message_name_string(id));

  switch (id) {
    case RPC_MSG_OPEN_NEW_SESSION_REQUEST: {
      ret = s_handle_open_new_session_request(rpc_session, id, body, len, reply_id,
                                              reply_body, reply_len, reply_flags);
      break;
    }
    case RPC_MSG_AUTH_REQUEST: {
      ret = s_handle_auth_request(rpc_session, id, body, len, reply_id, reply_body,
                                  reply_len, reply_flags);
      break;
    }
    case RPC_MSG_SUBMIT_JOB_REQUEST: {
      ret = s_handle_submit_job_request(rpc_session, id, body, len, reply_id, reply_body,
                                        reply_len, reply_flags);
      break;
    }
    case RPC_MSG_JOB_STATUS_REQUEST: {
      ret = s_handle_job_status_request(rpc_session, id, body, len, reply_id, reply_body,
                                        reply_len, reply_flags);
       break;
    }
    case RPC_MSG_CANCEL_JOB_REQUEST: {
      ret = s_handle_cancel_job_request(rpc_session, id, body, len, reply_id, reply_body,
                                        reply_len, reply_flags);
       break;
    }
    case RPC_MSG_DELETE_JOB_REQUEST: {
      ret = s_handle_delete_job_request(rpc_session, id, body, len, reply_id, reply_body,
                                        reply_len, reply_flags);
       break;
    }
    case RPC_MSG_JOB_LIST_REQUEST: {
      ret = s_handle_job_list_request(rpc_session, id, body, len, reply_id, reply_body,
                                      reply_len, reply_flags);
       break;
    }
    case RPC_MSG_ADM_DEL_JOBS_REQUEST: {
      ret = s_handle_adm_del_jobs_request(rpc_session, id, body, len, reply_id, reply_body,
                                          reply_len, reply_flags);
       break;
    }
    case RPC_MSG_ADM_ADD_USER_REQUEST: {
      ret = s_handle_adm_add_user_request(rpc_session, id, body, len, reply_id, reply_body,
                                          reply_len, reply_flags);
       break;
    }
    case RPC_MSG_ADM_SET_USER_STATUS_REQUEST: {
      ret = s_handle_adm_set_user_status_request(rpc_session, id, body, len, reply_id, reply_body,
                                                 reply_len, reply_flags);
       break;
    }
    default: {
      sqc_msg_debug(5, "RPC Session: Invalid request from client, msg_id=%u\n", (unsigned int) id);
      ret = SQC_RESULT_INVALID_OBJECT;
      break;
    }
  }

  return ret;
}


//
// Handle an RPC request from a client.
//
static inline sqc_result_t
s_process_request(rpc_session_server_t *rpc_session) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t recv_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t handle_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t send_result = SQC_RESULT_ANY_FAILURES;
  char *body = NULL;
  size_t len = 0u;
  rpc_msg_id_t id = 0u;
  rpc_msg_id_t reply_id = 0u;
  char *reply_body = NULL;
  size_t reply_len = 0u;
  rpc_request_handler_flags_t flags = 0u;
  bool tls_flag = false;
  bool mutual_tls_auth_flag = false;

  if (likely(rpc_session != NULL && *rpc_session != NULL)) {
    // Receive an RPC request.
    recv_result = rpc_session_server_recv_msg(rpc_session, &id, &body, &len);

    if (likely(recv_result == SQC_RESULT_OK)) {
      handle_result = SQC_RESULT_ANY_FAILURES;
      // Handle the received request.
      handle_result = s_dispatch_request(rpc_session, id, body, len, &reply_id, &reply_body,
                                         &reply_len, &flags);

      if (likely(handle_result == SQC_RESULT_OK)) {
        tls_flag = ((flags & RPC_REQUEST_HANDLER_FLAG_TLS) != 0u);
        mutual_tls_auth_flag = ((flags & RPC_REQUEST_HANDLER_FLAG_MUTUAL_TLS_AUTH) != 0);

        // Send an RPC reply.
        send_result = rpc_session_server_send_msg(rpc_session, reply_id, reply_body, reply_len);

        if (likely(send_result == SQC_RESULT_OK)) {
          // Establish a TLS session.
          if (likely(tls_flag == true)) {
            ret = rpc_session_server_establish_tls(rpc_session, mutual_tls_auth_flag, false);
            if (likely(ret == SQC_RESULT_OK)) {
              sqc_msg_debug(5, "Established a TLS session\n");
            } else {
              sqc_msg_debug(5, "Failed to establish a TLS session, %s\n", sqc_error_get_string(ret));
            }
          } else {
            ret = SQC_RESULT_OK;
          }
        } else {
          ret = send_result;
        }

      } else {
        ret = handle_result;
      }

    } else {
      ret = recv_result;
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_debug(5, "%s\n", sqc_error_get_string(ret));
  }

  free(body);
  free(reply_body);
  return ret;
}


/*
 * Exported APIs
 */

uint64_t
rpc_session_server_issue_connection_id(void) {
  return s_issue_connection_id();
}

sqc_result_t
rpc_session_server_create(rpc_session_server_t* rpc_session, int fd, struct sockaddr_storage *addr,
                          socklen_t addrlen, const sqc_tls_conf_t tls_conf,
                          rpc_jwt_server_ctx_t *jwt_ctx) {
  return s_create(rpc_session, fd, addr, addrlen, tls_conf, jwt_ctx);
}


sqc_result_t
rpc_session_server_create_from_conf_dir(rpc_session_server_t* rpc_session, int fd,
                                        struct sockaddr_storage *addr, socklen_t addrlen,
                                        const char *dir) {
  return s_create_from_conf_dir(rpc_session, fd, addr, addrlen, dir);
}


void
rpc_session_server_destroy(rpc_session_server_t *rpc_session) {
  s_destroy(rpc_session);
}

sqc_result_t
rpc_session_server_process_request(rpc_session_server_t *rpc_session) {
  return s_process_request(rpc_session);
}
