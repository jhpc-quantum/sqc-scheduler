#include "sqc_logger.h"

#ifdef perror
#undef perror
#endif /* perror */
#define perror(str)	sqc_msg_error("%s: %s\n", str, strerror(errno))
