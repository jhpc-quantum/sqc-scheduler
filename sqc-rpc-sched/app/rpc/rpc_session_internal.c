#include "sqc_apis.h"
#include "dbmgr.h"
#include "rpc_msg_util.h"
#include "rpc_session_internal.h"


//
// Establish a TLS session on the current TCP connection.
//
static inline sqc_result_t
s_establish_tls(sqc_session_t *session, sqc_tls_conf_t tls_conf, bool do_mutual_auth,
                bool use_proxy_cert) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t setup_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t establish_result = SQC_RESULT_ANY_FAILURES;

  setup_result = sqc_session_setup_tls(session, tls_conf, do_mutual_auth, use_proxy_cert);
  if (likely(setup_result == SQC_RESULT_OK)) {
    establish_result = sqc_session_establish_tls(session);
    if (likely(establish_result == SQC_RESULT_OK)) {
      ret = SQC_RESULT_OK;
      sqc_msg_info("RPC Session: Established a TLS session\n");
    } else {
      ret = establish_result;
      sqc_msg_info("RPC Session: Failed to establish a TLS session\n");
    }
  } else {
    ret = setup_result;
    sqc_msg_info("RPC Session: Failed to setup TLS session\n");
  }

  return ret;
}


//
// Get a local address and a local port number of the current TCP connection.
//
static inline sqc_result_t
s_get_local_name(sqc_session_t *session, char **name, int *port) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  *name = NULL;
  ret = sqc_endpoint_get_local_name((sqc_endpoint_t *)session, name, 0u, port, false);
  if (likely(ret == SQC_RESULT_OK)) {
    sqc_msg_debug(5, "Got localname of the RPC session: name=%s, port=%d\n", *name, *port);
  } else {
    sqc_msg_debug(5, "Failed to get localname of the RPC session, %s\n",
                  sqc_error_get_string(ret));
  }

  return ret;
}


//
// Get a peer address and a peer port number of the current TCP connection.
//
static inline sqc_result_t
s_get_peer_name(sqc_session_t *session, char **name, int *port) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  *name = NULL;
  ret = sqc_endpoint_get_peer_name((sqc_endpoint_t *)session, name, 0u, port, false);
  if (likely(ret == SQC_RESULT_OK)) {
    sqc_msg_debug(5, "Got peername of the RPC session: name=%s, port=%d\n", *name, *port);
  } else {
    sqc_msg_debug(5, "Failed to get peername of the RPC session, %s\n",
                  sqc_error_get_string(ret));
  }

  return ret;
}


