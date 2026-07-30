/**
 * @file 	tls.c
 */





#include "sqc_apis.h"
#include "tls_headers.h"
#include "tls_instances.h"
#include "tls_conf.h"
#include "tls_funcs.h"
#include "sqc_tls.h"





__BEGIN_DECLS





//
// Export functions provided as static inline functions by 'tls_conf.h'.
//
sqc_result_t
sqc_tls_conf_create(sqc_tls_conf_t *conf)
{
  return tls_conf_create(conf);
}


sqc_result_t
sqc_tls_conf_copy(struct tls_conf_struct **conf,
                  const struct tls_conf_struct *other) {
  return tls_conf_copy(conf, other);
}

sqc_result_t
sqc_tls_conf_create_from_conf_dir(sqc_tls_conf_t *conf, const char *dir,
                                  enum tls_role role)
{
  return tls_conf_create_from_conf_dir(conf, dir, role);
}

void
sqc_tls_conf_destroy(sqc_tls_conf_t conf)
{
  tls_conf_destroy(conf);
}


char *
sqc_tls_conf_get_cipher_suite(sqc_tls_conf_t conf)
{
  return tls_conf_get_cipher_suite(conf);
}


char *
sqc_tls_conf_get_ca_certificate_path(sqc_tls_conf_t conf)
{
  return tls_conf_get_ca_certificate_path(conf);
}


char *
sqc_tls_conf_get_ca_revocation_path(sqc_tls_conf_t conf)
{
  return tls_conf_get_ca_revocation_path(conf);
}


char *
sqc_tls_conf_get_ca_peer_verify_chain_path(sqc_tls_conf_t conf)
{
  return tls_conf_get_ca_peer_verify_chain_path(conf);
}


char *
sqc_tls_conf_get_certificate_file(sqc_tls_conf_t conf)
{
  return tls_conf_get_certificate_file(conf);
}


char *
sqc_tls_conf_get_certificate_chain_file(sqc_tls_conf_t conf)
{
  return tls_conf_get_certificate_chain_file(conf);
}


char *
sqc_tls_conf_get_key_file(sqc_tls_conf_t conf)
{
  return tls_conf_get_key_file(conf);
}


bool
sqc_tls_conf_get_key_update(sqc_tls_conf_t conf)
{
  return tls_conf_get_key_update(conf);
}


bool
sqc_tls_conf_get_build_chain_local(sqc_tls_conf_t conf)
{
  return tls_conf_get_build_chain_local(conf);
}


bool
sqc_tls_conf_get_allow_no_crl(sqc_tls_conf_t conf)
{
  return tls_conf_get_allow_no_crl(conf);
}


sqc_result_t
sqc_tls_conf_set_cipher_suite(sqc_tls_conf_t conf,
                              const char *val)
{
  return tls_conf_set_cipher_suite(conf, val);
}


sqc_result_t
sqc_tls_conf_set_ca_certificate_path(sqc_tls_conf_t conf,
                                     const char *val)
{
  return tls_conf_set_ca_certificate_path(conf, val);
}


sqc_result_t
sqc_tls_conf_set_ca_revocation_path(sqc_tls_conf_t conf,
                                    const char *val)
{
  return tls_conf_set_ca_revocation_path(conf, val);
}


sqc_result_t
sqc_tls_conf_set_ca_peer_verify_chain_path(sqc_tls_conf_t conf,
                                           const char *val)
{
  return tls_conf_set_ca_peer_verify_chain_path(conf, val);
}


sqc_result_t
sqc_tls_conf_set_certificate_file(sqc_tls_conf_t conf,
                                  const char *val)
{
  return tls_conf_set_certificate_file(conf, val);
}


sqc_result_t
sqc_tls_conf_set_certificate_chain_file(sqc_tls_conf_t conf,
                                        const char *val)
{
  return tls_conf_set_certificate_chain_file(conf, val);
}


sqc_result_t
sqc_tls_conf_set_key_file(sqc_tls_conf_t conf, const char *val)
{
  return tls_conf_set_key_file(conf, val);
}


sqc_result_t
sqc_tls_conf_set_key_update(sqc_tls_conf_t conf, bool val)
{
  return tls_conf_set_key_update(conf, val);
}


sqc_result_t
sqc_tls_conf_set_build_chain_local(sqc_tls_conf_t conf, bool val)
{
  return tls_conf_set_build_chain_local(conf, val);
}


sqc_result_t
sqc_tls_conf_set_allow_no_crl(sqc_tls_conf_t conf, bool val)
{
  return tls_conf_set_allow_no_crl(conf, val);
}


