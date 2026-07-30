#pragma once

/*
 * misc. utils.
 */
static inline int
escape_slash(const char *in, char **outptr, int outlen)
{
	int ret = 0;

	if (outptr == NULL) {
		*outptr = NULL;
	}

	if (likely(IS_VALID_STRING(in) == true)) {
		char *d;
		const char *s = in;
		int maxlen;
		int len;
		if (outptr != NULL) {
			d = *outptr;
			maxlen = (int)outlen - 1;
		} else {
			maxlen = (int)strlen(in) * 2 - 1;
			d = (char *)malloc((size_t)maxlen);
		}
		do {
			if (*s == '/') {
				*d++ = '\\';
			}
			*d++ = *s++;
			len++;
		} while (*s != '\0' && (s - in) < outlen);
		*d = '\0';
		ret = len;
	}

	return (ret);
}

static inline void
trim_string_tail(char *buf)
{
	if (IS_VALID_STRING(buf) == true) {
		size_t l = strlen(buf);
		char *s = buf;
		char *e = buf + l - 1;

		while (e >= s) {
			if (*e == '\r' || *e == '\n') {
				*e = '\0';
				e--;
			} else {
				break;
			}
		}
	}

	return;
}

static inline void
tty_lock(void)
{
	(void)pthread_mutex_lock(&pwd_cb_lock);
}

static inline void
tty_unlock(void)
{
	(void)pthread_mutex_unlock(&pwd_cb_lock);
}

static inline void
tty_save(int ttyfd)
{
	if (likely(ttyfd >= 0)) {
		if (is_tty_saved == false) {
			(void)tcgetattr(ttyfd, &saved_tty);
			is_tty_saved = true;
		}
	}
}

static inline void
tty_reset(int ttyfd)
{
	if (likely(ttyfd >= 0)) {
		if (is_tty_saved == true) {
			(void)tcsetattr(ttyfd, TCSAFLUSH, &saved_tty);
		} else {
			/*
			 * A wild guess: Assume only an ECHO flag is
			 * dropped.
			 */
			struct termios ts;

			(void)tcgetattr(ttyfd, &ts);
			ts.c_lflag |= ECHO;
			(void)tcsetattr(ttyfd, TCSAFLUSH, &ts);
		}
	}
}

static inline void
tty_echo_off(int ttyfd)
{
	if (likely(ttyfd >= 0)) {
		struct termios ts;

		tty_save(ttyfd);
		ts = saved_tty;
		ts.c_lflag &= ~(tcflag_t)ECHO;
		(void)tcsetattr(ttyfd, TCSAFLUSH, &ts);
	}
}

/*
 * A password must be acquired from the /dev/tty.
 */
static inline sqc_result_t
tty_get_passwd(char *buf, size_t maxlen, const char *prompt, int *lenptr)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

	if (likely(buf != NULL && maxlen > 1 && lenptr != NULL)) {
		int s_errno = -1;
		int ttyfd = -1;
		int is_tty = 0;

		*lenptr = 0;

		errno = 0;
		ttyfd = fileno(stdin);
		s_errno = errno;
		if (unlikely(s_errno != 0)) {
			goto ttyerr;
		}

		errno = 0;
		is_tty = isatty(ttyfd);
		s_errno = errno;
		if (likely(is_tty == 1)) {
			char *rst = NULL;

			(void)fprintf(stdout, "%s", prompt);

			tty_echo_off(ttyfd);

			(void)memset(buf, 0, maxlen);
			errno = 0;
			rst = fgets(buf, (int)maxlen, stdin);
			s_errno = errno;

			tty_reset(ttyfd);
			(void)fprintf(stdout, "\n");
			(void)fflush(stdout);

			if (likely(rst != NULL)) {
				trim_string_tail(buf);
				*lenptr = (int)strlen(buf);
				ret = SQC_RESULT_OK;
			} else {
				if (s_errno != 0) {
					sqc_tls_msg_error(
						"Failed to get a password: %s\n",
						strerror(s_errno));
					ret = SQC_RESULT_POSIX_API_ERROR;
				}
			}
		} else {
ttyerr:
			ret = SQC_RESULT_POSIX_API_ERROR;
			sqc_tls_msg_error(
				"stdin is not a terminal: %s\n",
				sqc_error_get_string(ret));
		}
	} else {
		sqc_tls_msg_error(
			"Invalid buffer and/or buffer length "
			"for password input: %p, %zu\n", buf, maxlen);
		ret = SQC_RESULT_INVALID_ARGS;
	}

	return (ret);
}


/*
 * Validators
 */

/*
 * Directory/File permission check primitives
 */

static inline sqc_result_t
is_user_in_group(uid_t uid, gid_t gid)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

	if (geteuid() == uid && getegid() == gid) {
		ret = SQC_RESULT_OK;
	} else {
		struct passwd u;
		struct passwd *ures = NULL;
		char ubuf[256];

		errno = 0;
		if (likely(getpwuid_r(uid, &u,
			ubuf, sizeof(ubuf), &ures) == 0 && ures != NULL)) {
			if (u.pw_gid == gid) {
				ret = SQC_RESULT_OK;
			} else {
				struct group g;
				struct group *gres = NULL;
				char gbuf[8192];

				errno = 0;
				if (likely(getgrgid_r(gid, &g,
					gbuf, sizeof(gbuf), &gres) == 0 &&
					gres != NULL)) {
					char **p = g.gr_mem;

					while (*p != NULL) {
						if (strcmp(u.pw_name,
							*p) == 0) {
							ret =
							SQC_RESULT_OK;
							break;
						}
						p++;
					}
				} else {
					if (errno != 0) {
						sqc_tls_msg_error(
							"Failed to acquire a "
							"group entry for "
							"gid %d: %s\n",
							gid, strerror(errno));
						ret = SQC_RESULT_POSIX_API_ERROR;
					} else {
						sqc_tls_msg_error(
							"Can't find the group "
							"%d.\n", gid);
						ret = SQC_RESULT_POSIX_API_ERROR;
					}
				}
			}
		} else {
			if (errno != 0) {
				sqc_tls_msg_error(
					"Failed to acquire a passwd entry "
					"for uid %d: %s\n",
					uid, strerror(errno));
				ret = SQC_RESULT_POSIX_API_ERROR;
			} else {
				sqc_tls_msg_error(
					"Can't find the user %d.\n", uid);
				ret = SQC_RESULT_INVALID_ARGS;
			}
		}
	}

	return (ret);
}

static inline sqc_result_t
is_file_readable(int fd, const char *file)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

	if (likely(IS_VALID_STRING(file) == true || fd >= 0)) {
		struct stat s;
		int st;

		errno = 0;
		if (fd >= 0) {
			st = fstat(fd, &s);
		} else {
			st = stat(file, &s);
		}
		if (likely((st == 0) &&	(S_ISDIR(s.st_mode) == 0))) {
			uid_t uid = geteuid();

			if (likely((s.st_uid == uid &&
				    (s.st_mode & S_IRUSR) != 0) ||
				   (is_user_in_group(uid, s.st_gid) ==
				    SQC_RESULT_OK &&
				    (s.st_mode & S_IRGRP) != 0) ||
				   ((s.st_mode & S_IROTH) != 0))) {
				ret = SQC_RESULT_OK;
			} else {
				ret = SQC_RESULT_NOT_ALLOWED;
				sqc_tls_msg_error(
					"%s: %s\n", file,
					sqc_error_get_string(ret));

			}
		} else {
			if (errno != 0) {
				sqc_tls_msg_error(
					"Failed to stat(\"%s\"): %s\n",
					file, strerror(errno));
				ret = SQC_RESULT_POSIX_API_ERROR;
			} else {
				sqc_tls_msg_error(
					"%s is a directory.\n", file);
				ret = SQC_RESULT_IS_A_DIRECTORY;
			}
		}
	} else {
		sqc_tls_msg_error(
			"Specified filename is nul or "
			"file invalid file descriptor.\n");
		ret = SQC_RESULT_INVALID_ARGS;
	}

	return (ret);
}

static inline sqc_result_t
is_valid_prvkey_file_permission(int fd, const char *file)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

	if (likely(IS_VALID_STRING(file) == true || fd >= 0)) {
		struct stat s;
		int st;

		errno = 0;
		if (fd >= 0) {
			st = fstat(fd, &s);
		} else {
			st = stat(file, &s);
		}
		if (likely((st == 0) &&	(S_ISDIR(s.st_mode) == 0))) {
			uid_t uid;

			if (likely((uid = geteuid()) == s.st_uid)) {
				if (likely(((s.st_mode &
					(S_IRGRP | S_IWGRP |
					 S_IROTH | S_IWOTH)) == 0) &&
					((s.st_mode &
					  S_IRUSR) != 0))) {
					ret = SQC_RESULT_OK;
				} else {
					sqc_tls_msg_error(
						"The file perrmssion of the "
						"specified file \"%s\" is "
						"open too widely. It would "
						"be nice if the file "
						"permission was 0600.\n", file);
					ret = SQC_RESULT_INVALID_PRIVATE_KEY_PERMISSION;
				}
			} else {
				sqc_tls_msg_error(
					"This process is about to read other "
					"uid(%d)'s private key file \"%s\", "
					"which is strongly discouraged even "
					"this process can read it for privacy "
					"and security.\n", uid, file);
				ret = SQC_RESULT_INVALID_PRIVATE_KEY_PERMISSION;
			}
		} else {
			if (errno != 0) {
				sqc_tls_msg_error(
					"Can't access %s: %s\n",
					file, strerror(errno));
				ret = SQC_RESULT_POSIX_API_ERROR;
			} else {
				sqc_tls_msg_error(
					"%s is a directory, not a file\n", file);
				ret = SQC_RESULT_IS_A_DIRECTORY;
			}
		}
	} else {
		sqc_tls_msg_error(
			"Specified filename is nul or "
			"file invalid file descriptor.\n");
		ret = SQC_RESULT_INVALID_ARGS;
	}

	return (ret);
}

static inline sqc_result_t
is_valid_cert_store_dir(const char *dir)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

	if (IS_VALID_STRING(dir) == true) {
		struct stat s;

		errno = 0;
		if (likely((stat(dir, &s) == 0) &&
			(S_ISDIR(s.st_mode) != 0))) {
			uid_t uid = geteuid();

			if (((s.st_mode & S_IRWXO) != 0) ||
				((s.st_mode & S_IRWXU) != 0 &&
				 s.st_uid == uid) ||
				((s.st_mode & S_IRWXG) != 0 &&
				 is_user_in_group(uid, s.st_gid) ==
				 true)) {
				ret = SQC_RESULT_OK;
			} else {
				ret = SQC_RESULT_NOT_ALLOWED;
				sqc_tls_msg_error(
					"%s: %s\n", dir,
					sqc_error_get_string(ret));
			}
		} else {
			if (errno != 0) {
				sqc_tls_msg_error(
					"Can't access to %s: %s\n",
					dir, strerror(errno));
				ret = SQC_RESULT_POSIX_API_ERROR;
			} else {
				sqc_tls_msg_error(
					"%s is not a directory.\n", dir);
				ret = SQC_RESULT_NOT_A_DIRECTORY;
			}
		}
	} else {
		sqc_tls_msg_error(
			"Specified CA cert directory name is nul.\n");
		ret = SQC_RESULT_INVALID_ARGS;
	}

	return (ret);
}

static inline char *
has_proxy_cert(void)
{
	char *ret = NULL;
	char *tmp = NULL;
	char buf[PATH_MAX];
	sqc_result_t ge = SQC_RESULT_ANY_FAILURES;

	if ((tmp =  getenv("X509_USER_PROXY")) != NULL) {
		snprintf(buf, sizeof(buf), "%s", tmp);
	} else {
		snprintf(buf, sizeof(buf), "/tmp/x509up_u%u", geteuid());
	}
	ge = is_valid_prvkey_file_permission(-1, buf);
	if (ge == SQC_RESULT_OK) {
		ret = strdup(buf);
	}
	return (ret);
}

/*
 * TLS thingies
 */

/*
 * Implementation: TLS support version of sqc_log_emit()
 */
static inline void
tls_log_emit(sqc_log_level_t priority, uint64_t debug_level,
	const char *file, int line_no, const char *func,
	const char *format, ...)
{
	char msgbuf[TLS_LOG_MSG_LEN];
	va_list ap;
	unsigned int err;

	va_start(ap, format);
	(void)vsnprintf(msgbuf, sizeof(msgbuf), format, ap);
	va_end(ap);

	if (ERR_peek_error() == 0) {
		sqc_log_emit(priority, debug_level, file, line_no, func,
			"%s", msgbuf);
	} else {
		char msgbuf2[TLS_LOG_MSG_LEN * 3];
		char tlsmsg[TLS_LOG_MSG_LEN];
		const char *tls_file, *tls_data;
		int tls_line, tls_flags;
#ifdef HAVE_ERR_GET_ERROR_ALL /* since OpenSSL-3.0 */
		const char *tls_func;

		err = (unsigned int)ERR_get_error_all(&tls_file, &tls_line, &tls_func,
			&tls_data, &tls_flags);
		ERR_error_string_n(err, tlsmsg, sizeof(tlsmsg));
		(void)snprintf(msgbuf2, sizeof(msgbuf2),
			"%s: [OpenSSL error info: %s:%d: %s%s%s%s%s]",
			msgbuf, tls_file, tls_line,
			tls_func,
			tls_func[0] != '\0' ? ": " : "",
			tlsmsg,
			(tls_flags & ERR_TXT_STRING) != 0 ? ": " : "",
			(tls_flags & ERR_TXT_STRING) != 0 ? tls_data : "");
#else /* deprecated since OpenSSL-3.0 */

		err = (unsigned int)ERR_get_error_line_data(&tls_file, &tls_line,
			&tls_data, &tls_flags);
		ERR_error_string_n(err, tlsmsg, sizeof(tlsmsg));
		(void)snprintf(msgbuf2, sizeof(msgbuf2),
			"%s: [OpenSSL error info: %s:%d: %s%s%s]",
			msgbuf, tls_file, tls_line, tlsmsg,
			(tls_flags & ERR_TXT_STRING) != 0 ? ": " : "",
			(tls_flags & ERR_TXT_STRING) != 0 ? tls_data : "");
#endif
		sqc_log_emit(priority, debug_level, file, line_no, func,
			"%s\n", msgbuf2);
	}
}

