/**
 * @file 	tls_conf.c
 */





__BEGIN_DECLS


#define TLS_CA_CERTIFICATE_SUBDIR "ca"
#define TLS_CA_REVOCATION_SUBDIR "crl"
#define TLS_PEER_VERIFY_CHAIN_SUBDIR "pvchain"

#define TLS_SERVER_CERTIFICATE_FILE "server.crt"
#define TLS_SERVER_CERTIFICATE_CHAIN_FILE "server_chain.crt"
#define TLS_SERVER_PRIVATE_KEY_FILE "server.key"

#define TLS_USER_CERTIFICATE_FILE "user.crt"
#define TLS_USER_CERTIFICATE_CHAIN_FILE "user_chain.crt"
#define TLS_USER_PRIVATE_KEY_FILE "user.key"


/*
 * TLS configuration.
 */
static inline sqc_result_t
tls_conf_create(struct tls_conf_struct **conf)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

	if (likely(conf != NULL)) {
		struct tls_conf_struct *tmp_conf =
			malloc(sizeof(struct tls_conf_struct));
		if (likely(tmp_conf != NULL)) {
			tmp_conf->cipher_suite = NULL;
			tmp_conf->ca_certificate_path = NULL;
			tmp_conf->ca_revocation_path = NULL;
			tmp_conf->ca_peer_verify_chain_path = NULL;
			tmp_conf->certificate_file = NULL;
			tmp_conf->certificate_chain_file = NULL;
			tmp_conf->key_file = NULL;
			tmp_conf->key_update = false;
			tmp_conf->build_chain_local = false;
			tmp_conf->allow_no_crl = false;
			*conf = tmp_conf;
			ret = SQC_RESULT_OK;
		} else {
			ret = SQC_RESULT_NO_MEMORY;
		}
	} else {
		ret = SQC_RESULT_INVALID_ARGS;
	}

	return ret;
}


static inline sqc_result_t
tls_conf_copy(struct tls_conf_struct **conf,
              const struct tls_conf_struct *other)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
    struct tls_conf_struct *tmp_conf = NULL;

    if (likely(conf != NULL && other != NULL)) {
        ret = tls_conf_create(&tmp_conf);
        if (likely(ret == SQC_RESULT_OK)) {
            do {
                if (other->cipher_suite != NULL) {
                    tmp_conf->cipher_suite =
                        strdup(other->cipher_suite);
                    if (likely(tmp_conf->cipher_suite == NULL)) {
                        ret = SQC_RESULT_NO_MEMORY;
                        break;
                    }
                }
                if (other->ca_certificate_path != NULL) {
                    tmp_conf->ca_certificate_path =
                        strdup(other->ca_certificate_path);
                    if (likely(tmp_conf->ca_certificate_path == NULL)) {
                        ret = SQC_RESULT_NO_MEMORY;
                        break;
                    }
                }
                if (other->ca_revocation_path != NULL) {
                    tmp_conf->ca_revocation_path =
                        strdup(other->ca_revocation_path);
                    if (likely(tmp_conf->ca_revocation_path == NULL)) {
                        ret = SQC_RESULT_NO_MEMORY;
                        break;
                    }
                }
                if (other->ca_peer_verify_chain_path != NULL) {
                    tmp_conf->ca_peer_verify_chain_path =
                        strdup(other->ca_peer_verify_chain_path);
                    if (likely(tmp_conf->ca_peer_verify_chain_path == NULL)) {
                        ret = SQC_RESULT_NO_MEMORY;
                        break;
                    }
                }
                if (other->certificate_file != NULL) {
                    tmp_conf->certificate_file =
                        strdup(other->certificate_file);
                    if (likely(tmp_conf->certificate_file == NULL)) {
                        ret = SQC_RESULT_NO_MEMORY;
                        break;
                    }
                }
                if (other->certificate_chain_file != NULL) {
                    tmp_conf->certificate_chain_file =
                        strdup(other->certificate_chain_file);
                    if (likely(tmp_conf->certificate_chain_file == NULL)) {
                        ret = SQC_RESULT_NO_MEMORY;
                        break;
                    }
                }
                if (other->key_file != NULL) {
                    tmp_conf->key_file = strdup(other->key_file);
                    if (likely(tmp_conf->key_file == NULL)) {
                        ret = SQC_RESULT_NO_MEMORY;
                        break;
                    }
                }
                tmp_conf->key_update = other->key_update;
                tmp_conf->build_chain_local = other->build_chain_local;
                tmp_conf->allow_no_crl = other->allow_no_crl;
                *conf = tmp_conf;
                ret = SQC_RESULT_OK;
            } while (0);
        } else {
            ret = SQC_RESULT_NO_MEMORY;
        }
    } else {
        ret = SQC_RESULT_INVALID_ARGS;
    }

    if (unlikely(ret != SQC_RESULT_OK)) {
       sqc_tls_conf_destroy(tmp_conf);
    }
	return ret;
}


