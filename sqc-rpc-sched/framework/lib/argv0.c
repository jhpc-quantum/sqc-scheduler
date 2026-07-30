#include "sqc_apis.h"





static char s_argv0[PATH_MAX];





sqc_result_t
sqc_set_command_name(const char *argv0) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(argv0) == true) {
    const char *p = strrchr(argv0, '/');
    const char *s = (p != NULL) ? ++p : argv0;
    (void)snprintf(s_argv0, sizeof(s_argv0), "%s", s);
    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


const char *
sqc_get_command_name(void) {
  return (IS_VALID_STRING(s_argv0) == true) ? (const char *)s_argv0 : NULL;
}