/*
 * TLS runtime library initialization
 */
static inline void
tls_runtime_init_once_body(void)
{
	/*
	 * XXX FIXME:
	 *	Are option flags sufficient enough or too much?
	 *	I'm not sure about it, hope it would be a OK.
	 */
	if (likely(OPENSSL_init_ssl(
			OPENSSL_INIT_LOAD_SSL_STRINGS |
			OPENSSL_INIT_LOAD_CRYPTO_STRINGS,
			NULL) == 1)) {
		is_tls_runtime_initd = true;
	}
}


static inline sqc_result_t
tls_session_runtime_initialize(void)
{
	(void)pthread_once(&tls_init_once, tls_runtime_init_once);
	return ((is_tls_runtime_initd == true) ?
		SQC_RESULT_OK : SQC_RESULT_ANY_FAILURES);
}

/*
 * TLS runtime error handling
 */
static inline bool
tls_has_runtime_error(void)
{
	return (ERR_peek_error() != 0);
}

static inline void
tls_runtime_flush_error(void)
{
	for ((void)ERR_get_error(); ERR_get_error() != 0;)
		;
}

/*
 * X509_NAME to string
 */
static inline sqc_result_t
get_peer_dn(X509_NAME *pn, int mode, char **nameptr, int maxlen)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
	BIO *bio = BIO_new(BIO_s_mem());

	if (likely(pn != NULL && nameptr != NULL && bio != NULL)) {
		int len = 0;
		char *buf = NULL;

		(void)X509_NAME_print_ex(bio, pn, 0, (unsigned long)mode);
		len = BIO_pending(bio);

		if (*nameptr != NULL && maxlen > 0) {
			buf = *nameptr;
			len = (maxlen > len) ? len : maxlen - 1;
		} else {
			*nameptr = NULL;
			buf = (char *)malloc((size_t)len + 1);
		}
		if (likely(len > 0 && buf != NULL)) {
			(void)BIO_read(bio, buf, len);
			buf[len] = '\0';
			ret = SQC_RESULT_OK;
			if (*nameptr != NULL) {
				*nameptr = buf;
			}
			*nameptr = buf;
		} else {
			if (buf == NULL && len > 0) {
				ret = SQC_RESULT_NO_MEMORY;
				sqc_tls_msg_error(
					"Can't allocate a %d bytes buffer for "
					"a peer SubjectDN.\n", len);
			} else if (len <= 0) {
				ret = SQC_RESULT_ANY_FAILURES;
				sqc_tls_msg_error(
					"Failed to acquire a length of peer "
					"SubjectDN.\n");
			}
		}
	} else {
		if (bio == NULL) {
			ret = SQC_RESULT_NO_MEMORY;
			sqc_tls_msg_error(
				"Can't allocate a BIO.\n");
		} else {
			ret = SQC_RESULT_INVALID_ARGS;
		}
	}

	if (bio != NULL) {
		BIO_free(bio);
	}

	return (ret);
}

static inline sqc_result_t
get_peer_dn_gsi_ish(X509_NAME *pn, char **nameptr, int maxlen)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

	if (likely(pn != NULL && nameptr != NULL)) {
		char buf[4096];
		char *dn = buf;
		char *cnp = NULL;

#define DN_FORMAT_GLOBUS						\
		(XN_FLAG_RFC2253 & ~(ASN1_STRFLGS_ESC_MSB|XN_FLAG_DN_REV))
		ret = get_peer_dn(pn, DN_FORMAT_GLOBUS,
				 &dn, sizeof(buf));
#undef DN_FORMAT_GLOBUS
		if (likely(ret == SQC_RESULT_OK &&
				(cnp = (char *)memmem(buf, sizeof(buf),
						"CN=", 3)) != NULL &&
				*(cnp += 3) != '\0')) {
			char result[4096];
			char *r = result;
			char *d = buf;

			*r++ = '/';
			do {
				switch (*d) {
				case '/':
					if (likely(r < cnp)) {
						*r++ = '\\';
					}
					*r++ = *d++;
					break;
				case ',':
					*r++ = '/';
					d++;
					break;
				case '\\':
					if (d[1] == ',')
						d++;
					/*FALLTHROUGH*/
				default:
					*r++ = *d++;
					break;
				}
			} while (*d != '\0' &&
				r < (&result[0] + sizeof(result)));
			result[r - &result[0]] = '\0';

			if (*nameptr != NULL && maxlen > 0) {
				snprintf(*nameptr, (size_t)maxlen, "%s", result);
				ret = SQC_RESULT_OK;
			} else {
				char *dn2 = strdup(result);

				if (likely(dn != NULL)) {
					ret = SQC_RESULT_OK;
					*nameptr = dn2;
				} else {
					ret = SQC_RESULT_NO_MEMORY;
					sqc_tls_msg_error(
						"Can't allocate a buffer for "
						"a GSI-compat SubjectDN.\n");
				}
			}
		} else {
			if (unlikely(ret == SQC_RESULT_OK &&
					cnp == NULL)) {
				ret = SQC_RESULT_INVALID_CREDENTIAL;
				sqc_tls_msg_error(
					"A SubjectDN \"%s\" has no CN.\n", buf);
			}
		}
	} else {
		ret = SQC_RESULT_INVALID_ARGS;
	}

	return (ret);
}

static inline sqc_result_t
get_peer_cn(X509_NAME *pn, char **nameptr, int maxlen, bool allow_many_cn)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

	if (likely(pn != NULL && nameptr != NULL)) {
		int pos = -1;
		int pos2 = -1;
		X509_NAME_ENTRY *ne = NULL;
		ASN1_STRING *as = NULL;

		/*
		 * Assumption: pn has only one CN.
		 */
		pos = X509_NAME_get_index_by_NID(pn, NID_commonName, pos);
		pos2 = X509_NAME_get_index_by_NID(pn, NID_commonName, pos);
		if (likely(((allow_many_cn == true && pos != -1) ||
			((pos != -1 && pos != -2) &&
			(pos2 == -1 || pos2 == -2))) &&
			(ne = X509_NAME_get_entry(pn, pos)) != NULL &&
			(as = X509_NAME_ENTRY_get_data(ne)) != NULL)) {
			unsigned char *u8 = NULL;
			int u8len = ASN1_STRING_to_UTF8(&u8, as);
			char *cn = NULL;

			if (likely(u8len > 0)) {
				if (*nameptr != NULL && maxlen > 0) {
					snprintf(*nameptr, (size_t)maxlen, "%s", u8);
					ret = SQC_RESULT_OK;
				} else {
					cn = strdup((char *)u8);
					if (likely(cn != NULL)) {
						ret = SQC_RESULT_OK;
						*nameptr = cn;
					} else {
						ret = SQC_RESULT_NO_MEMORY;
						sqc_tls_msg_error(
							"Can't allocate a "
							"buffer for a CN.\n");
						*nameptr = NULL;
					}
				}
			}
			if (u8 != NULL) {
				OPENSSL_free(u8);
			}
		} else if (pos >= 0 && pos2 >= 0) {
			ret = SQC_RESULT_INVALID_CREDENTIAL;
			sqc_tls_msg_notice(
				"More than one CNs are included.\n");
		} else if (pos == -1 || pos == -2) {
			ret = SQC_RESULT_INVALID_CREDENTIAL;
			sqc_tls_msg_notice(
				"No CN is included.\n");
		}
	} else {
		ret = SQC_RESULT_INVALID_ARGS;
	}

	return (ret);
}

/*
 * Private key loader
 */
static inline int
tty_passwd_callback_body(char *buf, int maxlen, int rwflag, void *u)
{
	int ret = 0;
	struct tls_passwd_cb_arg_struct *arg =
		(struct tls_passwd_cb_arg_struct *)u;

	(void)rwflag;

	if (likely(arg != NULL)) {
		char p[4096];
		bool has_passwd_cache = IS_VALID_STRING(arg->pw_buf_);
		bool do_passwd =
			(has_passwd_cache == false &&
			 arg->pw_buf_ != NULL &&
			 arg->pw_buf_maxlen_ > 0);

		if (unlikely(do_passwd == true)) {
			/*
			 * Set a prompt
			 */
			if (IS_VALID_STRING(arg->filename_) == true) {
				(void)snprintf(p, sizeof(p),
					"Passphrase for \"%s\": ",
					arg->filename_);
			} else {
				(void)snprintf(p, sizeof(p),
					"Passphrase: ");
			}
		}

		tty_lock();
		{
			if (unlikely(do_passwd == true)) {
				if (tty_get_passwd(arg->pw_buf_,
					arg->pw_buf_maxlen_, p, &ret) ==
					SQC_RESULT_OK) {
					goto copy_cache;
				}
			} else if (likely(has_passwd_cache == true)) {
copy_cache:
				ret = snprintf(buf, (size_t)maxlen, "%s",
						arg->pw_buf_);
			}
		}
		tty_unlock();
	}

	return (ret);

}

/*
 * An iterator for every file in a directory
 */
static inline sqc_result_t
iterate_file_in_a_dir(const char *dir,
	sqc_result_t (*func)(const char *, void *, int *), void *funcarg,
	int *nptr)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
	DIR *d = NULL;
	struct stat s;
	struct dirent *de = NULL;
	char filebuf[PATH_MAX];
	int nadd = 0;
	int iter_n = 0;

	errno = 0;
	if (unlikely(dir == NULL || nptr == NULL ||
		   (d = opendir(dir)) == NULL || errno != 0)) {
		if (errno != 0) {
			ret = SQC_RESULT_POSIX_API_ERROR;
			sqc_tls_msg_error(
				"Can't open a directory %s: %s\n", dir,
				sqc_error_get_string(ret));
		} else {
			ret = SQC_RESULT_INVALID_ARGS;
		}
		goto done;
	}

	*nptr = 0;
	do {
		errno = 0;
		if (likely((de = readdir(d)) != NULL)) {
			if ((de->d_name[0] == '.' && de->d_name[1] == '\0') ||
				(de->d_name[0] == '.' && de->d_name[1] == '.'
				 && de->d_name[2] == '\0')) {
				continue;
			}
			if (func == NULL) {
				nadd++;
				continue;
			}
			(void)snprintf(filebuf, sizeof(filebuf),
				"%s/%s", dir, de->d_name);
			errno = 0;
			if (stat(filebuf, &s) == 0 &&
				S_ISREG(s.st_mode) != 0 &&
				(ret = is_file_readable(-1, filebuf)) ==
				SQC_RESULT_OK) {
				iter_n = 0;
				ret = (func)(filebuf, funcarg, &iter_n);
				if (likely(ret == SQC_RESULT_OK)) {
					if (iter_n > 0) {
						nadd += iter_n;
					}
				}
				/*
				 * ignore errors. iterate all the files.
				 */
			} else {
				sqc_tls_msg_warning(
					"Skip to treat %s.\n", filebuf);
				continue;
			}
		} else {
			if (errno == 0) {
				ret = SQC_RESULT_OK;
			} else {
				ret = SQC_RESULT_POSIX_API_ERROR;
				sqc_tls_msg_error(
					"readdir(3) error: %s (%d)\n",
					sqc_error_get_string(ret), errno);
			}
			break;
		}
	} while (true);

done:
	if (likely(ret == SQC_RESULT_OK)) {
		*nptr = nadd;
	}
	if (d != NULL) {
		(void)closedir(d);
	}

	return (ret);
}

static inline sqc_result_t
tls_load_prvkey(const char *file, EVP_PKEY **keyptr)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

	if (likely(IS_VALID_STRING(file) == true && keyptr != NULL)) {
		FILE *f = NULL;
		EVP_PKEY *pkey = NULL;

		*keyptr = NULL;
		errno = 0;
		if (likely(((f = fopen(file, "r")) != NULL) &&
			((ret = is_valid_prvkey_file_permission(fileno(f),
					file)) == SQC_RESULT_OK))) {

			struct tls_passwd_cb_arg_struct a = {
				.pw_buf_maxlen_ = sizeof(the_privkey_passwd),
				.pw_buf_ = the_privkey_passwd,
				.filename_ = file
			};

			tls_runtime_flush_error();
			pkey = PEM_read_PrivateKey(f, NULL,
					tty_passwd_callback, (void *)&a);
			if (likely(pkey != NULL &&
				tls_has_runtime_error() == false)) {
				ret = SQC_RESULT_OK;
				*keyptr = pkey;
			} else {
				int rsn = ERR_GET_REASON(ERR_peek_error());
				if (rsn == PEM_R_BAD_DECRYPT ||
					rsn == EVP_R_BAD_DECRYPT) {
					sqc_tls_msg_error(
						"Wrong passphrase for "
						"private key file %s.\n", file);
				} else {
					sqc_tls_msg_error(
						"Can't read a PEM format "
						"private key from %s.\n", file);
				}
				ret = SQC_RESULT_PRIVATE_KEY_READ_FAILURE;
			}
		} else {
			if (errno != 0) {
				sqc_tls_msg_error(
					"Can't open %s: %s\n", file,
					strerror(errno));
				ret = SQC_RESULT_POSIX_API_ERROR;
			}
		}
		if (f != NULL) {
			(void)fclose(f);
		}

	}

	return (ret);
}