static inline sqc_result_t
s_join_path(char **out_path, const char *in_path, const char *file)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

    if (likely(out_path != NULL && IS_VALID_STRING(in_path) &&
               IS_VALID_STRING(file))) {
        char *tmp_out_path = NULL;
        size_t tmp_out_size = strlen(in_path) + 1u + strlen(file) + 1u;
        tmp_out_path = malloc(tmp_out_size);
        if (likely(tmp_out_path != NULL)) {
            ret = SQC_RESULT_OK;
            (void)snprintf(tmp_out_path, tmp_out_size, "%s/%s", in_path, file);
            *out_path = tmp_out_path;
        } else {
            ret = SQC_RESULT_NO_MEMORY;
        }
    } else {
        ret = SQC_RESULT_INVALID_ARGS;
    }

    return ret;
}


static inline bool
s_is_file(const char *path)
{
    bool ret = false;
    struct stat st;

    errno = 0;
    if (stat(path, &st) == 0 && S_ISREG(st.st_mode)) {
        ret = true;
    }

    return ret;
}


static inline bool
s_is_dir(const char *path)
{
    bool ret = false;
    struct stat st;

    errno = 0;
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        ret = true;
    }

    return ret;
}


static inline sqc_result_t
tls_conf_create_from_conf_dir(struct tls_conf_struct **conf, const char *dir,
                              enum tls_role role)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
	sqc_result_t tmp_ret = SQC_RESULT_ANY_FAILURES;
    struct tls_conf_struct *tmp_conf = NULL;
    const char *file = NULL;

    if (likely(conf != NULL && IS_VALID_STRING(dir))) {
        tmp_ret = tls_conf_create(&tmp_conf);
        if (likely(tmp_ret == SQC_RESULT_OK)) {
            do {
                tmp_ret = s_join_path(&tmp_conf->ca_certificate_path,
                                      dir, TLS_CA_CERTIFICATE_SUBDIR);
                if (likely(tmp_ret == SQC_RESULT_OK)) {
                    if (!s_is_dir(tmp_conf->ca_certificate_path)) {
                        free(tmp_conf->ca_certificate_path);
                        tmp_conf->ca_certificate_path = NULL;
                    }
                } else {
                    ret = tmp_ret;
                    break;
                }

                tmp_ret = s_join_path(&tmp_conf->ca_revocation_path,
                                      dir, TLS_CA_REVOCATION_SUBDIR);
                if (likely(tmp_ret == SQC_RESULT_OK)) {
                    if (!s_is_dir(tmp_conf->ca_revocation_path)) {
                        free(tmp_conf->ca_revocation_path);
                        tmp_conf->ca_revocation_path = NULL;
                    }
                } else {
                    ret = tmp_ret;
                    break;
                }

                tmp_ret = s_join_path(&tmp_conf->ca_peer_verify_chain_path,
                                      dir, TLS_PEER_VERIFY_CHAIN_SUBDIR);
                if (likely(tmp_ret == SQC_RESULT_OK)) {
                    if (!s_is_dir(tmp_conf->ca_peer_verify_chain_path)) {
                        free(tmp_conf->ca_peer_verify_chain_path);
                        tmp_conf->ca_peer_verify_chain_path = NULL;
                    }
                } else {
                    ret = tmp_ret;
                    break;
                }

                if (role == TLS_ROLE_SERVER) {
                    file = TLS_SERVER_CERTIFICATE_FILE;
                } else {
                    file = TLS_USER_CERTIFICATE_FILE;
                }
                tmp_ret = s_join_path(&tmp_conf->certificate_file, dir, file);
                if (likely(tmp_ret == SQC_RESULT_OK)) {
                    if (!s_is_file(tmp_conf->certificate_file)) {
                        free(tmp_conf->certificate_file);
                        tmp_conf->certificate_file = NULL;
                    }
                } else {
                    ret = tmp_ret;
                    break;
                }

                if (role == TLS_ROLE_SERVER) {
                    file = TLS_SERVER_CERTIFICATE_CHAIN_FILE;
                } else {
                    file = TLS_USER_CERTIFICATE_CHAIN_FILE;
                }
                tmp_ret = s_join_path(&tmp_conf->certificate_chain_file,
                                      dir, file);
                if (likely(tmp_ret == SQC_RESULT_OK)) {
                    if (!s_is_file(tmp_conf->certificate_chain_file)) {
                        free(tmp_conf->certificate_chain_file);
                        tmp_conf->certificate_chain_file = NULL;
                    }
                } else {
                    ret = tmp_ret;
                    break;
                }

                if (role == TLS_ROLE_SERVER) {
                    file = TLS_SERVER_PRIVATE_KEY_FILE;
                } else {
                    file = TLS_USER_PRIVATE_KEY_FILE;
                }
                tmp_ret = s_join_path(&tmp_conf->key_file, dir, file);
                if (likely(tmp_ret == SQC_RESULT_OK)) {
                    if (!s_is_file(tmp_conf->key_file)) {
                        free(tmp_conf->key_file);
                        tmp_conf->key_file = NULL;
                    }
                } else {
                    ret = tmp_ret;
                    break;
                }
                ret = SQC_RESULT_OK;
                *conf = tmp_conf;
            } while (0);
        } else {
           ret = SQC_RESULT_NO_MEMORY;
        }
    } else {
        ret = SQC_RESULT_INVALID_ARGS;
    }

    if (unlikely(ret != SQC_RESULT_OK)) {
       sqc_tls_conf_destroy(tmp_conf);
    }
    return ret;
}


