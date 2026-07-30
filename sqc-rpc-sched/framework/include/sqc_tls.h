#pragma once





/**
 * @file 	sqc_tls.h
 */





#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/obj_mac.h>
#include <openssl/objects.h>





__BEGIN_DECLS





struct tls_conf_struct;
struct tls_session_ctx_struct;
typedef struct tls_conf_struct *sqc_tls_conf_t;
typedef struct tls_session_ctx_struct *sqc_tls_session_ctx_t;


/*
 * TLS role
 */
enum tls_role {
	TLS_ROLE_UNKNOWN = 0,
	TLS_ROLE_CLIENT,
	TLS_ROLE_SERVER
};
#define TLS_ROLE_INITIATOR	TLS_ROLE_CLIENT
#define TLS_ROLE_ACCEPTOR	TLS_ROLE_SERVER




/*
  *Operate 'tls_conf_struct'.
 */
sqc_result_t
sqc_tls_conf_create(sqc_tls_conf_t *conf);

sqc_result_t
sqc_tls_conf_copy(struct tls_conf_struct **conf,
                  const struct tls_conf_struct *other);

sqc_result_t
sqc_tls_conf_create_from_conf_dir(sqc_tls_conf_t *conf, const char *dir,
                                  enum tls_role role);

void
sqc_tls_conf_destroy(sqc_tls_conf_t conf);

char *
sqc_tls_conf_get_cipher_suite(sqc_tls_conf_t conf);

char *
sqc_tls_conf_get_ca_certificate_path(sqc_tls_conf_t conf);

char *
sqc_tls_conf_get_ca_revocation_path(sqc_tls_conf_t conf);

char *
sqc_tls_conf_get_ca_peer_verify_chain_path(sqc_tls_conf_t conf);

char *
sqc_tls_conf_get_certificate_file(sqc_tls_conf_t conf);

char *
sqc_tls_conf_get_certificate_chain_file(sqc_tls_conf_t conf);

char *
sqc_tls_conf_get_key_file(sqc_tls_conf_t conf);

bool
sqc_tls_conf_get_key_update(sqc_tls_conf_t conf);

bool
sqc_tls_conf_get_build_chain_local(sqc_tls_conf_t conf);

bool
sqc_tls_conf_get_allow_no_crl(sqc_tls_conf_t conf);

sqc_result_t
sqc_tls_conf_set_cipher_suite(sqc_tls_conf_t conf,
                              const char *val);

sqc_result_t
sqc_tls_conf_set_ca_certificate_path(sqc_tls_conf_t cf,
                                     const char *val);

sqc_result_t
sqc_tls_conf_set_ca_revocation_path(sqc_tls_conf_t conf,
                                    const char *val);

sqc_result_t
sqc_tls_conf_set_ca_peer_verify_chain_path(sqc_tls_conf_t conf,
                                           const char *val);

sqc_result_t
sqc_tls_conf_set_certificate_file(sqc_tls_conf_t conf,
                                  const char *val);

sqc_result_t
sqc_tls_conf_set_certificate_chain_file(sqc_tls_conf_t conf,
                                        const char *val);

sqc_result_t
sqc_tls_conf_set_key_file(sqc_tls_conf_t conf,
                          const char *val);

sqc_result_t
sqc_tls_conf_set_key_update(sqc_tls_conf_t conf,
                            bool val);

sqc_result_t
sqc_tls_conf_set_build_chain_local(sqc_tls_conf_t conf,
                                   bool val);

sqc_result_t
sqc_tls_conf_set_allow_no_crl(sqc_tls_conf_t conf,
                              bool val);

void
sqc_tls_conf_dump(sqc_tls_conf_t conf);


/*
 * TLS session.
 */
sqc_result_t
sqc_tls_session_runtime_initialize(void);

bool
sqc_tls_has_runtime_error(void);

void
sqc_tls_runtime_flush_error(void);

sqc_result_t
sqc_tls_load_prvkey(const char *file, EVP_PKEY **keyptr);

sqc_result_t
sqc_tls_get_x509_name_stack_from_dir(const char *dir,
	STACK_OF(X509_NAME) (*stack), int *nptr);

sqc_result_t
sqc_tls_set_ca_path(SSL_CTX *ssl_ctx,
	const char *ca_path, const char* acceptable_ca_path,
	STACK_OF(X509_NAME) (**trust_ca_list));

sqc_result_t
sqc_tls_add_extra_certs(SSL_CTX *ssl_ctx, const char *file, int *n_added);

sqc_result_t
sqc_tls_load_cert_and_chain(SSL_CTX *ssl_ctx,
	const char *cert_file, const char *cert_chain_file, int *nptr);

