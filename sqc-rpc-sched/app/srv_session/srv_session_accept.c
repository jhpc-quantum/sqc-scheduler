#include "sqc_apis.h"
#include "sqc_thread_internal.h"
#include "srv_session.h"
#include "rpc_session_server.h"

typedef struct srvsession_handler_thread_record {
  struct sqc_thread_record thd_;

  int fd_;
  rpc_session_server_t session_;

  sqc_chrono_t interval_;

  sqc_rwlock_t rwlck_;

  volatile bool do_loop_;
  volatile bool is_started_;
  volatile shutdown_grace_level_t shutdown_level_;
} srvsession_handler_thread_record;
typedef srvsession_handler_thread_record *srvsession_handler_thread_t;

static srvsession_handler_thread_t handler_thd_array[SQC_RPC_SCHED_SRV_MAX_SESSION];
static sqc_mutex_t handler_thd_array_lck = NULL;

static sqc_result_t
s_handler_thd_array_remove(srvsession_handler_thread_t st);

static sqc_endpoint_t endpoint = NULL;


/*
 * thread methods
 */


static inline void
s_srvsession_handler_rlock(srvsession_handler_thread_t st) {
  if (likely(st != NULL)) {
    (void)sqc_rwlock_reader_lock(&(st->rwlck_));
  }
}


static inline void
s_srvsession_handler_wlock(srvsession_handler_thread_t st) {
  if (likely(st != NULL)) {
    (void)sqc_rwlock_writer_lock(&(st->rwlck_));
  }
}


static inline void
s_srvsession_handler_unlock(srvsession_handler_thread_t st) {
  if (likely(st != NULL)) {
    (void)sqc_rwlock_unlock(&(st->rwlck_));
  }
}


static void
s_srvsession_handler_finalize(const sqc_thread_t *tptr, bool is_canceled, void *arg) {
  srvsession_handler_thread_t st = (srvsession_handler_thread_t)*tptr;
  (void)arg;

  sqc_msg_debug(5, "called.\n");

  if (likely(st != NULL)) {
    if (is_canceled == true) {
      if (st->is_started_ == false) {
        /*
         * Means this thread is canceled while waiting for the global
         * state change.
         */
        global_state_cancel_janitor();
        s_srvsession_handler_unlock(st);
      }
    }
  }

  sqc_msg_debug(5, "called with %s self and the thread is %s.\n",
                ((st != NULL) ? "valid" : "invalid (NULL)"),
                ((is_canceled == true) ? "canceled" : "exited"));
}


static void
s_srvsession_handler_freeup(const sqc_thread_t *tptr, void *arg) {
  srvsession_handler_thread_t st = (srvsession_handler_thread_t)*tptr;

  (void)arg;

  sqc_msg_debug(5, "called with %s self.\n",
                ((st != NULL) ? "valid" : "invalid (NULL)"));
  if (st != NULL) {
    /*
     * Here we can free up all the resource related to this thread.
     */
    s_handler_thd_array_remove(st);
    if (st->session_ != NULL) {
      rpc_session_server_destroy(&(st->session_));
    }

    if (st->rwlck_ != NULL) {
      (void)sqc_rwlock_destroy(&(st->rwlck_));
    }
  }
}


static sqc_result_t
s_srvsession_handler_main(const sqc_thread_t *tptr, void *arg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  global_state_t s;
  shutdown_grace_level_t l;
  srvsession_handler_thread_t st = (srvsession_handler_thread_t)*tptr;

  (void)arg;

  sqc_msg_debug(5, "waiting for the gala opening...\n");

  ret = global_state_wait_for(GLOBAL_STATE_STARTED, &s, &l, -1LL);
  if (ret == SQC_RESULT_OK && s == GLOBAL_STATE_STARTED) {
    s_srvsession_handler_wlock(st);
    {
      st->is_started_ = true;
    }
    s_srvsession_handler_unlock(st);

    sqc_msg_debug(5, "gala opening.\n");

    /*
     * The main loop.
     */
    do {
      sqc_result_t request_result = SQC_RESULT_ANY_FAILURES;
      sqc_msg_debug(6, "looping...\n");

      request_result = rpc_session_server_process_request(&st->session_);
      if (request_result == SQC_RESULT_EOF) {
        break;
      } else if (request_result != SQC_RESULT_OK) {
        sqc_msg_debug(5, "RPC session error occurred, "
                      "give up communicating with the client\n");
        break;
      }

      /*
       * Create an explicit cancalation point since this loop has
       * none of it.
       */
      pthread_testcancel();

    } while (st->do_loop_ == true);

    /*
     * Reaching here means someone called a shutdown request.
     */
    if (st->shutdown_level_ == SHUTDOWN_GRACEFULLY) {
      /*
       * This is just emulating/mimicking a graceful shutdown by
       * sleep().  Don't do this on actual modules.
       */
      sqc_msg_debug(5, "mimicking gracefull shutdown...\n");
      sleep(5);
      sqc_msg_debug(5, "mimicking gracefull shutdown done.\n");
      ret = SQC_RESULT_OK;
    } else {
      ret = 1LL;
    }
  }

  return ret;
}