static inline void
tls_conf_destroy(struct tls_conf_struct* conf)
{
	if (likely(conf != NULL)) {
		free(conf->cipher_suite);
		free(conf->ca_certificate_path);
		free(conf->ca_revocation_path);
		free(conf->ca_peer_verify_chain_path);
		free(conf->certificate_file);
		free(conf->certificate_chain_file);
		free(conf->key_file);
		free(conf);
	}
}


static inline char *
tls_conf_get_cipher_suite(struct tls_conf_struct* conf)
{
	char *ret = NULL;
    char *tmp_val = NULL;

	if (likely(conf != NULL && conf->cipher_suite != NULL)) {
		tmp_val = strdup(conf->cipher_suite);
		if (likely(tmp_val != NULL)) {
			ret = tmp_val;
		}
	}

	return ret;
}


static inline char *
tls_conf_get_ca_certificate_path(struct tls_conf_struct* conf)
{
	char *ret = NULL;
    char *tmp_val = NULL;

	if (likely(conf != NULL && conf->ca_certificate_path != NULL)) {
		ret = strdup(conf->ca_certificate_path);
		if (likely(tmp_val != NULL)) {
			ret = tmp_val;
		}
	}

	return ret;
}


static inline char *
tls_conf_get_ca_revocation_path(struct tls_conf_struct* conf)
{
	char *ret = NULL;
    char *tmp_val = NULL;

	if (likely(conf != NULL && conf->ca_revocation_path != NULL)) {
		tmp_val = strdup(conf->ca_revocation_path);
		if (likely(tmp_val != NULL)) {
			ret = tmp_val;
		}
	}

	return ret;
}


static inline char *
tls_conf_get_ca_peer_verify_chain_path(struct tls_conf_struct* conf)
{
	char *ret = NULL;
    char *tmp_val = NULL;

	if (likely(conf != NULL && conf->ca_peer_verify_chain_path != NULL)) {
		tmp_val = strdup(conf->ca_peer_verify_chain_path);
		if (likely(tmp_val != NULL)) {
			ret = tmp_val;
		}
	}

	return ret;
}


static inline char *
tls_conf_get_certificate_file(struct tls_conf_struct* conf)
{
	char *ret = NULL;
    char *tmp_val = NULL;

	if (likely(conf != NULL && conf->certificate_file != NULL)) {
		tmp_val = strdup(conf->certificate_file);
		if (likely(tmp_val != NULL)) {
			ret = tmp_val;
		}
	}

	return ret;
}


