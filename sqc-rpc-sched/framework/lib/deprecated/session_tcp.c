#include "sqc_apis.h"
#include "sqc_session.h"
#include "session_internal.h"

sqc_result_t session_tcp_init(sqc_session_t );

static ssize_t
read_tcp(sqc_session_t s, void *buf, size_t n) {
  return read(s->sock, buf, n);
}

static ssize_t
write_tcp(sqc_session_t s, void *buf, size_t n) {
  return write(s->sock, buf, n);
}

sqc_result_t
session_tcp_init(sqc_session_t s) {
  s->read = read_tcp;
  s->write = write_tcp;

  return SQC_RESULT_OK;
}
