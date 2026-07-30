#pragma once

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/x509_vfy.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/obj_mac.h>
#include <openssl/objects.h>

#ifndef SQC_TLS_ROLE
#define SQC_TLS_ROLE    1
#endif /* SQC_TLS_ROLE */
#ifndef SQC_TLS_ACCEPT
#define SQC_TLS_ACCEPT    0
#endif /* SQC_TLS_ACCEPT */
#ifndef SQC_TLS_INITIATE
#define SQC_TLS_INITIATE    1
#endif /* SQC_TLS_INITIATE */
#ifndef SQC_TLS_CLIENT_AUTHENTICATION
#define SQC_TLS_CLIENT_AUTHENTICATION    2
#endif /* SQC_TLS_CLIENT_AUTHENTICATION */
#ifndef SQC_TLS_ROLE_IS_INITIATOR
#define SQC_TLS_ROLE_IS_INITIATOR(flags)			\
	(((flags) & SQC_TLS_ROLE) == SQC_TLS_INITIATE)
#endif /* SQC_TLS_ROLE_IS_INITIATOR */

/*
 * TLS configuration.
 */
struct tls_conf_struct {
	char *cipher_suite;
	char *ca_certificate_path;
	char *ca_revocation_path;
	char *ca_peer_verify_chain_path;
	char *certificate_file;
	char *certificate_chain_file;
	char *key_file;
	bool key_update;
	bool build_chain_local;
	bool allow_no_crl;
};

/*
 * The cookie for TLS
 */
struct tls_session_ctx_struct {
	SSL *ssl_;		/* API alloc'd */

	int last_ssl_error_;
	bool is_got_fatal_ssl_error_;
				/* got SSL_ERROR_SYSCALL or SSL_ERROR_SSL */
	size_t io_total_;	/* How many bytes transmitted */
	size_t io_key_update_accum_;
				/* KeyUpdate current water level (bytes) */
	ssize_t io_key_update_thresh_;
				/* KeyUpdate threshold (bytes) */

	sqc_result_t last_sqc_error_;

	/*
	 * Read-only parameters start
	 */
	enum tls_role role_;
	bool do_mutual_auth_;
	bool do_build_chain_;
	bool do_allow_no_crls_;
	bool do_allow_proxy_cert_;
	char *cert_file_;
	char *cert_chain_file_;
	char *prvkey_file_;
	char *ciphersuites_;
	char *ca_path_;
	char *acceptable_ca_path_;
	char *revoke_path_;
	/*
	 * Read-only parameters end
	 */

	char *peer_dn_oneline_;		/* malloc'd */
	char *peer_dn_rfc2253_;		/* malloc'd */
	char *peer_dn_gsi_;		/* malloc'd */
	char *peer_cn_;			/* malloc'd */

	bool is_handshake_tried_;
	bool is_verified_;
	bool is_got_proxy_cert_;

	int cert_verify_callback_error_;
	int cert_verify_result_error_;

	STACK_OF(X509_NAME) (*trusted_certs_);
					/* API alloc'd */

	SSL_CTX *ssl_ctx_;		/* API alloc'd */
	EVP_PKEY *prvkey_;		/* API alloc'd */
	X509_NAME *proxy_issuer_;	/* API alloc'd */
};

#define CTX_CLEAR_RECONN	1
#define CTX_CLEAR_VAR	2
#define	CTX_CLEAR_SSL	4
#define CTX_CLEAR_CTX	8

#define CTX_CLEAR_READY_FOR_RECONNECT \
	CTX_CLEAR_RECONN
#define CTX_CLEAR_READY_FOR_ESTABLISH \
	(CTX_CLEAR_VAR | CTX_CLEAR_SSL)
#define CTX_CLEAR_FREEUP \
	(CTX_CLEAR_VAR | CTX_CLEAR_SSL | CTX_CLEAR_CTX)

/*
 * Password callback arg
 */
struct tls_passwd_cb_arg_struct {
	size_t pw_buf_maxlen_;
	char *pw_buf_;
		/*
		 * == '\0':
		 *	the callback acquires a passwd string from a controll
		 *	terminal asigned for this process, copies the acquired
		 *	passwd string to pw_buf_.
		 *
		 * != '\0':
		 *	the callback returns pw_buf_.
		 *
		 * == NULL:
		 *	the callback does nothing and returns NULL.
		 */
	const char *filename_;
};

struct cert_add_method_struct {
	int (*f)(SSL_CTX *ctx, X509 *cert);
	const char *name;
};

/*
 * Pre-declarations
 */
static inline sqc_result_t
tls_session_shutdown(struct tls_session_ctx_struct *ctx);

static inline char *
tls_session_peer_cn(struct tls_session_ctx_struct *ctx);

/*
 * Logger
 */

#define TLS_LOG_MSG_LEN	2048

/*
 *TLS support version of gflog_message()
 */
static inline void
tls_log_emit(sqc_log_level_t priority, uint64_t debug_level,
	const char *file, int line_no, const char *func,
	const char *format, ...) __attr_format_printf__(6, 7);

/*
 * gflog with TLS runtime message
 */
#define sqc_tls_msg_error(...) \
	tls_log_emit(SQC_LOG_LEVEL_ERROR, 0LL, \
			__FILE__, __LINE__, __func__, __VA_ARGS__)
#define sqc_tls_msg_warning(...) \
	tls_log_emit(SQC_LOG_LEVEL_WARNING, 0LL, \
			__FILE__, __LINE__, __func__, __VA_ARGS__)
#define sqc_tls_msg_debug(level, ...) \
	tls_log_emit(SQC_LOG_LEVEL_DEBUG, level, \
			__FILE__, __LINE__, __func__, __VA_ARGS__)
#define sqc_tls_msg_info(...) \
	tls_log_emit(SQC_LOG_LEVEL_INFO, 0LL, \
			__FILE__, __LINE__, __func__, __VA_ARGS__)
#define sqc_tls_msg_notice(...)	\
	tls_log_emit(SQC_LOG_LEVEL_NOTICE, 0LL,	\
			__FILE__, __LINE__, __func__, __VA_ARGS__)
