#include "sqc_apis.h"
#include "rpc_file_util.h"

//
// Size of buffer to store a passwd entry for getpwnam_r() and getpwuid_r().
//
#define GETPW_R_BUF_SIZE 8192


static inline sqc_result_t
s_expand_path(const char* path, char **out_path) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  char *tmp_path = NULL;

  if (likely(IS_VALID_STRING(path) && out_path != NULL)) {
    if (*path == '~') {
      char username[LOGIN_NAME_MAX + 1];
      bool username_terminated = false;
      int i = 0;
      const char *fp = path + 1;
      char *up = username;
      while (likely(i < LOGIN_NAME_MAX)) {
        if (likely(*fp != '/' && *fp != '\0')) {
          *up++ = *fp++;
        } else {
          *up = '\0';
          username_terminated = true;
          break;
        }
        i++;
      }
      if (likely(username_terminated == true)) {
        struct passwd pw;
        struct passwd *pw_result = NULL;
        char buf[GETPW_R_BUF_SIZE];
        int tmp_err = 0;

        errno = 0;
        if (*username == '\0') {
          tmp_err = getpwuid_r(geteuid(), &pw, buf, sizeof(buf), &pw_result);
        } else {
          tmp_err = getpwnam_r(username, &pw, buf, sizeof(buf), &pw_result);
        }
        if (likely(pw_result != NULL)) {
          size_t tmp_size = strlen(pw.pw_dir) + strlen(path);
          tmp_path = malloc(tmp_size);
          if (likely(tmp_path != NULL)) {
            ret = SQC_RESULT_OK;
            (void) snprintf(tmp_path, tmp_size, "%s%s", pw.pw_dir, fp);
            *out_path = tmp_path;
          } else {
            ret = SQC_RESULT_NO_MEMORY;
            sqc_msg_debug(5, "%s\n", sqc_error_get_string(ret));
          }
        } else if (tmp_err == 0) {
          ret = SQC_RESULT_INVALID_ARGS;
          sqc_msg_debug(5, "Invalid path, no such user: '%s'\n", path);
        } else {
          ret = SQC_RESULT_POSIX_API_ERROR;
          sqc_msg_debug(5, "Failed to get the password entry: %s\n", strerror(errno));
        }
      } else {
        ret = SQC_RESULT_INVALID_ARGS;
        sqc_msg_debug(5, "Too long user name: '%s'\n", path);
      }
    } else {
      tmp_path = strdup(path);
      if (likely(tmp_path != NULL)) {
        ret = SQC_RESULT_OK;
        *out_path = tmp_path;
      } else {
        ret = SQC_RESULT_NO_MEMORY;
        sqc_msg_debug(5, "%s\n", sqc_error_get_string(ret));
      }
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  if (likely(ret == SQC_RESULT_OK)) {
    sqc_msg_debug(5, "Expanded a path: '%s' -> '%s'\n", path, *out_path);
  } else {
    free(tmp_path);
  }

  return ret;
}


//
// Loaded text from a file.
//
static inline sqc_result_t
s_read_text_file(const char *file, char **text, size_t *textlen) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  int fd = -1;
  char *tmp_text = NULL;
  off_t file_size = (off_t)-1;

  if (likely(IS_VALID_STRING(file) && text != NULL)) {
    //
    // Open the key and get tis current size.
    //
    errno = 0;
    fd = open(file, O_RDONLY);

    if (likely(fd >= 0)) {
      sqc_msg_debug(5, "Opened the file: %s\n", file);
      errno = 0;
      file_size = lseek(fd, 0, SEEK_END);
      if (unlikely(file_size == (off_t)-1)) {
        ret = SQC_RESULT_POSIX_API_ERROR;
        sqc_msg_debug(5, "Failed to seek the file, %s\n",
                        strerror(errno));
      } else if (unlikely(file_size == 0)) {
        ret = SQC_RESULT_PUBLIC_KEY_READ_FAILURE;
        sqc_msg_debug(5, "The file is empty\n");
      } else {
        sqc_msg_debug(5, "Got size of the file: %lu\n", (unsigned long)file_size);
        errno = 0;
        if (likely(lseek(fd, 0, SEEK_SET) != (off_t)-1)) {
          tmp_text = (char *)malloc((size_t)(file_size + 1u));
          if (likely(tmp_text != NULL)) {
            ret = SQC_RESULT_OK;
          } else {
            ret = SQC_RESULT_NO_MEMORY;
            sqc_msg_debug(5, "%s\n", sqc_error_get_string(ret));
          }
        } else {
          ret = SQC_RESULT_POSIX_API_ERROR;
          sqc_msg_debug(5, "Failed to seek the file, %s\n", strerror(errno));
        }
      }
    } else {
      ret = SQC_RESULT_POSIX_API_ERROR;
      sqc_msg_debug(5, "Failed to open the file, %s\n", strerror(errno));
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
    sqc_msg_debug(5, "%s\n", sqc_error_get_string(ret));
  }

  //
  // Read the file.
  //
  if (likely(fd >= 0 && tmp_text != NULL)) {
    char *tmp_textp = tmp_text;
    size_t len = (size_t)file_size;
    for (;;) {
      ssize_t read_result = -1;
      errno = 0;
      read_result = read(fd, tmp_textp, len);
      if (likely(read_result > 0)) {
        len -= (size_t)read_result;
        tmp_textp += read_result;
        sqc_msg_debug(5, "Read the file (%lu bytes)\n", (unsigned long)read_result);
        if (len == 0) {
          *tmp_textp = '\0';
          free(*text);
          *text = tmp_text;
          if (textlen != NULL) {
            *textlen = (size_t)(tmp_textp - tmp_text) + 1;
          }
          ret = SQC_RESULT_OK;
          sqc_msg_info("Loaded the file: %s\n", file);
          break;
        }
      } else if (read_result == 0) {
        ret = SQC_RESULT_PUBLIC_KEY_READ_FAILURE;
        sqc_msg_debug(5, "Read unexpected EOF of the file\n");
        break;
      } else if (errno == EINTR) {
        continue;
      } else {
        ret = SQC_RESULT_POSIX_API_ERROR;
        sqc_msg_debug(5, "Failed to read the file, %s\n", strerror(errno));
        break;
      }
    }
  }

  //
  // Clean up.
  //
  if (unlikely(ret != SQC_RESULT_OK)) {
    free(tmp_text);
  }
  if (likely(fd >= 0)) {
    (void)close(fd);
    sqc_msg_debug(5, "Closed the public key file\n");
  }

  return ret;
}


//
// Strip whitespaces at the end of the strig.
//
static inline void
s_strip_text(char *str) {
  if (likely(str != NULL)) {
    char *strp = str;
    while (*strp != '\0') {
      if (*strp == ' ' || *strp == '\t' || *strp == '\r' || *strp == '\n') {
        *strp = '\0';
        break;
      }
      strp++;
    }
  }
}


/*
  *Exported APIs
 */

sqc_result_t
rpc_expand_path(const char* path, char **out_path) {
  return s_expand_path(path, out_path);
}


sqc_result_t
rpc_read_text_file(const char *file, char **text, size_t *textlen) {
  return s_read_text_file(file, text, textlen);
}


void
rpc_strip_text(char *str) {
  s_strip_text(str);
}
