#include "sqc_apis.h"

#include "modtmpl.h"
#include "dbmgr.h"
#include "req_invoker.h"
#include "srv_session.h"
#include "grpc_broker.h"


#include "modules.h"





static pthread_once_t s_once = PTHREAD_ONCE_INIT;





static void
s_once_proc(void) {
  sqc_result_t r = modtmpl_register();
  if (r != SQC_RESULT_OK) {
    sqc_perror(r);
    sqc_exit_fatal("can't register the modtmpl modules.\n");
  }

  r = dbmgr_register();
  if (r != SQC_RESULT_OK) {
    sqc_perror(r);
    sqc_exit_fatal("can't register the dbmgr modules.\n");
  }

  r = req_invoker_register();
  if (r != SQC_RESULT_OK) {
    sqc_perror(r);
    sqc_exit_fatal("can't register the req invoker modules.\n");
  }

  r = srvsession_register();
  if (r != SQC_RESULT_OK) {
    sqc_perror(r);
    sqc_exit_fatal("can't register the srvsession modules.\n");
  }

  r = grpc_broker_register();
  if (r != SQC_RESULT_OK) {
    sqc_perror(r);
    sqc_exit_fatal("can't register the grpc broker modules.\n");
  }
}





void
module_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}