/*
 * Accumulate X509_NAMEs from X509s in a file.
 */
static inline sqc_result_t
accumulate_x509_names_from_file(const char *file,
	STACK_OF(X509_NAME) (*stack), int *n_added)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
	FILE *fd = NULL;

	errno = 0;
	tls_runtime_flush_error();
	if (likely(IS_VALID_STRING(file) == true && stack != NULL &&
		((fd = fopen(file, "r")) != NULL))) {
		X509 *x = NULL;
		X509_NAME *xn = NULL;
		X509_NAME *xndup = NULL;
		int n_certs = 0;
		int total_certs = 0;
		bool got_failure = false;
		char b[4096];
		char *bp = b;
		int found = INT_MAX;
		sqc_result_t got_dn = SQC_RESULT_ANY_FAILURES;

		if (n_added != NULL) {
			*n_added = 0;
		}

		while ((x = PEM_read_X509(fd, NULL, NULL, NULL)) != NULL &&
			got_failure == false) {
			if (likely((xn = X509_get_subject_name(x)) != NULL &&
				(found = sk_X509_NAME_find(stack, xn)) == -1 &&
				((got_dn = get_peer_dn_gsi_ish(xn, &bp,
					sizeof(b))) == SQC_RESULT_OK) &&
				(xndup = X509_NAME_dup(xn)) != NULL &&
				(n_certs = sk_X509_NAME_push(stack, xndup)) !=
				0)) {
				if (n_certs > 1) {
					sk_X509_NAME_sort(stack);
				}
				total_certs++;
				sqc_tls_msg_debug(1,
					"push a cert \"%s\" to a "
					"stack from %s.\n", b, file);
			} else if (found == -1 &&
					(xndup == NULL || n_certs == 0)) {
				got_failure = true;
				if (xndup == NULL) {
					ret = SQC_RESULT_NO_MEMORY;
				} else if (n_certs == 0) {
					ret = SQC_RESULT_ANY_RUNTIME_ERROR;
				}
				if (xndup != NULL) {
					X509_NAME_free(xndup);
				}
				sqc_tls_msg_debug(1,
					"failed to push a cert \"%s\" "
					"to a stack from %s.\n",
					b, file);
			} else if (found == -1) {
				got_failure = true;
				ret = SQC_RESULT_ANY_RUNTIME_ERROR;
				if (xn != NULL && got_dn ==
					SQC_RESULT_OK) {
					sqc_tls_msg_error(
						"Can't add a cert \"%s\" "
						"from %s.\n", b, file);
				} else {
					sqc_tls_msg_error(
						"Can't add a cert from %s.\n",
						file);
				}
			}
			found = INT_MAX;
			got_dn = SQC_RESULT_ANY_FAILURES;
			n_certs = 0;
			X509_free(x);
			x = NULL;
			b[0] = '\0';
			xn = NULL;
			xndup = NULL;
		}
		if (likely(got_failure == false)) {
			tls_runtime_flush_error();
			ret = SQC_RESULT_OK;
			if (n_added != NULL) {
				*n_added = total_certs;
			}
			if (total_certs == 0) {
				sqc_tls_msg_warning(
					"No cert is added from %s.\n", file);
			}
		}
	} else {
		if (IS_VALID_STRING(file) == true && fd == NULL) {
			ret = SQC_RESULT_POSIX_API_ERROR;
			sqc_tls_msg_error(
				"Can't open %s: %s.\n",
				file, sqc_error_get_string(ret));
		} else {
			ret = SQC_RESULT_INVALID_ARGS;
		}
	}

	if (fd != NULL) {
		(void)fclose(fd);
	}

	return (ret);
}

/*
 * Cert files collector for acceptable certs list.
 */
static inline sqc_result_t
tls_get_x509_name_stack_from_dir(const char *dir,
	STACK_OF(X509_NAME) (*stack), int *nptr)
{
	return (iterate_file_in_a_dir(dir,
			iterate_file_for_x509_name, stack, nptr));
}

static inline sqc_result_t
tls_set_ca_path(SSL_CTX *ssl_ctx,
	const char *ca_path, const char* acceptable_ca_path,
	STACK_OF(X509_NAME) (**trust_ca_list))
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

	/*
	 * NOTE: What Apache 2.4 does for this are:
	 *
	 *	SSL_CTX_load_verify_locations(ctx,
	 *		tls_ca_certificate_path);
	 *	if (tls_ca_peer_verify_chain_path) {
	 *		dir = tls_ca_peer_verify_chain_path;
	 *	} else {
	 *		dir = tls_ca_certificate_path;
	 *	}
	 *	STACK_OF(X509_NAME) *ca_list;
	 *
	 *	while (opendir(dir)/readdir()) {
	 *		SSL_add_file_cert_subjects_to_stack(ca_list,
	 *			file);
	 *	}
	 *	SSL_CTX_set_client_CA_list(ctx, ca_list);
	 */

	/*
	 * NOTE: And the above won't works since the CA list that
	 *	server sent is just an advisory.
	 *
	 *	What we do is:
	 *
	 *	Making the ca_list and compare the x509_NAME in
	 *	ca_list with peer cert one by one in verify callback
	 *	func, for both the client and the server.
	 *
	 *	And, in case we WON'T do this, it is going to be a
	 *	massive security problem since:
	 *
	 *	1) Sendig the CA list from server to client is easily
	 *	ignored by client side. E.g.) Apache 2.4 sends the CA
	 *	list in TLSv1.3 session, OpenSSL 1.1.1 s_client
	 *	ignores it and send a complet chained cert which
	 *	includes any certs not in the CA list sent by the
	 *	server, the server accepts it and returns "200 OK."
	 *
	 *	2) Futhere more, any clients can send a complete
	 *	chained certificate and servers won't reject it if the
	 *	server has the root CA cert of the given chain in CA
	 *	cert path.
	 *
	 *	3) In Gfarm, clients' end entity CN are the key for
	 *	authorization and the CN could be easily acquireable
	 *	by social hacking. If a malcious one somehow creates
	 *	an intermediate CA which root CA is in servers' CA
	 *	cert path, a cert having the CN could be forgeable for
	 *	impersonation.
	 */

#ifndef HAVE_OPENSSL_3_0
	if (likely(ssl_ctx != NULL && IS_VALID_STRING(ca_path) == true)) {
		if (trust_ca_list != NULL) {
			*trust_ca_list = NULL;
		}
		tls_runtime_flush_error();
		if (likely(SSL_CTX_load_verify_locations(ssl_ctx,
				NULL, ca_path) == 1)) {
			ret = SQC_RESULT_OK;
		} else {
			sqc_tls_msg_error(
				"Failed to set CA path to a SSL_CTX.\n");
			ret = SQC_RESULT_ANY_RUNTIME_ERROR;
			goto done;
		}
		if (IS_VALID_STRING(acceptable_ca_path) == true) {
			bool need_free_ca_list = false;
			int ncerts = 0;
			const char *dir = acceptable_ca_path;
			STACK_OF(X509_NAME) (*ca_list) =
				sk_X509_NAME_new(x509_name_compare);
			if (likely(ca_list != NULL &&
				(ret = tls_get_x509_name_stack_from_dir(
					dir, ca_list, &ncerts)) ==
				SQC_RESULT_OK)) {
				if (ncerts > 0) {
					if (trust_ca_list != NULL) {
						*trust_ca_list = ca_list;
					}
				} else {
					need_free_ca_list = true;
				}
			} else {
				if (ca_list == NULL) {
					sqc_tls_msg_error(
						"Can't allocate "
						"STACK_OF(X509_NAME).\n");
					ret = SQC_RESULT_NO_MEMORY;
					goto done;
				} else if (ret == SQC_RESULT_OK &&
						ncerts == 0) {
					sqc_tls_msg_warning(
						"No cert file is "
						"added as a valid cert under "
						"%s directory.\n", dir);
					need_free_ca_list = true;
				}
			}
			if (need_free_ca_list == true) {
				sk_X509_NAME_pop_free(ca_list,
					      X509_NAME_free);
			}
		}
	} else {
		ret = SQC_RESULT_INVALID_ARGS;
	}

done:

#else
	X509_STORE *ch = NULL;
	X509_STORE *ve = NULL;
	const char *ve_path = (IS_VALID_STRING(acceptable_ca_path) == true) ?
		acceptable_ca_path : ca_path;

	if (trust_ca_list != NULL) {
		*trust_ca_list = NULL;
	}

	/* chain */
	tls_runtime_flush_error();
	if (likely(ssl_ctx != NULL && IS_VALID_STRING(ca_path) == true &&
		(ch = X509_STORE_new()) != NULL)) {
		tls_runtime_flush_error();
		if (likely(X509_STORE_load_path(ch, ca_path) == 1)) {
			tls_runtime_flush_error();
			if (likely(SSL_CTX_set0_chain_cert_store(
					ssl_ctx, ch) == 1)) {
				ret = SQC_RESULT_OK;
			} else {
				sqc_tls_msg_error(
					"Failed to set a CA chain path to a "
					"SSL_CTX\n");
				ret = SQC_RESULT_ANY_RUNTIME_ERROR;
				goto done;
			}
		} else {
			sqc_tls_msg_error(
				"Failed to load a CA cnain path\n");
			ret = SQC_RESULT_ANY_RUNTIME_ERROR;
			goto done;
		}
	} else {
		if (ch == NULL) {
			ret = SQC_RESULT_NO_MEMORY;
			sqc_tls_msg_error(
				"Can't allocate a X509_STORE: %s\n",
				sqc_error_get_string(ret));
		} else {
			ret = SQC_RESULT_INVALID_ARGS;
		}
		goto done;
	}

	/* verify */
	tls_runtime_flush_error();
	if (likely(ssl_ctx != NULL && IS_VALID_STRING(ca_path) == true &&
		(ve = X509_STORE_new()) != NULL)) {
		tls_runtime_flush_error();
		if (likely(X509_STORE_load_path(ve, ve_path) == 1)) {
			tls_runtime_flush_error();
			if (likely(SSL_CTX_set0_verify_cert_store(
					ssl_ctx, ve) == 1)) {
				ret = SQC_RESULT_OK;
			} else {
				sqc_tls_msg_error(
					"Failed to set a CA verify path to a "
					"SSL_CTX\n");
				ret = SQC_RESULT_ANY_RUNTIME_ERROR;
				goto done;
			}
		} else {
			sqc_tls_msg_error(
				"Failed to load a CA verify path\n");
			ret = SQC_RESULT_ANY_RUNTIME_ERROR;
			goto done;
		}
	} else {
		if (ch == NULL) {
			ret = SQC_RESULT_NO_MEMORY;
			sqc_tls_msg_error(
				"Can't allocate a X509_STORE: %s\n",
				sqc_error_get_string(ret));
		} else {
			ret = SQC_RESULT_INVALID_ARGS;
		}
		goto done;
	}

done:
	if (unlikely(ret != SQC_RESULT_OK)) {
		if (ch != NULL) {
			X509_STORE_free(ch);
		}
		if (ve != NULL) {
			X509_STORE_free(ve);
		}
	}
#endif /* ! HAVE_OPENSSL_3_0 */

	return (ret);
}

/*
 * Add extra cert(s) from a file into SSL_CTX
 */
