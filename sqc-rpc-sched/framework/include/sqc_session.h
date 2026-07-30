#pragma once


/**
 * @file       sqc_session.h
 */


#include "sqc_endpoint.h"
#include "sqc_tls_context.h"





__BEGIN_DECLS





typedef struct sqc_session_record *sqc_session_t;


typedef enum {
  SQC_SESSION_TYPE_UNKNOWN = 0,
  SQC_SESSION_TYPE_RAW_CLIENT = 1,
  SQC_SESSION_TYPE_RAW_SERVER = 2,
  SQC_SESSION_TYPE_TLS_1WAY_AUTH_INITIATOR = 3,
  SQC_SESSION_TYPE_TLS_2WAY_AUTH_INITIATOR = 4,
  SQC_SESSION_TYPE_TLS_ACCEPTOR = 5,
} sqc_session_type_t;





sqc_result_t
sqc_session_create_client(sqc_session_t *sptr, size_t sz,
			  const char *spec, bool prefer_ipv4);

sqc_result_t
sqc_session_create_server(sqc_session_t *sptr, size_t sz, int fd,
			  struct sockaddr_storage *saptr, socklen_t salen);

void
sqc_session_destroy(sqc_session_t *sptr);

sqc_result_t
sqc_session_setup_tls(sqc_session_t *sptr, const sqc_tls_conf_t conf,
                      bool do_mutual_auth, bool use_proxy_cert);

bool
sqc_session_is_tls_prepared(sqc_session_t *sptr);

sqc_result_t
sqc_session_establish_tls(sqc_session_t *sptr);

sqc_result_t
sqc_session_read(sqc_session_t *sptr, void *buf, size_t len,
		 ssize_t *actual_read);

sqc_result_t
sqc_session_write(sqc_session_t *sptr, const void *buf, size_t len,
		  ssize_t *actual_write);

sqc_result_t
sqc_session_wait_readable(sqc_session_t *sptr, sqc_chrono_t to);

sqc_result_t
sqc_session_wait_writable(sqc_session_t *sptr, sqc_chrono_t to);


char *
sqc_session_peer_subjectdn_oneline(sqc_session_t *sptr);

char *
sqc_session_peer_subjectdn_rfc2253(sqc_session_t *sptr);

char *
sqc_session_peer_subjectdn_gsi(sqc_session_t *sptr);

char *
sqc_session_peer_cn(sqc_session_t *sptr);





__END_DECLS