sqc_result_t
sqc_tls_set_revoke_path(SSL_CTX *ssl_ctx, const char *revoke_path);

int
sqc_tls_verify_callback_body(int ok, X509_STORE_CTX *sctx);

void
sqc_tls_session_clear_ctx(sqc_tls_session_ctx_t ctx, int flags);

sqc_result_t
sqc_tls_session_clear_ctx_for_reconnect(sqc_tls_session_ctx_t ctx);

sqc_result_t
sqc_tls_session_clear_ctx_for_reestablish(sqc_tls_session_ctx_t ctx);

sqc_result_t
sqc_tls_session_setup_ssl(sqc_tls_session_ctx_t ctx);

sqc_result_t
sqc_tls_session_create_ctx(sqc_tls_session_ctx_t *ctxptr,
	const sqc_tls_conf_t conf, enum tls_role role,
	bool do_mutual_auth, bool use_proxy_cert);

void
sqc_tls_session_destroy_ctx(sqc_tls_session_ctx_t ctx);

bool
sqc_tls_session_io_continuable(int sslerr, sqc_tls_session_ctx_t ctx,
	bool in_handshake, const char *diag);

sqc_result_t
sqc_tls_session_wait_io(sqc_tls_session_ctx_t ctx,
	int fd, int tous, bool to_read);

sqc_result_t
sqc_tls_session_wait_readable(sqc_tls_session_ctx_t ctx, int fd,
	int tous);

sqc_result_t
sqc_tls_session_wait_writable(sqc_tls_session_ctx_t ctx, int fd,
	int tous);

sqc_result_t
sqc_tls_session_get_pending_read_bytes_n(sqc_tls_session_ctx_t ctx,
	int *nptr);

sqc_result_t
sqc_tls_session_verify(sqc_tls_session_ctx_t ctx, bool *is_verified);

sqc_result_t
sqc_tls_session_establish(sqc_tls_session_ctx_t ctx, int fd);

sqc_result_t
sqc_tls_session_update_key(sqc_tls_session_ctx_t ctx, int delta);

sqc_result_t
sqc_tls_session_read(sqc_tls_session_ctx_t ctx, void *buf, int len,
	int *actual_io_bytes);

sqc_result_t
sqc_tls_session_write(sqc_tls_session_ctx_t ctx, const void *buf,
	int len, int *actual_io_bytes);

sqc_result_t
sqc_tls_session_timeout_read(sqc_tls_session_ctx_t ctx,
	int fd, void *buf, int len, int timeout, int *actual_read);

sqc_result_t
sqc_tls_session_timeout_write(sqc_tls_session_ctx_t ctx,
	int fd, const void *buf, int len, int timeout, int *actual_io_bytes);

sqc_result_t
sqc_tls_session_shutdown(sqc_tls_session_ctx_t ctx);

char *
sqc_tls_session_peer_subjectdn_oneline(sqc_tls_session_ctx_t ctx);

char *
sqc_tls_session_peer_subjectdn_rfc2253(sqc_tls_session_ctx_t ctx);

char *
sqc_tls_session_peer_subjectdn_gsi(sqc_tls_session_ctx_t ctx);

char *
sqc_tls_session_peer_cn(sqc_tls_session_ctx_t ctx);

enum tls_role
sqc_tls_session_get_role(sqc_tls_session_ctx_t ctx);

bool
sqc_tls_session_get_do_mutual_auth(sqc_tls_session_ctx_t ctx);

bool
sqc_tls_session_get_do_build_chain(sqc_tls_session_ctx_t ctx);

bool
sqc_tls_session_get_do_allow_no_crls(sqc_tls_session_ctx_t ctx);

bool
sqc_tls_session_get_do_allow_proxy_cert(sqc_tls_session_ctx_t ctx);

char *
sqc_tls_session_get_cert_file(sqc_tls_session_ctx_t ctx);

char *
sqc_tls_session_get_cert_chain_file(sqc_tls_session_ctx_t ctx);

char *
sqc_tls_session_get_prvkey_file(sqc_tls_session_ctx_t ctx);

char *
sqc_tls_session_get_ciphersuites(sqc_tls_session_ctx_t ctx);

char *
sqc_tls_session_get_ca_path(sqc_tls_session_ctx_t ctx);

char *
sqc_tls_session_get_acceptable_ca_path(sqc_tls_session_ctx_t ctx);

char *
sqc_tls_session_get_revoke_path(sqc_tls_session_ctx_t ctx);

/*
 * Logger.
 */
void
sqc_tls_runtime_init_once_body(void);

void
sqc_tls_log_emit(sqc_log_level_t priority, uint64_t debug_level,
	const char *file, int line_no, const char *func,
	const char *format, ...) __attr_format_printf__(6, 7);




__END_DECLS
