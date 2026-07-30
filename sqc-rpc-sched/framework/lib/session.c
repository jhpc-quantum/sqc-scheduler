#include "sqc_apis.h"
#include "endpoint_internal.h"
#include "session_internal.h"





typedef struct sqc_session_record {
  sqc_endpoint_record ep_;

  sqc_session_type_t type_;

  sqc_tls_session_ctx_t tls_ctx_;

  sqc_session_read_proc_t reader_;
  sqc_session_write_proc_t writer_;
} sqc_session_record;





static sqc_result_t
s_read_fd(const sqc_session_t *sptr, void *buf, size_t len,
	  ssize_t *actual_read) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  int fd = -1;

  if (likely(sptr != NULL && *sptr != NULL &&
             (fd = (*sptr)->ep_.fd_) >= 0 && actual_read != NULL)) {
    ssize_t n = -1;
    *actual_read = 0;
    errno = 0;
    if (likely((n = read(fd, buf, len)) >= 0)) {
      *actual_read = n;
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static sqc_result_t
s_write_fd(const sqc_session_t *sptr, const void *buf, size_t len,
	   ssize_t *actual_write) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  int fd = -1;

  if (likely(sptr != NULL && *sptr != NULL &&
             (fd = (*sptr)->ep_.fd_) >= 0 && actual_write != NULL)) {
    ssize_t n = -1;
    *actual_write = 0;
    errno = 0;
    if (likely((n = write(fd, buf, len)) >= 0)) {
      *actual_write = n;
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static sqc_result_t
s_read_tls_session(const sqc_session_t *sptr, void *buf, size_t len,
                   ssize_t *actual_read) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t read_result = SQC_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL && (*sptr)->tls_ctx_ != NULL &&
             ((*sptr)->type_ == SQC_SESSION_TYPE_TLS_1WAY_AUTH_INITIATOR ||
              (*sptr)->type_ == SQC_SESSION_TYPE_TLS_2WAY_AUTH_INITIATOR ||
              (*sptr)->type_ == SQC_SESSION_TYPE_TLS_ACCEPTOR) &&
             actual_read != NULL)) {
    int n = -1;
    *actual_read = 0;
    errno = 0;
    read_result = sqc_tls_session_read((*sptr)->tls_ctx_, buf, (int)len, &n);
    if (read_result == SQC_RESULT_OK) {
      *actual_read = n;
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static sqc_result_t
s_write_tls_session(const sqc_session_t *sptr, const void *buf, size_t len,
                    ssize_t *actual_write) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t write_result = SQC_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL && (*sptr)->tls_ctx_ != NULL &&
             ((*sptr)->type_ == SQC_SESSION_TYPE_TLS_1WAY_AUTH_INITIATOR ||
              (*sptr)->type_ == SQC_SESSION_TYPE_TLS_2WAY_AUTH_INITIATOR ||
              (*sptr)->type_ == SQC_SESSION_TYPE_TLS_ACCEPTOR) &&
             actual_write != NULL)) {
    int n = -1;
    *actual_write = 0;
    errno = 0;
    write_result = sqc_tls_session_write((*sptr)->tls_ctx_, buf, (int)len, &n);
    if (write_result == SQC_RESULT_OK) {
      *actual_write = n;
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_wait_io_fd(sqc_session_t *sptr, sqc_chrono_t to, bool to_read) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  int fd = -1;

  if (likely(sptr != NULL && *sptr != NULL && (fd = (*sptr)->ep_.fd_) >= 0)) {
    struct pollfd fds[1];
    struct timespec ts;
    struct timespec *tsptr = NULL;
    int st;
    bool loop = true;

    if (to >= 0) {
      NSEC_TO_TS(to, ts);
      tsptr = &ts;
    }
    fds[0].fd = fd;
    fds[0].events = (to_read == true) ? POLLIN : POLLOUT;

    do {
      errno = 0;
      st = ppoll(fds, 1, tsptr, NULL);
      switch (st) {
        case 0: {
          ret = SQC_RESULT_TIMEDOUT;
          loop = false;
          break;
        }
        case -1: {
          if (errno != EINTR) {
            ret = SQC_RESULT_POSIX_API_ERROR;
            loop = false;
          }
          break;
        }
        default: {
          ret = SQC_RESULT_OK;
          loop = false;
        }
      }
    } while (loop == true);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_create_session_client(sqc_session_t *sptr, size_t sz,
                        const char *spec, bool prefer_ipv4) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL)) {
    if (likely(sz < sizeof(sqc_session_record))) {
      sz = sizeof(sqc_session_record);
    }
    if (likely((ret = sqc_endpoint_create((sqc_endpoint_t *)sptr, sz,
                                          SQC_ENDPOINT_TYPE_INET_INITIATOR,
                                          spec, prefer_ipv4)) ==
               SQC_RESULT_OK)) {
      (*sptr)->type_ = SQC_SESSION_TYPE_RAW_CLIENT;
      (*sptr)->tls_ctx_ = NULL;
      (*sptr)->reader_ = s_read_fd;
      (*sptr)->writer_ = s_write_fd;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_create_session_server(sqc_session_t *sptr, size_t sz, int fd,
                        struct sockaddr_storage *saptr, socklen_t salen) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL)) {
    if (likely(sz < sizeof(sqc_session_record))) {
      sz = sizeof(sqc_session_record);
    }
#define f__(...) sqc_endpoint_create_by_accepted_fd_and_sockaddr(__VA_ARGS__)
    if (likely((ret = f__((sqc_endpoint_t *)sptr, sz, fd, saptr, salen)) ==
               SQC_RESULT_OK)) {
      (*sptr)->type_ = SQC_SESSION_TYPE_RAW_SERVER;
      (*sptr)->tls_ctx_ = NULL;
      (*sptr)->reader_ = s_read_fd;
      (*sptr)->writer_ = s_write_fd;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }
#undef f__
  return ret;
}


static inline sqc_result_t
s_setup_tls(sqc_session_t *sptr, const sqc_tls_conf_t conf,
            bool do_mutual_auth, bool use_proxy_cert) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t create_result = SQC_RESULT_ANY_FAILURES;
  sqc_tls_session_ctx_t tls_ctx = NULL;
  enum tls_role role;

  if (likely(sptr != NULL &&
             ((*sptr)->type_ == SQC_SESSION_TYPE_RAW_CLIENT ||
              (*sptr)->type_ == SQC_SESSION_TYPE_RAW_SERVER) &&
             (*sptr)->tls_ctx_ == NULL &&
             conf != NULL)) {

    if ((*sptr)->type_ == SQC_SESSION_TYPE_RAW_CLIENT) {
      role = TLS_ROLE_CLIENT;
    } else {
      role = TLS_ROLE_SERVER;
    }
    create_result = sqc_tls_session_create_ctx(&tls_ctx, conf, role,
                                               do_mutual_auth,
                                               use_proxy_cert);
    if (likely(create_result == SQC_RESULT_OK)) {
      (*sptr)->tls_ctx_ = tls_ctx;
      ret = SQC_RESULT_OK;
    } else {
      ret = create_result;
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline bool
s_is_tls_prepared(sqc_session_t *sptr)
{
  sqc_result_t ret = false;

  if (likely(sptr != NULL && *sptr != NULL)) {
    ret = ((*sptr)->tls_ctx_ != NULL);
  }

  return ret;
}


static inline sqc_result_t
s_establish_tls(sqc_session_t *sptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t establish_result = SQC_RESULT_ANY_FAILURES;
  bool do_mutual_auth = false;

  if (likely(sptr != NULL &&
             ((*sptr)->type_ == SQC_SESSION_TYPE_RAW_CLIENT ||
              (*sptr)->type_ == SQC_SESSION_TYPE_RAW_SERVER) &&
             (*sptr)->tls_ctx_ != NULL &&
             (*sptr)->ep_.fd_ >= 0)) {
    establish_result = sqc_tls_session_establish((*sptr)->tls_ctx_,
                                                 (*sptr)->ep_.fd_);
    do_mutual_auth = sqc_tls_session_get_do_mutual_auth((*sptr)->tls_ctx_);

    if (establish_result == SQC_RESULT_OK) {
      if ((*sptr)->type_ == SQC_SESSION_TYPE_RAW_CLIENT) {
        if (do_mutual_auth == true) {
          (*sptr)->type_ = SQC_SESSION_TYPE_TLS_1WAY_AUTH_INITIATOR;
        } else {
          (*sptr)->type_ = SQC_SESSION_TYPE_TLS_2WAY_AUTH_INITIATOR;
        }
      } else {
        (*sptr)->type_ = SQC_SESSION_TYPE_TLS_ACCEPTOR;
      }
      (*sptr)->reader_ = s_read_tls_session;
      (*sptr)->writer_ = s_write_tls_session;
      ret = SQC_RESULT_OK;
    } else {
      ret = establish_result;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_session_read(sqc_session_t *sptr, void *buf, size_t len,
               ssize_t *actual_read) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL)) {
    ret = (*sptr)->reader_(sptr, buf, len, actual_read);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_session_write(sqc_session_t *sptr, const void *buf, size_t len,
                ssize_t *actual_write) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(sptr != NULL && *sptr != NULL)) {
    ret = (*sptr)->writer_(sptr, buf, len, actual_write);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline char *
s_peer_subjectdn_oneline(sqc_session_t *sptr)
{
  char *ret = NULL;

  if (likely(sptr != NULL && *sptr != NULL && (*sptr)->tls_ctx_ != NULL)) {
    return sqc_tls_session_peer_subjectdn_oneline((*sptr)->tls_ctx_);
  }

  return ret;
}


static inline char *
s_peer_subjectdn_rfc2253(sqc_session_t *sptr)
{
  char *ret = NULL;

  if (likely(sptr != NULL && *sptr != NULL && (*sptr)->tls_ctx_ != NULL)) {
    return sqc_tls_session_peer_subjectdn_rfc2253((*sptr)->tls_ctx_);
  }

  return ret;
}


static inline char *
s_peer_subjectdn_gsi(sqc_session_t *sptr)
{
  char *ret = NULL;

  if (likely(sptr != NULL && *sptr != NULL && (*sptr)->tls_ctx_ != NULL)) {
    return sqc_tls_session_peer_subjectdn_gsi((*sptr)->tls_ctx_);
  }

  return ret;
}


static inline char *
s_peer_cn(sqc_session_t *sptr)
{
  char *ret = NULL;

  if (likely(sptr != NULL && *sptr != NULL && (*sptr)->tls_ctx_ != NULL)) {
    return sqc_tls_session_peer_cn((*sptr)->tls_ctx_);
  }

  return ret;
}





/*
 * Exported APIs
 */


sqc_result_t
sqc_session_create_client(sqc_session_t *sptr, size_t sz,
			  const char *spec, bool prefer_ipv4) {
  return s_create_session_client(sptr, sz, spec, prefer_ipv4);
}


sqc_result_t
sqc_session_create_server(sqc_session_t *sptr, size_t sz, int fd,
                          struct sockaddr_storage *saptr, socklen_t salen) {
  return s_create_session_server(sptr, sz, fd, saptr, salen);
}


void
sqc_session_destroy(sqc_session_t *sptr) {
  if (likely(sptr != NULL && *sptr != NULL)) {
    sqc_tls_session_destroy_ctx((*sptr)->tls_ctx_);
    sqc_endpoint_destroy((sqc_endpoint_t *)sptr);
    *sptr = NULL;
  }
}


sqc_result_t
sqc_session_setup_tls(sqc_session_t *sptr, const sqc_tls_conf_t conf,
                      bool do_mutual_auth, bool use_proxy_cert) {
  return s_setup_tls(sptr, conf, do_mutual_auth, use_proxy_cert);
}


bool
sqc_session_is_tls_prepared(sqc_session_t *sptr) {
  return s_is_tls_prepared(sptr);
}


sqc_result_t
sqc_session_establish_tls(sqc_session_t *sptr) {
  return s_establish_tls(sptr);
}


sqc_result_t
sqc_session_read(sqc_session_t *sptr, void *buf, size_t len,
                 ssize_t *actual_read) {
  return s_session_read(sptr, buf, len, actual_read);
}


sqc_result_t
sqc_session_write(sqc_session_t *sptr, const void *buf, size_t len,
                  ssize_t *actual_write) {
  return s_session_write(sptr, buf, len, actual_write);
}


sqc_result_t
sqc_session_wait_readable(sqc_session_t *sptr, sqc_chrono_t to) {
  return s_wait_io_fd(sptr, to, true);
}


sqc_result_t
sqc_session_wait_writable(sqc_session_t *sptr, sqc_chrono_t to) {
  return s_wait_io_fd(sptr, to, true);
}


char *
sqc_session_peer_subjectdn_oneline(sqc_session_t *sptr)
{
  return s_peer_subjectdn_oneline(sptr);
}


char *
sqc_session_peer_subjectdn_rfc2253(sqc_session_t *sptr)
{
  return s_peer_subjectdn_rfc2253(sptr);
}


char *
sqc_session_peer_subjectdn_gsi(sqc_session_t *sptr)
{
  return s_peer_subjectdn_gsi(sptr);
}


char *
sqc_session_peer_cn(sqc_session_t *sptr)
{
  return s_peer_cn(sptr);
}