void
sqc_tls_conf_dump(sqc_tls_conf_t conf) {
  tls_conf_dump(conf);
}


static inline enum tls_role
tls_session_get_role(sqc_tls_session_ctx_t ctx)
{
  enum tls_role ret;

  if (likely(ctx != NULL)) {
    ret = ctx->role_;
  }

  return ret;
}

static inline bool
tls_session_get_do_mutual_auth(sqc_tls_session_ctx_t ctx)
{
  enum tls_role ret = TLS_ROLE_UNKNOWN;

  if (likely(ctx != NULL)) {
    ret = ctx->do_mutual_auth_;
  }

  return ret;
}


static inline bool
tls_session_get_do_build_chain(sqc_tls_session_ctx_t ctx)
{
  bool ret = false;

  if (likely(ctx != NULL)) {
    ret = ctx->do_build_chain_;
  }

  return ret;
}


static inline bool
tls_session_get_do_allow_no_crls(sqc_tls_session_ctx_t ctx)
{
  bool ret = false;

  if (likely(ctx != NULL)) {
    ret = ctx->do_allow_no_crls_;
  }

  return ret;
}


static inline bool
tls_session_get_do_allow_proxy_cert(sqc_tls_session_ctx_t ctx)
{
  bool ret = false;

  if (likely(ctx != NULL)) {
    ret = ctx->do_allow_proxy_cert_;
  }

  return ret;
}


static inline char *
tls_session_get_cert_file(sqc_tls_session_ctx_t ctx)
{
  char *ret = NULL;
  char *tmp_val = NULL;

  if (likely(ctx != NULL && ctx->cert_file_ != NULL)) {
    tmp_val = strdup(ctx->cert_file_);
    if (likely(tmp_val != NULL)) {
      ret = tmp_val;
    }
  }

  return ret;
}


static inline char *
tls_session_get_cert_chain_file(sqc_tls_session_ctx_t ctx)
{
  char *ret = NULL;
  char *tmp_val = NULL;

  if (likely(ctx != NULL && ctx->cert_chain_file_ != NULL)) {
    tmp_val = strdup(ctx->cert_chain_file_);
    if (likely(tmp_val != NULL)) {
      ret = tmp_val;
    }
  }

  return ret;
}


static inline char *
tls_session_get_prvkey_file(sqc_tls_session_ctx_t ctx)
{
  char *ret = NULL;
  char *tmp_val = NULL;

  if (likely(ctx != NULL && ctx->prvkey_file_ != NULL)) {
    tmp_val = strdup(ctx->prvkey_file_);
    if (likely(tmp_val != NULL)) {
      ret = tmp_val;
    }
  }

  return ret;
}


static inline char *
tls_session_get_ciphersuites(sqc_tls_session_ctx_t ctx)
{
  char *ret = NULL;
  char *tmp_val = NULL;

  if (likely(ctx != NULL && ctx->ciphersuites_ != NULL)) {
    tmp_val = strdup(ctx->ciphersuites_);
    if (likely(tmp_val != NULL)) {
      ret = tmp_val;
    }
  }

  return ret;
}


static inline char *
tls_session_get_ca_path(sqc_tls_session_ctx_t ctx)
{
  char *ret = NULL;
  char *tmp_val = NULL;

  if (likely(ctx != NULL &&ctx->ca_path_ != NULL)) {
    tmp_val = strdup(ctx->ca_path_);
    if (likely(tmp_val != NULL)) {
      ret = tmp_val;
    }
  }

  return ret;
}


static inline char *
tls_session_get_acceptable_ca_path(sqc_tls_session_ctx_t ctx)
{
  char *ret = NULL;
  char *tmp_val = NULL;

  if (likely(ctx != NULL && ctx->acceptable_ca_path_ != NULL)) {
    tmp_val = strdup(ctx->acceptable_ca_path_);
    if (likely(tmp_val != NULL)) {
      ret = tmp_val;
    }
  }

  return ret;
}


static inline char *
tls_session_get_revoke_path(sqc_tls_session_ctx_t ctx)
{
  char *ret = NULL;
  char *tmp_val = NULL;

  if (likely(ctx != NULL && ctx->revoke_path_ != NULL)) {
    tmp_val = strdup(ctx->revoke_path_);
    if (likely(tmp_val != NULL)) {
      ret = tmp_val;
    }
  }

  return ret;
}


//
// Export functions provided as static inline functions by 'tls_funcs.h'.
//
void
sqc_tls_runtime_init_once_body(void)
{
  tls_runtime_init_once_body();
}