//
// Send data with the specified size.
//
static inline sqc_result_t
s_send_sized_data(sqc_session_t *session, const char *data, size_t len) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t write_result = SQC_RESULT_ANY_FAILURES;
  const char *data_p = data;
  ssize_t left_len = (ssize_t) len;
  ssize_t result_len = 0;

  if (likely(session != NULL && *session != NULL && data != NULL)) {
    while (left_len > 0) {
      write_result = sqc_session_write(session, data_p, (size_t)left_len, &result_len);
      if (likely(write_result == SQC_RESULT_OK)) {
        if (likely(result_len == left_len)) {
          ret = SQC_RESULT_OK;
          sqc_msg_debug(5, "Finished sending data to the peer: %u bytes\n",
                        (unsigned int) len);
          break;
        } else if (likely(result_len > 0)) {
          left_len -= result_len;
          data_p += result_len;
          sqc_msg_debug(5, "Sent data to the peer: %u / %u bytes\n",
                        (unsigned int) len - (unsigned int) left_len,
                        (unsigned int) len);
        } else {
          ret = SQC_RESULT_EOF;
          sqc_msg_debug(5, "Received EOF from the peer\n");
          break;
        }
      } else if (write_result == SQC_RESULT_POSIX_API_ERROR &&
                 errno == EINTR) {
        continue;
      } else {
        if (write_result == SQC_RESULT_POSIX_API_ERROR &&
            (errno == 0 || errno == EPIPE || errno == ECONNRESET)) {
          ret = SQC_RESULT_EOF;
        } else {
          ret = write_result;
        }
        sqc_msg_notice("Failed to send data to the peer, %s: errno=%s\n",
                       sqc_error_get_string(ret), strerror(errno));
        break;
      }
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_info("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


//
// Send an RPC message.
//
static inline sqc_result_t
s_send_msg(sqc_session_t *session, rpc_msg_id_t id, const char *body, size_t len) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t send_result = SQC_RESULT_ANY_FAILURES;
  char header[SQC_PRC_HEADER_LEN];

  if (likely(session != NULL && *session != NULL && (body != NULL || len == 0))) {
    *(uint32_t *) (header + SQC_PRC_HEADER_ID_OFFSET) = htonl(id);
    *(uint32_t *) (header + SQC_PRC_HEADER_LEN_OFFSET) = htonl((uint32_t) len);
    send_result = s_send_sized_data(session, header, sizeof(header));

    if (likely(send_result == SQC_RESULT_OK)) {
      if (len > 0) {
        ret = s_send_sized_data(session, body, len);
      } else {
        ret = SQC_RESULT_OK;
      }
      if (likely(ret == SQC_RESULT_OK)) {
        sqc_msg_debug(5, "Sent an RPC message: id=%u (%s), len=%u\n",
                      (unsigned int) id, rpc_message_id_string(id), (unsigned int) len);
      } else {
        sqc_msg_info("Failed to send an RPC message, %s\n",
                     sqc_error_get_string(ret));
      }
    } else {
      ret = send_result;
      sqc_msg_info("RPC Session: Failed to send a message, %s\n",
                   sqc_error_get_string(ret));
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_info("RPC Session: %s\n", sqc_error_get_string(ret));
  }

  return ret;
}


//
// Receive data with the specified size.
//
static inline sqc_result_t
s_recv_sized_data(sqc_session_t *session, char *data, size_t len) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t read_result = SQC_RESULT_ANY_FAILURES;
  char *data_p = data;
  ssize_t left_len = (ssize_t) len;
  ssize_t result_len = 0;

  if (likely(session != NULL && *session != NULL && data != NULL)) {
    while (left_len > 0) {
      read_result = sqc_session_read(session, data_p, (size_t)left_len, &result_len);
      if (likely(read_result == SQC_RESULT_OK)) {
        if (likely(result_len == left_len)) {
          ret = SQC_RESULT_OK;
          sqc_msg_debug(5, "Finished receiving data from the peer: %u bytes\n",
                        (unsigned int) len);
          break;
        } else if (likely(result_len > 0)) {
          left_len -= result_len;
          data_p += result_len;
          sqc_msg_debug(5, "Received data from the peer: %u / %u bytes\n",
                        (unsigned int) len - (unsigned int) left_len,
                        (unsigned int) len);
        } else {
          ret = SQC_RESULT_EOF;
          sqc_msg_debug(5, "Received EOF from the peer\n");
          break;
        }
      } else if (read_result == SQC_RESULT_POSIX_API_ERROR &&
                 errno == EINTR) {
        continue;
      } else {
        if (read_result == SQC_RESULT_POSIX_API_ERROR &&
            (errno == 0 || errno == EPIPE || errno == ECONNRESET)) {
          ret = SQC_RESULT_EOF;
        } else {
          ret = read_result;
          sqc_msg_debug(5, "failed to receive data from the peer, %s: errno=%s\n",
                        sqc_error_get_string(ret), strerror(errno));
        }
        break;
      }
    }

    if (likely(left_len == 0)) {
      ret = SQC_RESULT_OK;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_info("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


//
// Receive an RPC message.
//
static inline sqc_result_t
s_recv_msg(sqc_session_t *session, rpc_msg_id_t *id, char **body, size_t *len) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t recv_result = SQC_RESULT_ANY_FAILURES;
  char header[SQC_PRC_HEADER_LEN];

  if (likely(session != NULL && *session != NULL && id != NULL &&
             len != NULL)) {
    recv_result = s_recv_sized_data(session, header, sizeof(header));

    if (likely(recv_result == SQC_RESULT_OK)) {
      *id = ntohl(*(uint32_t *) (header + SQC_PRC_HEADER_ID_OFFSET));
      *len = ntohl(*(uint32_t *) (header + SQC_PRC_HEADER_LEN_OFFSET));
      *body = malloc(*len);

      if (likely(*body != NULL)) {
        ret = s_recv_sized_data(session, *body, *len);
        if (ret == SQC_RESULT_OK) {
          sqc_msg_debug(5, "RPC Session: Received a message: id=%u (%s), len=%u\n",
                        (unsigned int) *id, rpc_message_id_string(*id), (unsigned int) *len);
        } else if (ret == SQC_RESULT_EOF) {
          sqc_msg_info("RPC Session: Disconnected\n");
        } else {
          sqc_msg_info("RPC Session: Failed to receive a message\n");
        }
      } else {
        ret = SQC_RESULT_NO_MEMORY;
        sqc_msg_notice("RPC Session: Failed to receive a message, %s\n",
                       sqc_error_get_string(ret));
      }

    } else {
      ret = recv_result;
      sqc_msg_notice("RPC Session: Received data that is not an RPC message such as EOF, %s\n",
                     sqc_error_get_string(ret));
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_info("RPC Session: %s\n", sqc_error_get_string(ret));
  }

  return ret;
}


//
// Exported APIs
//

sqc_result_t
rpc_session_client_establish_tls(rpc_session_client_t *rpc_session, bool do_mutual_auth,
                                 bool use_proxy_cert) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(rpc_session != NULL && *rpc_session != NULL)) {
    ret = s_establish_tls(&(*rpc_session)->session_, (*rpc_session)->tls_conf_,
                          do_mutual_auth, use_proxy_cert);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_debug(5, "%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


sqc_result_t
rpc_session_server_establish_tls(rpc_session_server_t *rpc_session, bool do_mutual_auth,
                                 bool use_proxy_cert) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(rpc_session != NULL && *rpc_session != NULL)) {
    ret = s_establish_tls(&(*rpc_session)->session_, (*rpc_session)->tls_conf_,
                          do_mutual_auth, use_proxy_cert);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_info("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


sqc_result_t
rpc_session_client_get_local_name(rpc_session_client_t *rpc_session, char **name, int *port) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(rpc_session != NULL && *rpc_session != NULL)) {
    ret = s_get_local_name(&(*rpc_session)->session_, name, port);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_info("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


sqc_result_t
rpc_session_server_get_local_name(rpc_session_server_t *rpc_session, char **name, int *port) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(rpc_session != NULL && *rpc_session != NULL)) {
    ret = s_get_local_name(&(*rpc_session)->session_, name, port);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_info("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


sqc_result_t
rpc_session_client_get_peer_name(rpc_session_client_t *rpc_session, char **name, int *port) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(rpc_session != NULL && *rpc_session != NULL)) {
    ret = s_get_peer_name(&(*rpc_session)->session_, name, port);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_info("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


sqc_result_t
rpc_session_server_get_peer_name(rpc_session_server_t *rpc_session, char **name, int *port) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(rpc_session != NULL && *rpc_session != NULL)) {
    ret = s_get_peer_name(&(*rpc_session)->session_, name, port);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_info("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


sqc_result_t
rpc_session_client_send_msg(rpc_session_client_t *rpc_session, rpc_msg_id_t id, const char *body,
                            size_t len) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(rpc_session != NULL && *rpc_session != NULL)) {
    ret = s_send_msg(&(*rpc_session)->session_, id, body, len);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_info("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


sqc_result_t
rpc_session_server_send_msg(rpc_session_server_t *rpc_session, rpc_msg_id_t id, const char *body,
                            size_t len) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(rpc_session != NULL && *rpc_session != NULL)) {
    ret = s_send_msg(&(*rpc_session)->session_, id, body, len);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_info("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


sqc_result_t
rpc_session_client_recv_msg(rpc_session_client_t *rpc_session, rpc_msg_id_t *id, char **body,
                            size_t *len) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(rpc_session != NULL && *rpc_session != NULL)) {
    ret = s_recv_msg(&(*rpc_session)->session_, id, body, len);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_info("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


sqc_result_t
rpc_session_server_recv_msg(rpc_session_server_t *rpc_session, rpc_msg_id_t *id, char **body,
                            size_t *len) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(rpc_session != NULL && *rpc_session != NULL)) {
    ret = s_recv_msg(&(*rpc_session)->session_, id, body, len);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_info("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}