static inline sqc_result_t
s_srvsession_handler_create(srvsession_handler_thread_t *tptr, int fd,
                            rpc_session_server_t session, sqc_chrono_t interval) {
  sqc_result_t rc = SQC_RESULT_INVALID_ARGS;

  sqc_msg_debug(5, "called.\n");

  if (likely(fd >= 0 && session != NULL && interval >= 1000 * 1000)) {
    if ((rc = sqc_thread_create_with_size((sqc_thread_t *)tptr,
                                          sizeof(srvsession_handler_thread_record),
                                          s_srvsession_handler_main,
                                          s_srvsession_handler_finalize,
                                          s_srvsession_handler_freeup,
                                          "srvsession_handler_thread",
                                          NULL)) == SQC_RESULT_OK) {
      (*tptr)->rwlck_ = NULL;
      if ((rc = sqc_rwlock_create(&((*tptr)->rwlck_))) == SQC_RESULT_OK) {
        (*tptr)->fd_ = fd;
        (*tptr)->session_ = session;
        (*tptr)->interval_ = interval;
        (*tptr)->do_loop_ = false;
        (*tptr)->is_started_ = false;
        (*tptr)->shutdown_level_ = SHUTDOWN_UNKNOWN;
        goto done;
      }

      sqc_thread_destroy((sqc_thread_t *)tptr);
    }
  } else {
    rc = SQC_RESULT_INVALID_ARGS;
  }

done:
  return rc;
}


static inline sqc_result_t
s_srvsession_handler_start(srvsession_handler_thread_t tptr) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  if (likely(tptr != NULL)) {
    if (likely((rc = sqc_thread_start((sqc_thread_t *)&tptr, true)) ==
               SQC_RESULT_OK)) {

      s_srvsession_handler_wlock(tptr);
      {
        tptr->do_loop_ = true;
      }
      s_srvsession_handler_unlock(tptr);
    } else {
      rc = SQC_RESULT_INVALID_OBJECT;
    }
  } else {
    rc = SQC_RESULT_INVALID_ARGS;
  }

  return rc;
}


static inline void
s_srvsession_handler_destroy(srvsession_handler_thread_t tptr) {
  sqc_msg_debug(5, "called.\n");

  if (tptr != NULL) {
    sqc_msg_info("Destroy thread: fd=%d\n", tptr->fd_);
    s_handler_thd_array_remove(tptr);
    if (tptr->session_ != NULL) {
      rpc_session_server_destroy(&(tptr->session_));
    }

    if (tptr->rwlck_ != NULL) {
      (void)sqc_rwlock_destroy(&(tptr->rwlck_));
    }
  }

  sqc_thread_destroy((sqc_thread_t *)tptr);
}


/*
 * methods for operating handler_thd_array
 */

static inline void
s_handler_thd_array_lock(void) {
  if (likely(handler_thd_array_lck != NULL)) {
    (void)sqc_mutex_lock(&handler_thd_array_lck);
  }
}


static inline void
s_handler_thd_array_unlock(void) {
  if (likely(handler_thd_array_lck != NULL)) {
    (void)sqc_mutex_unlock(&handler_thd_array_lck);
  }
}

static sqc_result_t
s_handler_thd_array_initialize(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  for (int i = 0; i < SQC_RPC_SCHED_SRV_MAX_SESSION; i++) {
    handler_thd_array[i] = NULL;
  }

  rc = sqc_mutex_create(&handler_thd_array_lck);
  if (likely(rc == SQC_RESULT_OK)) {
    sqc_msg_debug(5, "thread handler table initialized\n");
  } else {
    sqc_msg_error("Failed to create lock: %s\n", sqc_error_get_string(rc));
  }

  return rc;
}