sqc_result_t
sqc_tls_session_runtime_initialize(void)
{
  return tls_session_runtime_initialize();
}


bool
sqc_tls_has_runtime_error(void)
{
  return tls_has_runtime_error();
}


void
sqc_tls_runtime_flush_error(void)
{
  tls_runtime_flush_error();
}


sqc_result_t
sqc_tls_load_prvkey(const char *file, EVP_PKEY **keyptr)
{
  return tls_load_prvkey(file, keyptr);
}


sqc_result_t
sqc_tls_get_x509_name_stack_from_dir(const char *dir,
                                     STACK_OF(X509_NAME) (*stack), int *nptr)
{
  return tls_get_x509_name_stack_from_dir(dir, stack, nptr);
}


sqc_result_t
sqc_tls_set_ca_path(SSL_CTX *ssl_ctx,
                    const char *ca_path, const char* acceptable_ca_path,
                    STACK_OF(X509_NAME) (**trust_ca_list))
{
  return tls_set_ca_path(ssl_ctx, ca_path, acceptable_ca_path,
                         trust_ca_list);
}


sqc_result_t
sqc_tls_add_extra_certs(SSL_CTX *ssl_ctx, const char *file, int *n_added)
{
  return tls_add_extra_certs(ssl_ctx, file, n_added);
}


sqc_result_t
sqc_tls_load_cert_and_chain(SSL_CTX *ssl_ctx,
                            const char *cert_file, const char *cert_chain_file, int *nptr)
{
  return tls_load_cert_and_chain(ssl_ctx, cert_file, cert_chain_file, nptr);
}


sqc_result_t
sqc_tls_set_revoke_path(SSL_CTX *ssl_ctx, const char *revoke_path)
{
  return tls_set_revoke_path(ssl_ctx, revoke_path);
}


int
sqc_tls_verify_callback_body(int ok, X509_STORE_CTX *sctx)
{
  return tls_verify_callback_body(ok, sctx);
}


void
sqc_tls_session_clear_ctx(sqc_tls_session_ctx_t ctx, int flags)
{
  tls_session_clear_ctx(ctx, flags);
}


sqc_result_t
sqc_tls_session_clear_ctx_for_reconnect(sqc_tls_session_ctx_t ctx)
{
  return tls_session_clear_ctx_for_reconnect(ctx);
}


sqc_result_t
sqc_tls_session_clear_ctx_for_reestablish(sqc_tls_session_ctx_t ctx)
{
  return tls_session_clear_ctx_for_reestablish(ctx);
}


sqc_result_t
sqc_tls_session_setup_ssl(sqc_tls_session_ctx_t ctx)
{
  return tls_session_setup_ssl(ctx);
}


sqc_result_t
sqc_tls_session_create_ctx(sqc_tls_session_ctx_t *ctxptr,
                           const sqc_tls_conf_t conf,
                           enum tls_role role, bool do_mutual_auth, bool use_proxy_cert)
{
  return tls_session_create_ctx(ctxptr, conf, role, do_mutual_auth,
                                use_proxy_cert);
}


void
sqc_tls_session_destroy_ctx(sqc_tls_session_ctx_t ctx)
{
  tls_session_destroy_ctx(ctx);
}


bool
sqc_tls_session_io_continuable(int sslerr, sqc_tls_session_ctx_t ctx,
                               bool in_handshake, const char *diag)
{
  return tls_session_io_continuable(sslerr, ctx, in_handshake, diag);
}


sqc_result_t
sqc_tls_session_wait_io(sqc_tls_session_ctx_t ctx,
                        int fd, int tous, bool to_read)
{
  return tls_session_wait_io(ctx, fd, tous, to_read);
}


sqc_result_t
sqc_tls_session_wait_readable(sqc_tls_session_ctx_t ctx, int fd,
                              int tous)
{
  return tls_session_wait_readable(ctx, fd, tous);
}


sqc_result_t
sqc_tls_session_wait_writable(sqc_tls_session_ctx_t ctx, int fd,
                              int tous)
{
  return tls_session_wait_writable(ctx, fd, tous);
}


sqc_result_t
sqc_tls_session_get_pending_read_bytes_n(sqc_tls_session_ctx_t ctx,
                                         int *nptr)
{
  return tls_session_get_pending_read_bytes_n(ctx, nptr);
}


sqc_result_t
sqc_tls_session_verify(sqc_tls_session_ctx_t ctx, bool *is_verified)
{
  return tls_session_verify(ctx, is_verified);
}


