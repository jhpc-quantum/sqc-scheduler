#include "sqc_apis.h"
#include "endpoint_internal.h"





typedef enum {
  SEEMS_UNKNOWN = 0,
  SEEMS_V4 = 1,
  SEEMS_V6 = 2,
} af_guess_t;


static inline sqc_result_t
s_getfl(int fd, int *stat) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(fd >= 0 && stat != NULL)) {
    int st;

    errno = 0;
    if (likely((st = fcntl(fd, F_GETFL, 0)) >= 0)) {
      *stat = st;
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
s_setfl(int fd, int stat) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(fd >= 0)) {
    int st;

    errno = 0;
    if (likely((st = fcntl(fd, F_SETFL, stat)) >= 0)) {
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
s_parse_endpoint(const char *str,
                 char *host, size_t hostlen, int *port,
                 af_guess_t *guess, bool *is_onlynum) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(IS_VALID_STRING(str) == true &&
             host != NULL && hostlen > 0 && port != NULL && guess != NULL &&
             is_onlynum != NULL)) {
    char *buf = NULL;
    char *host_ptr = NULL;
    char *port_ptr = NULL;
    af_guess_t tmp_guess = SEEMS_UNKNOWN;
    bool is_str_valid = true;

    buf = strdup(str);
    if (likely(buf != NULL)) {
      char *colon = NULL;
      int tmp_port = -1;

      if (*buf == '[') {
        // 'str' seems to be '[V6ADDR]' or '[V6ADDR]:PORT'.
        char *bracket_r = NULL;
        tmp_guess = SEEMS_V6;
        host_ptr = buf + 1;

        if (likely((bracket_r = strchr(buf, ']')) != NULL)) {
          *bracket_r = '\0';

          if (*(bracket_r + 1) == ':') {
            // 'str' seems to be '[V6ADDR]:PORT'.
            port_ptr = bracket_r + 2;
          } else if (unlikely(*(bracket_r + 1) != '\0')) {
            is_str_valid = false;
          }
        } else {
          is_str_valid = false;
        }

      } else if ((colon = strchr(buf, ':')) != NULL) {
        // 'str' seems to be 'V6ADDR', 'V4ADDR:PORT' or 'DOMAIN:PORT'.
        host_ptr = buf;

        if (strchr(colon + 1, ':') != NULL) {
          // 'str' seems to be 'V6ADDR', since it has at least two colons.
          tmp_guess = SEEMS_V6;
        } else {
          // 'str' seems to containing a port.
          *colon = '\0';
          port_ptr = colon + 1;
        }
      } else {
        // 'str' seems to be 'DOMAIN'.
        host_ptr = buf;
      }

      if (likely(is_str_valid == true && host_ptr != NULL)) {
        if (tmp_guess == SEEMS_UNKNOWN) {
          // determine whether 'str' is SEEMS_V4 or not.
          for (char *hp = host_ptr; ; hp++) {
            if (unlikely(*hp == '\0')) {
              // 'str' seems to be 'V4ADDR:PORT'.
              tmp_guess = SEEMS_V4;
              break;
            } else if (!isdigit((int) *hp) && *hp != '.') {
              // 'str' seems to be 'DOMAIN:PORT'.
              break;
            }
          }
        }
      }

      if (likely(is_str_valid == true)) {
        if (port_ptr != NULL) {
          // Convert the port number to an integer.
          if (likely(sqc_str_parse_int32(port_ptr, &tmp_port) ==
                     SQC_RESULT_OK && tmp_port >= 0 && tmp_port <= 0xffff)) {
          } else {
            is_str_valid = false;
            ret = SQC_RESULT_INVALID_ARGS;
          }
        } else {
          tmp_port = 0;
        }
      }

      if (likely(is_str_valid == true)) {
        if (likely(host_ptr != NULL && strlen(host_ptr) < hostlen)) {
          // Set the result.
          snprintf(host, hostlen, "%s", host_ptr);
          *port = tmp_port;
          *guess = tmp_guess;
          *is_onlynum = (tmp_guess == SEEMS_V6 || tmp_guess == SEEMS_V4);
          ret = SQC_RESULT_OK;
        } else {
          ret = SQC_RESULT_INVALID_ARGS;
        }
      }
    } else {
      ret = SQC_RESULT_NO_MEMORY;
    }
    free(buf);
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}

sqc_result_t
parse_endpoint(const char *str,
               char *host, size_t hostlen, int *port,
               af_guess_t *guess, bool *is_onlynum) {
  return s_parse_endpoint(str, host, hostlen, port,
                          guess, is_onlynum);
}

static inline sqc_result_t
s_lookup_endpoint(const char *str, bool want_v4,
                  char *host, size_t hostlen,
                  struct sockaddr_storage *saddrptr, socklen_t *slenptr,
                  int *fam, int *port) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(saddrptr != NULL && slenptr != NULL && port != NULL)) {
    struct addrinfo hints;
    int fampri[2];
    bool hints_created = false;

    (void)memset(&hints, 0, sizeof(hints));

    if (likely(IS_VALID_STRING(str) == true)) {
      af_guess_t guess = SEEMS_UNKNOWN;
      bool numonly = false;

      if (likely(ret = s_parse_endpoint(str, host, hostlen,
                                        port, &guess, &numonly))
          == SQC_RESULT_OK) {
        switch (guess) {
          case SEEMS_UNKNOWN: {
            if (want_v4 == true) {
              fampri[0] = AF_INET;
              fampri[1] = AF_INET6;
            } else {
              fampri[0] = AF_INET6;
              fampri[1] = AF_INET;
            }
            break;
          }
          case SEEMS_V4: {
            fampri[0] = AF_INET;
            fampri[1] = AF_INET6;
            break;
          }
          case SEEMS_V6: {
            fampri[0] = AF_INET6;
            fampri[1] = AF_INET;
            break;
          }
        }

        hints.ai_socktype = 0;
        hints.ai_flags = (numonly == true) ? AI_NUMERICHOST : 0;
        hints.ai_protocol = 0;
        hints.ai_canonname = NULL;
        hints.ai_addr = NULL;
        hints.ai_next = NULL;
        hints_created = true;
      }
    } else {
      if (want_v4 == true) {
        fampri[0] = AF_INET;
        fampri[1] = AF_INET6;
      } else {
        fampri[0] = AF_INET6;
        fampri[1] = AF_INET;
      }

      hints.ai_socktype = 0;
      hints.ai_flags = AI_PASSIVE;
      hints.ai_protocol = 0;
      hints.ai_canonname = NULL;
      hints.ai_addr = NULL;
      hints.ai_next = NULL;
      hints_created = true;
    }

    if (likely(hints_created == true)) {
      struct addrinfo *info = NULL;
      int rc = -1;
      int i;
      for (i = 0; i < 2 && rc != 0; i++) {
        hints.ai_family = fampri[i];
        rc = getaddrinfo(host, NULL, &hints, &info);
      }

      if (rc == 0) {
        *slenptr = info->ai_addrlen;
        *fam = info->ai_family;
        memcpy(saddrptr, info->ai_addr, *slenptr);
        if (saddrptr->ss_family == AF_INET) {
          struct sockaddr_in *sin = (struct sockaddr_in *) saddrptr;
          sin->sin_port = htons((uint16_t) *port);
          ret = SQC_RESULT_OK;
        } else if (saddrptr->ss_family == AF_INET6) {
          struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *) saddrptr;
          sin6->sin6_port = htons((uint16_t) *port);
          ret = SQC_RESULT_OK;
        } else {
          ret = SQC_RESULT_UNSUPPORTED;
        }
      } else {
        ret = SQC_RESULT_NOT_FOUND;
      }
      freeaddrinfo(info);
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_setup_port_to_sockaddr(sqc_endpoint_info *si) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(si != NULL && si->inited_ == true &&
             si->port_ >= 0 && si->port_ <= 0xffff)) {
    if (si->family_ == AF_INET) {
      struct sockaddr_in *sinptr =
          (struct sockaddr_in *)&(si->sockaddr_);
      sinptr->sin_port = htons((unsigned short int)(si->port_));
      ret = SQC_RESULT_OK;
    } else if (si->family_ == AF_INET6) {
      struct sockaddr_in6 *sin6ptr =
          (struct sockaddr_in6 *)&(si->sockaddr_);
      sin6ptr->sin6_port = htons((unsigned short int)(si->port_));
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_UNSUPPORTED;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_close_endpoint(const sqc_endpoint_t *eptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  bool posix_err_occurred = false;

  if (likely(eptr != NULL)) {
    if (likely((*eptr)->fd_ >= 0)) {
      // Close the socket.
      // We try to close the socket even when shutdown() is failed.
      for (;;) {
        errno = 0;
        if (likely((ret = close((*eptr)->fd_)) == 0)) {
          break;
        } else if (errno == EINTR) {
          continue;
        } else {
          posix_err_occurred = true;
        }
      }
    }

    // We unset 'fd_' even when shutdown() and/or close() is failed.
    (*eptr)->fd_ = -1;

    if (unlikely(posix_err_occurred == false)) {
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_delete_endpoint(sqc_endpoint_t *eptr) {
  if (likely(eptr != NULL && *eptr != NULL)) {
    (void) s_close_endpoint(eptr);
    free(*eptr);
  }
  *eptr = NULL;
}


static inline sqc_result_t
s_create_endpoint(sqc_endpoint_t *eptr, size_t sz, sqc_endpoint_type_t type,
                  const char *spec, bool prefer_ipv4) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(eptr != NULL && (type == SQC_ENDPOINT_TYPE_INET_ACCEPTOR ||
                              type == SQC_ENDPOINT_TYPE_INET_INITIATOR))) {
    int port = -1;
    int fam = -1;
    char host[SQC_HOST_NAME_MAX + 1];
    struct sockaddr_storage saddr = { 0 };
    socklen_t slen = 0;
    if (likely((ret = s_lookup_endpoint(spec, prefer_ipv4,
                                        host, sizeof(host),
                                        &saddr, &slen, &fam, &port)) ==
               SQC_RESULT_OK)) {
      if (*eptr == NULL) {
        if (unlikely(sz < sizeof(sqc_endpoint_record))) {
          sz = sizeof(sqc_endpoint_record);
        }
        *eptr = (sqc_endpoint_t)malloc(sz);
      }
      if (likely(*eptr != NULL)) {
        sqc_endpoint_info *si = (type == SQC_ENDPOINT_TYPE_INET_ACCEPTOR) ?
                                &((*eptr)->local_) : &((*eptr)->peer_);
        (void)memset(*eptr, 0, sz);
        (*eptr)->fd_ = -1;
        (*eptr)->type_ = type;
        memcpy(si->hint_addr_str_, host, SQC_HOST_NAME_MAX);
        si->sockaddr_ = saddr;
        si->sockaddr_len_ = slen;
        si->port_ = port;
        si->family_ = fam;
        si->inited_ = true;
        ret = SQC_RESULT_OK;
      } else {
        ret = SQC_RESULT_NO_MEMORY;
      }
    }
  } else if (unlikely(type == SQC_ENDPOINT_TYPE_UNIX_ACCEPTOR ||
                      type == SQC_ENDPOINT_TYPE_UNIX_INITIATOR ||
                      type == SQC_ENDPOINT_TYPE_UNIX_PASSIVE ||
                      type == SQC_ENDPOINT_TYPE_INET_DGRAM ||
                      type == SQC_ENDPOINT_TYPE_UNIX_DGRAM)) {
    ret = SQC_RESULT_UNSUPPORTED;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_create_endpoint_by_fd(sqc_endpoint_t *eptr, size_t sz, int fd) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(fd >= 0)) {
    struct sockaddr_storage localaddr;
    struct sockaddr_storage peeraddr;
    socklen_t locallen = sizeof(localaddr);
    socklen_t peerlen = sizeof(peeraddr);
    int rc;

    if (likely((rc = getsockname(fd, (struct sockaddr *)&localaddr,
                                 &locallen)) == 0 &&
               (rc = getpeername(fd, (struct sockaddr *)&peeraddr,
                                 &peerlen)) == 0)) {
      if (*eptr == NULL) {
        if (unlikely(sz < sizeof(sqc_endpoint_record))) {
          sz = sizeof(sqc_endpoint_record);
        }
        *eptr = (sqc_endpoint_t)malloc(sz);
      }
      if (likely(*eptr != NULL)) {
        struct sockaddr_in *sinptr = NULL;
        struct sockaddr_in6 *sin6ptr = NULL;

        (void)memset(*eptr, 0, sz);
        (*eptr)->type_ = SQC_ENDPOINT_TYPE_INET_PASSIVE;
        (*eptr)->fd_ = fd;

        (*eptr)->local_.sockaddr_ = localaddr;
        (*eptr)->local_.sockaddr_len_ = locallen;
        (*eptr)->local_.family_ = localaddr.ss_family;
        if (localaddr.ss_family == AF_INET) {
          sinptr = (struct sockaddr_in *)&localaddr;
          (*eptr)->local_.port_ = ntohs(sinptr->sin_port);
        } else if (localaddr.ss_family == AF_INET6) {
          sin6ptr = (struct sockaddr_in6 *)&localaddr;
          (*eptr)->local_.port_ = ntohs(sin6ptr->sin6_port);
        }
        (*eptr)->local_.inited_ = true;

        (*eptr)->peer_.sockaddr_ = localaddr;
        (*eptr)->peer_.sockaddr_len_ = locallen;
        (*eptr)->peer_.family_ = localaddr.ss_family;
        if (peeraddr.ss_family == AF_INET) {
          sinptr = (struct sockaddr_in *)&peeraddr;
          (*eptr)->peer_.port_ = ntohs(sinptr->sin_port);
        } else if (peeraddr.ss_family == AF_INET6) {
          sin6ptr = (struct sockaddr_in6 *)&peeraddr;
          (*eptr)->peer_.port_ = ntohs(sin6ptr->sin6_port);
        }
        (*eptr)->peer_.inited_ = true;

        (*eptr)->is_accepted_ = true;
        ret = SQC_RESULT_OK;

      } else {
        ret = SQC_RESULT_NO_MEMORY;
      }
    } else {
      ret = SQC_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_create_endpoint_by_fd_and_sockaddr(sqc_endpoint_t *eptr, size_t sz, int fd,
                                     struct sockaddr_storage *saptr,
                                     socklen_t salen) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(fd >= 0 && saptr != NULL && salen > 0)) {
    if (likely(saptr->ss_family == AF_INET || saptr->ss_family == AF_INET6)) {
      if (*eptr == NULL) {
        if (unlikely(sz < sizeof(sqc_endpoint_record))) {
          sz = sizeof(sqc_endpoint_record);
        }
        *eptr = (sqc_endpoint_t)malloc(sz);
      }
      if (likely(*eptr != NULL)) {
        struct sockaddr_in *sinptr = NULL;
        struct sockaddr_in6 *sin6ptr = NULL;

        (void)memset(*eptr, 0, sz);
        (*eptr)->type_ = SQC_ENDPOINT_TYPE_INET_PASSIVE;
        (*eptr)->fd_ = fd;
        (*eptr)->peer_.sockaddr_ = *saptr;
        (*eptr)->peer_.sockaddr_len_ = salen;
        (*eptr)->peer_.family_ = saptr->ss_family;
        if (saptr->ss_family == AF_INET) {
          sinptr = (struct sockaddr_in *)saptr;
          (*eptr)->peer_.port_ = ntohs(sinptr->sin_port);
        } else if (saptr->ss_family == AF_INET6) {
          sin6ptr = (struct sockaddr_in6 *)saptr;
          (*eptr)->peer_.port_ = ntohs(sin6ptr->sin6_port);
        }
        (*eptr)->peer_.inited_ = true;
        (*eptr)->is_accepted_ = true;
        ret = SQC_RESULT_OK;
      } else {
        ret = SQC_RESULT_NO_MEMORY;
      }
    } else {
      ret = SQC_RESULT_UNSUPPORTED;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_set_endpoint_local_port(const sqc_endpoint_t *eptr, int port) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(eptr != NULL && *eptr != NULL && port >= 0 && port <= 0xffff)) {
    (*eptr)->local_.port_ = port;
    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_set_endpoint_peer_port(const sqc_endpoint_t *eptr, int port) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(eptr != NULL && *eptr != NULL && port >= 0 && port <= 0xffff)) {
    (*eptr)->peer_.port_ = port;
    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_open_endpoint(const sqc_endpoint_t *eptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(eptr != NULL && *eptr != NULL)) {
    if (likely((*eptr)->fd_ == -1)) {
      sqc_endpoint_type_t type = (*eptr)->type_;
      if (likely((type == SQC_ENDPOINT_TYPE_INET_ACCEPTOR ||
                  type == SQC_ENDPOINT_TYPE_INET_INITIATOR))) {
        errno = 0;
        if (type == SQC_ENDPOINT_TYPE_INET_ACCEPTOR) {
          (*eptr)->fd_ = socket((*eptr)->local_.family_, SOCK_STREAM, 0);
        } else {
          (*eptr)->fd_ = socket((*eptr)->peer_.family_, SOCK_STREAM, 0);
        }
        if ((*eptr)->fd_ >= 0) {
          ret = SQC_RESULT_OK;
        } else {
          ret = SQC_RESULT_POSIX_API_ERROR;
        }
      } else {
        ret = SQC_RESULT_UNSUPPORTED;
      }
    } else {
      ret = SQC_RESULT_ALREADY_EXISTS;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_setup_local_bind(const sqc_endpoint_t *eptr,
                   const char *spec, bool prefer_ipv4, int port) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(eptr != NULL && *eptr != NULL && spec != NULL &&
             (*eptr)->type_ == SQC_ENDPOINT_TYPE_INET_INITIATOR)) {
    if (likely((*eptr)->local_.inited_ == false)) {
      const char *lookup_spec = NULL;
      int eport = -1;
      int fam = -1;
      char host[SQC_HOST_NAME_MAX + 1];
      struct sockaddr_storage saddr = { 0 };
      socklen_t slen = 0;

      if (likely(*spec != '\0')) {
        lookup_spec = spec;
      } else if ((*eptr)->peer_.family_ == AF_INET) {
        lookup_spec = "0.0.0.0";
      } else if ((*eptr)->peer_.family_ == AF_INET6) {
        lookup_spec = "::";
      } else {
        ret = SQC_RESULT_UNSUPPORTED;
        goto done;
      }

      if (likely((ret = s_lookup_endpoint(lookup_spec, prefer_ipv4,
                                          host, sizeof(host),
                                          &saddr, &slen, &fam, &eport)) ==
                 SQC_RESULT_OK)) {
        (*eptr)->local_.sockaddr_ = saddr;
        (*eptr)->local_.sockaddr_len_ = slen;
        memcpy((*eptr)->local_.hint_addr_str_, host, SQC_HOST_NAME_MAX);
        if (eport != -1) {
          (*eptr)->local_.port_ = eport;
        } else if (port != -1) {
          (*eptr)->local_.port_ = port;
        } else {
          ret = SQC_RESULT_INVALID_ARGS;
          goto done;
        }
        (*eptr)->local_.family_ = fam;
        (*eptr)->local_.inited_ = true;
        ret = SQC_RESULT_OK;
      }
    } else {
      ret = SQC_RESULT_INVALID_STATE;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

done:

  return ret;
}


static inline sqc_result_t
s_bind_endpoint(const sqc_endpoint_t *eptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  const int one = 1;

  if (likely(eptr != NULL && *eptr != NULL &&
             (*eptr)->local_.port_ >= 0 &&
             (*eptr)->local_.port_ <= 0xffff)) {
    sqc_endpoint_info *si = &((*eptr)->local_);

    if (likely(si->inited_ == true && (*eptr)->fd_ >= 0)) {
      if (likely((ret = s_setup_port_to_sockaddr(si)) == SQC_RESULT_OK)) {
        errno = 0;
        if (likely(setsockopt((*eptr)->fd_, SOL_SOCKET, SO_REUSEADDR,
                              &one, sizeof(int)) == 0)) {
          errno = 0;
          if (likely(bind((*eptr)->fd_,
                          (struct sockaddr *)&(si->sockaddr_),
                          si->sockaddr_len_) == 0)) {
            (*eptr)->is_bound_ = true;
            ret = SQC_RESULT_OK;
          } else {
            ret = SQC_RESULT_POSIX_API_ERROR;
          }
        } else {
          ret = SQC_RESULT_POSIX_API_ERROR;
        }
      }

    } else {
      ret =  SQC_RESULT_INVALID_STATE;
    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_connect_endpoint(const sqc_endpoint_t *eptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(eptr != NULL && *eptr != NULL &&
             (*eptr)->type_ == SQC_ENDPOINT_TYPE_INET_INITIATOR)) {
    if (likely((*eptr)->peer_.inited_ == true &&
               (*eptr)->fd_ >= 0)) {
      if (likely((ret = s_setup_port_to_sockaddr(&((*eptr)->peer_)) ==
                  SQC_RESULT_OK))) {
        errno = 0;
        if (likely(connect((*eptr)->fd_,
                           (struct sockaddr *)&((*eptr)->peer_.sockaddr_),
                           (*eptr)->peer_.sockaddr_len_) == 0)) {
          (*eptr)->is_connected_ = true;
          ret = SQC_RESULT_OK;
        } else {
          ret = SQC_RESULT_POSIX_API_ERROR;
        }
      }
    } else {
      ret = SQC_RESULT_INVALID_STATE;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_accept_endpoint(const sqc_endpoint_t *eptr,
                  int *newfdptr,
                  struct sockaddr_storage *saptr,
                  socklen_t *salen) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(eptr != NULL && *eptr != NULL && newfdptr != NULL &&
             (*eptr)->type_ == SQC_ENDPOINT_TYPE_INET_ACCEPTOR)) {
    if (likely((*eptr)->fd_ >= 0 && (*eptr)->is_bound_ == true &&
               (*eptr)->local_.inited_ == true)) {
      int newfd = -1;

      *newfdptr = -1;

      if (unlikely((*eptr)->is_listening_ == false)) {
        errno = 0;
        /*
         * XXX FIXME
         *	Use more platform-aware'ish backlog size
         */
        if (likely(listen((*eptr)->fd_, 1024) == 0)) {
          (*eptr)->is_listening_ = true;
        } else {
          ret = SQC_RESULT_POSIX_API_ERROR;
          goto done;
        }
      }

      errno = 0;
      if (likely((newfd = accept((*eptr)->fd_,
                                 (struct sockaddr *)saptr, salen)) >= 0)) {
        *newfdptr = newfd;
        ret = SQC_RESULT_OK;
      } else {
        ret = SQC_RESULT_POSIX_API_ERROR;
      }
    } else {
      ret = SQC_RESULT_INVALID_STATE;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

done:

  return ret;
}


static inline sqc_result_t
s_set_tcp_nodelay_endpoint(const sqc_endpoint_t *eptr, bool do_nodelay) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(eptr != NULL && *eptr != NULL && (*eptr)->fd_ >= 0)) {
    if (likely((*eptr)->type_ == SQC_ENDPOINT_TYPE_INET_INITIATOR ||
               (*eptr)->type_ == SQC_ENDPOINT_TYPE_INET_ACCEPTOR ||
               (*eptr)->type_ == SQC_ENDPOINT_TYPE_INET_PASSIVE)) {
      int val = (do_nodelay == true) ? 1 : 0;

      errno = 0;
      if (likely(setsockopt((*eptr)->fd_, SOL_TCP, TCP_NODELAY,
                          &val, sizeof(val)) == 0)) {
        ret = SQC_RESULT_OK;
        (*eptr)->is_nodelay_ = val;
      } else {
        ret = SQC_RESULT_POSIX_API_ERROR;
      }
    } else {
      ret = SQC_RESULT_UNSUPPORTED;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_set_nonblocking_endpoint(const sqc_endpoint_t *eptr, bool nonblock) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(eptr != NULL && *eptr != NULL && (*eptr)->fd_ >= 0)) {
    if (likely((*eptr)->type_ == SQC_ENDPOINT_TYPE_INET_INITIATOR ||
               (*eptr)->type_ == SQC_ENDPOINT_TYPE_INET_ACCEPTOR ||
               (*eptr)->type_ == SQC_ENDPOINT_TYPE_INET_PASSIVE)) {
      int stat = 0;

      if (likely((ret = s_getfl((*eptr)->fd_, &stat)) == SQC_RESULT_OK)) {
        if (nonblock == true) {
          stat |= O_NONBLOCK;
        } else {
          stat &= ~O_NONBLOCK;
        }
        if (likely((ret = s_setfl((*eptr)->fd_, stat)) == SQC_RESULT_OK)) {
          ret = SQC_RESULT_OK;
        }
      }
    } else {
      ret = SQC_RESULT_POSIX_API_ERROR;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_get_sockaddr_name(sqc_endpoint_info *si, bool want_fqdn) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(si != NULL)) {
    char *buf = NULL;
    int flags = 0;
    int rc = -1;
    bool falldowned = false;

 retry:
    buf = (want_fqdn == true) ? si->addr_fqdn_str_ : si->addr_num_str_;
    if (IS_VALID_STRING(buf) == true) {
      ret = SQC_RESULT_OK;
    } else {
      flags = (want_fqdn == true) ? NI_NAMEREQD
                                  : (NI_NUMERICHOST | NI_NUMERICSERV);

      errno = 0;
      if (likely((rc = getnameinfo((struct sockaddr *)&(si->sockaddr_),
                                   si->sockaddr_len_,
                                   buf, SQC_HOST_NAME_MAX,
                                   NULL, 0,
                                   flags)) == 0)) {
        ret = SQC_RESULT_OK;
      } else if (rc == EAI_NONAME) {
        if (falldowned == false && want_fqdn == true) {
          falldowned = true;
          want_fqdn = false;
          goto retry;
        } else {
          ret = SQC_RESULT_NOT_FOUND;
        }
      } else {
        switch (rc) {
          case EAI_AGAIN:
            ret = SQC_RESULT_TEMPORARY_UNAVAILABLE;
            break;
          case EAI_MEMORY:
            ret = SQC_RESULT_NO_MEMORY;
            break;
          case EAI_SYSTEM:
            ret = SQC_RESULT_POSIX_API_ERROR;
            break;
          default:
            ret = SQC_RESULT_ANY_RUNTIME_ERROR;
            break;
        }
      }
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_get_name_endpoint(const sqc_endpoint_t *eptr, char **buf, size_t buflen,
                    int *port, bool want_fqdn, bool peer) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(eptr != NULL && *eptr != NULL && buf != NULL)) {
    if (likely((*eptr)->type_ == SQC_ENDPOINT_TYPE_INET_INITIATOR ||
               (*eptr)->type_ == SQC_ENDPOINT_TYPE_INET_PASSIVE)) {
      struct sockaddr_storage tmp_addr;
      socklen_t tmp_addrlen = sizeof(tmp_addr);
      int getname_retval = -1;
      char *srcname = NULL;
      sqc_endpoint_info *si = NULL;

      errno = 0;
      if (peer == true) {
        si = &((*eptr)->peer_);
        getname_retval = getpeername((*eptr)->fd_,
                                     (struct sockaddr *)&tmp_addr,
                                     &tmp_addrlen);
      } else {
        si = &((*eptr)->local_);
        getname_retval = getsockname((*eptr)->fd_,
                                     (struct sockaddr *)&tmp_addr,
                                     &tmp_addrlen);
      }

      if (likely(getname_retval == 0)) {
        si->sockaddr_ = tmp_addr;
        si->sockaddr_len_ = tmp_addrlen;
        if (tmp_addr.ss_family == AF_INET) {
          si->port_ = ntohs(((struct sockaddr_in *)&tmp_addr)->sin_port);
        } else if (tmp_addr.ss_family == AF_INET6) {
          si->port_ = ntohs(((struct sockaddr_in6 *)&tmp_addr)->sin6_port);
        }

        if (likely((ret = s_get_sockaddr_name(si, want_fqdn)) ==
                   SQC_RESULT_OK)) {
          if (likely(port != NULL)) {
            *port = si->port_;
          }
          if (want_fqdn == true) {
            if (IS_VALID_STRING(si->addr_fqdn_str_) == true) {
              srcname = si->addr_fqdn_str_;
            } else if (IS_VALID_STRING(si->addr_num_str_) == true) {
              srcname = si->addr_num_str_;
            } else {
              ret = SQC_RESULT_NOT_FOUND;
            }
          } else {
            if (IS_VALID_STRING(si->addr_num_str_) == true) {
              srcname = si->addr_num_str_;
            } else if (IS_VALID_STRING(si->addr_fqdn_str_) == true) {
              srcname = si->addr_fqdn_str_;
            } else {
              ret = SQC_RESULT_NOT_FOUND;
            }
          }
          if (likely(IS_VALID_STRING(srcname) == true)) {
            if (*buf == NULL) {
              *buf = strdup(srcname);
              if (likely(IS_VALID_STRING(*buf) == true)) {
                ret = SQC_RESULT_OK;
              } else {
                ret = SQC_RESULT_NO_MEMORY;
              }
            } else if (buflen > 0) {
              snprintf(*buf, buflen, "%s", srcname);
              ret = SQC_RESULT_OK;
            }
          }
        }
      } else {
        ret = SQC_RESULT_POSIX_API_ERROR;
      }
    } else {
      ret = SQC_RESULT_UNSUPPORTED;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_shutdown_endpoint(sqc_endpoint_t *eptr, int how) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(eptr != NULL && *eptr != NULL)) {
    sqc_endpoint_type_t type = (*eptr)->type_;
    if (likely(type == SQC_ENDPOINT_TYPE_INET_ACCEPTOR ||
               type == SQC_ENDPOINT_TYPE_INET_INITIATOR ||
               type == SQC_ENDPOINT_TYPE_INET_PASSIVE ||
               type == SQC_ENDPOINT_TYPE_UNIX_ACCEPTOR ||
               type == SQC_ENDPOINT_TYPE_UNIX_INITIATOR ||
               type == SQC_ENDPOINT_TYPE_UNIX_PASSIVE)) {
      if (likely((*eptr)->fd_ >= 0)) {
        if (shutdown((*eptr)->fd_, how) == 0) {
          ret = SQC_RESULT_OK;
        } else {
          ret = SQC_RESULT_POSIX_API_ERROR;
        }
      } else {
        ret = SQC_RESULT_INVALID_STATE;
      }
    } else {
      ret = SQC_RESULT_UNSUPPORTED;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





/*
 * Exported APIs
 */


sqc_result_t
sqc_endpoint_create(sqc_endpoint_t *eptr, size_t sz, sqc_endpoint_type_t type,
		    const char *spec, bool prefer_ipv4) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  if (likely((ret = s_create_endpoint(eptr, sz, type, spec, prefer_ipv4))
             == SQC_RESULT_OK)) {
    ret = s_open_endpoint(eptr);
  }
  return ret;
}


sqc_result_t
sqc_endpoint_create_by_accepted_fd(sqc_endpoint_t *eptr, size_t sz, int fd) {
  return s_create_endpoint_by_fd(eptr, sz, fd);
}


sqc_result_t
sqc_endpoint_create_by_accepted_fd_and_sockaddr(sqc_endpoint_t *eptr,
                                                size_t sz, int fd,
                                                struct sockaddr_storage *saptr,
                                                socklen_t salen) {
  return s_create_endpoint_by_fd_and_sockaddr(eptr, sz, fd, saptr, salen);
}


void
sqc_endpoint_destroy(sqc_endpoint_t *eptr) {
  s_delete_endpoint(eptr);
}


sqc_result_t
sqc_endpoint_set_local_port(const sqc_endpoint_t *eptr, int port) {
  return s_set_endpoint_local_port(eptr, port);
}


sqc_result_t
sqc_endpoint_set_peer_port(const sqc_endpoint_t *eptr, int port) {
  return s_set_endpoint_peer_port(eptr, port);
}


sqc_result_t
sqc_endpoint_prepare_initiator_bind(const sqc_endpoint_t *eptr,
                                    const char *spec,
                                    bool prefer_ipv4, int port) {
  return s_setup_local_bind(eptr, spec, prefer_ipv4, port);
}


sqc_result_t
sqc_endpoint_bind(const sqc_endpoint_t *eptr) {
  return s_bind_endpoint(eptr);
}


sqc_result_t
sqc_endpoint_connect(const sqc_endpoint_t *eptr) {
  return s_connect_endpoint(eptr);
}


sqc_result_t
sqc_endpoint_accept(const sqc_endpoint_t *eptr, int *newfdptr,
                    struct sockaddr_storage *saptr, socklen_t *salen) {
  return s_accept_endpoint(eptr, newfdptr, saptr, salen);
}


sqc_result_t
sqc_endpoint_get_fd(const sqc_endpoint_t *eptr, int *fdptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(eptr != NULL && *eptr != NULL && fdptr != NULL)) {
    *fdptr = (*eptr)->fd_;
    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_endpoint_get_local_name(const sqc_endpoint_t *eptr,
			    char **name, size_t maxlen, int *port,
                            bool want_fqdn) {
  return s_get_name_endpoint(eptr, name, maxlen, port, want_fqdn, false);
}


sqc_result_t
sqc_endpoint_get_peer_name(const sqc_endpoint_t *eptr,
                           char **name, size_t maxlen, int *port,
                           bool want_fqdn) {
  return s_get_name_endpoint(eptr, name, maxlen, port, want_fqdn, true);
}


sqc_result_t
sqc_endpoint_get_local_address_family(const sqc_endpoint_t *eptr,
                                      int *famptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  if (likely(eptr != NULL && *eptr != NULL && famptr != NULL &&
             (*eptr)->local_.inited_ == true)) {
    *famptr = (*eptr)->local_.family_;
    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_endpoint_get_peer_address_family(const sqc_endpoint_t *eptr, int *famptr) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  if (likely(eptr != NULL && *eptr != NULL && famptr != NULL &&
             (*eptr)->peer_.inited_ == true)) {
    *famptr = (*eptr)->peer_.family_;
    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


sqc_result_t
sqc_endpoint_set_nonblock(const sqc_endpoint_t *eptr, bool do_nonblock) {
  return s_set_nonblocking_endpoint(eptr, do_nonblock);
}


sqc_result_t
sqc_endpoint_set_tcp_nodelay(const sqc_endpoint_t *eptr, bool do_nodelay) {
  return s_set_tcp_nodelay_endpoint(eptr, do_nodelay);
}


sqc_result_t
sqc_endpoint_shutdown(sqc_endpoint_t *eptr, int how) {
  return s_shutdown_endpoint(eptr, how);
}
