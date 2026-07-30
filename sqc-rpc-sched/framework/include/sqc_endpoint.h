#pragma once


/**
 * @file 	sqc_endpoint.h
 */





#define SQC_HOST_NAME_MAX	NI_MAXHOST





__BEGIN_DECLS





typedef struct sqc_endpoint_record *sqc_endpoint_t;


typedef enum {
  SQC_ENDPOINT_TYPE_UNKNOWN = 0,

  SQC_ENDPOINT_TYPE_INET_ACCEPTOR = 1,
  SQC_ENDPOINT_TYPE_INET_INITIATOR = 2,
  SQC_ENDPOINT_TYPE_INET_PASSIVE = 3,

  SQC_ENDPOINT_TYPE_UNIX_ACCEPTOR = 4,
  SQC_ENDPOINT_TYPE_UNIX_INITIATOR = 5,
  SQC_ENDPOINT_TYPE_UNIX_PASSIVE = 6,

  SQC_ENDPOINT_TYPE_INET_DGRAM = 7,
  SQC_ENDPOINT_TYPE_UNIX_DGRAM = 8,
} sqc_endpoint_type_t;





sqc_result_t
sqc_endpoint_create(sqc_endpoint_t *eptr, size_t sz, sqc_endpoint_type_t type,
		    const char *spec, bool prefer_ipv4);

sqc_result_t
sqc_endpoint_create_by_accepted_fd(sqc_endpoint_t *eptr, size_t sz, int fd);

sqc_result_t
sqc_endpoint_create_by_accepted_fd_and_sockaddr(sqc_endpoint_t *eptr,
						size_t sz, int fd,
						struct sockaddr_storage *saptr,
						socklen_t salen);

void
sqc_endpoint_destroy(sqc_endpoint_t *eptr);

sqc_result_t
sqc_endpoint_set_local_port(const sqc_endpoint_t *eptr, int port);

sqc_result_t
sqc_endpoint_set_peer_port(const sqc_endpoint_t *eptr, int port);

sqc_result_t
sqc_endpoint_prepare_initiator_bind(const sqc_endpoint_t *eptr,
				    const char *spec,
				    bool prefer_ipv4, int port);

sqc_result_t
sqc_endpoint_bind(const sqc_endpoint_t *eptr);

sqc_result_t
sqc_endpoint_connect(const sqc_endpoint_t *eptr);

sqc_result_t
sqc_endpoint_accept(const sqc_endpoint_t *eptr, int *newfdptr,
		    struct sockaddr_storage *saptr, socklen_t *salen);

sqc_result_t
sqc_endpoint_get_fd(const sqc_endpoint_t *eptr, int *fdptr);

sqc_result_t
sqc_endpoint_get_local_name(const sqc_endpoint_t *eptr,
			    char **name, size_t maxlen, int *port,
			    bool want_fqdn);

sqc_result_t
sqc_endpoint_get_peer_name(const sqc_endpoint_t *eptr,
			   char **name, size_t maxlen, int *port,
			   bool want_fqdn);

sqc_result_t
sqc_endpoint_get_local_address_family(const sqc_endpoint_t *eptr, int *famptr);

sqc_result_t
sqc_endpoint_get_peer_address_family(const sqc_endpoint_t *eptr, int *famptr);

sqc_result_t
sqc_endpoint_set_nonblock(const sqc_endpoint_t *eptr, bool do_nonblock);

sqc_result_t
sqc_endpoint_set_tcp_nodelay(const sqc_endpoint_t *eptr, bool do_nodelay);

sqc_result_t
sqc_endpoint_shutdown(sqc_endpoint_t *eptr, int how);





__END_DECLS