static inline sqc_result_t
tls_add_extra_certs(SSL_CTX *ssl_ctx, const char *file, int *n_added)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
	FILE *fd = NULL;
	int osst = -INT_MAX;

	errno = 0;
	tls_runtime_flush_error();
	if (likely(ssl_ctx != NULL && IS_VALID_STRING(file) == true &&
		((fd = fopen(file, "r")) != NULL))) {
		X509 *x = NULL;
		X509_NAME *xn = NULL;
		int n_certs = 0;
		bool got_failure = false;
		char b[4096];
		char *bp = b;

		if (n_added != NULL) {
			*n_added = 0;
		}

		(void)SSL_CTX_clear_extra_chain_certs(ssl_ctx);
		while ((x = PEM_read_X509(fd, NULL, NULL, NULL)) != NULL &&
			got_failure == false) {
			tls_runtime_flush_error();
			osst = (int)SSL_CTX_add_extra_chain_cert(ssl_ctx, x);
			if (likely(osst == 1)) {
				n_certs++;
				if ((xn = X509_get_subject_name(x)) != NULL) {
					get_peer_dn_gsi_ish(xn,
						&bp, sizeof(b));
					sqc_tls_msg_debug(1,
						"Add a cert \"%s\" from %s.\n",
						b, file);
				}
			} else {
				got_failure = true;
				xn = X509_get_subject_name(x);
				if (xn != NULL) {
					get_peer_dn_gsi_ish(xn,
						&bp, sizeof(b));
					sqc_tls_msg_error(
						"Can't add a cert \"%s\" "
						"from %s.\n", b, file);
				} else {
					sqc_tls_msg_error(
						"Can't add a cert from %s.\n",
						file);
				}
				X509_free(x);
			}
		}
		if (likely(got_failure == false)) {
			tls_runtime_flush_error();
			ret = SQC_RESULT_OK;
			if (n_added != NULL) {
				*n_added = n_certs;
			}
			if (n_certs == 0) {
				sqc_tls_msg_warning(
					"No cert is added from %s.\n", file);
			}
		} else {
			ret = SQC_RESULT_ANY_RUNTIME_ERROR;
		}
	} else {
		if (osst == -INT_MAX && fd == NULL) {
			ret = SQC_RESULT_INVALID_ARGS;
		} else if (fd == NULL) {
			ret = SQC_RESULT_POSIX_API_ERROR;
			sqc_tls_msg_error(
				"Can't open %s: %s.\n",
				file, sqc_error_get_string(ret));
		}
	}

	if (fd != NULL) {
		(void)fclose(fd);
	}

	return (ret);
}

/*
 * Load both cert file and cert chain file
 */
static inline sqc_result_t
tls_load_cert_and_chain(SSL_CTX *ssl_ctx,
	const char *cert_file, const char *cert_chain_file, int *nptr)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

	if (likely(ssl_ctx != NULL)) {
		int n_certs = 0;
		int n;
		int ost = INT_MAX - 1;

		if (nptr != NULL) {
			*nptr = 0;
		}

		tls_runtime_flush_error();
		if (IS_VALID_STRING(cert_file) == true &&
			IS_VALID_STRING(cert_chain_file) == false &&
			(ost = SSL_CTX_use_certificate_chain_file(ssl_ctx,
				cert_file)) == 1) {
			ret = SQC_RESULT_OK;
			n_certs = 1;
		} else if (IS_VALID_STRING(cert_file) == false &&
			IS_VALID_STRING(cert_chain_file) == true &&
			(ost = SSL_CTX_use_certificate_chain_file(ssl_ctx,
				cert_chain_file)) == 1) {
			ret = SQC_RESULT_OK;
			n_certs = 1;
		} else {
			if (IS_VALID_STRING(cert_file) == true &&
				(ost = SSL_CTX_use_certificate_chain_file(
					ssl_ctx, cert_file)) == 1) {
				if (IS_VALID_STRING(cert_chain_file) == true) {
					n = 0;
					ret = tls_add_extra_certs(ssl_ctx,
						cert_chain_file, &n);
					if (likely(ret ==
						SQC_RESULT_OK)) {
						n_certs += n;
					} else {
						goto done;
					}
				}
			}
		}

		if (nptr != NULL) {
			*nptr = n_certs;
		}
	}

done:
	return (ret);
}

/*
 * Set revocation path
 */
static inline sqc_result_t
tls_set_revoke_path(SSL_CTX *ssl_ctx, const char *revoke_path)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
	X509_STORE *store = NULL;
	int nent = -1;

	tls_runtime_flush_error();
	if (likely(ssl_ctx != NULL &&
		(ret = iterate_file_in_a_dir(revoke_path,
			NULL, NULL, &nent)) == SQC_RESULT_OK &&
		nent > 0 &&
		(store = SSL_CTX_get_cert_store(ssl_ctx)) != NULL &&
		IS_VALID_STRING(revoke_path) == true)) {
		int st;

		tls_runtime_flush_error();
		st = X509_STORE_load_locations(store, NULL, revoke_path);
		if (likely(st == 1)) {
			tls_runtime_flush_error();
			st = X509_STORE_set_flags(store,
				X509_V_FLAG_CRL_CHECK |
				X509_V_FLAG_CRL_CHECK_ALL);
			if (likely(st == 1)) {
				ret = SQC_RESULT_OK;
			} else {
				sqc_tls_msg_error(
					"Failed to set CRL flags "
					"to an X509_STORE.\n");
				ret = SQC_RESULT_ANY_RUNTIME_ERROR;
			}
		} else {
			sqc_tls_msg_error(
				"Failed to set CRL path to an SSL_CTX.\n");
			ret = SQC_RESULT_ANY_RUNTIME_ERROR;
		}
	} else if (ret != SQC_RESULT_OK) {
		if (tls_has_runtime_error() == true) {
			sqc_tls_msg_error(
				"Failed to get current X509_STORE from "
				"an SSL_CTX.\n");
			ret = SQC_RESULT_ANY_RUNTIME_ERROR;
		} else {
			ret = SQC_RESULT_INVALID_ARGS;
		}
	}

	return (ret);
}

/*
 * Certificate verification callback
 */
static inline int
tls_verify_callback_body(int ok, X509_STORE_CTX *sctx)
{
	int ret = ok;
	int org_ok = ok;
	SSL *ssl = X509_STORE_CTX_get_ex_data(sctx,
			SSL_get_ex_data_X509_STORE_CTX_idx());
	struct tls_session_ctx_struct *ctx = (ssl != NULL) ?
		(struct tls_session_ctx_struct *)SSL_get_app_data(ssl) : NULL;
	int verr = X509_STORE_CTX_get_error(sctx);
	int org_verr = verr;
	int vdepth = X509_STORE_CTX_get_error_depth(sctx);
	const char *verrstr = NULL;
	X509 *p = X509_STORE_CTX_get_current_cert(sctx);
	X509_NAME *pn = (p != NULL) ? X509_get_subject_name(p) : NULL;

	if (likely(ok == 1)) {

		/*
		 * Here we can deny auth for our own purpose even it
		 * is accpetable.
		 */

		/*
		 * NOTE: The certs gonna coming here in order of top
		 *	to bottom (root CA, ... some intermediate CAs,
		 *	... EEC, proxy cert, proxy cert 1, ...)
		 */

		PROXY_CERT_INFO_EXTENSION *pci = NULL;

		if (ctx->is_got_proxy_cert_ == false &&
			(p != NULL &&
			((X509_get_extension_flags(p) & EXFLAG_PROXY) != 0) &&
			(pci = X509_get_ext_d2i(p, NID_proxyCertInfo,
					NULL, NULL)) != NULL)) {
			/*
			 * got a proxy cert.
			 */
			if (ctx->do_allow_proxy_cert_ == true) {
				X509_NAME *xn = X509_get_issuer_name(p);
				if (likely(xn != NULL)) {
					/*
					 * Acquire X509_NAME of the
					 * issuer only for the first
					 * proxy cert.
					 */
					char b[4096];
					char *bp = b;
					ctx->is_got_proxy_cert_ = true;
					ctx->proxy_issuer_ =
						X509_NAME_dup(xn);

					get_peer_dn_gsi_ish(
						ctx->proxy_issuer_,
						&bp, sizeof(b));
					sqc_tls_msg_debug(1,
						"got proxy issure: "
						"\"%s\"\n", b);
				} else {
					sqc_tls_msg_error(
						"Can't acquire an issure name "
						"of the proxy cert.\n");
					/* make the auth failure. */
					ok = ret = 0;
/* checkpatch */
#define VFYERR_GET_ISSUER \
        X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT
					verr = VFYERR_GET_ISSUER;
#undef VFYERR_GET_ISSUER
					X509_STORE_CTX_set_error(sctx, verr);
				}
			} else {
				/*
				 * Must not happen.
				 */
				sqc_tls_msg_warning(
					"Something wrong: got a proxy cert "
					"and it's authorized by the verify "
					"flags, but not "
					"allow the internal proxy use??\n");
			}
			PROXY_CERT_INFO_EXTENSION_free(pci);
			goto done;
		}

		/*
		 * Trusted cert check
		 */
		if (likely(vdepth > 0 && pn != NULL &&
			ctx->trusted_certs_ != NULL &&
			sk_X509_NAME_find(ctx->trusted_certs_, pn) == -1)) {
			ok = ret = 0;
			verr = X509_V_ERR_CERT_UNTRUSTED;
			X509_STORE_CTX_set_error(sctx, verr);
			goto done;
		}

	} else {

		/*
		 * Here we can accept auth for our own purpose even it
		 * must be denied.
		 */

		if (ctx->do_allow_no_crls_ == true &&
			verr == X509_V_ERR_UNABLE_TO_GET_CRL) {
			/* CRL error recovery */
			X509_STORE_CTX_set_error(sctx, X509_V_OK);
			verr = X509_V_OK;
			ok = ret = 1;
			goto done;
		}
	}

done:
	ctx->cert_verify_callback_error_ = verr;

	do {
		char dnbuf[4096];
		char *dn = dnbuf;

		if (org_ok == 0 && org_verr != X509_V_OK) {
			verrstr = X509_verify_cert_error_string(org_verr);
		} else {
			verrstr = X509_verify_cert_error_string(verr);
		}

		if (pn != NULL &&
			get_peer_dn_gsi_ish(pn, &dn, sizeof(dnbuf)) ==
			SQC_RESULT_OK) {
			dn = dnbuf;
		} else {
			dn = NULL;
		}

		sqc_tls_msg_debug(1, "depth %d; ok %d -> %d; "
			" cert \"%s\"; error %d -> %d: error string \"%s.\"\n",
			vdepth, org_ok, ok, dn, org_verr, verr, verrstr);
	} while (0);

	return (ret);
}

/*
 * Internal TLS context constructor/destructor
 */
static inline void
tls_session_clear_ctx(struct tls_session_ctx_struct *ctx, int flags)
{
#define free_n_nullify(free_func, obj)			\
	do {						\
		if (ctx->obj != NULL) {		\
			(void)free_func(ctx->obj);	\
			ctx->obj = NULL;		\
		}					\
	} while (false)

	if (likely(ctx != NULL)) {
		if ((flags & CTX_CLEAR_RECONN) != 0) {
			if (ctx->ssl_ != NULL) {
				(void)SSL_clear(ctx->ssl_);
			}
		}
		if ((flags & CTX_CLEAR_SSL) != 0) {
			free_n_nullify(SSL_free, ssl_);
		}
		if ((flags & (CTX_CLEAR_VAR | CTX_CLEAR_RECONN)) != 0) {
			ctx->last_ssl_error_ = SSL_ERROR_SSL;
			ctx->is_got_fatal_ssl_error_ = false;
			ctx->io_total_ = 0;
			ctx->io_key_update_accum_ = 0;
		}

		/*
		 * ssize_t io_key_update_thresh_;
		 */

		if ((flags & (CTX_CLEAR_VAR | CTX_CLEAR_RECONN)) != 0) {
			ctx->last_sqc_error_ = SQC_RESULT_ANY_FAILURES;
		}

		/*
		 * enum tls_role role_;
		 * bool do_mutual_auth_;
		 * bool do_build_chain_;
		 * bool do_allow_no_crls_;
		 * bool do_allow_proxy_cert_;
		 */

		if ((flags & CTX_CLEAR_CTX) != 0) {
			free_n_nullify(free, cert_file_);
			free_n_nullify(free, cert_chain_file_);
			free_n_nullify(free, prvkey_file_);
			free_n_nullify(free, ciphersuites_);
			free_n_nullify(free, ca_path_);
			free_n_nullify(free, acceptable_ca_path_);
			free_n_nullify(free, revoke_path_);
		}

		if ((flags & (CTX_CLEAR_CTX | CTX_CLEAR_VAR)) != 0) {
			free_n_nullify(free, peer_dn_oneline_);
			free_n_nullify(free, peer_dn_rfc2253_);
			free_n_nullify(free, peer_dn_gsi_);
			free_n_nullify(free, peer_cn_);
		}

		if ((flags & CTX_CLEAR_VAR) != 0) {
			ctx->is_handshake_tried_ = false;
			ctx->is_verified_ = false;
			ctx->is_got_proxy_cert_ = false;

			ctx->cert_verify_callback_error_ =
				X509_V_ERR_UNSPECIFIED;
			ctx->cert_verify_result_error_ =
				X509_V_ERR_UNSPECIFIED;
		}

		if ((flags & CTX_CLEAR_CTX) != 0) {
			if (ctx->trusted_certs_ != NULL) {
				sk_X509_NAME_pop_free(ctx->trusted_certs_,
						X509_NAME_free);
			}
			ctx->trusted_certs_ = NULL;
			free_n_nullify(SSL_CTX_free, ssl_ctx_);
			free_n_nullify(EVP_PKEY_free, prvkey_);
			free_n_nullify(X509_NAME_free, proxy_issuer_);

			free(ctx);
		}
	}
#undef free_n_nullify
}

static inline sqc_result_t
tls_session_clear_ctx_for_reconnect(struct tls_session_ctx_struct *ctx)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

	if (likely(ctx != NULL)) {
		tls_session_clear_ctx(ctx,
			CTX_CLEAR_READY_FOR_RECONNECT);
	} else {
		ret = SQC_RESULT_INVALID_ARGS;
	}

	return (ret);
}

static inline sqc_result_t
tls_session_clear_ctx_for_reestablish(struct tls_session_ctx_struct *ctx)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

	if (likely(ctx != NULL)) {
		(void)tls_session_shutdown(ctx);
		tls_session_clear_ctx(ctx,
		      CTX_CLEAR_READY_FOR_ESTABLISH);
	} else {
		ret = SQC_RESULT_INVALID_ARGS;
	}

	return (ret);
}

