#pragma once


///
/// @file        rpc_session_client.h
///

#include "rpc_auth_method.h"
#include "rpc_msg_id.h"
#include "rpc_job_info.h"
#include "rpc_jwt_client.h"
#include "dbmgr.h"


__BEGIN_DECLS


struct rpc_session_client;
typedef struct rpc_session_client *rpc_session_client_t;


///
/// @brief   Create an RPC client.
///
/// @param[out]    rpc_session   An RPC session for client.
/// @param[in]     server        An IP address and a port the scheduler listens.
/// @param[in]     prefer_ipv4   Prefer IPv4 to IPv6.
/// @param[in]     auth_method   Authentication method.
/// @param[in]     tls_conf      TLS configuration object.
/// @param[in]     jwt_ctx       JSON Web Token context.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR  Failed, communication error.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_UNSUPPORTED      Failed, unsupported server type.
/// @retval SQC_RESULT_EOF              Failed, the RPC connection has closed.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_session_client_create()</tt> creates \c sqc_session_client_t
/// object and then connects with the scheduler.
///
/// The argument \c server must be one of the following forms:
///   - <tt>domain-name</tt>
///   - <tt>domain-name:port-number</tt>
///   - <tt>ipv4-address</tt>
///   - <tt>ipv4-address:port-number</tt>
///   - <tt>[ipv6-address]</tt>
///   - <tt>[ipv6-address]:port-number</tt>
///
/// If \c domain-name (e.g. \c localhost) is used in \c server, the function tries
/// getting its address.  It choses the first address found, by default.
/// If \c prefer_ipv4 is true, an IPv4 address is chosen.
/// Note that \c prefer_ipv4 has effect only when \c server is <tt>domain-name</tt>
/// or <tt>domain-name:port-number</tt>.
///
/// The function also tries establishing a TLS session on the TCP connection if \c auth_method
/// requires TLS.  If \c tls_conf is NULL, TLS session cannot be established.
///
/// If \c jwt_file is NULL, JWT authentication method never succeeds.
///
sqc_result_t
rpc_session_client_create(rpc_session_client_t *rpc_session, const char *server,
                          bool prefer_ipv4, rpc_auth_method_t auth_method,
                          const sqc_tls_conf_t tls_conf, const rpc_jwt_client_ctx_t *jwt_ctx);

///
/// @brief   Create an RPC client.
///
/// @param[out]    rpc_session   An RPC session for client.
/// @param[in]     server        An IP address and a port the scheduler listens.
/// @param[in]     prefer_ipv4   Prefer IPv4 to IPv6.
/// @param[in]     auth_method   Authentication method.
/// @param[in]     dir           A directory where files are located.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR  Failed, communication error.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_UNSUPPORTED      Failed, unsupported server type.
/// @retval SQC_RESULT_EOF              Failed, the RPC connection has closed.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_session_client_create_from_conf_dir()</tt> is the same
/// as <tt>rpc_session_client_create()</tt>, but it internally creates a TLS configuration
/// object and a JSON Web Token context from files in \c dir.
///
sqc_result_t
rpc_session_client_create_from_conf_dir(rpc_session_client_t *rpc_session, const char *server,
                                        bool prefer_ipv4, rpc_auth_method_t auth_method,
                                        const char *conf_dir);

///
/// @brief   Destroy an RPC session.
///
/// @param[in]     rpc_session   An RPC session for client.
///
/// @details The function <tt>rpc_session_client_destroy()</tt> destroys
/// \c sqc_session_client_t object.
///
void
rpc_session_client_destroy(rpc_session_client_t *rpc_session);

///
/// @brief   Establish a TLS session.
///
/// @param[in]     rpc_session      An RPC session for client.
/// @param[in]     do_mutual_auth   Whether or not to do mutual authentication.
/// @param[in]     use_proxy_cert   Whether or not to use proxy certificates.
///
/// @retval SQC_RESULT_OK                     Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR        Failed, an error occurred in POSIX API.
/// @retval SQC_RESULT_NO_MEMORY              Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS           Failed, invalid arguments.
/// @retval SQC_RESULT_AUTHENTICATION_ERROR   Failed, not valid user.
/// @retval SQC_RESULT_INVALID_CREDENTIAL     Failed, invalid credential.
/// @retval SQC_RESULT_CERTIFICATE_VERIFY_FAILURE
////                                          Certificate verification failure.
/// @retval SQC_RESULT_ANY_FAILURES           Failed, any other reason.
///
/// @details The function <tt>rpc_session_client_establish_tls()</tt> establishes
/// a TLS session on the current TCP connection.
///
sqc_result_t
rpc_session_client_establish_tls(rpc_session_client_t *rpc_session, bool do_mutual_auth,
                                 bool use_proxy_cert);

