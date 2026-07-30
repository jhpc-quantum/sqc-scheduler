#include <munge.h>

#include "sqc_apis.h"
#include "rpc_munge.h"

#define MUNGE_SSH_CRED_CMD "ssh %s 'munge -s %s:%d'"

//
// Issue a MUNGE credential for RPC client.
//
static inline sqc_result_t
s_get_cred(rpc_session_client_t *rpc_session, char **cred, size_t *credlen) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t getlname_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t getpname_result = SQC_RESULT_ANY_FAILURES;
  char *local_name = NULL;
  int local_port = 0;
  char *peer_name = NULL;
  int peer_port = 0;
  char cmd[BUFSIZ] = {0};
  char result[BUFSIZ] = {0};
  char *cred_end;
  FILE *fp = NULL;

  if (likely(rpc_session != NULL && *rpc_session != NULL && cred != NULL)) {
    getlname_result = rpc_session_client_get_local_name(rpc_session, &local_name, &local_port);
    getpname_result = rpc_session_client_get_peer_name(rpc_session, &peer_name, &peer_port);

    if (likely(getlname_result == SQC_RESULT_OK && getpname_result == SQC_RESULT_OK)) {
      snprintf(cmd, sizeof(cmd), MUNGE_SSH_CRED_CMD, peer_name, local_name, local_port); // cmd to define
      fp = popen(cmd, "r"); // XXX: It assumed to no passphrase ssh login.
      if (likely(fp != NULL)) {
        fgets(result, sizeof(result), fp);
        pclose(fp);

        cred_end = strchr(result, '\n');
        if (likely(cred_end != NULL)) {
          *cred_end = '\0';
          *cred = strdup(result);
          if (likely(*cred != NULL)) {
            ret = SQC_RESULT_OK;
            if (credlen != NULL) {
              *credlen = strlen(*cred) + 1; // include '\0'
            }
            sqc_msg_debug(5, "MUNGE credential created\n");
          } else {
            ret = SQC_RESULT_NO_MEMORY;
            sqc_msg_error("%s\n", sqc_error_get_string(ret));
          }
        } else {
          ret = SQC_RESULT_INVALID_OBJECT;
          sqc_msg_error("Failed to decode MUNGE credential, due to unexpected "
                        "credential result: %s\n", result);
        }
      } else {
        ret = SQC_RESULT_POSIX_API_ERROR;
        sqc_msg_error("Failed to create MUNGE credential: %s\n", cmd);
      }
    } else {
      if (likely(getlname_result != SQC_RESULT_OK)) {
        ret = getlname_result;
      } else if (likely(getpname_result != SQC_RESULT_OK)) {
        ret = getpname_result;
      }
      sqc_msg_error("Failed to get sockname: %s\n", sqc_error_get_string(ret));
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  free(local_name);
  free(peer_name);
  return ret;
}


//
// Deode a Munge credential in 'rpc_auth_data_munge_t' object to UID, GID and
// a port number.
//
static inline sqc_result_t
s_munge_decode(const char *cred, uid_t *uid, gid_t *gid, char **cred_addr_port) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  void *buf = NULL;
  int buflen = 0;
  uid_t tmp_uid = 0u;
  gid_t tmp_gid = 0u;

  if (likely(IS_VALID_STRING(cred) && uid != NULL && gid != NULL && cred_addr_port != NULL)) {

    munge_err_t munge_err = munge_decode((const char *)cred, NULL, &buf, &buflen, &tmp_uid,
                                         &tmp_gid);
    if (likely(munge_err == EMUNGE_SUCCESS)) {
      if (likely((size_t) buflen > strlen("0.0.0.0:0"))) {
        ret = SQC_RESULT_OK;
        *uid = tmp_uid;
        *gid = tmp_gid;
        *cred_addr_port = buf;
        sqc_msg_debug(5, "A MUNGE credential decoded");
      } else {
        ret = SQC_RESULT_INVALID_OBJECT;
        sqc_msg_error("Failed to decode MUNGE credential, due to unexpected "
                      "credential length\n");
      }
    } else {
      ret = SQC_RESULT_AUTHENTICATION_ERROR;
      sqc_msg_error("MUNGE authentation failed, %s\n", munge_strerror(munge_err));
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s\n", sqc_error_get_string(ret));
  }

  return ret;
}


//
// Size of buffer to store a passwd entry for getpwnam_r() and getpwuid_r().
//
#define GETPW_R_BUF_SIZE 8192

//
// Validate a munge credential.
//
static inline sqc_result_t
s_validate_cred(rpc_session_server_t *rpc_session, const char *cred, uint64_t conn_id,
                char **pwname) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t getname_result = SQC_RESULT_ANY_FAILURES;
  sqc_result_t decode_result = SQC_RESULT_ANY_FAILURES;
  char *peer_name = NULL;
  int peer_port = 0;
  uid_t cred_uid = 0u;
  gid_t cred_gid = 0u;
  char *cred_addr_port = NULL;
  char addr_port[BUFSIZ] = {0};
  int addr_port_len = 0;

  if (likely(rpc_session != NULL && *rpc_session != NULL && IS_VALID_STRING(cred) &&
             pwname != NULL)) {
    decode_result = s_munge_decode(cred, &cred_uid, &cred_gid, &cred_addr_port);

    if (likely(decode_result == SQC_RESULT_OK)) {
      getname_result = rpc_session_server_get_peer_name(rpc_session, &peer_name, &peer_port);

      if (likely(getname_result == SQC_RESULT_OK)) {
        addr_port_len = snprintf(addr_port, sizeof(addr_port), "%s:%d", peer_name, peer_port);
        if ((size_t) addr_port_len == strlen(cred_addr_port)
              && strncmp(addr_port, cred_addr_port, (size_t) addr_port_len) == 0) {
          struct passwd pw;
          struct passwd *pw_result = NULL;
          char buf[GETPW_R_BUF_SIZE];
          int tmp_err = 0;

          errno = 0;
          tmp_err = getpwuid_r(cred_uid, &pw, buf, sizeof(buf), &pw_result);

          if (likely(pw_result != NULL)) {
            if (likely(cred_uid == pw.pw_uid && cred_gid == pw.pw_gid)) {
              *pwname = strdup(pw.pw_name);
              if (likely(*pwname != NULL)) {
                ret = SQC_RESULT_OK;
                sqc_msg_info("MUNGE authentication succeeded: conn-id=%llu\n",
                             (unsigned long long) conn_id);

              } else {
                ret = SQC_RESULT_NO_MEMORY;
                sqc_msg_error("%s\n", sqc_error_get_string(ret));
              }
            } else {
              ret = SQC_RESULT_AUTHENTICATION_ERROR;
              sqc_msg_error("MUNGE authentication failed due to UID/GID mismatch: "
                            "cred=%u/%u, server=%u%u, conn-id=%llu\n",
                            (unsigned int) cred_uid, (unsigned int) cred_gid,
                            (unsigned int) pw.pw_uid, (unsigned int) pw.pw_gid,
                            (unsigned long long) conn_id);
            }
          } else if (tmp_err == 0) {
            ret = SQC_RESULT_AUTHENTICATION_ERROR;
          } else {
            ret = SQC_RESULT_POSIX_API_ERROR;
            sqc_msg_error("MUNGE authentication failed due to "
                          "an error at getpwuid_r(), %s: conn-id=%llu\n",
                          strerror(tmp_err), (unsigned long long) conn_id);
            errno = tmp_err;
          }
        } else {
          ret = SQC_RESULT_AUTHENTICATION_ERROR;
          sqc_msg_error("MUNGE authentication failed due to port number mismatch: "
                        "credential=%s, socket=%u, conn-id=%llu\n",
                        cred_addr_port, (unsigned int) peer_port,
                        (unsigned long long) conn_id);
        }
      } else {
        ret = getname_result;
        sqc_msg_error("MUNGE authentication failed due to an error "
                      "at getting a peer name of the socket, %s: conn-id=%llu\n",
                      sqc_error_get_string(getname_result), (unsigned long long) conn_id);
      }

    } else {
      ret = decode_result;
      sqc_msg_error("MUNGE authentication failed due to an error at decoding a credential, "
                    "%s: conn-id-%llu\n",
                    sqc_error_get_string(decode_result), (unsigned long long) conn_id);
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("%s: conn-id=%llu\n", sqc_error_get_string(ret), (unsigned long long) conn_id);
  }

  free(cred_addr_port);
  free(peer_name);
  return ret;
}


/*
 * Exported APIs
 */

sqc_result_t
rpc_munge_client_get_cred(rpc_session_client_t *rpc_session, char **cred, size_t *credlen) {
  return s_get_cred(rpc_session, cred, credlen);
}


sqc_result_t
rpc_munge_server_validate_cred(rpc_session_server_t *rpc_session, const char *cred,
                               uint64_t conn_id, char **user_name) {
  return s_validate_cred(rpc_session, cred, conn_id, user_name);
}