static inline sqc_result_t
tls_session_setup_ssl(struct tls_session_ctx_struct *ctx)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
	SSL *ssl = NULL;
	SSL_CTX *ssl_ctx = NULL;
	enum tls_role role = TLS_ROLE_UNKNOWN;

	if (unlikely(ctx == NULL || (ssl_ctx = ctx->ssl_ctx_) == NULL ||
		(role = ctx->role_) == TLS_ROLE_UNKNOWN)) {
		ret = SQC_RESULT_INVALID_ARGS;
		goto done;
	}

	tls_runtime_flush_error();
	if (ctx->ssl_ == NULL) {
		ssl = SSL_new(ssl_ctx);
	} else {
		ssl = ctx->ssl_;
	}
	if (likely(ssl != NULL)) {
		/*
		 * Make this SSL only for TLSv1.3, for sure.
		 */
		int osst = -1;

		if ((osst = (int)SSL_get_min_proto_version(ssl)) !=
			TLS1_3_VERSION ||
			(osst = (int)SSL_get_max_proto_version(ssl)) !=
			TLS1_3_VERSION) {

			tls_runtime_flush_error();
			if (unlikely((osst = (int)SSL_set_min_proto_version(ssl,
						TLS1_3_VERSION)) != 1 ||
					(osst = (int)SSL_set_max_proto_version(ssl,
						TLS1_3_VERSION)) != 1)) {
				sqc_tls_msg_error(
					"Failed to set an SSL "
					"only using TLSv1.3.\n");
				ret = SQC_RESULT_ANY_RUNTIME_ERROR;
				goto done;
			} else {
				if ((osst = (int)SSL_get_min_proto_version(ssl)) !=
					TLS1_3_VERSION ||
					(osst = (int)SSL_get_max_proto_version(
						ssl)) != TLS1_3_VERSION) {
					sqc_tls_msg_error(
						"Failed to check if the SSL "
						"only using TLSv1.3.\n");
					ret = SQC_RESULT_ANY_RUNTIME_ERROR;
					goto done;
				} else {
					ret = SQC_RESULT_OK;
				}
			}
		} else {
			ret = SQC_RESULT_OK;
		}

		if (ret == SQC_RESULT_OK) {
			/*
			 * Set a verify callback user arg.
			 */
			tls_runtime_flush_error();
			osst = SSL_set_app_data(ssl, ctx);
			if (osst != 1) {
				sqc_tls_msg_error(
					"Failed to set an arg for the verify "
					"callback\n");
				ret = SQC_RESULT_ANY_RUNTIME_ERROR;
			}
		}

#if 0
		/*
		 * XXX FIXME:
		 *
		 * calling SSL_verify_client_post_handshake() always
		 * returns 0, with "wrong protocol version" error even
		 * the SSL is setup for TLSv1.3. Is calling the API
		 * not needed?  Actually, there's no source code
		 * calling the function in OpenSSL sources. I thought
		 * s_server calls it but it does not. Or maybe it must
		 * be called "AFTER" the handshake done, when the
		 * server really needs client certs...
		 */
		if (ctx->do_mutual_auth_ == true &&
			role == TLS_ROLE_SERVER) {
			tls_runtime_flush_error();
			if (likely(SSL_verify_client_post_handshake(
					ssl) == 1)) {
				ret = SQC_RESULT_OK;
			} else {
				sqc_tls_msg_error(
					"Failed to set a "
					"server SSL to use "
					"post-handshake.\n");
				ret = SQC_RESULT_ANY_RUNTIME_ERROR;
			}
		}
#endif

		if (ret == SQC_RESULT_OK) {
			tls_session_clear_ctx(ctx,
				CTX_CLEAR_READY_FOR_ESTABLISH);
			ctx->ssl_ = ssl;
		}
	} else {
		ret = SQC_RESULT_INVALID_ARGS;
	}

done:
	if (ssl != NULL && ret != SQC_RESULT_OK) {
		SSL_free(ssl);
		ctx->ssl_ = NULL;
	}

	return (ret);
}

/*
 * Official xported APIs
 */

/*
 * Constructor
 */
static inline sqc_result_t
tls_session_create_ctx(struct tls_session_ctx_struct **ctxptr,
			   const struct tls_conf_struct *conf,
		       enum tls_role role,
		       bool do_mutual_auth, bool use_proxy_cert)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

	struct tls_session_ctx_struct *ctxret = NULL;

	EVP_PKEY *prvkey = NULL;
	SSL_CTX *ssl_ctx = NULL;

	bool do_build_chain = false;
	bool need_self_cert = false;
	bool do_proxy_auth = false;

	char *tmp = NULL;
	char *tmp_proxy_cert_file = NULL;

	/*
	 * Following strings must be copied to *ctxret
	 */
	char *cert_file = NULL;		/* required always */
	char *cert_chain_file = NULL;	/* required for server/mutual */
	char *prvkey_file = NULL;	/* required for server/mutual */
	char *ca_path = NULL;		/* required for server/mutual */
	char *acceptable_ca_path = NULL;
	char *revoke_path = NULL;
	char *ciphersuites = NULL;
	STACK_OF(X509_NAME) (*trust_ca_list) = NULL;

	/*
	 * Parameter check
	 */
	if (unlikely(ctxptr == NULL)) {
		sqc_tls_msg_error(
			"return pointer is NULL.\n");
		ret = SQC_RESULT_INVALID_ARGS;
		goto bailout;
	} else {
		*ctxptr = NULL;
	}
	if (unlikely(role != TLS_ROLE_SERVER && role != TLS_ROLE_CLIENT)) {
		sqc_tls_msg_error(
			"fatal: invalid TLS role.\n");
		ret = SQC_RESULT_INVALID_ARGS;
		goto bailout;
	}
	if (unlikely(role == TLS_ROLE_CLIENT && use_proxy_cert == true &&
		do_mutual_auth == false)) {
		ret = SQC_RESULT_INVALID_ARGS;
		goto bailout;
	}

	/*
	 * No doamin check for following variables in conf:
	 *	tls_build_chain_local
	 *	tls_allow_no_crl
	 * Callers must guarantee that values are 0 or 1.
	 */

	/*
	 * Gfarm context check
	 */
	if (unlikely(conf == NULL)) {
		sqc_tls_msg_error(
			"fatal: NULL conf.\n");
		ret = SQC_RESULT_ANY_FAILURES;
		goto bailout;
	}

#define str_or_NULL(x)					\
	((IS_VALID_STRING((x)) == true) ? (x) : NULL)

	/*
	 * CA certs path (mandatory always)
	 */
	tmp = str_or_NULL(conf->ca_certificate_path);
	if ((IS_VALID_STRING(tmp) == true) &&
		((ret = is_valid_cert_store_dir(tmp))
		== SQC_RESULT_OK)) {
		ca_path = strdup(tmp);
		if (unlikely(ca_path == NULL)) {
			ret = SQC_RESULT_NO_MEMORY;
			sqc_tls_msg_error(
				"Can't duplicate a CA certs directory "
				" name: %s\n", sqc_error_get_string(ret));
			goto bailout;
		}
	} else {
		if (tmp == NULL) {
			sqc_tls_msg_error(
				"A CA cert path is not specified.\n");
			ret = SQC_RESULT_INVALID_ARGS;
		} else {
			sqc_tls_msg_error(
				"Failed to check a CA certs directory %s: %s\n",
				tmp, sqc_error_get_string(ret));
		}
		goto bailout;
	}

	/*
	 * Revocation path (optional)
	 */
	tmp = str_or_NULL(conf->ca_revocation_path);
	if ((IS_VALID_STRING(tmp) == true) &&
		((ret = is_valid_cert_store_dir(tmp)) == SQC_RESULT_OK)) {
		revoke_path = strdup(tmp);
		if (unlikely(revoke_path == NULL)) {
			ret = SQC_RESULT_NO_MEMORY;
			sqc_tls_msg_error(
				"Can't duplicate a revoked CA certs "
				"directory name: %s\n",
				sqc_error_get_string(ret));
			goto bailout;
		}
	} else {
		if (tmp != NULL) {
			sqc_tls_msg_warning(
				"Failed to check revoked certs directory "
				"%s: %s\n", tmp, sqc_error_get_string(ret));
		}
	}

	/*
	 * Acceptable CA cert path (optional)
	 */
	tmp = str_or_NULL(conf->ca_peer_verify_chain_path);
	if (IS_VALID_STRING(tmp) == true &&
		((ret = is_valid_cert_store_dir(tmp)) == SQC_RESULT_OK)) {
		acceptable_ca_path = strdup(tmp);
		if (unlikely(acceptable_ca_path == NULL)) {
			ret = SQC_RESULT_NO_MEMORY;
			sqc_tls_msg_error(
				"Can't duplicate an acceptable CA "
				"certs directory nmae: %s\n",
				sqc_error_get_string(ret));
			goto bailout;
		}
	} else {
		if (tmp != NULL) {
			sqc_tls_msg_warning(
				"Failed to check peer certs verification "
				"directory %s: %s\n",
				tmp, sqc_error_get_string(ret));
		}
	}

	/*
	 * Self certificate check
	 */
	if (do_mutual_auth == true || role == TLS_ROLE_SERVER) {
		need_self_cert = true;
	}
	if (need_self_cert == true) {
		char *tmp_cert_file =
			str_or_NULL(conf->certificate_file);
		char *tmp_cert_chain_file =
			str_or_NULL(conf->certificate_chain_file);
		char *tmp_prvkey_file =
			str_or_NULL(conf->key_file);

		tmp_proxy_cert_file =
			(use_proxy_cert == true && role == TLS_ROLE_CLIENT) ?
			has_proxy_cert() : NULL;

		/*
		 * cert/cert chain file (mandatory)
		 */
		if ((IS_VALID_STRING(tmp_cert_chain_file) == true) &&
			((ret = is_file_readable(-1, tmp_cert_chain_file))
			== SQC_RESULT_OK)) {
			cert_chain_file = strdup(tmp_cert_chain_file);
			if (unlikely(cert_chain_file == NULL)) {
				ret = SQC_RESULT_NO_MEMORY;
				sqc_tls_msg_warning(
					"can't duplicate a cert chain "
					"filename: %s\n",
					sqc_error_get_string(ret));
			}
		}
		if ((IS_VALID_STRING(tmp_cert_file) == true) &&
			((ret = is_file_readable(-1, tmp_cert_file))
			== SQC_RESULT_OK)) {
			cert_file = strdup(tmp_cert_file);
			if (unlikely(cert_file == NULL)) {
				ret = SQC_RESULT_NO_MEMORY;
				sqc_tls_msg_warning(
					"Can't duplicate a cert filename: %s\n",
					sqc_error_get_string(ret));
			}
		}
		if (unlikely(IS_VALID_STRING(cert_chain_file) == false &&
			IS_VALID_STRING(cert_file) == false &&
			IS_VALID_STRING(tmp_proxy_cert_file) == false)) {
			/*
			 * We still have a chance to go if we had a
			 * usable proxy cert.
			 */
			sqc_tls_msg_error(
				"None of a cert file, a cert chain "
				"file, and a proxy cert file is specified.\n");
			/* Don't overwrite return code ever set */
			if (ret == SQC_RESULT_ANY_FAILURES ||
				ret == SQC_RESULT_OK) {
				ret = SQC_RESULT_INVALID_ARGS;
			}
			goto bailout;
		}

		/*
		 * Private key (mandatory)
		 */
		if (likely(IS_VALID_STRING(tmp_prvkey_file) == true)) {
			prvkey_file = strdup(tmp_prvkey_file);
			if (unlikely(prvkey_file == NULL)) {
				ret = SQC_RESULT_NO_MEMORY;
				sqc_tls_msg_error(
					"Can't duplicate a private key "
					"filename: %s\n",
					sqc_error_get_string(ret));
				goto bailout;
			}
		} else if (IS_VALID_STRING(tmp_proxy_cert_file) == false) {
			/*
			 * We still have a chance to go if we had a
			 * usable proxy cert.
			 */
			sqc_tls_msg_error(
				"A private key file is not specified.\n");
			ret = SQC_RESULT_INVALID_ARGS;
			goto bailout;
		}

	}

	/*
	 * Ciphersuites (optional)
	 * Set only TLSv1.3 allowed ciphersuites
	 */
	ciphersuites =
		(tmp = str_or_NULL(conf->cipher_suite)) != NULL ?
		strdup(tmp) : NULL;

	/*
	 * Final parameter check
	 */
	if (role == TLS_ROLE_SERVER) {
		if (unlikely(IS_VALID_STRING(ca_path) != true ||
			(IS_VALID_STRING(cert_file) != true &&
			IS_VALID_STRING(cert_chain_file) != true) ||
			IS_VALID_STRING(prvkey_file) != true)) {
			sqc_tls_msg_error(
				"As a TLS server, at least a CA ptth, a cert "
				"file/cert chain file and a private key file "
				"must be presented.\n");
			goto bailout;
		}
	} else {
		if (do_mutual_auth == true) {
			if (IS_VALID_STRING(ca_path) == true &&
			    IS_VALID_STRING(tmp_proxy_cert_file) == true) {
				free(cert_file);
				free(prvkey_file);
				cert_file = strdup(tmp_proxy_cert_file);
				prvkey_file = strdup(tmp_proxy_cert_file);
				do_proxy_auth = true;
				goto runtime_init;
			} else if (likely(IS_VALID_STRING(ca_path) == true &&
				(IS_VALID_STRING(cert_file) == true ||
				IS_VALID_STRING(cert_chain_file) == true) &&
				IS_VALID_STRING(prvkey_file) == true)) {
				goto runtime_init;
			} else {
				sqc_tls_msg_error(
					"For TLS client auth, at least "
					"a CA ptth, a cert file/cert chain "
					"file and a private key file, or a "
					"CA path and GSI/GCT proxy cert "
					"must be presented.\n");
				goto bailout;
			}
		} else {
			if (unlikely(IS_VALID_STRING(ca_path) != true)) {
				sqc_tls_msg_error(
					"At least a CA path must be "
					"specified.\n");
				goto bailout;
			}
		}
	}