///
/// @brief   Return a bound address and a port of the RPC session socket.
///
/// @param[in]     rpc_session   An RPC session for client.
/// @param[out]    name          A bound address.
/// @param[out]    port          A bound port.
///
/// @retval SQC_RESULT_OK                    Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR       Failed, an error occurred in POSIX API.
/// @retval SQC_RESULT_NO_MEMORY             Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS          Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES          Failed, any other reason.
///
/// @details The function <tt>rpc_session_client_get_local_name()</tt> returns a bound address
/// and a port of the socket that \c rpc_session uses.
/// The caller is responsible for freeing \c name.
///
sqc_result_t
rpc_session_client_get_local_name(rpc_session_client_t *rpc_session, char **name, int *port);

///
/// @brief   Return a bound address and a port of the RPC session socket.
///
/// @param[in]     rpc_session   An RPC session for client.
/// @param[out]    name          A connected address.
/// @param[out]    port          A connected port.
///
/// @retval SQC_RESULT_OK                    Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR       Failed, an error occurred in POSIX API.
/// @retval SQC_RESULT_NO_MEMORY             Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS          Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES          Failed, any other reason.
///
/// @details The function <tt>rpc_session_client_get_peer_name()</tt> returns a connected
/// address and a port of the socket that \c rpc_session uses.
/// The caller is responsible for freeing \c name.
///
sqc_result_t
rpc_session_client_get_peer_name(rpc_session_client_t *rpc_session, char **name, int *port);

///
/// @brief   Submit a job.
///
/// @param[in]     rpc_session  An RPC session for client.
/// @param[in]     priority     Priority of the job.
/// @param[in]     qprogram     QPROGRAM text to submit.
/// @param[in]     circuit_fmt  Format type of QPROGRAM.
/// @param[in]     shots        The number of shots.
/// @param[in]     qc_type      QC type of the job.
/// @param[in]     transpiler   Transpiler type of the job.
/// @param[in]     remark       Remark comment for the job.
/// @param[in]     user_token   User token for the job.
/// @param[in]     group_id     Group ID.
/// @param[out]    code         A result code of the submission the scheduler returns.
/// @param[out]    msg          A message about the result the scheduler reports.
/// @param[out]    job_id       An issued job ID.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR  Failed, communication error.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_INVALID_OBJECT   Failed, received an unexpected reply from the scheduler.
/// @retval SQC_RESULT_EOF              Failed, the RPC session has closed.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_session_client_submit_job()</tt> submits a job.
/// Upon success, <tt>*code</tt>, <tt>*msg</tt> and <tt>*job_id</tt> are set respectively.
/// The format of the job ID is an UUID terminated by a NUL character.
///
/// Note that <tt>*code</tt> may indicate an error even when the function returns
/// \c SQC_RESULT_OK.  The return value from this function represents whether
/// communication with the scheduler is succeeded, while <tt>*code</tt> represents
/// a result code of the request returned from the scheduler.
///
/// \c msg and \c job_id must be freed by the caller.
///
sqc_result_t
rpc_session_client_submit_job(rpc_session_client_t *rpc_session, uint8_t priority,
                              const char *qprogram, sqc_rpc_sched_circuit_fmt_t circuit_fmt, size_t shots,
                              sqc_rpc_sched_qc_type_t qc_type, sqc_rpc_sched_transpiler_t transpiler,
                              const char *remark, const char *user_token, const char *group_id,
                              sqc_result_t *code, char **msg, char **job_id);

