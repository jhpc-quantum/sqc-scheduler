#include "sqc_rpc_sched.h"
#include "rpc_jwt_server.h"
#include "sqc_rpc_sched_paths.h"

#pragma once

__BEGIN_DECLS

sqc_tls_conf_t
srvsession_get_tls_conf(void);

rpc_jwt_server_ctx_t *
srvsession_get_jwt_ctx(void);

sqc_result_t
srvsession_register(void);

__END_DECLS