runtime_init:
	/*
	 * TLS runtime initialize
	 */
	if (unlikely((ret = tls_session_runtime_initialize())
		!= SQC_RESULT_OK)) {
		sqc_tls_msg_error(
			"TLS runtime library initialization failed.\n");
		goto bailout;
	}

	if (need_self_cert == true) {
		/*
		 * Load a private key
		 */
		ret = tls_load_prvkey(prvkey_file, &prvkey);
		if (unlikely(ret != SQC_RESULT_OK || prvkey == NULL)) {
			sqc_tls_msg_error(
				"Can't load a private key file \"%s\".\n",
				prvkey_file);
			goto bailout;
		}
	}

	/*
	 * Create a SSL_CTX
	 */
	tls_runtime_flush_error();
	if (role == TLS_ROLE_SERVER) {
		ssl_ctx = SSL_CTX_new(TLS_server_method());
	} else if (role == TLS_ROLE_CLIENT) {
		ssl_ctx = SSL_CTX_new(TLS_client_method());
	}
	if (likely(ssl_ctx != NULL)) {
		int osst;
		X509_VERIFY_PARAM *tmpvpm = NULL;

		/*
		 * Clear cert chain for our sanity.
		 */
		(void)SSL_CTX_clear_chain_certs(ssl_ctx);

		/*
		 * Inhibit other than TLSv1.3
		 */
		tls_runtime_flush_error();
		if (unlikely((osst = (int)SSL_CTX_set_min_proto_version(ssl_ctx,
					TLS1_3_VERSION)) != 1 ||
			     (osst = (int)SSL_CTX_set_max_proto_version(ssl_ctx,
					TLS1_3_VERSION)) != 1)) {
			sqc_tls_msg_error(
					"Failed to set an SSL_CTX "
					"only using TLSv1.3.\n");
			ret = SQC_RESULT_ANY_RUNTIME_ERROR;
			goto bailout;
		} else {
			if ((osst = (int)SSL_CTX_get_min_proto_version(ssl_ctx)) !=
				TLS1_3_VERSION ||
				(osst = (int)SSL_CTX_get_max_proto_version(
						ssl_ctx)) != TLS1_3_VERSION) {
				sqc_tls_msg_error(
					"Failed to check if the SSL_CTX "
					"only using TLSv1.3.\n");
				ret = SQC_RESULT_ANY_RUNTIME_ERROR;
				goto bailout;
			}
		}

#define VERIFY_DEPTH	50
		/*
		 * XXX FIXME:
		 *	50 is too much?
		 */
		if (role == TLS_ROLE_SERVER) {
			if (do_mutual_auth == true) {
				SSL_CTX_set_verify_depth(ssl_ctx,
					VERIFY_DEPTH);
#define SERVER_MUTUAL_VERIFY_FLAGS			     \
	(SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT | \
	 SSL_VERIFY_CLIENT_ONCE)
				SSL_CTX_set_verify(ssl_ctx,
					SERVER_MUTUAL_VERIFY_FLAGS,
					tls_verify_callback);
#undef SERVER_MUTUAL_VERIFY_FLAGS
			} else {
				SSL_CTX_set_verify(ssl_ctx,
					SSL_VERIFY_NONE, NULL);
			}
		} else {
			SSL_CTX_set_verify_depth(ssl_ctx,
				VERIFY_DEPTH);
#define CLIENT_VERIFY_FLAGS					\
	(SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT)
			SSL_CTX_set_verify(ssl_ctx, CLIENT_VERIFY_FLAGS,
				tls_verify_callback);
#undef CLIENT_VERIFY_FLAGS
			if (do_mutual_auth == true) {
				SSL_CTX_set_post_handshake_auth(ssl_ctx, 1);
			}
		}
#undef VERIFY_DEPTH

		/*
		 * Set ciphersuites
		 */
		if (IS_VALID_STRING(ciphersuites) == true) {
			tls_runtime_flush_error();
			if (unlikely(SSL_CTX_set_ciphersuites(ssl_ctx,
						ciphersuites) != 1)) {
				sqc_tls_msg_error(
					"Failed to set ciphersuites "
					"\"%s\" to the SSL_CTX.\n",
					ciphersuites);
				ret = SQC_RESULT_INVALID_CIPHER;
				goto bailout;
			}
		}

		/*
		 * Set CA path
		 */
		ret = tls_set_ca_path(ssl_ctx, ca_path, acceptable_ca_path,
				&trust_ca_list);
		if (likely(ret == SQC_RESULT_OK)) {
			if (IS_VALID_STRING(acceptable_ca_path) == true) {
				if (trust_ca_list != NULL &&
					sk_X509_NAME_num(trust_ca_list) > 0) {
					if (sk_X509_NAME_is_sorted(
						    trust_ca_list) != 1) {
						sk_X509_NAME_sort(
							trust_ca_list);
					}
				} else if (sk_X509_NAME_num(
						   trust_ca_list) <= 0) {
					sqc_tls_msg_warning(
						"No cert is collected "
						"in %s for peer chain "
						"verifiation.\n",
						acceptable_ca_path);
					if (trust_ca_list != NULL) {
						sk_X509_NAME_pop_free(
							trust_ca_list,
							X509_NAME_free);
					}
					trust_ca_list = NULL;
				}
			}
		} else {
			goto bailout;
		}

		if (need_self_cert == true) {
			/*
			 * Load a cert/cert chain into the SSL_CTX
			 */
			int n_certs = 0;
			ret = tls_load_cert_and_chain(
				ssl_ctx, cert_file, cert_chain_file, &n_certs);
			if (unlikely(ret != SQC_RESULT_OK)) {
				if (IS_VALID_STRING(cert_file) == true &&
					IS_VALID_STRING(cert_chain_file) ==
					true) {
					sqc_tls_msg_error(
						"Can't load both %s and %s: "
						"%s.\n",
						cert_file, cert_chain_file,
						sqc_error_get_string(ret));
				} else if (IS_VALID_STRING(cert_file) ==
						true) {
					sqc_tls_msg_error(
						"Can't load %s: %s.\n",
						cert_file,
						sqc_error_get_string(ret));
				} else if (IS_VALID_STRING(cert_chain_file) ==
						true) {
					sqc_tls_msg_error(
						"Can't load %s: %s.\n",
						cert_chain_file,
						sqc_error_get_string(ret));
				}
				goto bailout;
			} else if (unlikely(n_certs == 0)) {
				sqc_tls_msg_error(
					"No cert is load both %s and %s: %s.\n",
					cert_file, cert_chain_file,
					sqc_error_get_string(ret));
				goto bailout;
			}

			/*
			 * Set a private key into the SSL_CTX
			 */
			tls_runtime_flush_error();
			osst = SSL_CTX_use_PrivateKey(ssl_ctx, prvkey);
			if (unlikely(osst != 1)) {
				sqc_tls_msg_error(
					"Can't set a private key to a "
					"SSL_CTX.\n");
				/* ?? GFARM_ERRMSG_TLS_IBVALID_KEY ?? */
				ret = SQC_RESULT_ANY_RUNTIME_ERROR;
				goto bailout;
			}

			/*
			 * Then check prvkey in SSL_CTX by
			 * SSL_CTX_check_private_key
			 */
			tls_runtime_flush_error();
			osst = SSL_CTX_check_private_key(ssl_ctx);
			if (unlikely(osst != 1)) {
				sqc_tls_msg_error(
					"Wrong private key file for the "
					"current certificate.\n");
				ret = SQC_RESULT_ANY_RUNTIME_ERROR;
				goto bailout;
			}

			/* no domain check */
			do_build_chain = conf->build_chain_local;
			if (do_build_chain == true) {
				/*
				 * Build a complete cert chain locally.
				 */
				/*
				 * Don't set
				 * SSL_BUILD_CHAIN_FLAG_NO_ROOT or
				 * build chain always fail.
				 *
				 * Neither SSL_BUILD_CHAIN_FLAG_CHECK
				 * shouldn't be set since the
				 * SSL_CTX_build_cert_chain() seems
				 * try to use all the existing chain
				 * cert file under CA path for build
				 * ths local chain.
				 *
				 * Conlusion: only zero is suitable
				 * for our usage.
				 */
				tls_runtime_flush_error();
				osst = (int)SSL_CTX_build_cert_chain(ssl_ctx, 0);
				if (unlikely(osst != 1)) {
					sqc_tls_msg_error(
						"Can't build a certificate "
						"chain.\n");
					ret = SQC_RESULT_ANY_RUNTIME_ERROR;
					goto bailout;
				}
			}
		}

		/*
		 * Set revocation path
		 */
		/*
		 * NOTE: XXX FIXME:
		 *
		 *	Setup the revoke thingies AFTER building cert
		 *	chain since setting a revocation path not
		 *	containing any CLRs (e.g. cert chain path)
		 *	makes invalidates all the certs. It's too
		 *	annoying to check all the CLRs under
		 *	directories...
		 */
		if (IS_VALID_STRING(revoke_path) == true) {
			ret = tls_set_revoke_path(ssl_ctx,
				revoke_path);
			if (unlikely(ret != SQC_RESULT_OK)) {
				goto bailout;
			}
		}

		/*
		 * Final verify param tweaks
		 */
		tmpvpm = SSL_CTX_get0_param(ssl_ctx);

		if (likely(tmpvpm != NULL)) {
			unsigned long flags = 0;

			/*
			 * Seems revoked certs in
			 * tls_ca_certificate_path should be
			 * rejected. openssl s_{client|server} add
			 * following flags.
			 */
			flags |= (X509_V_FLAG_CRL_CHECK |
					X509_V_FLAG_CRL_CHECK_ALL);

			/*
			 * Allow RFC 3820 proxy cert authentication
			 */
			if (role == TLS_ROLE_SERVER &&
				do_mutual_auth == true &&
				use_proxy_cert == true) {
				flags |= X509_V_FLAG_ALLOW_PROXY_CERTS;
			}

			tls_runtime_flush_error();
			osst = X509_VERIFY_PARAM_set_flags(tmpvpm, flags);
			if (unlikely(osst != 1)) {
				sqc_tls_msg_error(
					"Failed to set CRL check, etc. flags "
					"to a X509_VERIFY_PARAM\n");
				ret = SQC_RESULT_ANY_RUNTIME_ERROR;
				goto bailout;
			}
		}

	} else {
		sqc_tls_msg_error(
			"Failed to create a SSL_CTX.\n");
		ret = SQC_RESULT_ANY_RUNTIME_ERROR;
		goto bailout;
	}

	/*
	 * Create a new tls_session_ctx_struct
	 */
	ctxret = (struct tls_session_ctx_struct *)malloc(
			sizeof(struct tls_session_ctx_struct));
	if (likely(ctxret != NULL)) {
		tls_runtime_flush_error();

		(void)memset(ctxret, 0,
			sizeof(struct tls_session_ctx_struct));
		tls_session_clear_ctx(ctxret, CTX_CLEAR_READY_FOR_ESTABLISH);

		ctxret->role_ = role;
		ctxret->do_mutual_auth_ = do_mutual_auth;
		if (conf->key_update == true) {
#ifndef TLS_TEST
#define TLS_KEY_UPDATE_THRESH	512 * 1024 * 1024;
			ctxret->io_key_update_thresh_ = TLS_KEY_UPDATE_THRESH;
#undef TLS_KEY_UPDATE_THRESH
#else
			ctxret->io_key_update_thresh_ =
				conf->key_update;
#endif /* ! TLS_TEST */
		} else {
			ctxret->io_key_update_thresh_ = 0;
		}
		ctxret->prvkey_ = prvkey;
		ctxret->ssl_ctx_ = ssl_ctx;
		/* no domain check */
		ctxret->do_build_chain_ = do_build_chain;
		ctxret->do_allow_no_crls_ = conf->allow_no_crl;
		ctxret->do_allow_proxy_cert_ = (role == TLS_ROLE_SERVER) ?
			use_proxy_cert : do_proxy_auth;
		ctxret->cert_file_ = cert_file;
		ctxret->cert_chain_file_ = cert_chain_file;
		ctxret->prvkey_file_ = prvkey_file;
		ctxret->ciphersuites_ = ciphersuites;
		ctxret->ca_path_ = ca_path;
		ctxret->acceptable_ca_path_ = acceptable_ca_path;
		ctxret->revoke_path_ = revoke_path;
		ctxret->trusted_certs_ = trust_ca_list;

		/*
		 * All done.
		 */
		*ctxptr = ctxret;
		ret = SQC_RESULT_OK;
		goto ok;
	} else {
		sqc_tls_msg_error(
			"Can't allocate a TLS session context.\n");
		ret = SQC_RESULT_NO_MEMORY;
	}

