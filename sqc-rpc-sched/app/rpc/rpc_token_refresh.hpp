#pragma once

/* Optional JHPCQ client-side token provider. No OAuth credentials in SQC. */

//
// Header files for C++.
//
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wshadow"
#include <jwt-cpp/jwt.h>
#pragma GCC diagnostic pop

#include <chrono>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <string>
#include <spawn.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>

extern char **environ;

namespace sqc_token_refresh {

static inline bool
number(const char *name, long def, long max, long &result) {
  const char *s = getenv(name);
  char *end = nullptr;

  if (!s || !*s) {
    result = def;
    return true;
  }

  errno = 0;
  result = strtol(s, &end, 10);
  return errno == 0 && end != s && *end == '\0' && result > 0 && result <= max;
}

static inline bool
fresh(const std::string &file, long margin) {
  char *raw = nullptr;
  bool ok = false;
  sqc_result_t r = rpc_read_text_file(file.c_str(), &raw, nullptr);

  if (r != SQC_RESULT_OK || !raw) {
    free(raw);
    return false;
  }

  try {
    rpc_strip_text(raw);
    // A scheduling hint only. The SQC server still verifies the signature.
    auto decoded = jwt::decode(raw);
    ok = decoded.get_expires_at() >
         std::chrono::system_clock::now() + std::chrono::seconds(margin);
  } catch (...) {
    ok = false;
  }
  free(raw);

  return ok;
}

struct cancel_guard {
  int previous;

  cancel_guard() {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &previous);
  }

  ~cancel_guard() noexcept(false) {
    pthread_setcancelstate(previous, nullptr);
  }
};

static inline sqc_result_t
ensure(const char *dir) {
  const char *value = getenv("SQC_TOKEN_REFRESH_HELPER");

  if (!value || !*value) {
    return SQC_RESULT_OK;
  }

  if (!dir || !*dir) {
    return SQC_RESULT_INVALID_ARGS;
  }

  std::string helper(value);
  std::string file = std::string(dir) + "/jwt.token";
  struct stat st;
  long margin;
  long timeout;

  if (helper[0] != '/' ||
      stat(helper.c_str(), &st) != 0 ||
      !S_ISREG(st.st_mode) ||
      access(helper.c_str(), X_OK) != 0 ||
      !number("SQC_TOKEN_MIN_VALIDITY", 300, 86399, margin) ||
      !number("SQC_TOKEN_REFRESH_TIMEOUT", 60, 300, timeout)) {
    sqc_msg_error("Token helper configuration is invalid.\n");
    return SQC_RESULT_INVALID_ARGS;
  }

  if (fresh(file, margin)) {
    return SQC_RESULT_OK;
  }

  std::string margin_s = std::to_string(margin);
  char *argv[] = {
    const_cast<char *>(helper.c_str()),
    const_cast<char *>("--token-file"),
    const_cast<char *>(file.c_str()),
    const_cast<char *>("--min-validity"),
    const_cast<char *>(margin_s.c_str()),
    nullptr
  };
  cancel_guard guard;
  posix_spawn_file_actions_t actions;
  posix_spawnattr_t attr;
  int err = 0;
  pid_t pid = -1;

  if (posix_spawn_file_actions_init(&actions) != 0) {
    return SQC_RESULT_POSIX_API_ERROR;
  }

  if (posix_spawnattr_init(&attr) != 0) {
    posix_spawn_file_actions_destroy(&actions);
    return SQC_RESULT_POSIX_API_ERROR;
  }

  for (int fd = 0; fd < 3; ++fd) {
    int e = posix_spawn_file_actions_addopen(&actions, fd, "/dev/null",
                                             fd ? O_WRONLY : O_RDONLY, 0);
    if (e) {
      err = e;
    }
  }

  if (posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETPGROUP) != 0 ||
      posix_spawnattr_setpgroup(&attr, 0) != 0) {
    err = EINVAL;
  }

  if (!err) {
    err = posix_spawn(&pid, helper.c_str(), &actions, &attr, argv, environ);
  }
  posix_spawn_file_actions_destroy(&actions);
  posix_spawnattr_destroy(&attr);

  if (err) {
    sqc_msg_error("Token helper could not be started.\n");
    return SQC_RESULT_POSIX_API_ERROR;
  }

  auto until = std::chrono::steady_clock::now() + std::chrono::seconds(timeout);
  int status = 0;

  for (;;) {
    pid_t done = waitpid(pid, &status, WNOHANG);
    if (done == pid) {
      break;
    }

    if (done < 0 && errno != EINTR) {
      return SQC_RESULT_POSIX_API_ERROR;
    }

    if (std::chrono::steady_clock::now() >= until) {
      kill(-pid, SIGKILL);
      while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
        ; // retry.
      }
      sqc_msg_error("Token helper timed out; authentication aborted.\n");
      return SQC_RESULT_TIMEDOUT;
    }

    struct timespec pause = {0, 20000000};
    nanosleep(&pause, nullptr);
  }

  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    sqc_msg_error("Token helper failed; authentication aborted.\n");
    return SQC_RESULT_AUTHENTICATION_ERROR;
  }

  if (!fresh(file, margin)) {
    sqc_msg_error("Token helper returned without a sufficiently fresh token.\n");
    return SQC_RESULT_INVALID_JWT_TOKEN;
  }

  return SQC_RESULT_OK;
}

} // namespace sqc_token_refresh
