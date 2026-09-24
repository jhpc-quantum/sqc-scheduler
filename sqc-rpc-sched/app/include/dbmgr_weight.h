#pragma once

#include "sqc_rpc_sched.h"
#include "dbmgr_types.h"

__BEGIN_DECLS

sqc_result_t
dbmgr_wi_get_weight(const uint8_t priority, uint64_t *weight);

sqc_result_t
dbmgr_wi_get_created_time(const uint8_t priority, sqc_chrono_t *created_time);

sqc_result_t
dbmgr_wi_get_update_time(const uint8_t priority, sqc_chrono_t *update_time);

__END_DECLS