bailout:
	free(cert_file);
	free(cert_chain_file);
	free(prvkey_file);
	free(ciphersuites);
	free(ca_path);
	free(acceptable_ca_path);
	free(revoke_path);

	/*
	 * not forget to release trusted certs if it is used.
	 */

	if (prvkey != NULL) {
		EVP_PKEY_free(prvkey);
	}
	if (ssl_ctx != NULL) {
		(void)SSL_CTX_clear_chain_certs(ssl_ctx);
		SSL_CTX_free(ssl_ctx);
	}
	free(ctxret);

ok:
	free(tmp_proxy_cert_file);

	return (ret);

#undef str_or_NULL
}

/*
 * Destructor
 */
static inline void
tls_session_destroy_ctx(struct tls_session_ctx_struct *ctx)
{
	tls_session_clear_ctx(ctx, CTX_CLEAR_FREEUP);
}

/*
 * TLS I/O operations
 */

/*
 * SSL_ERROR_* handler
 */
static inline bool
tls_session_io_continuable(int sslerr, struct tls_session_ctx_struct *ctx,
	bool in_handshake, const char *diag)
{
	bool ret = false;

	/*
	 * NOTE:
	 *	This routine must be transparent among all the type of
	 *	BIO.  So whole the causable SSL_ERROR_* must be care
	 *	about.
	 */

	ctx->last_ssl_error_ = sslerr;

	switch (sslerr) {

	case SSL_ERROR_NONE:
	case SSL_ERROR_WANT_READ:
	case SSL_ERROR_WANT_ASYNC:
	case SSL_ERROR_WANT_ASYNC_JOB:
		/*
		 * just retry.
		 */
		ctx->last_sqc_error_ = SQC_RESULT_OK;
		ret = true;
		break;

	case SSL_ERROR_SYSCALL:
		/*
		 * fetch errno
		 */
		if (unlikely(errno == 0)) {
			/*
			 * NOTE:
			 *	This happend on OpenSSL version < 3.0.0
			 *	means "unexpected EOF from the peer."
			 */
			ctx->last_sqc_error_ = SQC_RESULT_EOF;
		} else {
			ctx->last_sqc_error_ = SQC_RESULT_POSIX_API_ERROR;
		}
		ctx->is_got_fatal_ssl_error_ = true;
		break;

	case SSL_ERROR_SSL:
		/*
		 * TLS runtime error
		 */
		ctx->last_sqc_error_ = SQC_RESULT_ANY_RUNTIME_ERROR;
		ctx->is_got_fatal_ssl_error_ = true;
		sqc_tls_msg_debug(1,
		    "TLS error during %s\n", diag);
		break;

	case SSL_ERROR_ZERO_RETURN:
		/*
		 * Peer sent close_notify. Not retryable.
		 */
		ctx->last_sqc_error_ = SQC_RESULT_TLS_CONN_ERROR;
		break;

	case SSL_ERROR_WANT_X509_LOOKUP:
	case SSL_ERROR_WANT_CLIENT_HELLO_CB:
	case SSL_ERROR_WANT_CONNECT:
	case SSL_ERROR_WANT_ACCEPT:
		if (likely(in_handshake == false)) {
			/*
			 * MUST not occured, connect/accept must be
			 * done BEFORE gfp_* thingies call this
			 * function.
			 */
			sqc_tls_msg_error(
				    "The TLS handshake must be done before "
				    "begining data I/O.\n");
			ctx->last_sqc_error_ = SQC_RESULT_ANY_FAILURES;
		} else {
			ctx->last_sqc_error_ = SQC_RESULT_OK;
			ret = true;
		}
		break;

	default:
		sqc_tls_msg_error(
			"All the TLS I/O error must be handled, but got "
			"TLS I/O error %d.\n", sslerr);
		ctx->last_sqc_error_ = SQC_RESULT_ANY_FAILURES;
		break;
	}

	return (ret);
}

/*
 * TLS session read/write timeout checker
 */
static inline sqc_result_t
tls_session_wait_io(struct tls_session_ctx_struct *ctx,
	int fd, int tous, bool to_read)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
	const char *method = NULL;

	sqc_tls_msg_debug(1, "%s(): wait enter.\n",
		__func__);

	if (to_read && SSL_has_pending(ctx->ssl_)) {
		method = "SSL_has_pending";
		ret = SQC_RESULT_OK;
	} else {
		int st;
		bool loop = true;

#ifdef HAVE_POLL
		struct pollfd fds[1];
		int tos_save = (tous >= 0) ? tous / 1000 : -1;
		int tos;

		method = "poll";

		while (loop == true) {
			fds[0].fd = fd;
			fds[0].events = to_read ? POLLIN : POLLOUT;
			tos = tos_save;

			st = poll(fds, 1, tos);
#else
		fd_set fds;
		struct timeval tv;
		struct timeval tv_save;
		struct timeval *tvp = NULL;
		if (tous >= 0) {
			tv_save.tv_usec = tous % (1000 * 1000);
			tv_save.tv_sec = tous / (1000 * 1000);
		}

		method = "select";

		while (loop == true) {
			FD_ZERO(&fds);
			FD_SET(fd, &fds);
			tv = tv_save;
			tvp = &tv;

			if (to_read)
				st = select(fd + 1, &fds, NULL, NULL, tvp);
			else
				st = select(fd + 1, NULL, &fds, NULL, tvp);
#endif /* HAVE_POLL */

			switch (st) {
			case 0:
				ret = SQC_RESULT_TIMEDOUT;
				loop = false;
				break;

			case -1:
				if (errno != EINTR) {
					ret = SQC_RESULT_POSIX_API_ERROR;
					loop = false;
				}
				break;

			default:
				ret = SQC_RESULT_OK;
				loop = false;
				break;
			}
		}
	}

	ctx->last_sqc_error_ = ret;

	sqc_tls_msg_debug(1, "%s(): wait (%s) end : %s\n",
		__func__, method, sqc_error_get_string(ret));

	return (ret);
}

static inline sqc_result_t
tls_session_wait_readable(struct tls_session_ctx_struct *ctx, int fd, int tous)
{
	return (tls_session_wait_io(ctx, fd, tous, true));
}

static inline sqc_result_t
tls_session_wait_writable(struct tls_session_ctx_struct *ctx, int fd, int tous)

{
	return (tls_session_wait_io(ctx, fd, tous, false));
}

static inline sqc_result_t
tls_session_get_pending_read_bytes_n(struct tls_session_ctx_struct *ctx,
	int *nptr)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
	SSL *ssl = NULL;

	if (likely(ctx != NULL && (ssl = ctx->ssl_) != NULL && nptr != NULL)) {
		*nptr = SSL_pending(ssl);
		ret = SQC_RESULT_OK;
	} else {
		if (nptr != NULL) {
			*nptr = 0;
		}
		ret = SQC_RESULT_INVALID_ARGS;
	}

	return (ret);
}

/*
 * Session establish
 */
static inline sqc_result_t
tls_session_verify(struct tls_session_ctx_struct *ctx, bool *is_verified)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
	SSL *ssl = NULL;
	X509 *p = NULL;
	X509_NAME *pn = NULL;

	if (is_verified != NULL) {
		*is_verified = false;
	}

	if (likely(ctx != NULL && (ssl = ctx->ssl_) != NULL &&
		(ctx->role_ == TLS_ROLE_CLIENT ||
		ctx->do_mutual_auth_ == true))) {
		/*
		 * No matter verified or not, get a peer cert.
		 */
		ctx->peer_dn_oneline_ = NULL;
		ctx->peer_dn_rfc2253_ = NULL;
		ctx->peer_dn_gsi_ = NULL;
		ctx->peer_cn_ = NULL;
		tls_runtime_flush_error();

		if (likely(((ctx->do_allow_proxy_cert_ == true &&
			ctx->is_got_proxy_cert_ == true &&
			(pn = ctx->proxy_issuer_) != NULL)) ||
			(((p = SSL_get_peer_certificate(ssl)) != NULL) &&
			((pn = X509_get_subject_name(p)) != NULL)))) {
			char *dn_oneline = NULL;
			char *dn_rfc2253 = NULL;
			char *dn_gsi = NULL;
			char *cn = NULL;
			bool v = false;
			int vres = -INT_MAX;

			X509_free(p);
			p = NULL;
#define DN_FORMAT_ONELINE	(XN_FLAG_ONELINE & ~ASN1_STRFLGS_ESC_MSB)
#define DN_FORMAT_RFC2253	(XN_FLAG_RFC2253 & ~ASN1_STRFLGS_ESC_MSB)
			if (likely((ret = get_peer_dn(pn,
						DN_FORMAT_ONELINE,
						&dn_oneline, 0)) ==
				SQC_RESULT_OK)) {
				ctx->peer_dn_oneline_ = dn_oneline;
			}
			if (likely((ret = get_peer_dn(pn,
						DN_FORMAT_RFC2253,
						&dn_rfc2253, 0)) ==
				SQC_RESULT_OK)) {
				ctx->peer_dn_rfc2253_ = dn_rfc2253;
			}
			if (likely((ret = get_peer_dn_gsi_ish(pn,
						&dn_gsi, 0)) ==
				SQC_RESULT_OK)) {
				ctx->peer_dn_gsi_ = dn_gsi;
			}
			if (likely((ret = get_peer_cn(pn, &cn, 0,
						ctx->do_allow_proxy_cert_))
				   == SQC_RESULT_OK)) {
				ctx->peer_cn_ = cn;
			}
#undef DN_FORMAT_ONELINE
#undef DN_FORMAT_RFC2253
			if (unlikely(ret != SQC_RESULT_OK)) {
				goto done;
			}

			tls_runtime_flush_error();
			vres = (int)SSL_get_verify_result(ssl);
			if (vres == X509_V_OK) {
				v = true;
			} else {
				v = false;
				sqc_tls_msg_notice(
					"Certificate verification failed: %s\n",
					X509_verify_cert_error_string(vres));
				ret = ctx->last_sqc_error_ =
					SQC_RESULT_CERTIFICATE_VERIFY_FAILURE;
			}
			ctx->cert_verify_result_error_ = vres;
			ctx->is_verified_ = v;
		} else {
			ret = SQC_RESULT_ANY_RUNTIME_ERROR;
			sqc_tls_msg_notice(
				"Failed to acquire peer certificate.\n");
		}
	} else {
		if (ctx == NULL || ssl == NULL) {
			ret = SQC_RESULT_INVALID_ARGS;
		} else {
			/* not mutual auth */
			ctx->peer_dn_oneline_ = NULL;
			ctx->peer_dn_rfc2253_ = NULL;
			ctx->peer_dn_gsi_ = NULL;
			ctx->peer_cn_ = NULL;
			ctx->cert_verify_result_error_ = X509_V_OK;
			ctx->is_verified_ = true;
			ret = SQC_RESULT_OK;
		}
	}

done:
	if (ctx != NULL) {
		ctx->last_sqc_error_ = ret;
		if (is_verified != NULL) {
			*is_verified = ctx->is_verified_;
		}
	}

	return (ret);
}