static sqc_result_t
s_handler_thd_array_add(srvsession_handler_thread_t st) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  srvsession_handler_thread_t old_st = NULL;

  if (likely(st != NULL)) {
    s_srvsession_handler_rlock(st);
    if (likely(st->fd_ >=0 && st->fd_ < SQC_RPC_SCHED_SRV_MAX_SESSION)) {
      s_handler_thd_array_lock();
      old_st = handler_thd_array[st->fd_];
      if (old_st == st) {
        sqc_msg_debug(5, "The thread handler is already added to the table\n");
      } else if (old_st != NULL) {
        sqc_msg_warning("An old thread hanlder is remained in the table\n");
      }
      rc = SQC_RESULT_OK;
      handler_thd_array[st->fd_] = st;
      s_handler_thd_array_unlock();
      sqc_msg_debug(5, "Add fd=%d (handler=%p) to the thread handler\n",
                    st->fd_, (void *)st);
    } else {
      rc = SQC_RESULT_INVALID_ARGS;
      sqc_msg_error("File descriptor is out of range: fd=%d\n", st->fd_);
    }
    s_srvsession_handler_unlock(st);
  } else {
    rc = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("Failed to add a thread handler to the table: %s\n",
                  sqc_error_get_string(rc));
  }

  return rc;
}


static sqc_result_t
s_handler_thd_array_remove(srvsession_handler_thread_t st) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  srvsession_handler_thread_t old_st = NULL;

  if (likely(st != NULL)) {
    s_srvsession_handler_rlock(st);
    if (likely(st->fd_ >=0 && st->fd_ < SQC_RPC_SCHED_SRV_MAX_SESSION)) {
      s_handler_thd_array_lock();
      old_st = handler_thd_array[st->fd_];
      if (old_st == NULL) {
        sqc_msg_debug(5, "The thread handler is already removed from the table\n");
      } else if (old_st != st) {
        sqc_msg_warning("a different thread hanlder is added to the table, skipped\n");
      }
      rc = SQC_RESULT_OK;
      handler_thd_array[st->fd_] = NULL;
      s_handler_thd_array_unlock();
      sqc_msg_debug(5, "Remove fd=%d (handler=%p) from the table\n", st->fd_, (void *)st);
    } else {
      rc = SQC_RESULT_INVALID_ARGS;
      sqc_msg_error("File descriptor is out of range: fd=%d\n", st->fd_);
    }
    s_srvsession_handler_unlock(st);
  } else {
    rc = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("Failed to add a thread handler to the table: %s\n",
                  sqc_error_get_string(rc));
  }

  return rc;
}


/*
 * accept methods
 */


static sqc_result_t
s_srvsession_accept(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;
  int new_fd = -1;
  struct sockaddr_storage addr;
  socklen_t addrlen = sizeof(addr);

  if (likely(endpoint != NULL)) {
    rc = sqc_endpoint_accept(&endpoint, &new_fd, &addr, &addrlen);
    if (likely(rc == SQC_RESULT_OK && new_fd >= 0)) {
      rpc_session_server_t session = NULL;
      rc = rpc_session_server_create(&session, new_fd, &addr, addrlen,
                                     srvsession_get_tls_conf(),
                                     srvsession_get_jwt_ctx());
      if (likely(rc == SQC_RESULT_OK && session != NULL)) {
        srvsession_handler_thread_t tptr = NULL;
        sqc_msg_debug(5, "Created and set up an RPC session\n");
        rc = s_srvsession_handler_create(&tptr, new_fd, session, SQC_RPC_SCHED_SRV_THD_INTERVAL);
        if (rc == SQC_RESULT_OK) {
          sqc_msg_info("Create thread: fd=%d\n", new_fd);

          rc = s_srvsession_handler_start(tptr);
          if (rc == SQC_RESULT_OK) {
            sqc_msg_info("Start thread: fd=%d\n", new_fd);
            s_handler_thd_array_add(tptr);
          } else {
            sqc_msg_error("Failed to start thread: %s\n", sqc_error_get_string(rc));
            s_srvsession_handler_destroy(tptr);
          }
        } else {
          sqc_msg_error("Failed to create thread: %s\n", sqc_error_get_string(rc));
        }
      } else {
        sqc_msg_error("Failed to create a session for a connected client: %s\n",
                      sqc_error_get_string(rc));
      }
    } else {
      sqc_msg_error("Failed to accept a connection from a client: %s\n", sqc_error_get_string(rc));
    }
  } else {
    rc = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("Failed to accept: %s\n", sqc_error_get_string(rc));
  }

  return rc;
}


