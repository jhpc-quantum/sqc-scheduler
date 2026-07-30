#include "sqc_apis.h"
#include "dbmgr.h"
#include "rpc.pb-c.h"
#include "rpc_auth_method.h"
#include "rpc_file_util.h"
#include "rpc_msg_id.h"
#include "rpc_msg_util.h"
#include "rpc_session_internal.h"
#include "rpc_munge.h"

//
// Send an RPC request and receive its reply.
//
static inline sqc_result_t
s_request(rpc_session_client_t *rpc_session, rpc_msg_id_t id, const char *body, size_t len,
          rpc_msg_id_t *reply_id, char **reply_body, size_t *reply_len, char **err_msg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t send_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t recv_result = SQC_RESULT_ANY_FAILURES;

  if (likely(rpc_session != NULL && *rpc_session != NULL && err_msg != NULL)) {
    // Send an RPC request.
    send_result = rpc_session_client_send_msg(rpc_session, id, body, len);

    if (likely(send_result == SQC_RESULT_OK)) {
      // Receives an RPC reply.
      recv_result = rpc_session_client_recv_msg(rpc_session, reply_id, reply_body, reply_len);

      if (likely(recv_result == SQC_RESULT_OK)) {
        ret = SQC_RESULT_OK;
        sqc_msg_debug(5, "RPC %s: Send a request\n", rpc_message_name_string(id));
        *err_msg = NULL;
      } else {
        ret = recv_result;
        rpc_create_message_text(err_msg, "RPC %s: Failed to receive a reply, %s",
                                rpc_message_name_string(id), sqc_error_get_string(ret));
        rpc_log_debug_msg(5, *err_msg);
      }

    } else {
      ret = send_result;
      rpc_create_message_text(err_msg, "RPC %s: Failed to send a request, %s",
                              rpc_message_name_string(id), sqc_error_get_string(ret));
      rpc_log_debug_msg(5, *err_msg);
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_debug(5, "RPC %s: %s\n", rpc_message_name_string(id), sqc_error_get_string(ret));
    rpc_create_message_text(err_msg, "RPC %s: %s", rpc_message_name_string(id), sqc_error_get_string(ret));
  }

  return ret;
}


//
// Send a 'open_new_session_request' and retrieve its reply.
//
static inline sqc_result_t
s_open_new_session(rpc_session_client_t *rpc_session, rpc_auth_method_t auth_method,
                   sqc_result_t *code, char **msg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t pack_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t rpc_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t unpack_result = SQC_RESULT_ANY_FAILURES;
  char *body = NULL;
  size_t len = 0u;
  const rpc_msg_id_t id = RPC_MSG_OPEN_NEW_SESSION_REQUEST;
  rpc_msg_id_t reply_id = 0u;
  char *reply_body = NULL;
  size_t reply_len = 0u;

  if (likely(rpc_session != NULL && *rpc_session != NULL && code != NULL && msg != NULL)) {
    pack_result = rpc_pack_open_new_session_request(auth_method, &body, &len);

    if (likely(pack_result == SQC_RESULT_OK)) {
      rpc_result = s_request(rpc_session, id, body, len, &reply_id, &reply_body, &reply_len, msg);
      if (likely(rpc_result == SQC_RESULT_OK)) {
        free(*msg);
        *msg = NULL;
        if (likely(reply_id == RPC_MSG_OPEN_NEW_SESSION_REPLY)) {
          unpack_result = rpc_unpack_open_new_session_reply(reply_body, reply_len, code, msg);

          if (likely(unpack_result == SQC_RESULT_OK)) {
            ret = *code;
            sqc_msg_debug(5, "RPC %s: Received a reply: code=%d(%s), msg='%s'\n",
                          rpc_message_name_string(id), (int) *code, sqc_error_get_string(*code), *msg);
          } else {
            ret = unpack_result;
            rpc_create_message_text(msg, "RPC %s: Failed to unpack the received reply data, %s",
                                    rpc_message_name_string(id), sqc_error_get_string(ret));
            rpc_log_debug_msg(5, *msg);
          }

        } else {
          ret = SQC_RESULT_INVALID_OBJECT;
          rpc_create_message_text(msg, "RPC %s: Received an unexpected reply: msg_id=%u (%s)",
                                  rpc_message_name_string(id), (unsigned int) reply_id,
                                  rpc_message_id_string(reply_id));
          rpc_log_debug_msg(5, *msg);
        }
      } else {
        ret = rpc_result;
        rpc_log_debug_msg(5, *msg);
      }

    } else {
      ret = pack_result;
      rpc_create_message_text(msg, "RPC %s: Failed to create a request, %s",
                              rpc_message_name_string(id), sqc_error_get_string(ret));
      rpc_log_debug_msg(5, *msg);
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_debug(5, "RPC %s: %s\n", rpc_message_name_string(id), sqc_error_get_string(ret));
    rpc_create_message_text(msg, "RPC %s: %s", rpc_message_name_string(id), sqc_error_get_string(ret));
  }

  free(reply_body);
  free(body);

  return ret;
}


//
// Send a 'auth_request' and retrieve its reply.
//
static inline sqc_result_t
s_auth(rpc_session_client_t *rpc_session, const uint8_t *data, size_t datalen, sqc_result_t *code,
       char **msg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t pack_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t rpc_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t unpack_result = SQC_RESULT_ANY_FAILURES;
  char *body = NULL;
  size_t len = 0u;
  const rpc_msg_id_t id = RPC_MSG_AUTH_REQUEST;
  rpc_msg_id_t reply_id = 0u;
  char *reply_body = NULL;
  size_t reply_len = 0u;

  if (likely(rpc_session != NULL && *rpc_session != NULL && code != NULL && msg != NULL)) {
    pack_result = rpc_pack_auth_request(data, datalen, &body, &len);

    if (likely(pack_result == SQC_RESULT_OK)) {
      rpc_result = s_request(rpc_session, id, body, len, &reply_id, &reply_body, &reply_len, msg);
      if (likely(rpc_result == SQC_RESULT_OK)) {
        free(*msg);
        *msg = NULL;
        if (likely(reply_id == RPC_MSG_AUTH_REPLY)) {
          unpack_result = rpc_unpack_auth_reply(reply_body, reply_len, code, msg);

          if (likely(unpack_result == SQC_RESULT_OK)) {
            ret = *code;
            sqc_msg_debug(5, "RPC %s: Received a reply: code=%d (%s), msg='%s'\n",
                          rpc_message_name_string(id), (int) *code,
                          sqc_error_get_string(*code), *msg);
          } else {
            ret = unpack_result;
            rpc_create_message_text(msg, "RPC %s: Failed to unpack the received reply data: %s",
                                    rpc_message_name_string(id), rpc_message_id_string(unpack_result));
            rpc_log_debug_msg(5, *msg);
          }

        } else {
          ret = SQC_RESULT_INVALID_OBJECT;
          rpc_create_message_text(msg, "RPC %s: Received an unexpected reply: msg_id=%u (%s)",
                                  rpc_message_name_string(id), (unsigned int) reply_id,
                                  rpc_message_id_string(reply_id));
          rpc_log_debug_msg(5, *msg);
        }
      } else {
        ret = rpc_result;
        rpc_create_message_text(msg, "RPC %s: Procedure failed, %s",
                                rpc_message_name_string(id), sqc_error_get_string(ret));
        rpc_log_debug_msg(5, *msg);
      }
    } else {
      ret = pack_result;
      rpc_create_message_text(msg, "RPC %s: Failed to create a request, %s",
                              rpc_message_name_string(id), sqc_error_get_string(ret));
      rpc_log_debug_msg(5, *msg);
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_debug(5, "RPC %s: %s\n", rpc_message_name_string(id), sqc_error_get_string(ret));
    rpc_create_message_text(msg, "RPC %s: %s", rpc_message_name_string(id), sqc_error_get_string(ret));
  }

  free(body);
  free(reply_body);

  return ret;
}


static inline sqc_result_t
s_start(rpc_session_client_t *rpc_session, rpc_auth_method_t auth_method, const uint8_t *auth_data,
        size_t auth_datalen, char **err_msg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t open_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t open_code = SQC_RESULT_ANY_FAILURES;
  sqc_result_t tls_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t auth_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t auth_code = SQC_RESULT_ANY_FAILURES;

  if (likely(rpc_session != NULL && *rpc_session != NULL && err_msg != NULL)) {
    open_result = s_open_new_session(rpc_session, auth_method, &open_code, err_msg);
    if (likely(open_result == SQC_RESULT_OK && open_code == SQC_RESULT_OK)) {

      if (rpc_auth_method_uses_tls(auth_method) == true) {
        tls_result = rpc_session_client_establish_tls(rpc_session,
                                                      auth_method == RPC_AUTH_METHOD_MUTUAL_TLS,
                                                      false);
        if (likely(tls_result == SQC_RESULT_OK)) {
          sqc_msg_debug(5, "Established TLS session\n");
        } else {
          ret = tls_result;
          rpc_create_message_text(err_msg, "Failed to setup TLS: %s", sqc_error_get_string(ret));
          rpc_log_debug_msg(5, *err_msg);
        }
      } else {
        tls_result = SQC_RESULT_OK;
      }

      if (likely(tls_result == SQC_RESULT_OK)) {
        auth_result = s_auth(rpc_session, auth_data, auth_datalen, &auth_code, err_msg);
        if (likely(auth_result == SQC_RESULT_OK)) {
          if (likely(auth_code == SQC_RESULT_OK)) {
            ret = SQC_RESULT_OK;
            sqc_msg_debug(5, "Authenticated\n");
          } else {
            sqc_msg_info("Authentication failed, %s\n", sqc_error_get_string(auth_code));
            ret = auth_code;
          }
        } else {
          sqc_msg_info("Authentication failed, %s\n", sqc_error_get_string(auth_result));
          ret = auth_result;
        }
      }

    } else if (likely(open_result == SQC_RESULT_OK)) {
      ret = open_code;
    } else {
      ret = open_result;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_debug(5, "%s\n", sqc_error_get_string(ret));
    rpc_create_message_text(err_msg, "%s", sqc_error_get_string(ret));
  }

  return ret;
}


//
// Create a 'sqc_session_t' object, and then connect with a server.
//
static inline sqc_result_t
s_create(rpc_session_client_t *rpc_session, const char *server, bool prefer_ipv4,
         rpc_auth_method_t auth_method, const sqc_tls_conf_t tls_conf,
         const rpc_jwt_client_ctx_t *jwt_ctx) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t create_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t nodelay_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t copy_tls_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t copy_jwt_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t auth_data_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t connect_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t start_result = SQC_RESULT_ANY_FAILURES;
  rpc_session_client_t tmp_rpc_session = NULL;
  char *auth_data = NULL;
  size_t auth_datalen = 0u;

  if (likely(rpc_session != NULL && IS_VALID_STRING(server) && tls_conf != NULL &&
             jwt_ctx != NULL)) {
    tmp_rpc_session = malloc(sizeof(struct rpc_session_client));

    if (likely(tmp_rpc_session != NULL)) {
      // Create a rpc_session_client_t object as 'tmp_rpc_session'.
      tmp_rpc_session->session_ = NULL;
      tmp_rpc_session->tls_conf_ = NULL;
      tmp_rpc_session->jwt_ctx_ = NULL;

      copy_tls_result = sqc_tls_conf_copy(&tmp_rpc_session->tls_conf_, tls_conf);
      if (unlikely(copy_tls_result == SQC_RESULT_OK)) {
        (void)sqc_tls_conf_set_allow_no_crl(tmp_rpc_session->tls_conf_, true);
        sqc_msg_debug(5, "Register TLS configuration to the RPC session\n");
      } else {
        ret = copy_tls_result;
        sqc_msg_error("Failed to register TLS configuration to the RPC session\n");
      }

      copy_jwt_result = rpc_jwt_client_copy_ctx(&tmp_rpc_session->jwt_ctx_, jwt_ctx);
      if (likely(copy_jwt_result == SQC_RESULT_OK)) {
        sqc_msg_debug(5, "Register JWT context to the RPC session\n");
      } else {
        ret = copy_jwt_result;
        sqc_msg_error("Failed to register JWT context to the RPC session: %s\n",
                      sqc_error_get_string(ret));
      }

      // Create an RPC session object and bind a local port.
      if (likely(copy_tls_result == SQC_RESULT_OK && copy_jwt_result == SQC_RESULT_OK)) {
        (void)sqc_tls_conf_set_allow_no_crl(tmp_rpc_session->tls_conf_, true);
        create_result = sqc_session_create_client(&(tmp_rpc_session->session_), 0u, server,
                                                  prefer_ipv4);
        if (likely(create_result == SQC_RESULT_OK)) {
          nodelay_result = sqc_endpoint_set_tcp_nodelay((sqc_endpoint_t *)&(tmp_rpc_session->session_),
                                                        true);
          if (likely(nodelay_result == SQC_RESULT_OK)) {
            // Connect with the server.
            connect_result = sqc_endpoint_connect((sqc_endpoint_t *)&(tmp_rpc_session->session_));
            if (likely(connect_result == SQC_RESULT_OK)) {

              // Create authentication data.
              switch (auth_method) {
              case RPC_AUTH_METHOD_MUTUAL_TLS: {
                auth_data_result = SQC_RESULT_OK;
                auth_data = NULL;
                auth_datalen = 0u;
                break;
              }
              case RPC_AUTH_METHOD_MUNGE: {
                auth_data_result = rpc_munge_client_get_cred(&tmp_rpc_session, &auth_data,
                                                             &auth_datalen);
                if (unlikely(auth_data_result != SQC_RESULT_OK)) {
                  ret = auth_data_result;
                  sqc_msg_error("Failed to create a MUNGE creadential: %s\n",
                                sqc_error_get_string(ret));
                }
                break;
              }
              case RPC_AUTH_METHOD_JWT: {
                auth_data_result = rpc_jwt_client_get_token(&tmp_rpc_session->jwt_ctx_,
                                                            &auth_data);
                if (likely(auth_data_result == SQC_RESULT_OK && auth_data != NULL)) {
                  auth_datalen = strlen(auth_data) + 1u;
                } else {
                  ret = auth_data_result;
                  sqc_msg_error("Failed to get a JWT token: %s\n", sqc_error_get_string(ret));
                }
                break;
              }
              default: {
                sqc_msg_error("Invalid authentication method: %s\n",
                              rpc_auth_method_string(auth_method));
                break;
              }
              }

            } else {
              ret = connect_result;
              sqc_msg_error("Failed to connect the socket, %s\n", sqc_error_get_string(ret));
            }
          } else {
            ret = nodelay_result;
            sqc_msg_error("Failed to setsockopt(TCP_NODELAY) for the socket, %s\n", sqc_error_get_string(ret));
          }
        } else {
          ret = create_result;
          sqc_msg_error("Failed to create an RPC session, %s\n", sqc_error_get_string(ret));
        }
      }

      if (likely(create_result == SQC_RESULT_OK &&
                 nodelay_result == SQC_RESULT_OK &&
                 connect_result == SQC_RESULT_OK &&
                 auth_data_result == SQC_RESULT_OK)) {
        // Establish an RPC session.
        char *err_msg = NULL;
        start_result = s_start(&tmp_rpc_session, auth_method, (const uint8_t *)auth_data,
                               auth_datalen, &err_msg);
        if (likely(start_result == SQC_RESULT_OK)) {
          ret = SQC_RESULT_OK;
          *rpc_session = tmp_rpc_session;
          sqc_msg_debug(5, "Connected to the server\n");
        } else {
          ret = start_result;
          sqc_msg_error("%s\n", err_msg);
        }
        free(err_msg);
      }
    } else {
      ret = SQC_RESULT_NO_MEMORY;
      sqc_msg_error("%s\n", sqc_error_get_string(ret));
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  free(auth_data);
  if (likely(ret == SQC_RESULT_OK)) {
    sqc_tls_conf_dump((*rpc_session)->tls_conf_);
    rpc_jwt_client_dump_ctx(&(*rpc_session)->jwt_ctx_);
  } else {
    sqc_msg_error("Failed to establish an RPC session\n");
    rpc_session_client_destroy(&tmp_rpc_session);
  }

  return ret;
}


//
// Create a 'sqc_session_t' object by using files under the configuration directory,
// and then connect with a server.
//
static inline sqc_result_t
s_create_from_conf_dir(rpc_session_client_t *rpc_session, const char *server, bool prefer_ipv4,
                       rpc_auth_method_t auth_method, const char *dir) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t expand_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t tls_conf_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t jwt_ctx_result = SQC_RESULT_ANY_FAILURES;
  char *exp_dir = NULL;
  sqc_tls_conf_t tls_conf = NULL;
  rpc_jwt_client_ctx_t jwt_ctx = NULL;

  if (likely(rpc_session != NULL && IS_VALID_STRING(server) && IS_VALID_STRING(dir))) {
    expand_result = rpc_expand_path(dir, &exp_dir);

    if (likely(expand_result == SQC_RESULT_OK)) {
      tls_conf_result = sqc_tls_conf_create_from_conf_dir(&tls_conf, exp_dir, TLS_ROLE_CLIENT);
      if (likely(tls_conf_result == SQC_RESULT_OK)) {
        sqc_tls_conf_dump(tls_conf);
        jwt_ctx_result = rpc_jwt_client_create_ctx_from_conf_dir(&jwt_ctx, exp_dir);
        if (likely(jwt_ctx_result == SQC_RESULT_OK)) {
          rpc_jwt_client_dump_ctx(&jwt_ctx);
          ret = rpc_session_client_create(rpc_session, server, prefer_ipv4, auth_method, tls_conf,
                                          &jwt_ctx);
        } else {
          ret = jwt_ctx_result;
        }
      } else {
        ret = tls_conf_result;
      }
    } else {
      ret = expand_result;
    }

  } else {
    ret = SQC_RESULT_POSIX_API_ERROR;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  free(exp_dir);
  free(tls_conf);
  free(jwt_ctx);

  return ret;
}


//
// Destroy the RPC session.
//
static inline void
s_destroy(rpc_session_client_t *rpc_session) {
  if (likely(rpc_session != NULL && *rpc_session != NULL)) {
    sqc_tls_conf_destroy((*rpc_session)->tls_conf_);
    sqc_session_destroy(&(*rpc_session)->session_);
    rpc_jwt_client_destroy_ctx(&(*rpc_session)->jwt_ctx_);
    free(*rpc_session);
    *rpc_session = NULL;
  }
}


//
// Send a 'submit_job_request' and retrieve its reply.
//
static inline sqc_result_t
s_submit_job(rpc_session_client_t *rpc_session, uint8_t priority, const char *qprogram,
             sqc_rpc_sched_circuit_fmt_t circuit_fmt, size_t shots, sqc_rpc_sched_qc_type_t qc_type,
             sqc_rpc_sched_transpiler_t transpiler, const char *remark, const char *user_token,
             sqc_result_t *code, char **msg, char **job_id) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t pack_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t rpc_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t unpack_result = SQC_RESULT_ANY_FAILURES;
  char *body = NULL;
  size_t len = 0u;
  const rpc_msg_id_t id = RPC_MSG_SUBMIT_JOB_REQUEST;
  rpc_msg_id_t reply_id = 0u;
  char *reply_body = NULL;
  size_t reply_len = 0u;

  if (likely(rpc_session != NULL && qprogram != NULL && remark != NULL &&
             code != NULL && msg != NULL && job_id != NULL)) {
    pack_result = rpc_pack_submit_job_request(priority, qprogram, circuit_fmt, shots, qc_type, transpiler,
                                              remark, user_token, &body, &len);

    if (likely(pack_result == SQC_RESULT_OK)) {
      rpc_result = s_request(rpc_session, id, body, len, &reply_id, &reply_body, &reply_len, msg);

      if (likely(rpc_result == SQC_RESULT_OK)) {
        free(*msg);
        *msg = NULL;
        if (likely(reply_id == RPC_MSG_SUBMIT_JOB_REPLY)) {
          unpack_result = rpc_unpack_submit_job_reply(reply_body, reply_len, code, msg, job_id);

          if (likely(unpack_result == SQC_RESULT_OK)) {
            ret = *code;
            sqc_msg_debug(5, "RPC %s: Received a reply: code=%d (%s), msg='%s', job_id=%s\n",
                          rpc_message_name_string(id), (int) *code, sqc_error_get_string(*code),
                          *msg, *job_id == NULL ? "(none)" : *job_id);
          } else {
            ret = unpack_result;
            rpc_create_message_text(msg, "RPC %s: Failed to unpack the received reply data: %s",
                                    rpc_message_name_string(id), rpc_message_id_string(unpack_result));
            rpc_log_debug_msg(5, *msg);
          }

        } else {
          ret = SQC_RESULT_INVALID_OBJECT;
          rpc_create_message_text(msg, "RPC %s: Received an unexpected reply: msg_id=%u (%s)",
                                  rpc_message_name_string(id), (unsigned int) reply_id,
                                  rpc_message_id_string(reply_id));
          rpc_log_debug_msg(5, *msg);
        }
      } else {
        ret = rpc_result;
        rpc_log_debug_msg(5, *msg);
      }

    } else {
      ret = pack_result;
      rpc_create_message_text(msg, "RPC %s: %s", rpc_message_name_string(id), sqc_error_get_string(ret));
      rpc_log_debug_msg(5, *msg);
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("RPC %s: %s\n", rpc_message_name_string(id), sqc_error_get_string(ret));
    rpc_create_message_text(msg, "RPC %s: %s", rpc_message_name_string(id), sqc_error_get_string(ret));
  }

  free(body);
  free(reply_body);

  return ret;
}


//
// Send a 'job_status_request' and retrieve its reply.
//
static inline sqc_result_t
s_job_status(rpc_session_client_t *rpc_session, const char *job_id, sqc_result_t *code, char **msg,
             sqc_rpc_sched_job_status_t *status, char **qc_job_id, char **result) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t pack_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t rpc_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t unpack_result = SQC_RESULT_ANY_FAILURES;
  char *body = NULL;
  size_t len = 0u;
  const rpc_msg_id_t id = RPC_MSG_JOB_STATUS_REQUEST;
  rpc_msg_id_t reply_id = 0u;
  char *reply_body = NULL;
  size_t reply_len = 0u;

  if (likely(rpc_session != NULL && code != NULL && msg != NULL && status != NULL &&
             qc_job_id != NULL && result != NULL)) {
    pack_result = rpc_pack_job_status_request(job_id, &body, &len);

    if (likely(pack_result == SQC_RESULT_OK)) {
      rpc_result = s_request(rpc_session, id, body, len, &reply_id, &reply_body, &reply_len, msg);

      if (likely(rpc_result == SQC_RESULT_OK)) {
        free(*msg);
        *msg = NULL;
        if (likely(reply_id == RPC_MSG_JOB_STATUS_REPLY)) {
          unpack_result = rpc_unpack_job_status_reply(reply_body, reply_len, code, msg, status,
                                                      qc_job_id, result);

          if (likely(unpack_result == SQC_RESULT_OK)) {
            ret = *code;
            sqc_msg_debug(5, "RPC %s: Received a reply: code=%d (%s), msg='%s', status=%u, "
                          "qc_job_id=%s, result=%s\n",
                          rpc_message_name_string(id), (int) *code, sqc_error_get_string(*code),
                          *msg, (unsigned int) *status,
                          *qc_job_id != NULL ? "(with qc job id)" : "(no qc job id)",
                          *result != NULL ? "(with result)" : "(no result)");
          } else {
            ret = unpack_result;
            rpc_create_message_text(msg, "RPC %s: Failed to unpack the received reply data, %s",
                                    rpc_message_name_string(id), rpc_message_id_string(unpack_result));
            rpc_log_debug_msg(5, *msg);
          }

        } else {
          ret = SQC_RESULT_INVALID_OBJECT;
          rpc_create_message_text(msg, "RPC %s: Received an unexpected reply: msg_id=%u (%s)",
                                  rpc_message_name_string(id), (unsigned int) reply_id,
                                  rpc_message_id_string(reply_id));
          rpc_log_debug_msg(5, *msg);
        }
      } else {
        ret = rpc_result;
        rpc_log_debug_msg(5, *msg);
      }

    } else {
      ret = SQC_RESULT_NO_MEMORY;
      rpc_create_message_text(msg, "RPC %s: %s",
                              rpc_message_name_string(id), sqc_error_get_string(ret));
      rpc_log_debug_msg(5, *msg);
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("RPC %s: %s\n", rpc_message_name_string(id), sqc_error_get_string(ret));
    rpc_create_message_text(msg, "RPC %s: %s", rpc_message_name_string(id), sqc_error_get_string(ret));
  }

  free(body);
  free(reply_body);

  return ret;
}


//
// Send a 'cancel_job_request' and retrieve its reply.
//
static inline sqc_result_t
s_cancel_job(rpc_session_client_t *rpc_session, const char *job_id, sqc_result_t *code,
             char **msg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t pack_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t rpc_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t unpack_result = SQC_RESULT_ANY_FAILURES;
  char *body = NULL;
  size_t len = 0u;
  const rpc_msg_id_t id = RPC_MSG_CANCEL_JOB_REQUEST;
  rpc_msg_id_t reply_id = 0u;
  char *reply_body = NULL;
  size_t reply_len = 0u;

  if (likely(rpc_session != NULL && code != NULL && msg != NULL)) {
    pack_result = rpc_pack_cancel_job_request(job_id, &body, &len);

    if (likely(pack_result == SQC_RESULT_OK)) {
      rpc_result = s_request(rpc_session, id, body, len, &reply_id, &reply_body, &reply_len, msg);

      if (likely(rpc_result == SQC_RESULT_OK)) {
        free(*msg);
        *msg = NULL;
        if (likely(reply_id == RPC_MSG_CANCEL_JOB_REPLY)) {
          unpack_result = rpc_unpack_cancel_job_reply(reply_body, reply_len, code, msg);

          if (likely(unpack_result == SQC_RESULT_OK)) {
            ret = *code;
            sqc_msg_debug(5, "RPC %s: Received a reply: code=%d (%s), msg='%s'",
                          rpc_message_name_string(id), (int) *code, sqc_error_get_string(*code), *msg);
          } else {
            ret = unpack_result;
            rpc_create_message_text(msg, "RPC %s: Failed to unpack the received reply data, %s",
                                    rpc_message_name_string(id), rpc_message_id_string(unpack_result));
            rpc_log_debug_msg(5, *msg);
          }

        } else {
          ret = SQC_RESULT_INVALID_OBJECT;
          rpc_create_message_text(msg, "RPC %s: Received an unexpected reply: msg_id=%u (%s)",
                                  rpc_message_name_string(id), (unsigned int) reply_id,
                                  rpc_message_id_string(reply_id));
          rpc_log_debug_msg(5, *msg);
        }
      } else {
        ret = rpc_result;
        rpc_create_message_text(msg, "RPC %s: Procedure failed, %s",
                                rpc_message_name_string(id), sqc_error_get_string(ret));
        rpc_log_debug_msg(5, *msg);
      }

    } else {
      ret = SQC_RESULT_NO_MEMORY;
      rpc_create_message_text(msg, "RPC %s: Failed to create a request, %s",
                              rpc_message_name_string(id), sqc_error_get_string(ret));
      rpc_log_debug_msg(5, *msg);
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("RPC %s: %s\n", rpc_message_name_string(id), sqc_error_get_string(ret));
    rpc_create_message_text(msg, "RPC %s: %s", rpc_message_name_string(id), sqc_error_get_string(ret));
  }

  free(body);
  free(reply_body);

  return ret;
}


//
// Send a 'delete_job_request' and retrieve its reply.
//
static inline sqc_result_t
s_delete_job(rpc_session_client_t *rpc_session, const char *job_id, sqc_result_t *code,
             char **msg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t pack_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t rpc_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t unpack_result = SQC_RESULT_ANY_FAILURES;
  char *body = NULL;
  size_t len = 0u;
  const rpc_msg_id_t id = RPC_MSG_DELETE_JOB_REQUEST;
  rpc_msg_id_t reply_id = 0u;
  char *reply_body = NULL;
  size_t reply_len = 0u;

  if (likely(rpc_session != NULL && code != NULL && msg != NULL)) {
    pack_result = rpc_pack_delete_job_request(job_id, &body, &len);

    if (likely(pack_result == SQC_RESULT_OK)) {
      rpc_result = s_request(rpc_session, id, body, len, &reply_id, &reply_body, &reply_len, msg);

      if (likely(rpc_result == SQC_RESULT_OK)) {
        free(*msg);
        *msg = NULL;
        if (likely(reply_id == RPC_MSG_DELETE_JOB_REPLY)) {
          unpack_result = rpc_unpack_delete_job_reply(reply_body, reply_len, code, msg);

          if (likely(unpack_result == SQC_RESULT_OK)) {
            ret = *code;
            sqc_msg_debug(5, "RPC %s: Received a reply: code=%d (%s), msg='%s'",
                          rpc_message_name_string(id), (int) *code, sqc_error_get_string(*code), *msg);
          } else {
            ret = unpack_result;
            rpc_create_message_text(msg, "RPC %s: Failed to unpack the received reply data: %s",
                                    rpc_message_name_string(id), rpc_message_id_string(unpack_result));
            rpc_log_debug_msg(5, *msg);
          }

        } else {
          ret = SQC_RESULT_INVALID_OBJECT;
          rpc_create_message_text(msg, "RPC %s: Received an unexpected reply: msg_id=%u (%s)",
                                  rpc_message_name_string(id), (unsigned int) reply_id,
                                  rpc_message_id_string(reply_id));
          rpc_log_debug_msg(5, *msg);
        }
      } else {
        ret = rpc_result;
        rpc_create_message_text(msg, "RPC %s: Procedure failed, %s",
                                rpc_message_name_string(id), sqc_error_get_string(ret));
        rpc_log_debug_msg(5, *msg);
      }

    } else {
      ret = SQC_RESULT_NO_MEMORY;
      rpc_create_message_text(msg, "RPC %s: Failed to create a request, %s",
                              rpc_message_name_string(id), sqc_error_get_string(ret));
      rpc_log_debug_msg(5, *msg);
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("RPC %s: %s\n", rpc_message_name_string(id), sqc_error_get_string(ret));
    rpc_create_message_text(msg, "RPC %s: %s", rpc_message_name_string(id), sqc_error_get_string(ret));
  }

  free(body);
  free(reply_body);

  return ret;
}


//
// Send a 'job_list_request' and retrieve its reply.
//
static inline sqc_result_t
s_job_list(rpc_session_client_t *rpc_session, sqc_result_t *code, char **msg,
           rpc_job_info_t ***jobs, size_t *n_jobs) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t rpc_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t unpack_result = SQC_RESULT_ANY_FAILURES;
  const rpc_msg_id_t id = RPC_MSG_JOB_LIST_REQUEST;
  rpc_msg_id_t reply_id = 0u;
  char *reply_body = NULL;
  size_t reply_len = 0u;

  if (likely(rpc_session != NULL && code != NULL && msg != NULL && jobs != NULL &&
             n_jobs != NULL)) {
    rpc_result = s_request(rpc_session, id, NULL, 0, &reply_id, &reply_body, &reply_len, msg);

    if (likely(rpc_result == SQC_RESULT_OK)) {
      free(*msg);
      *msg = NULL;
      if (likely(reply_id == RPC_MSG_JOB_LIST_REPLY)) {
        unpack_result = rpc_unpack_job_list_reply(reply_body, reply_len, code, msg, jobs, n_jobs);

        if (likely(unpack_result == SQC_RESULT_OK)) {
          ret = *code;
          sqc_msg_debug(5, "RPC %s: Received a reply: code=%d (%s), msg='%s'",
                        rpc_message_name_string(id), (int) *code, sqc_error_get_string(*code), *msg);
        } else {
          ret = unpack_result;
          rpc_create_message_text(msg, "RPC %s: Failed to unpack the received reply data: %s",
                        rpc_message_name_string(id), rpc_message_id_string(ret));
          rpc_log_debug_msg(5, *msg);
        }

      } else {
        ret = SQC_RESULT_INVALID_OBJECT;
        rpc_create_message_text(msg, "RPC %s: Received an unexpected reply: msg_id=%u (%s)",
                      rpc_message_name_string(id), (unsigned int) reply_id,
                      rpc_message_id_string(reply_id));
        rpc_log_debug_msg(5, *msg);
      }
    } else {
      ret = rpc_result;
      rpc_log_debug_msg(5, *msg);
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("RPC %s: %s\n", rpc_message_name_string(id), sqc_error_get_string(ret));
    rpc_create_message_text(msg, "RPC %s: %s", rpc_message_name_string(id), sqc_error_get_string(ret));
  }

  free(reply_body);

  return ret;
}


static sqc_result_t
s_adm_del_jobs(rpc_session_client_t *rpc_session, const char *user_id, sqc_chrono_t from_time,
               sqc_chrono_t to_time, sqc_result_t *code, char **msg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *body = NULL;
  size_t len = 0u;
  const rpc_msg_id_t id = RPC_MSG_ADM_DEL_JOBS_REQUEST;
  rpc_msg_id_t reply_id = 0u;
  char *reply_body = NULL;
  size_t reply_len = 0u;

  if (rpc_session == NULL || code == NULL || msg == NULL) {
    ret = SQC_RESULT_INVALID_ARGS;
    rpc_create_message_text(msg, "RPC %s: %s", rpc_message_name_string(id), sqc_error_get_string(ret));
    sqc_msg_error("RPC %s: %s\n", rpc_message_name_string(id), sqc_error_get_string(ret));
    return ret;
  }

  do {
    ret = rpc_pack_adm_del_jobs_request(user_id, from_time, to_time, &body, &len);
    if (ret != SQC_RESULT_OK) {
      ret = SQC_RESULT_NO_MEMORY;
      rpc_create_message_text(msg, "RPC %s: Failed to create a request: %s",
                    rpc_message_name_string(id), sqc_error_get_string(ret));
      rpc_log_debug_msg(5, *msg);
      break;
    }
    ret = s_request(rpc_session, id, body, len, &reply_id, &reply_body, &reply_len, msg);
    if (ret != SQC_RESULT_OK) {
      rpc_create_message_text(msg, "RPC %s: Procedure failed, %s",
                    rpc_message_name_string(id), sqc_error_get_string(ret));
      rpc_log_debug_msg(5, *msg);
      break;
    }
    free(*msg);
    *msg = NULL;
    if (reply_id != RPC_MSG_ADM_DEL_JOBS_REPLY) {
      ret = SQC_RESULT_INVALID_OBJECT;
      rpc_create_message_text(msg, "RPC %s: Received an unexpected reply: msg_id=%u (%s)",
                    rpc_message_name_string(id), (unsigned int) reply_id,
                    rpc_message_id_string(reply_id));
      rpc_log_debug_msg(5, *msg);
      break;
    }
    ret = rpc_unpack_adm_del_jobs_reply(reply_body, reply_len, code, msg);
    if (ret != SQC_RESULT_OK) {
      rpc_create_message_text(msg, "RPC %s: Failed to unpack the received reply data: %s",
                    rpc_message_name_string(id), rpc_message_id_string(ret));
      rpc_log_debug_msg(5, *msg);
      break;
    }
    ret = *code;
    sqc_msg_debug(5, "RPC %s: Received a reply: code=%d (%s), msg='%s'",
                  rpc_message_name_string(id), (int) *code, sqc_error_get_string(*code), *msg);
  } while (0);

  free(body);
  free(reply_body);

  return ret;
}


static sqc_result_t
s_adm_add_user(rpc_session_client_t *rpc_session, const char *user_id, sqc_result_t *code, char **msg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *body = NULL;
  size_t len = 0u;
  const rpc_msg_id_t id = RPC_MSG_ADM_ADD_USER_REQUEST;
  rpc_msg_id_t reply_id = 0u;
  char *reply_body = NULL;
  size_t reply_len = 0u;

  if (rpc_session == NULL || code == NULL || msg == NULL) {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("RPC %s: %s\n", rpc_message_name_string(id), sqc_error_get_string(ret));
    rpc_create_message_text(msg, "RPC %s: %s", rpc_message_name_string(id), sqc_error_get_string(ret));
    return ret;
  }

  do {
    ret = rpc_pack_adm_add_user_request(user_id, &body, &len);
    if (ret != SQC_RESULT_OK) {
      ret = SQC_RESULT_NO_MEMORY;
      rpc_create_message_text(msg, "RPC %s: Failed to create a request: %s",
                    rpc_message_name_string(id), sqc_error_get_string(ret));
      rpc_log_debug_msg(5, *msg);
      break;
    }
    ret = s_request(rpc_session, id, body, len, &reply_id, &reply_body, &reply_len, msg);
    if (ret != SQC_RESULT_OK) {
      rpc_create_message_text(msg, "RPC %s: Procedure failed, %s",
                    rpc_message_name_string(id), sqc_error_get_string(ret));
      rpc_log_debug_msg(5, *msg);
      break;
    }
    if (reply_id != RPC_MSG_ADM_ADD_USER_REPLY) {
      ret = SQC_RESULT_INVALID_OBJECT;
      rpc_create_message_text(msg, "RPC %s: Received an unexpected reply: msg_id=%u (%s)",
                    rpc_message_name_string(id), (unsigned int) reply_id,
                    rpc_message_id_string(reply_id));
      rpc_log_debug_msg(5, *msg);
      break;
    }
    ret = rpc_unpack_adm_add_user_reply(reply_body, reply_len, code, msg);
    if (ret != SQC_RESULT_OK) {
      rpc_create_message_text(msg, "RPC %s: Failed to unpack the received reply data: %s",
                    rpc_message_name_string(id), rpc_message_id_string(ret));
      rpc_log_debug_msg(5, *msg);
      break;
    }
    ret = *code;
    sqc_msg_debug(5, "RPC %s: Received a reply: code=%d (%s), msg='%s'",
                  rpc_message_name_string(id), (int) *code, sqc_error_get_string(*code), *msg);
  } while (0);

  free(body);
  free(reply_body);

  return ret;
}


static sqc_result_t
s_adm_set_user_status(rpc_session_client_t *rpc_session, const char *user_id, bool enabled,
                      sqc_result_t *code, char **msg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *body = NULL;
  size_t len = 0u;
  const rpc_msg_id_t id = RPC_MSG_ADM_SET_USER_STATUS_REQUEST;
  rpc_msg_id_t reply_id = 0u;
  char *reply_body = NULL;
  size_t reply_len = 0u;

  if (rpc_session == NULL || code == NULL || msg == NULL) {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("RPC %s: %s\n", rpc_message_name_string(id), sqc_error_get_string(ret));
    rpc_create_message_text(msg, "RPC %s: %s", rpc_message_name_string(id), sqc_error_get_string(ret));
    return ret;
  }

  do {
    ret = rpc_pack_adm_set_user_status_request(user_id, enabled, &body, &len);
    if (ret != SQC_RESULT_OK) {
      ret = SQC_RESULT_NO_MEMORY;
      rpc_create_message_text(msg, "RPC %s: Failed to create a request: %s",
                    rpc_message_name_string(id), sqc_error_get_string(ret));
      rpc_log_debug_msg(5, *msg);
      break;
    }
    ret = s_request(rpc_session, id, body, len, &reply_id, &reply_body, &reply_len, msg);
    if (ret != SQC_RESULT_OK) {
      rpc_create_message_text(msg, "RPC %s: Procedure failed, %s",
                              rpc_message_name_string(id), sqc_error_get_string(ret));
      rpc_log_debug_msg(5, *msg);
      break;
    }
    if (reply_id != RPC_MSG_ADM_SET_USER_STATUS_REPLY) {
      ret = SQC_RESULT_INVALID_OBJECT;
      rpc_create_message_text(msg, "RPC %s: Received an unexpected reply: msg_id=%u (%s)",
                              rpc_message_name_string(id), (unsigned int) reply_id,
                              rpc_message_id_string(reply_id));
      rpc_log_debug_msg(5, *msg);
      break;
    }
    ret = rpc_unpack_adm_set_user_status_reply(reply_body, reply_len, code, msg);
    if (ret != SQC_RESULT_OK) {
      rpc_create_message_text(msg, "RPC %s: Failed to unpack the received reply data: %s",
                    rpc_message_name_string(id), rpc_message_id_string(ret));
      rpc_log_debug_msg(5, *msg);
      break;
    }
    ret = *code;
    sqc_msg_debug(5, "RPC %s: Received a reply: code=%d (%s), msg='%s'",
                  rpc_message_name_string(id), (int) *code, sqc_error_get_string(*code), *msg);
  } while (0);

  free(body);
  free(reply_body);

  return ret;
}


//
// Exported APIs
//

sqc_result_t
rpc_session_client_create(rpc_session_client_t *rpc_session, const char *server, bool prefer_ipv4,
                          rpc_auth_method_t auth_method, const sqc_tls_conf_t tls_conf,
                          const rpc_jwt_client_ctx_t *jwt_ctx) {
  return s_create(rpc_session, server, prefer_ipv4, auth_method, tls_conf, jwt_ctx);
}


sqc_result_t
rpc_session_client_create_from_conf_dir(rpc_session_client_t *rpc_session, const char *server,
                                        bool prefer_ipv4, rpc_auth_method_t auth_method,
                                        const char *dir) {
  return s_create_from_conf_dir(rpc_session, server, prefer_ipv4, auth_method, dir);
}


void
rpc_session_client_destroy(rpc_session_client_t *rpc_session) {
  s_destroy(rpc_session);
}


sqc_result_t
rpc_session_client_submit_job(rpc_session_client_t *rpc_session, uint8_t priority, const char *qprogram,
                              sqc_rpc_sched_circuit_fmt_t circuit_fmt, size_t shots, sqc_rpc_sched_qc_type_t qc_type,
                              sqc_rpc_sched_transpiler_t transpiler, const char *remark, const char *user_token,
                              sqc_result_t *code, char **msg, char **job_id) {
  return s_submit_job(rpc_session, priority, qprogram, circuit_fmt, shots, qc_type, transpiler,
                      remark, user_token, code, msg, job_id);
}


sqc_result_t
rpc_session_client_job_status(rpc_session_client_t *rpc_session, const char *job_id,
                              sqc_result_t *code, char **msg, sqc_rpc_sched_job_status_t *status,
                              char **qc_job_id, char **result) {
  return s_job_status(rpc_session, job_id, code, msg, status, qc_job_id, result);
}


sqc_result_t
rpc_session_client_cancel_job(rpc_session_client_t *rpc_session, const char *job_id,
                              sqc_result_t *code, char **msg) {
  return s_cancel_job(rpc_session, job_id, code, msg);
}


sqc_result_t
rpc_session_client_delete_job(rpc_session_client_t *rpc_session, const char *job_id,
                              sqc_result_t *code, char **msg) {
  return s_delete_job(rpc_session, job_id, code, msg);
}


sqc_result_t
rpc_session_client_job_list(rpc_session_client_t *rpc_session, sqc_result_t *code,
                            char **msg, rpc_job_info_t ***jobs, size_t *n_jobs) {
  return s_job_list(rpc_session, code, msg, jobs, n_jobs);
}


sqc_result_t
rpc_session_client_adm_del_jobs(rpc_session_client_t *rpc_session, const char *user_id, sqc_chrono_t from_time,
                                sqc_chrono_t to_time, sqc_result_t *code, char **msg) {
  return s_adm_del_jobs(rpc_session, user_id, from_time, to_time, code, msg);
}


sqc_result_t
rpc_session_client_adm_add_user(rpc_session_client_t *rpc_session, const char *user_id,
                                sqc_result_t *code, char **msg) {
  return s_adm_add_user(rpc_session, user_id, code, msg);
}


sqc_result_t
rpc_session_client_adm_set_user_status(rpc_session_client_t *rpc_session, const char *user_id, bool enabled,
                                       sqc_result_t *code, char **msg) {
  return s_adm_set_user_status(rpc_session, user_id, enabled, code, msg);
}