///
/// @brief   Get status of the submitted job.
///
/// @param[in]     rpc_session  An RPC session for client.
/// @param[in]     job_id       A job ID issued by the scheduler.
/// @param[out]    code         A result code of the submission the scheduler returns.
/// @param[out]    msg          A message about the result the scheduler reports.
/// @param[out]    status       The current status of the job.
/// @param[out]    qc_job_id    A job ID issued by the QC.
/// @param[out]    result       Result text of the job.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR  Failed, communication error.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_INVALID_OBJECT   Failed, received an unexpected reply from the scheduler.
/// @retval SQC_RESULT_EOF              Failed, the RPC session has closed.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_session_client_job_status()</tt> queries status of
/// the submitted job to the scheduler.
///
/// Upon success, <tt>*code</tt>, <tt>*status</tt> and <tt>*msg</tt> are set respectively.
/// In addition, result text is set to \c result if the job has been complete
/// (i.e. <tt>*status</tt> is set to \t SQC_RPC_SCHED_JOB_STATUS_DONE).
///
/// Note that <tt>*code</tt> may indicate an error even when the function returns
/// \c SQC_RESULT_OK.  The return value from this function represents whether
/// communication with the scheduler is succeeded, while <tt>*code</tt> represents
/// a result code of the request returned from the scheduler.
///
/// \c msg and \c result must be freed by the caller.
///
sqc_result_t
rpc_session_client_job_status(rpc_session_client_t *rpc_session, const char *job_id,
                              sqc_result_t *code, char **msg, sqc_rpc_sched_job_status_t *status,
                              char **qc_job_id, char **result);

///
/// @brief   Cancel a submitted job.
///
/// @param[in]     rpc_session  An RPC session for client.
/// @param[out]    job_id       An issued job ID.
/// @param[out]    code         A result code of the cancellation the scheduler returns.
/// @param[out]    msg          A message about the result the scheduler reports.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR  Failed, communication error.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_INVALID_OBJECT   Failed, received an unexpected reply from the scheduler.
/// @retval SQC_RESULT_EOF              Failed, the RPC session has closed.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_session_client_cancel_job()</tt> cancel a submitted job.
/// Upon success, <tt>*code</tt> and <tt>*msg</tt> are set respectively.
/// The format of the job ID is an UUID terminated by a NUL character.
///
/// Note that <tt>*code</tt> may indicate an error even when the function returns
/// \c SQC_RESULT_OK.  The return value from this function represents whether
/// communication with the scheduler is succeeded, while <tt>*code</tt> represents
/// a result code of the request returned from the scheduler.
///
/// \c msg must be freed by the caller.
///
sqc_result_t
rpc_session_client_cancel_job(rpc_session_client_t *rpc_session, const char *job_id,
                              sqc_result_t *code, char **msg);

///
/// @brief   Delete a submitted job.
///
/// @param[in]     rpc_session  An RPC session for client.
/// @param[in]     job_id       An issued job ID.
/// @param[out]    code         A result code of the cancellation the scheduler returns.
/// @param[out]    msg          A message about the result the scheduler reports.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR  Failed, communication error.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_INVALID_OBJECT   Failed, received an unexpected reply from the scheduler.
/// @retval SQC_RESULT_EOF              Failed, the RPC session has closed.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_session_client_delete_job()</tt> delete a submitted job.
/// Upon success, <tt>*code</tt> and <tt>*msg</tt> are set respectively.
/// The format of the job ID is an UUID terminated by a NUL character.
///
/// Note that <tt>*code</tt> may indicate an error even when the function returns
/// \c SQC_RESULT_OK.  The return value from this function represents whether
/// communication with the scheduler is succeeded, while <tt>*code</tt> represents
/// a result code of the request returned from the scheduler.
///
/// \c msg must be freed by the caller.
///
sqc_result_t
rpc_session_client_delete_job(rpc_session_client_t *rpc_session, const char *job_id,
                              sqc_result_t *code, char **msg);

///
/// @brief   Get information about jobs submitted by the current user.
///
/// @param[in]     rpc_session  An RPC session for client.
/// @param[out]    code         The result code of the request.
/// @param[out]    msg          The result message.
/// @param[out]    jobs         An array of jobs submitted by the current user.
/// @param[out]    n_jobs       Length of the \c jobs.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_session_client_job_list()</tt> gets an array of
/// information about jobs submitted by the current user.
///
/// If the function returns \c SQC_RESULT_OK, it sets \c code, \c msg and \c jobs and
/// \c n_jobs.
/// \c msg is a NUL-terminated strings.  The caller needs to free it.
/// \c jobs is an array of information about jobs submitted by the current user.
/// The caller needs to free it by calling <tt>rpc_job_list_destroy_array()</tt>.
///
sqc_result_t
rpc_session_client_job_list(rpc_session_client_t *rpc_session, sqc_result_t *code,
                            char **msg, rpc_job_info_t ***jobs, size_t *n_jobs);