static inline char *
tls_conf_get_certificate_chain_file(struct tls_conf_struct* conf)
{
	char *ret = NULL;
    char *tmp_val = NULL;

	if (likely(conf != NULL && conf->certificate_chain_file != NULL)) {
		tmp_val = strdup(conf->certificate_chain_file);
		if (likely(tmp_val != NULL)) {
			ret = tmp_val;
		}
	}

	return ret;
}


static inline char *
tls_conf_get_key_file(struct tls_conf_struct* conf)
{
	char *ret = NULL;
    char *tmp_val = NULL;

	if (likely(conf != NULL && conf->key_file != NULL)) {
		tmp_val = strdup(conf->key_file);
		if (likely(tmp_val != NULL)) {
			ret = tmp_val;
		}
	}

	return ret;
}


static inline bool
tls_conf_get_key_update(struct tls_conf_struct* conf)
{
	sqc_result_t ret = false;

	if (likely(conf != NULL)) {
		ret = conf->key_update;
	}

	return ret;
}


static inline bool
tls_conf_get_build_chain_local(struct tls_conf_struct* conf)
{
	sqc_result_t ret = false;

	if (likely(conf != NULL)) {
		ret = conf->build_chain_local;
	}

	return ret;
}


static inline bool
tls_conf_get_allow_no_crl(struct tls_conf_struct* conf)
{
	sqc_result_t ret = false;

	if (likely(conf != NULL)) {
		ret = conf->allow_no_crl;
	}

	return ret;
}


static inline sqc_result_t
tls_conf_set_cipher_suite(struct tls_conf_struct* conf, const char *val)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
	char *tmp_val = NULL;

	if (likely(conf != NULL && val != NULL)) {
		tmp_val = strdup(val);
		if (likely(tmp_val != NULL)) {
			free(conf->cipher_suite);
			conf->cipher_suite = tmp_val;
			ret = SQC_RESULT_OK;
		} else {
			ret = SQC_RESULT_NO_MEMORY;
		}
	} else {
		ret = SQC_RESULT_INVALID_ARGS;
	}

	return ret;
}


static inline sqc_result_t
tls_conf_set_ca_certificate_path(struct tls_conf_struct* conf, const char *val)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
	char *tmp_val = NULL;

	if (likely(conf != NULL && val != NULL)) {
		tmp_val = strdup(val);
		if (likely(tmp_val != NULL)) {
			free(conf->ca_certificate_path);
			conf->ca_certificate_path = tmp_val;
			ret = SQC_RESULT_OK;
		} else {
			ret = SQC_RESULT_NO_MEMORY;
		}
	} else {
		ret = SQC_RESULT_INVALID_ARGS;
	}

	return ret;
}


static inline sqc_result_t
tls_conf_set_ca_revocation_path(struct tls_conf_struct* conf, const char *val)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
	char *tmp_val = NULL;

	if (likely(conf != NULL && val != NULL)) {
		tmp_val = strdup(val);
		if (likely(tmp_val != NULL)) {
			free(conf->ca_revocation_path);
			conf->ca_revocation_path = tmp_val;
			ret = SQC_RESULT_OK;
		} else {
			ret = SQC_RESULT_NO_MEMORY;
		}
	} else {
		ret = SQC_RESULT_INVALID_ARGS;
	}

	return ret;
}


static inline sqc_result_t
tls_conf_set_ca_peer_verify_chain_path(struct tls_conf_struct* conf,
	const char *val)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
	char *tmp_val = NULL;

	if (likely(conf != NULL && val != NULL)) {
		tmp_val = strdup(val);
		if (likely(tmp_val != NULL)) {
			free(conf->ca_peer_verify_chain_path);
			conf->ca_peer_verify_chain_path = tmp_val;
			ret = SQC_RESULT_OK;
		} else {
			ret = SQC_RESULT_NO_MEMORY;
		}
	} else {
		ret = SQC_RESULT_INVALID_ARGS;
	}

	return ret;
}