static inline sqc_result_t
tls_session_establish(struct tls_session_ctx_struct *ctx, int fd)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
	struct sockaddr sa;
	socklen_t salen = sizeof(sa);
	int pst = -1;
	typedef int (*tls_handshake_proc_t)(SSL *ssl);
	tls_handshake_proc_t p = NULL;
	SSL *ssl = NULL;

	errno = 0;
	if (likely(fd >= 0 &&
		(pst = getpeername(fd, &sa, &salen)) == 0 &&
		ctx != NULL)) {

		/*
		 * Create an SSL
		 */
		ret = tls_session_setup_ssl(ctx);
		if (unlikely(ret != SQC_RESULT_OK || ctx->ssl_ == NULL)) {
			goto bailout;
		}
		ssl = ctx->ssl_;

		tls_runtime_flush_error();
		if (likely(SSL_set_fd(ssl, fd) == 1)) {
			int st;
			int ssl_err;
			bool do_cont = false;

			ctx->is_handshake_tried_ = true;
			p = (ctx->role_ == TLS_ROLE_SERVER) ?
				SSL_accept : SSL_connect;

retry:
			errno = 0;
			tls_runtime_flush_error();
			st = p(ssl);
			ssl_err = SSL_get_error(ssl, st);
			do_cont = tls_session_io_continuable(
					ssl_err, ctx, false, "SSL handshake");
			if (likely(st == 1 && ssl_err == SSL_ERROR_NONE)) {
				ret = ctx->last_sqc_error_ =
					SQC_RESULT_OK;
			} else if (st == 0 && do_cont == true) {
				goto retry;
			} else {
				/*
				 * st < 0 but SSL_ERROR_NONE ???
				 */
				if (ctx->last_sqc_error_ ==
					SQC_RESULT_OK) {
					ret = ctx->last_sqc_error_ =
						SQC_RESULT_ANY_RUNTIME_ERROR;
				} else {
					ret = ctx->last_sqc_error_;
				}
				sqc_tls_msg_notice(
					"SSL handshake failed: %s\n",
					sqc_error_get_string(ret));
			}
		} else {
			sqc_tls_msg_error(
				"Failed to set a file "
				"descriptor %d to an SSL.\n", fd);
			ret = SQC_RESULT_ANY_RUNTIME_ERROR;
		}
	} else {
		if (pst != 0 && errno != 0) {
			ret = SQC_RESULT_POSIX_API_ERROR;
			if (errno == ENOTCONN) {
				sqc_tls_msg_notice(
					"The file descriptor %d is not yet "
					"connected: %s\n",
					fd, sqc_error_get_string(ret));
			} else if (errno == ENOTSOCK) {
				sqc_tls_msg_error(
					"The file descriptor %d is not a "
					"socket: %s\n",
					fd, sqc_error_get_string(ret));
			} else {
				sqc_tls_msg_notice(
					"Failed to check connection status of "
					"the file descriptor %d: %s\n",
					fd, sqc_error_get_string(ret));
			}
		} else {
			ret = SQC_RESULT_INVALID_ARGS;
			sqc_tls_msg_error(
				"The tls context is not initialized.\n");
		}
	}

	if (ret == SQC_RESULT_OK && ctx != NULL) {
		bool is_verified = false;
		ret = tls_session_verify(ctx, &is_verified);
		if (is_verified == false) {
			ret = ctx->last_sqc_error_ =
				SQC_RESULT_AUTHENTICATION_ERROR;
			if (IS_VALID_STRING(ctx->peer_dn_oneline_) == true) {
				sqc_tls_msg_notice(
					"Authentication failed between peer: "
					"'%s' with %s.\n",
					ctx->peer_cn_,
					(ctx->is_got_proxy_cert_ == true) ?
						"proxy certificate" :
						"end-entity certificate");
			} else {
				sqc_tls_msg_notice(
					"Authentication failed "
					"(no cert acquired.)\n");
			}
		}

		if (IS_VALID_STRING(ctx->peer_dn_oneline_) == true) {
			sqc_tls_msg_debug(1,
				"Authentication between \"%s\" %s and a "
				"TLS session %s with %s.\n",
				ctx->peer_dn_gsi_,
				(is_verified == true) ?
					"verified" : "not verified",
				(is_verified == true) ?
					"established" : "not established",
				(ctx->is_got_proxy_cert_ == true) ?
					"proxy certificate" :
					"end-entity certificate");
			sqc_tls_msg_debug(1,
				"peer CN \"%s\"\n", tls_session_peer_cn(ctx));
		}
	}

bailout:
	return (ret);
}

/*
 * TLS 1.3 key update
 */
static inline sqc_result_t
tls_session_update_key(struct tls_session_ctx_struct *ctx, int delta)
{
	/*
	 * Only clients initiate KeyUpdate.
	 */
	sqc_result_t ret = SQC_RESULT_OK;
	SSL *ssl;

	if (likely(ctx != NULL && (ssl = ctx->ssl_) != NULL &&
		ctx->role_ == TLS_ROLE_CLIENT &&
		ctx->io_key_update_thresh_ > 0 &&
		ctx->is_got_fatal_ssl_error_ == false &&
		((ctx->io_key_update_accum_ += (size_t)delta) >=
		(size_t)ctx->io_key_update_thresh_))) {
		if (likely(SSL_key_update(ssl,
				SSL_KEY_UPDATE_REQUESTED) == 1)) {
			ret = ctx->last_sqc_error_ = SQC_RESULT_OK;
			sqc_tls_msg_debug(1,
				"TLS shared key updated after "
				" %zu bytes I/O.\n",
				ctx->io_key_update_accum_);
		} else {
			/*
			 * XXX FIXME:
			 *	OpenSSL 1.1.1 manual doesn't refer
			 *	what to do when SSL_key_update()
			 *	failure.
			 */
			sqc_tls_msg_warning(
				"SSL_update_key() failed but we don't know "
				"how to deal with it.\n");
			ret = ctx->last_sqc_error_ =
				SQC_RESULT_ANY_FAILURES;
		}
		ctx->io_key_update_accum_ = 0;
	} else {
		ret = ctx->last_sqc_error_;
	}

	return (ret);
}

/*
 * TLS session read(2)'ish
 */
static inline sqc_result_t
tls_session_read(struct tls_session_ctx_struct *ctx, void *buf, int len,
	int *actual_io_bytes)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
	SSL *ssl = NULL;

	if (likely(ctx != NULL && (ssl = ctx->ssl_) != NULL && buf != NULL &&
			len > 0 && actual_io_bytes != NULL &&
			ctx->is_verified_ == true &&
			ctx->is_got_fatal_ssl_error_ == false)) {
		int n = 0;
		int ssl_err;
		bool continuable;

		sqc_tls_msg_debug(1,
			"%s(%s): about to read %d (remains %d)\n",
			__func__, ctx->peer_cn_, len,
			SSL_pending(ssl));

		if (unlikely(len == 0)) {
			ret = ctx->last_sqc_error_ = SQC_RESULT_OK;
			goto done;
		}

		*actual_io_bytes = 0;

retry:
		sqc_tls_msg_debug(1,
			"%s(%s): read %d/%d\n", __func__,
			ctx->peer_cn_, n, len);

		errno = 0;
		n = SSL_read(ssl, buf, len);
		/*
		 * NOTE:
		 *	To avoid sending key update request on broken
		 *	TLS stream, check SSL_ERROR_ for the session
		 *	continuity.
		 */
		ssl_err = SSL_get_error(ssl, n);
		continuable = tls_session_io_continuable(
			ssl_err, ctx, false, "SSL_read");
		if (likely(n > 0 && ssl_err == SSL_ERROR_NONE)) {
			ctx->last_sqc_error_ = SQC_RESULT_OK;
			ctx->last_ssl_error_ = ssl_err;
			*actual_io_bytes = n;
			ctx->io_total_ += (unsigned long)n;
			ret = tls_session_update_key(ctx, n);
		} else {
			if (likely(continuable == true)) {
				goto retry;
			} else {
				ret = ctx->last_sqc_error_;
			}
		}

		sqc_tls_msg_debug(1,
			"%s(%s): read done %d (remains %d) : %s\n",
			__func__, ctx->peer_cn_, n, SSL_pending(ssl),
			sqc_error_get_string(ret));

	} else {
		ret = ctx->last_sqc_error_ = SQC_RESULT_EOF;
	}

done:
	return (ret);
}

/*
 * TLS session write(2)'ish
 */
static inline sqc_result_t
tls_session_write(struct tls_session_ctx_struct *ctx, const void *buf, int len,
	int *actual_io_bytes)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
	SSL *ssl = NULL;

	if (likely(ctx != NULL && (ssl = ctx->ssl_) != NULL && buf != NULL &&
			len > 0 && actual_io_bytes != NULL &&
			ctx->is_verified_ == true &&
			ctx->is_got_fatal_ssl_error_ == false)) {
		int n = 0;
		int ssl_err;
		bool continuable;

		sqc_tls_msg_debug(1,
			"%s(%s): about to write %d\n", __func__,
			ctx->peer_cn_, len);

		if (unlikely(len == 0)) {
			ret = ctx->last_sqc_error_ = SQC_RESULT_OK;
			goto done;
		}

		*actual_io_bytes = 0;

retry:
		sqc_tls_msg_debug(1,
			"%s(%s): write %d/%d\n", __func__,
			ctx->peer_cn_, n, len);

		errno = 0;
		n = SSL_write(ssl, buf, len);
		/*
		 * NOTE:
		 *	To avoid sending key update request on broken
		 *	TLS stream, check SSL_ERROR_ for the session
		 *	continuity.
		 */
		ssl_err = SSL_get_error(ssl, n);
		continuable = tls_session_io_continuable(
			ssl_err, ctx, false, "SSL_write");
		if (likely(n > 0 && ssl_err == SSL_ERROR_NONE)) {
			ctx->last_sqc_error_ = SQC_RESULT_OK;
			ctx->last_ssl_error_ = ssl_err;
			*actual_io_bytes = n;
			ctx->io_total_ += (unsigned long)n;
			ret = tls_session_update_key(ctx, n);
		} else {
			if (likely(continuable == true)) {
				goto retry;
			} else {
				ret = ctx->last_sqc_error_;
			}
		}

		sqc_tls_msg_debug(1,
			"%s(%s): write done %d : %s\n", __func__,
			ctx->peer_cn_, n, sqc_error_get_string(ret));

	} else {
		ret = ctx->last_sqc_error_ = SQC_RESULT_EOF;
	}

done:
	return (ret);
}

/*
 * tls session io with timeout (includes "forever")
 */
static inline sqc_result_t
tls_session_timeout_read(struct tls_session_ctx_struct *ctx,
	int fd, void *buf, int len, int timeout, int *actual_read)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

	ret = tls_session_wait_readable(ctx, fd, timeout);
	if (likely(ret == SQC_RESULT_OK)) {
		ret = tls_session_read(ctx, buf, len, actual_read);
	}

	return (ret);
}

static inline sqc_result_t
tls_session_timeout_write(struct tls_session_ctx_struct *ctx,
	int fd, const void *buf, int len, int timeout, int *actual_io_bytes)
{
	sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

	ret = tls_session_wait_writable(ctx, fd, timeout);
	if (likely(ret == SQC_RESULT_OK)) {
		ret = tls_session_write(ctx, buf, len, actual_io_bytes);
	}

	return (ret);
}

/*
 * TLS session shutdown
 */
static inline sqc_result_t
tls_session_shutdown(struct tls_session_ctx_struct *ctx)
{
	sqc_result_t ret;
	SSL *ssl;

	if (unlikely(ctx == NULL))
		return (SQC_RESULT_OK);

	if (unlikely((ssl = ctx->ssl_) == NULL)) {
		ctx->last_sqc_error_ = SQC_RESULT_ANY_FAILURES;
		return (SQC_RESULT_ANY_FAILURES);
	}

	sqc_tls_msg_debug(1,
		"%s(%s): about to shutdown SSL.\n",
		__func__, ctx->peer_cn_);

	if (!ctx->is_handshake_tried_) {
		ret = SQC_RESULT_OK;
	} else if (ctx->is_got_fatal_ssl_error_) {
		/* ctx->last_ssl_error_ is already set, do not override */
		ret = SQC_RESULT_OK;
	} else {
#if 1 /* do not call SSL_shutdown() to avoid protocol interaction here */
		ret = SQC_RESULT_OK;
#else
		int st = SSL_shutdown(ssl);

		sqc_tls_msg_debug(1,
			"%s(%s): shutdown SSL issued : %s\n",
			__func__, ctx->peer_cn_,
			(st == 1) ? "OK" : "NG");

		if (st == 1) {
			ctx->last_ssl_error_ = SSL_ERROR_SSL;
			ctx->is_got_fatal_ssl_error_ = true;
			ret = SQC_RESULT_OK;
		} else if (st == 0) {
			/*
			 * SSL Bi-diectional shutdown, by calling
			 * SSL_read and waiting for
			 * SSL_ERROR_ZERO_RETURN or SSL_ERROR_NONE
			 * (SSL_read returns >0)
			 */
			uint8_t buf[65536];
			int s_n = -1;

			ret = tls_session_read(ctx, buf, sizeof(buf), &s_n);

			sqc_tls_msg_debug(1,
				"%s(%s): shutdown SSL replies read "
				"%d : %s\n", __func__, ctx->peer_cn_,
				s_n, sqc_error_get_string(ret));

			if ((ret == SQC_RESULT_OK && s_n > 0) ||
				(ret == SQC_RESULT_TLS_CONN_ERROR)) {
				ctx->last_ssl_error_ = SSL_ERROR_SSL;
				ctx->is_got_fatal_ssl_error_ = true;
				ret = SQC_RESULT_OK;
			}
		} else {
			ret = SQC_RESULT_ANY_FAILURES;
		}
		ctx->is_got_fatal_ssl_error_ = true;
		ctx->is_verified_ = false;
		ctx->io_key_update_accum_ = 0;
		ctx->io_total_ = 0;
#endif /* do not call SSL_shutdown() */
	}

	ctx->last_sqc_error_ = ret;

	sqc_tls_msg_debug(1,
		"%s(%s): shutdown SSL done : %s\n",
		__func__, ctx->peer_cn_,
		sqc_error_get_string(ret));

	return (ret);
}

/*
 * DN, CN
 */
static inline char *
tls_session_peer_subjectdn_oneline(struct tls_session_ctx_struct *ctx)
{
	if (likely(ctx != NULL)) {
		return (ctx->peer_dn_oneline_);
	} else {
		return (NULL);
	}
}

static inline char *
tls_session_peer_subjectdn_rfc2253(struct tls_session_ctx_struct *ctx)
{
	if (likely(ctx != NULL)) {
		return (ctx->peer_dn_rfc2253_);
	} else {
		return (NULL);
	}
}

static inline char *
tls_session_peer_subjectdn_gsi(struct tls_session_ctx_struct *ctx)
{
	if (likely(ctx != NULL)) {
		return (ctx->peer_dn_gsi_);
	} else {
		return (NULL);
	}
}

static inline char *
tls_session_peer_cn(struct tls_session_ctx_struct *ctx)
{
	if (likely(ctx != NULL)) {
		return (ctx->peer_cn_);
	} else {
		return (NULL);
	}
}