///
/// @brief   Delete jobs by administrator privilege.
///
/// @param[in]     rpc_session  An RPC session for client.
/// @param[in]     user_id      ID of the target user or 'ALL'.
/// @param[in]     from_time    Start date of deletaion target.
/// @param[in]     to_time      End date of deletaion target.
/// @param[out]    code         A result code of the cancellation the scheduler returns.
/// @param[out]    msg          A message about the result the scheduler reports.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR  Failed, communication error.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_INVALID_OBJECT   Failed, received an unexpected reply from the scheduler.
/// @retval SQC_RESULT_EOF              Failed, the RPC session has closed.
/// @retval SQC_RESULT_NOT_ADMIN_USER   Failed, the RPC message is not issued by an administrator.
/// @retval SQC_RESULT_DISABLED_USER    Failed, the RPC message is issued by an disabled user.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_session_client_adm_del_jobs()</tt> delete all jobs submitted by \c user_id.
/// between \c from_time and \c to_time.  If \c user_id is 'ALL' jobs, jobs of all users will be removed.
///
/// Only the system administrator can performs the operation.
/// Upon success, <tt>*code</tt> and <tt>*msg</tt> are set respectively.
///
/// \c from_time and \c to_time are the number of nanoseconds since 1970-01-01 00:00:00 in local time.
///
/// Note that <tt>*code</tt> may indicate an error even when the function returns
/// \c SQC_RESULT_OK.  The return value from this function represents whether
/// communication with the scheduler is succeeded, while <tt>*code</tt> represents
/// a result code of the request returned from the scheduler.
///
/// \c msg must be freed by the caller.
///
sqc_result_t
rpc_session_client_adm_del_jobs(rpc_session_client_t *rpc_session, const char *user_id, sqc_chrono_t from_time,
                                sqc_chrono_t to_time, sqc_result_t *code, char **msg);

///
/// @brief   Add a user.
///
/// @param[in]     rpc_session  An RPC session for client.
/// @param[in]     user_id      A User ID.
/// @param[out]    code         A result code of the cancellation the scheduler returns.
/// @param[out]    msg          A message about the result the scheduler reports.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR  Failed, communication error.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_INVALID_OBJECT   Failed, received an unexpected reply from the scheduler.
/// @retval SQC_RESULT_EOF              Failed, the RPC session has closed.
/// @retval SQC_RESULT_NOT_ADMIN_USER   Failed, the RPC message is not issued by an administrator.
/// @retval SQC_RESULT_DISABLED_USER    Failed, the RPC message is issued by an disabled user.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_session_client_adm_add_user()</tt> creates an user without
/// administrator pirivileges.  Only the system administrator can performs the operation.
/// Upon success, <tt>*code</tt> and <tt>*msg</tt> are set respectively.
///
/// Note that <tt>*code</tt> may indicate an error even when the function returns
/// \c SQC_RESULT_OK.  The return value from this function represents whether
/// communication with the scheduler is succeeded, while <tt>*code</tt> represents
/// a result code of the request returned from the scheduler.
///
/// \c msg must be freed by the caller.
///
sqc_result_t
rpc_session_client_adm_add_user(rpc_session_client_t *rpc_session, const char *user_id,
                                sqc_result_t *code, char **msg);

///
/// @brief   Activate / inactivate a user.
///
/// @param[in]     rpc_session  An RPC session for client.
/// @param[in]     user_id      A User ID.
/// @param[in]     enabled      Acitivate (true) / Inactivate (false)
/// @param[out]    code         A result code of the cancellation the scheduler returns.
/// @param[out]    msg          A message about the result the scheduler reports.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR  Failed, communication error.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_INVALID_OBJECT   Failed, received an unexpected reply from the scheduler.
/// @retval SQC_RESULT_EOF              Failed, the RPC session has closed.
/// @retval SQC_RESULT_NOT_ADMIN_USER   Failed, the RPC message is not issued by an administrator.
/// @retval SQC_RESULT_DISABLED_USER    Failed, the RPC message is issued by an disabled user.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_session_client_adm_set_user_status()</tt> activites or
/// inactivates a user.  Only the system administrator can performs the operation.
/// Upon success, <tt>*code</tt> and <tt>*msg</tt> are set respectively.
///
/// Note that <tt>*code</tt> may indicate an error even when the function returns
/// \c SQC_RESULT_OK.  The return value from this function represents whether
/// communication with the scheduler is succeeded, while <tt>*code</tt> represents
/// a result code of the request returned from the scheduler.
///
/// \c msg must be freed by the caller.
///
sqc_result_t
rpc_session_client_adm_set_user_status(rpc_session_client_t *rpc_session, const char *user_id, bool enabled,
                                       sqc_result_t *code, char **msg);