static sqc_result_t
s_srvsession_accept_initialize(const char *spec, const bool prefer_ipv4) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  if (likely(spec != NULL)) {
    rc = sqc_endpoint_create(&endpoint, 0, SQC_ENDPOINT_TYPE_INET_ACCEPTOR, spec, prefer_ipv4);
    if (likely(rc == SQC_RESULT_OK)) {
      // Instead of setting NODELAY option for each TCP socket issued by accept(2),
      // we set the option for a listen socket here.
      // On Linux, the option is inherited to the accepted sockets automatically.
      rc = sqc_endpoint_set_tcp_nodelay(&endpoint, true);
      if (likely(rc == SQC_RESULT_OK)) {
        rc = sqc_endpoint_bind(&endpoint);
        if (likely(rc == SQC_RESULT_OK)) {
          sqc_msg_info("Binding of endpoint succeeded: %s(%d)\n", spec, prefer_ipv4);
          rc = s_handler_thd_array_initialize();
          if (likely(rc == SQC_RESULT_OK)) {
            sqc_msg_info("An array of thread handlers initialized\n");
          } else {
            sqc_msg_error("Failed to initialize an array of thread handlers: %s\n",
                          sqc_error_get_string(rc));
          }
        } else {
          sqc_msg_error("Failed to bind endpoint: %s\n", sqc_error_get_string(rc));
        }
      } else {
        sqc_msg_error("Failed to setsockopt(TCP_NODELAY) for endpoint: %s\n", sqc_error_get_string(rc));
      }
    } else {
      sqc_msg_error("Failed to create endpoint: %s\n", sqc_error_get_string(rc));
    }
  } else {
    rc = SQC_RESULT_INVALID_ARGS;
    sqc_msg_error("Failed to initialize: %s\n", sqc_error_get_string(rc));
  }

  return rc;
}


static void
s_srvsession_accept_shutdown(shutdown_grace_level_t l) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  s_handler_thd_array_lock();
  for (int i = 0; i < SQC_RPC_SCHED_SRV_MAX_SESSION; i++) {
    srvsession_handler_thread_t handler_thd = handler_thd_array[i];
    if (likely(handler_thd != NULL)) {
      s_srvsession_handler_wlock(handler_thd);
      {
        if (handler_thd->is_started_ == true) {
          handler_thd->shutdown_level_ = l;
          handler_thd->do_loop_ = false;
          rc = SQC_RESULT_OK;
        } else {
          rc = SQC_RESULT_INVALID_STATE_TRANSITION;
        }
      }
      s_srvsession_handler_unlock(handler_thd);

      if (rc != SQC_RESULT_OK) {
        sqc_msg_warning("Failed to shutdown: %s\n", sqc_error_get_string(rc));
      }
    }
  }
  s_handler_thd_array_unlock();
}


static void
s_srvsession_accept_stop(void) {
  sqc_result_t rc = SQC_RESULT_ANY_FAILURES;

  s_handler_thd_array_lock();
  for (int i = 0; i < SQC_RPC_SCHED_SRV_MAX_SESSION; i++) {
    srvsession_handler_thread_t handler_thd = handler_thd_array[i];
    if (likely(handler_thd != NULL)) {
      s_srvsession_handler_wlock(handler_thd);
      {
        if (handler_thd->is_started_ == true) {
          rc = sqc_thread_cancel((sqc_thread_t *)&handler_thd);
        } else {
          rc = SQC_RESULT_INVALID_STATE_TRANSITION;
        }
      }
      s_srvsession_handler_unlock(handler_thd);

      if (rc != SQC_RESULT_OK) {
        sqc_msg_warning("Failed to stop: %s\n", sqc_error_get_string(rc));
      }
    }
  }
  s_handler_thd_array_unlock();
}


static void
s_srvsession_accept_finalize(void) {
  s_handler_thd_array_lock();
  for (int i = 0; i < SQC_RPC_SCHED_SRV_MAX_SESSION; i++) {
    if (handler_thd_array[i] != NULL) {
      s_srvsession_handler_destroy(handler_thd_array[i]);
      handler_thd_array[i] = NULL;
    }
  }
  s_handler_thd_array_unlock();
  sqc_msg_debug(5, "The thread handler table is cleared\n");

  sqc_endpoint_destroy(&endpoint);
  endpoint = NULL;
}
