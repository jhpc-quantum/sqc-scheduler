#ifndef __SESSION_INTERNAL_H__
#define __SESSION_INTERNAL_H__

#define SESSION_BUFSIZ 4096

struct session {
  /* socket descriptor */
  int sock;
  int family;
  int type;
  int protocol;
  session_type_t session_type;
  short events; /* for session_event_poll */
  short revents; /* for session_event_poll */
  struct session_buf {
    char *rp;
    char *ep;
    char buf[SESSION_BUFSIZ];
  } rbuf;
  /* function pointers */
  sqc_result_t (*connect)(sqc_session_t s, const char *host,
                             const char *port);
  sqc_result_t (*accept)(sqc_session_t s1, sqc_session_t *s2);
  ssize_t (*read)(sqc_session_t, void *, size_t);
  ssize_t (*write)(sqc_session_t, void *, size_t);
  void (*close)(sqc_session_t);
  void (*destroy)(sqc_session_t);
  sqc_result_t (*connect_check)(sqc_session_t);
  /* protocol depended context object */
  void *ctx;
};

#endif /* __SESSION_INTERNAL_H__ */