///
/// @brief   Activate / inactivate a user-group association.
///
/// @param[in]     rpc_session  An RPC session for client.
/// @param[in]     user_id      A User ID.
/// @param[in]     group_id     A Group ID.
/// @param[in]     enabled      Acitivate (true) / Inactivate (false)
/// @param[out]    code         A result code of the cancellation the scheduler returns.
/// @param[out]    msg          A message about the result the scheduler reports.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR  Failed, communication error.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_INVALID_OBJECT   Failed, received an unexpected reply from the scheduler.
/// @retval SQC_RESULT_EOF              Failed, the RPC session has closed.
/// @retval SQC_RESULT_NOT_ADMIN_USER   Failed, the RPC message is not issued by an administrator.
/// @retval SQC_RESULT_DISABLED_USER    Failed, the RPC message is issued by an disabled user.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_session_client_adm_set_user_group_status()</tt> activates or
/// inactivates the association between a user and a group.  Only the system administrator can
/// perform the operation.  Upon success, <tt>*code</tt> and <tt>*msg</tt> are set respectively.
///
/// Note that <tt>*code</tt> may indicate an error even when the function returns
/// \c SQC_RESULT_OK.  The return value from this function represents whether
/// communication with the scheduler is succeeded, while <tt>*code</tt> represents
/// a result code of the request returned from the scheduler.
///
/// \c msg must be freed by the caller.
///
sqc_result_t
rpc_session_client_adm_set_user_group_status(rpc_session_client_t *rpc_session, const char *user_id,
                                             const char *group_id, bool enabled,
                                             sqc_result_t *code, char **msg);

///
/// @brief   Set the execution time limit for the group.
///
/// @param[in]     rpc_session          An RPC session for client.
/// @param[in]     group_id             A Group ID.
/// @param[in]     exec_time_limit      Execution time limit.
/// @param[out]    code                 A result code of the cancellation the scheduler returns.
/// @param[out]    msg                  A message about the result the scheduler reports.
///
/// @retval SQC_RESULT_OK               Succeeded.
/// @retval SQC_RESULT_POSIX_API_ERROR  Failed, communication error.
/// @retval SQC_RESULT_NO_MEMORY        Failed, no memory.
/// @retval SQC_RESULT_INVALID_ARGS     Failed, invalid arguments.
/// @retval SQC_RESULT_INVALID_OBJECT   Failed, received an unexpected reply from the scheduler.
/// @retval SQC_RESULT_EOF              Failed, the RPC session has closed.
/// @retval SQC_RESULT_NOT_ADMIN_USER   Failed, the RPC message is not issued by an administrator.
/// @retval SQC_RESULT_DISABLED_USER    Failed, the RPC message is issued by an disabled user.
/// @retval SQC_RESULT_ANY_FAILURES     Failed, any other reason.
///
/// @details The function <tt>rpc_session_client_adm_set_group_exec_time_limit()</tt> activites or
/// inactivates a user.  Only the system administrator can performs the operation.
/// Upon success, <tt>*code</tt> and <tt>*msg</tt> are set respectively.
///
/// Note that <tt>*code</tt> may indicate an error even when the function returns
/// \c SQC_RESULT_OK.  The return value from this function represents whether
/// communication with the scheduler is succeeded, while <tt>*code</tt> represents
/// a result code of the request returned from the scheduler.
///
/// \c msg must be freed by the caller.
///
sqc_result_t
rpc_session_client_adm_set_group_exec_time_limit(rpc_session_client_t *rpc_session,
                                                 const char *group_id, uint64_t exec_time_limit,
                                                 sqc_result_t *code, char **msg);

__END_DECLS
