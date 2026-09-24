#include "sqc_apis.h"
#include "rpc_file_util.h"
#include "sqc_rpc_client.h"
#include "sqc_rpc_sched_conv_enums.h"


const char *program_name = "sqc_rpc_client";
const char *default_conf_dir = "~/.sqc-scheduler";
const char *default_remark = "";

//
// Read text from a file.
//
sqc_result_t
read_text_file(const char* filename, size_t max_len, char **data) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  sqc_result_t expand_result = SQC_RESULT_ANY_FAILURES;
  int fd = -1;
  ssize_t read_result = -1;
  char *tmp_data = NULL;
  char *exp_filename = NULL;

  if (likely(filename != NULL && data != NULL)) {
    expand_result = rpc_expand_path(filename, &exp_filename);
    if (likely(expand_result == SQC_RESULT_OK)) {
      tmp_data = malloc(max_len + 1);

      if (likely(tmp_data != NULL)) {
        errno = 0;
        fd = open(exp_filename, O_RDONLY);

        if (likely(fd >= 0)) {
          for (;;) {
            read_result = -1;
            errno = 0;
            read_result = read(fd, tmp_data, max_len + 1);

            if (likely(read_result >= 0)) {
              if (likely((size_t) read_result <= max_len)) {
                ret = SQC_RESULT_OK;
                *(tmp_data + read_result) = '\0';
                *data = tmp_data;
                sqc_msg_debug(5, "Read the file: %s\n", filename);
              } else {
                ret = SQC_RESULT_TOO_LARGE;
                sqc_msg_error("The file exceeds the size limit (> %zu): %s\n", max_len, filename);
              }
            } else if (errno == EINTR) {
              continue;
            } else {
              ret = SQC_RESULT_POSIX_API_ERROR;
              sqc_msg_error("Failed to read the file, %s: %s\n", strerror(errno), filename);
            }
            break;
          }
        } else {
          ret = SQC_RESULT_POSIX_API_ERROR;
          sqc_msg_error("Failed to open the file, %s: %s\n", strerror(errno), filename);
        }
        close(fd);

      } else {
        ret = SQC_RESULT_NO_MEMORY;
        sqc_msg_error("%s\n", sqc_error_get_string(ret));
      }
    } else {
      ret = expand_result;
      sqc_msg_error("%s\n", sqc_error_get_string(ret));
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  if (unlikely(ret != SQC_RESULT_OK)) {
    free(tmp_data);
  }

  return ret;
}


//
// Parse an unsigned integer.
//
bool
parse_uint(const char *arg, unsigned long long max_value, unsigned long long *value) {
  bool ret = false;
  unsigned long long ull = 0u;
  char *endp = NULL;

  if (likely(arg != NULL && value != NULL)) {
    if (likely(isdigit(*arg))) {
      errno = 0;
      ull = strtoull(arg, &endp, 10);
      if (likely(endp != NULL && *endp == '\0' && errno == 0 && ull <= max_value)) {
        *value = ull;
        ret = true;
      }
    }
  }

  return ret;
}


//
// Parse a uint8_t integer.
//
bool
parse_uint8(const char *arg, uint8_t *value) {
  bool ret = false;
  unsigned long long ull = 0ull;

  if (likely(arg != NULL && value != NULL)) {
    if (likely(parse_uint(arg, UINT8_MAX, &ull) == true)) {
      *value = (uint8_t) ull;
      ret = true;
    }
  }

  return ret;
}


//
// Parse a uint32_t integer.
//
bool
parse_uint32(const char *arg, uint32_t *value) {
  bool ret = false;
  unsigned long long ull = 0ull;

  if (likely(arg != NULL && value != NULL)) {
    if (likely(parse_uint(arg, UINT32_MAX, &ull) == true)) {
      *value = (uint32_t) ull;
      ret = true;
    }
  }

  return ret;
}


//
// Parse a uint64_t integer.
//
bool
parse_uint64(const char *arg, uint64_t *value) {
  bool ret = false;
  unsigned long long ull = 0ull;

  if (likely(arg != NULL && value != NULL)) {
    if (likely(parse_uint(arg, UINT64_MAX, &ull) == true)) {
      *value = (uint64_t) ull;
      ret = true;
    }
  }

  return ret;
}


//
// Parse a size_t integer.
//
bool
parse_size_t(const char *arg, size_t *value) {
  bool ret = false;
  unsigned long long ull = 0ull;

  if (likely(arg != NULL && value != NULL)) {
    if (likely(parse_uint(arg, SIZE_MAX, &ull) == true)) {
      *value = (size_t) ull;
      ret = true;
    }
  }

  return ret;
}



//
// Parse an option with value.
// If the given 'opt' is '--remark=xxx', 'opt_value' is set to 'xxx'.
//
bool
parse_option_with_value(const char *arg, const char *opt, const char **opt_value) {
  bool ret = false;
  size_t opt_len = 0u;

  if (likely(arg != NULL && opt != NULL && opt_value != NULL)) {
    opt_len = strlen(opt);
    if (strncmp(arg, opt, opt_len) == 0 && *(arg + opt_len) == '=') {
      *opt_value = arg + opt_len + 1;
      ret = true;
    }
  }

  return ret;
}


//
// Print common options for the help message.
//
void
print_common_options(void) {
    printf("  --conf-dir=DIR          use configuration files in DIR\n");
    printf("                          (default: %s)\n", default_conf_dir);
    printf("  --server=HOST:PORT      RPC server (default: env $SQC_RPC_SERVER)\n");
    printf("  --prefer-ipv4           prefer IPv4 to IPv6\n");
    printf("  --auth=METHOD           auth method (mutual-tls, munge or jwt)\n");
}


//
// Parse command line options that all sub commands recognize.
//
bool
parse_common_option(const char *arg, const char **server, bool *prefer_ipv4,
                    rpc_auth_method_t *auth_method, const char **conf_dir) {
  bool ret = false;
  const char *opt_value = NULL;

  if (likely(arg != NULL && server != NULL && prefer_ipv4 != NULL && auth_method != NULL &&
             conf_dir != NULL)) {
    if (parse_option_with_value(arg, "--server", server) == true) {
      ret = true;
    } else if (strcmp(arg, "--prefer-ipv4") == 0) {
      *prefer_ipv4 = true;
      ret = true;
    } else if (parse_option_with_value(arg, "--auth", &opt_value) == true) {
      rpc_auth_method_t tmp_auth_method = rpc_auth_method_value(opt_value);
      if (likely(tmp_auth_method != RPC_AUTH_METHOD_UNKNOWN)) {
        *auth_method = tmp_auth_method;
        ret = true;
      } else {
        ret = false;
      }
    } else if (parse_option_with_value(arg, "--conf-dir", conf_dir) == true) {
      ret = true;
    }
  }

  return ret;
}

//
// Print common part of RPC execution results.
//
void
print_rpc_common_result(sqc_result_t ret, sqc_result_t code, const char *msg) {
  printf("result: %s\n", (ret == SQC_RESULT_OK) ? "success" : "failed");
  printf("code: %lld\n", (long long) code);
  printf("message: %s\n", (msg != NULL) ? msg : "");
}