sqc_result_t
sqc_tls_session_establish(sqc_tls_session_ctx_t ctx, int fd)
{
  return tls_session_establish(ctx, fd);
}


sqc_result_t
sqc_tls_session_update_key(sqc_tls_session_ctx_t ctx, int delta)
{
  return tls_session_update_key(ctx, delta);
}


sqc_result_t
sqc_tls_session_read(sqc_tls_session_ctx_t ctx, void *buf, int len,
                     int *actual_io_bytes)
{
  return tls_session_read(ctx, buf, len, actual_io_bytes);
}


sqc_result_t
sqc_tls_session_write(sqc_tls_session_ctx_t ctx, const void *buf,
                      int len, int *actual_io_bytes)
{
  return tls_session_write(ctx, buf, len, actual_io_bytes);
}


sqc_result_t
sqc_tls_session_timeout_read(sqc_tls_session_ctx_t ctx,
                             int fd, void *buf, int len, int timeout, int *actual_read)
{
  return tls_session_timeout_read(ctx, fd, buf, len, timeout, actual_read);
}


sqc_result_t
sqc_tls_session_timeout_write(sqc_tls_session_ctx_t ctx,
                              int fd, const void *buf, int len, int timeout, int *actual_io_bytes)
{
  return tls_session_timeout_write(ctx, fd, buf, len, timeout,
                                   actual_io_bytes);
}


sqc_result_t
sqc_tls_session_shutdown(sqc_tls_session_ctx_t ctx)
{
  return tls_session_shutdown(ctx);
}


char *
sqc_tls_session_peer_subjectdn_oneline(sqc_tls_session_ctx_t ctx)
{
  return tls_session_peer_subjectdn_oneline(ctx);
}


char *
sqc_tls_session_peer_subjectdn_rfc2253(sqc_tls_session_ctx_t ctx)
{
  return tls_session_peer_subjectdn_rfc2253(ctx);
}


char *
sqc_tls_session_peer_subjectdn_gsi(sqc_tls_session_ctx_t ctx)
{
  return tls_session_peer_subjectdn_gsi(ctx);
}


char *
sqc_tls_session_peer_cn(sqc_tls_session_ctx_t ctx)
{
  return tls_session_peer_cn(ctx);
}


enum tls_role
sqc_tls_session_get_role(sqc_tls_session_ctx_t ctx)
{
  return tls_session_get_role(ctx);
}

bool
sqc_tls_session_get_do_mutual_auth(sqc_tls_session_ctx_t ctx)
{
  return tls_session_get_do_mutual_auth(ctx);
}


bool
sqc_tls_session_get_do_build_chain(sqc_tls_session_ctx_t ctx)
{
  return tls_session_get_do_build_chain(ctx);
}


bool
sqc_tls_session_get_do_allow_no_crls(sqc_tls_session_ctx_t ctx)
{
  return tls_session_get_do_allow_no_crls(ctx);
}


bool
sqc_tls_session_get_do_allow_proxy_cert(sqc_tls_session_ctx_t ctx)
{
  return tls_session_get_do_allow_proxy_cert(ctx);
}


char *
sqc_tls_session_get_cert_file(sqc_tls_session_ctx_t ctx)
{
  return tls_session_get_cert_file(ctx);
}


char *
sqc_tls_session_get_cert_chain_file(sqc_tls_session_ctx_t ctx)
{
  return tls_session_get_cert_chain_file(ctx);
}


char *
sqc_tls_session_get_prvkey_file(sqc_tls_session_ctx_t ctx)
{
  return tls_session_get_prvkey_file(ctx);
}


char *
sqc_tls_session_get_ciphersuites(sqc_tls_session_ctx_t ctx)
{
  return tls_session_get_ciphersuites(ctx);
}


char *
sqc_tls_session_get_ca_path(sqc_tls_session_ctx_t ctx)
{
  return tls_session_get_ca_path(ctx);
}


char *
sqc_tls_session_get_acceptable_ca_path(sqc_tls_session_ctx_t ctx)
{
  return tls_session_get_acceptable_ca_path(ctx);
}


char *
sqc_tls_session_get_revoke_path(sqc_tls_session_ctx_t ctx)
{
  return tls_session_get_revoke_path(ctx);
}


void
sqc_tls_log_emit(sqc_log_level_t priority, uint64_t debug_level,
                 const char *file, int line_no, const char *func,
                 const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  return tls_log_emit(priority, debug_level, file, line_no, func,
                      format, ap);
}





__END_DECLS