static inline sqc_result_t
tls_conf_set_certificate_file(struct tls_conf_struct* conf, const char *val)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
	char *tmp_val = NULL;

	if (likely(conf != NULL && val != NULL)) {
		tmp_val = strdup(val);
		if (likely(tmp_val != NULL)) {
			free(conf->certificate_file);
			conf->certificate_file = tmp_val;
			ret = SQC_RESULT_OK;
		} else {
			ret = SQC_RESULT_NO_MEMORY;
		}
	} else {
		ret = SQC_RESULT_INVALID_ARGS;
	}

	return ret;
}


static inline sqc_result_t
tls_conf_set_certificate_chain_file(struct tls_conf_struct* conf,
	const char *val)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
	char *tmp_val = NULL;

	if (likely(conf != NULL && val != NULL)) {
		tmp_val = strdup(val);
		if (likely(tmp_val != NULL)) {
			free(conf->certificate_chain_file);
			conf->certificate_chain_file = tmp_val;
			ret = SQC_RESULT_OK;
		} else {
			ret = SQC_RESULT_NO_MEMORY;
		}
	} else {
		ret = SQC_RESULT_INVALID_ARGS;
	}

	return ret;
}


static inline sqc_result_t
tls_conf_set_key_file(struct tls_conf_struct* conf, const char *val)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
	char *tmp_val = NULL;

	if (likely(conf != NULL && val != NULL)) {
		tmp_val = strdup(val);
		if (likely(tmp_val != NULL)) {
			free(conf->key_file);
			conf->key_file = tmp_val;
			ret = SQC_RESULT_OK;
		} else {
			ret = SQC_RESULT_NO_MEMORY;
		}
	} else {
		ret = SQC_RESULT_INVALID_ARGS;
	}

	return ret;
}


static inline sqc_result_t
tls_conf_set_key_update(struct tls_conf_struct* conf, bool val)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

	if (likely(conf != NULL)) {
		conf->key_update = val;
		ret = SQC_RESULT_OK;
	} else {
		ret = SQC_RESULT_INVALID_ARGS;
	}

	return ret;
}


static inline sqc_result_t
tls_conf_set_build_chain_local(struct tls_conf_struct* conf, bool val)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

	if (likely(conf != NULL)) {
		conf->build_chain_local = val;
		ret = SQC_RESULT_OK;
	} else {
		ret = SQC_RESULT_INVALID_ARGS;
	}

	return ret;
}


static inline sqc_result_t
tls_conf_set_allow_no_crl(struct tls_conf_struct* conf, bool val)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

	if (likely(conf != NULL)) {
		conf->allow_no_crl = val;
		ret = SQC_RESULT_OK;
	} else {
		ret = SQC_RESULT_INVALID_ARGS;
	}

	return ret;
}


static inline void
tls_conf_dump(sqc_tls_conf_t conf) {
    if (conf != NULL) {
        sqc_msg_debug(5, "tls_conf.cipher_suite = %s\n",
                      conf->cipher_suite != NULL ?
                      conf->cipher_suite : "(null)");
        sqc_msg_debug(5, "tls_conf.ca_certificate_path = %s\n",
                      conf->ca_certificate_path != NULL ?
                      conf->ca_certificate_path : "(null)");
        sqc_msg_debug(5, "tls_conf.ca_revocation_path = %s\n",
                      conf->ca_revocation_path != NULL ?
                      conf->ca_revocation_path : "(null)");
        sqc_msg_debug(5, "tls_conf.ca_peer_verify_chain_path = %s\n",
                      conf->ca_peer_verify_chain_path != NULL ?
                      conf->ca_peer_verify_chain_path : "(null)");
        sqc_msg_debug(5, "tls_conf.certificate_file = %s\n",
                      conf->certificate_file != NULL ?
                      conf->certificate_file : "(null)");
        sqc_msg_debug(5, "tls_conf.certificate_chain_file = %s\n",
                      conf->certificate_chain_file != NULL ?
                      conf->certificate_chain_file : "(null)");
        sqc_msg_debug(5, "tls_conf.key_file = %s\n",
                      conf->key_file);
        sqc_msg_debug(5, "tls_conf.key_update = %s\n",
                      conf->key_update ? "true" : "false");
        sqc_msg_debug(5, "tls_conf.build_chain_local = %s\n",
                      conf->build_chain_local ? "true" : "false");
        sqc_msg_debug(5, "tls_conf.allow_no_crl = %s\n",
                      conf->allow_no_crl ? "true" : "false");
    } else {
        sqc_msg_debug(5, "tls_conf = (null)\n");
    }
}
