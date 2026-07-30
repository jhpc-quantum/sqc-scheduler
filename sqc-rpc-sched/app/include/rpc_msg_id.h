#pragma once


__BEGIN_DECLS


/**
 * @file 	rpc_msg_id.h
 */

/**
 * @brief RPC Message ID.
 */
enum rpc_msg_id {
  RPC_MSG_OPEN_NEW_SESSION_REQUEST     =  0,
  RPC_MSG_OPEN_NEW_SESSION_REPLY       =  1,

  RPC_MSG_SUBMIT_JOB_REQUEST           =  2,
  RPC_MSG_SUBMIT_JOB_REPLY             =  3,

  RPC_MSG_JOB_STATUS_REQUEST           =  4,
  RPC_MSG_JOB_STATUS_REPLY             =  5,

  RPC_MSG_AUTH_REQUEST                 =  6,
  RPC_MSG_AUTH_REPLY                   =  7,

  RPC_MSG_CANCEL_JOB_REQUEST           =  8,
  RPC_MSG_CANCEL_JOB_REPLY             =  9,

  RPC_MSG_DELETE_JOB_REQUEST           = 10,
  RPC_MSG_DELETE_JOB_REPLY             = 11,

  RPC_MSG_JOB_LIST_REQUEST             = 12,
  RPC_MSG_JOB_LIST_REPLY               = 13,

  RPC_MSG_ADM_DEL_JOBS_REQUEST         = 14,
  RPC_MSG_ADM_DEL_JOBS_REPLY           = 15,

  RPC_MSG_ADM_ADD_USER_REQUEST         = 16,
  RPC_MSG_ADM_ADD_USER_REPLY           = 17,

  RPC_MSG_ADM_SET_USER_STATUS_REQUEST  = 18,
  RPC_MSG_ADM_SET_USER_STATUS_REPLY    = 19,
};

typedef enum rpc_msg_id rpc_msg_id_t;

__END_DECLS
