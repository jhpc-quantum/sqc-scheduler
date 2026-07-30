#include "sqc_apis.h"





#define ONE_SEC	1000LL * 1000LL * 1000LL

#define MAINLOOP_IDLE_PROC_INTERVAL	ONE_SEC
#define MAINLOOP_SHUTDOWN_TIMEOUT	5LL * ONE_SEC

#define DEFAULT_PIDFILE_DIR	"/var/run/"

#if defined(SQC_OS_LINUX) && defined(HAVE_SYS_PRCTL_H)
#define USE_PRCTL
#endif /* defined(SQC_OS_LINUX) && defined(HAVE_SYS_PRCTL_H) */


#define PARENT_EXIT_WAIT_TIMEOUT	ONE_SEC





typedef sqc_result_t (*mainloop_proc_t)(
  int argc, const char *const argv[],
  sqc_mainloop_startup_hook_proc_t pre_hook,
  sqc_mainloop_startup_hook_proc_t post_hook,
  int ipcfd);


typedef struct {
  mainloop_proc_t m_proc;
  int m_argc;
  const char *const *m_argv;
  sqc_mainloop_startup_hook_proc_t m_pre_hook;
  sqc_mainloop_startup_hook_proc_t m_post_hook;
  int m_ipcfd;
} mainloop_args_t;





static shutdown_grace_level_t s_gl = SHUTDOWN_UNKNOWN;

static char s_pidfile_buf[PATH_MAX];
static const char *s_pidfile;
static volatile bool s_do_pidfile = false;

static volatile bool s_do_fork = false;
static volatile bool s_do_abort = false;

static size_t s_n_callout_workers = 1;

static sqc_chrono_t s_idle_proc_interval =
  MAINLOOP_IDLE_PROC_INTERVAL;
static sqc_chrono_t s_shutdown_timeout =
  MAINLOOP_SHUTDOWN_TIMEOUT;


static sqc_thread_t s_thd = NULL;
static mainloop_args_t s_args = { NULL };

static bool s_org_multi_proc_mode = false;

#ifdef USE_PRCTL

static sqc_mutex_t s_parent_sync_lck = NULL;
static sqc_cond_t s_parent_sync_cnd = NULL;
static volatile bool s_is_parent_exit = false;
static bool s_is_inited = false;
static pthread_once_t s_once = PTHREAD_ONCE_INIT;

static void s_ctors(void) __attr_constructor__(114);
static void s_dtors(void) __attr_destructor__(114);

static void
s_once_proc(void) {
  sqc_result_t r;

  if ((r = sqc_mutex_create(&s_parent_sync_lck)) != SQC_RESULT_OK) {
    sqc_perror(r);
    sqc_exit_fatal("can't initialize the parent sync mutex.\n");
  }

  if ((r = sqc_cond_create(&s_parent_sync_cnd)) != SQC_RESULT_OK) {
    sqc_perror(r);
    sqc_exit_fatal("can't initialize the parent sync cond.\n");
  }

  s_is_inited = true;
}


static inline void
s_init(void) {
  (void)pthread_once(&s_once, s_once_proc);
}


static void
s_ctors(void) {
  s_init();
}


static inline void
s_final(void) {
  if (s_parent_sync_cnd != NULL) {
    (void)sqc_cond_destroy(&s_parent_sync_cnd);
  }
  if (s_parent_sync_lck != NULL) {
    (void)sqc_mutex_destroy(&s_parent_sync_lck);
  }
}


static void
s_dtors(void) {
  if (s_is_inited == true) {
    if (sqc_module_is_unloading() &&
        sqc_module_is_finalized_cleanly()) {
      s_final();
    }
  }
}


static void
s_temp_USR1_handler(int sig) {
  if (sig == SIGUSR1) {
    (void)sqc_mutex_lock(&s_parent_sync_lck);
    {
      s_is_parent_exit = true;
      (void)sqc_cond_notify(&s_parent_sync_cnd, true);
    }
    (void)sqc_mutex_unlock(&s_parent_sync_lck);
  }
}


static inline sqc_result_t
s_wait_parent_exit(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  pid_t ppid = getppid();
  sqc_chrono_t stop;
  sqc_chrono_t now;

  WHAT_TIME_IS_IT_NOW_IN_NSEC(stop);
  stop += PARENT_EXIT_WAIT_TIMEOUT;	/* 1 sec. */

  sqc_msg_debug(5, "Waiting parent %d exit ...\n", ppid);

  (void)sqc_mutex_lock(&s_parent_sync_lck);
  {
    errno = 0;
    while (s_is_parent_exit == false) {
      WHAT_TIME_IS_IT_NOW_IN_NSEC(now);
      if (now < stop) {
        errno = 0;
        if (kill(ppid, 0) == 0) {
          ret = sqc_cond_wait(&s_parent_sync_cnd, &s_parent_sync_lck,
                                 100LL * 1000LL * 1000LL /* 100ms. */);
          continue;
        } else {
          if (errno == ESRCH) {
            ret = SQC_RESULT_OK;
          } else {
            ret = SQC_RESULT_POSIX_API_ERROR;
            sqc_perror(ret);
          }
          break;
        }
      }
    }
  }
  (void)sqc_mutex_unlock(&s_parent_sync_lck);

  sqc_msg_debug(5, "Waiting parent %d exit ... done: %s.\n",
                   ppid, sqc_error_get_string(ret));

  (void)sqc_signal(SIGUSR1, SIG_DFL, NULL);

  return ret;
}


static inline sqc_result_t
s_prepare_wait_parent_exit(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  pid_t ppid = getppid();

  if (kill(ppid, 0) == 0) {
    (void)sqc_signal(SIGUSR1, s_temp_USR1_handler, NULL);
    if (prctl(PR_SET_PDEATHSIG, SIGUSR1) == 0) {
      ret = SQC_RESULT_OK;
    } else {
      ret = SQC_RESULT_NOT_FOUND;
      sqc_perror(ret);
    }
  } else {
    ret = SQC_RESULT_NOT_FOUND;
  }

  return ret;
}


#else


static inline sqc_result_t
s_wait_parent_exit(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  pid_t ppid = getppid();
  sqc_chrono_t stop;
  sqc_chrono_t now;

  WHAT_TIME_IS_IT_NOW_IN_NSEC(stop);
  stop += PARENT_EXIT_WAIT_TIMEOUT;	/* 1 sec. */

  while (s_is_parent_exit == false) {
    WHAT_TIME_IS_IT_NOW_IN_NSEC(now);
    if (now < stop) {
      errno = 0;
      if (kill(ppid, 0) == 0) {
        continue;
      } else {
        if (errno == ESRCH) {
          ret = SQC_RESULT_OK;
        } else {
          ret = SQC_RESULT_POSIX_API_ERROR;
          sqc_perror(ret);
        }
        break;
      }
    }
  }
}

return ret;
}


static inline sqc_result_t
s_prepare_wait_parent_exit(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  pid_t ppid = getppid();

  if (kill(ppid, 0) == 0) {
    ret = SQC_RESULT_OK;
  } else {
    ret = SQC_RESULT_NOT_FOUND;
  }

  return ret;
}


#endif /* USE_PRCTL */





static inline pid_t
s_setsid(void) {
  pid_t ret = (pid_t)-1;

#if 0
  int fd = open("/dev/tty", O_RDONLY);

  if (fd >= 0) {
#ifdef TIOCNOTTY
    int one = 1;
    (void)ioctl(fd, TIOCNOTTY, &one);
#endif /* TIOCNOTTY */
    (void)close(fd);
  }
#endif

  errno = 0;
  if (unlikely((ret = setsid()) < 0)) {
    perror("setsid");
    errno = 0;
    if (likely(setpgid(0, 0) == 0)) {
      errno = 0;
      ret = getpgrp();
    } else {
      perror("getpgrp");
      ret = (pid_t)-1;
    }
  }

  return ret;
}


static inline void
s_daemonize(int exclude_fd) {
  int i;

  sqc_log_sync_for_fork();
  (void)s_setsid();

  for (i = 0; i < 1024; i++) {
    if (i != exclude_fd) {
      (void)close(i);
    }
  }

  i = open("/dev/zero", O_RDONLY);
  if (i > 0) {
    (void)dup2(0, i);
    (void)close(i);
  }

  i = open("/dev/null", O_WRONLY);
  if (i > 1) {
    (void)dup2(1, i);
    (void)close(i);
  }

  i = open("/dev/null", O_WRONLY);
  if (i > 2) {
    (void)dup2(2, i);
    (void)close(i);
  }
}


static inline void
s_gen_pidfile(void) {
  FILE *fd = NULL;

  if (IS_VALID_STRING(s_pidfile) == true) {
    snprintf(s_pidfile_buf, sizeof(s_pidfile_buf), "%s", s_pidfile);
  } else {
    snprintf(s_pidfile_buf, sizeof(s_pidfile_buf),
             DEFAULT_PIDFILE_DIR "%s.pid",
             sqc_get_command_name());
  }

  /* TODO: umask(2) */

  fd = fopen(s_pidfile_buf, "w");
  if (fd != NULL) {
    fprintf(fd, "%d\n", (int)getpid());
    fflush(fd);
    (void)fclose(fd);
  } else {
    sqc_perror(SQC_RESULT_POSIX_API_ERROR);
    sqc_msg_error("can't create a pidfile \"%s\".\n",
                     s_pidfile_buf);
  }
}


static inline void
s_del_pidfile(void) {
  if (IS_VALID_STRING(s_pidfile_buf) == true) {
    struct stat st;

    if (stat(s_pidfile_buf, &st) == 0) {
      if (S_ISREG(st.st_mode)) {
        (void)unlink(s_pidfile_buf);
      }
    }
  }
}





static void
s_term_handler(int sig) {
  sqc_result_t r = SQC_RESULT_ANY_FAILURES;
  global_state_t gs = GLOBAL_STATE_UNKNOWN;

  if ((r = global_state_get(&gs)) == SQC_RESULT_OK) {

    if ((int)gs == (int)GLOBAL_STATE_STARTED) {

      shutdown_grace_level_t l = SHUTDOWN_UNKNOWN;
      if (sig == SIGTERM || sig == SIGINT) {
        l = SHUTDOWN_GRACEFULLY;
      } else if (sig == SIGQUIT) {
        l = SHUTDOWN_RIGHT_NOW;
      }
      if (IS_VALID_SHUTDOWN(l) == true) {
        sqc_msg_info("About to request shutdown(%s)...\n",
                        (l == SHUTDOWN_RIGHT_NOW) ?
                        "RIGHT_NOW" : "GRACEFULLY");
        if ((r = global_state_request_shutdown(l)) == SQC_RESULT_OK) {
          sqc_msg_info("The shutdown request accepted.\n");
        } else {
          sqc_perror(r);
          sqc_msg_error("can't request shutdown.\n");
        }
      }

    } else if ((int)gs < (int)GLOBAL_STATE_STARTED) {

      if (sig == SIGTERM || sig == SIGINT || sig == SIGQUIT) {
        sqc_abort_before_mainloop();
      }

    } else {
      sqc_msg_debug(5, "The system is already shutting down.\n");
    }

  }

}


static sqc_result_t
s_mainloop_thd_main(const sqc_thread_t *tptr, void *arg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  (void)tptr;

  if (likely(arg != NULL)) {
    mainloop_args_t *mlarg = (mainloop_args_t *)arg;
    if (likely(mlarg->m_proc != NULL &&
               mlarg->m_argc > 0 &&
               mlarg->m_argv != NULL &&
               IS_VALID_STRING(mlarg->m_argv[0]) == true)) {
      ret = mlarg->m_proc(mlarg->m_argc,
                          mlarg->m_argv,
                          mlarg->m_pre_hook,
                          mlarg->m_post_hook,
                          mlarg->m_ipcfd);
    } else {
      ret = SQC_RESULT_INVALID_ARGS;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline sqc_result_t
s_start_mainloop_thd(mainloop_proc_t proc, int argc, const char *const argv[],
                     sqc_mainloop_startup_hook_proc_t pre_hook,
                     sqc_mainloop_startup_hook_proc_t post_hook,
                     int ipcfd) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(proc != NULL &&
             argc > 0 &&
             argv != NULL &&
             IS_VALID_STRING(argv[0]) == true)) {

    s_args.m_proc = proc;
    s_args.m_argc = argc;
    s_args.m_argv = argv;
    s_args.m_pre_hook = pre_hook;
    s_args.m_post_hook = post_hook;
    s_args.m_ipcfd = ipcfd;

    ret = sqc_thread_create(&s_thd,
                               s_mainloop_thd_main,
                               NULL,
                               NULL,
                               "mainlooper",
                               (void *)&s_args);
    if (likely(ret == SQC_RESULT_OK)) {
      ret = sqc_thread_start(&s_thd, false);
      if (likely(ret == SQC_RESULT_OK)) {
        global_state_t s;
        shutdown_grace_level_t l;

        ret = global_state_wait_for(GLOBAL_STATE_STARTED, &s, &l, -1LL);
        if (likely(ret == SQC_RESULT_OK)) {
          if (unlikely(s != GLOBAL_STATE_STARTED)) {
            ret = SQC_RESULT_INVALID_STATE;
          }
        }
      }
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


static inline void
s_wait_mainloop_thd(void) {
  sqc_result_t r =
    sqc_thread_wait(&s_thd, MAINLOOP_SHUTDOWN_TIMEOUT);
  if (likely(r == SQC_RESULT_OK)) {
    sqc_thread_destroy(&s_thd);
  } else if (r == SQC_RESULT_TIMEDOUT) {
    sqc_msg_warning("the mainloop thread doesn't stop. cancel it.\n");
    r = sqc_thread_cancel(&s_thd);
    if (likely(r == SQC_RESULT_OK)) {
      r = sqc_thread_wait(&s_thd, MAINLOOP_SHUTDOWN_TIMEOUT);
      if (likely(r == SQC_RESULT_OK)) {
        sqc_thread_destroy(&s_thd);
      } else if (r == SQC_RESULT_TIMEDOUT) {
        sqc_msg_error("the mainloop thread still exists.\n");
      }
    } else {
      sqc_perror(r);
      sqc_msg_error("can't cancel the mainloop thread.\n");
    }
  } else {
    sqc_perror(r);
    sqc_msg_error("failed to wait the mainloop thread stop.\n");
  }
}





static inline sqc_result_t
s_prologue(int argc, const char *const argv[],
           sqc_mainloop_startup_hook_proc_t pre_hook,
           sqc_mainloop_startup_hook_proc_t post_hook,
           int ipcfd) {
  int st;
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  sqc_msg_info("%s: Initializing all the modules.\n",
                  sqc_get_command_name());

  (void)global_state_set(GLOBAL_STATE_INITIALIZING);

  ret = sqc_module_initialize_all(argc, (const char *const *)argv);
  if (likely(ret == SQC_RESULT_OK)) {
    sqc_msg_info("%s: All the modules are initialized.\n",
                    sqc_get_command_name());

    if (pre_hook != NULL) {
      ret = pre_hook(argc, argv);
      if (unlikely(ret != SQC_RESULT_OK)) {
        sqc_perror(ret);
        sqc_msg_error("pre-startup hook failed.\n");
        goto done;
      }
    }

    sqc_msg_info("%s: Starting all the modules.\n",
                    sqc_get_command_name());
    (void)global_state_set(GLOBAL_STATE_STARTING);

    ret = sqc_module_start_all();
    if (likely(ret == SQC_RESULT_OK)) {

      sqc_msg_info("%s: All the modules are started and ready to go.\n",
                      sqc_get_command_name());

      (void)global_state_set(GLOBAL_STATE_STARTED);

      if (post_hook != NULL) {
        ret = post_hook(argc, argv);
        if (unlikely(ret != SQC_RESULT_OK)) {
          sqc_perror(ret);
          sqc_msg_error("post-startup hook failed.\n");
          goto done;
        }
      }
    }
  }

done:
  if (likely(ret == SQC_RESULT_OK)) {
    if (s_do_pidfile == true) {
      s_gen_pidfile();
    }
    st = 0;
  } else {
    st = 1;
  }
  if (s_do_fork == true && ipcfd >= 0) {
    (void)s_prepare_wait_parent_exit();
    (void)write(ipcfd, &st, sizeof(int));
    (void)close(ipcfd);
    ret = s_wait_parent_exit();
    if (ret == SQC_RESULT_OK) {
      (void)sqc_log_set_multi_process(s_org_multi_proc_mode);
    } else {
      sqc_msg_warning("Wainting exit of parent process failed.\n");
      ret = SQC_RESULT_OK;
    }
  }

  return ret;
}


static inline sqc_result_t
s_epilogue(shutdown_grace_level_t l, sqc_chrono_t to) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  (void)global_state_set(GLOBAL_STATE_ACCEPT_SHUTDOWN);

  sqc_msg_info("%s: About to shutdown all the modules...\n",
                  sqc_get_command_name());

  ret = sqc_module_shutdown_all(l);
  if (likely(ret == SQC_RESULT_OK)) {
    ret = sqc_module_wait_all(to);
    if (likely(ret == SQC_RESULT_OK)) {
      sqc_msg_info("%s: Shutdown all the modules succeeded.\n",
                      sqc_get_command_name());
    } else if (ret == SQC_RESULT_TIMEDOUT) {
    do_cancel:
      sqc_msg_warning("%s: Shutdown failed. Trying to stop al the module "
                         "forcibly...\n", sqc_get_command_name());
      ret = sqc_module_stop_all();
      if (likely(ret == SQC_RESULT_OK)) {
        ret = sqc_module_wait_all(to);
        if (likely(ret == SQC_RESULT_OK)) {
          sqc_msg_warning("%s: All the modules are stopped forcibly.\n",
                             sqc_get_command_name());
        } else {
          sqc_perror(ret);
          sqc_msg_error("%s: can't stop all the modules.\n",
                           sqc_get_command_name());
        }
      }
    }
  } else if (ret == SQC_RESULT_TIMEDOUT) {
    goto do_cancel;
  }

  sqc_module_finalize_all();

  if (s_do_pidfile == true) {
    s_del_pidfile();
  }

  return ret;
}





static inline sqc_result_t
s_default_idle_proc(void) {
  return global_state_wait_for_shutdown_request(&s_gl,
         s_idle_proc_interval);
}


static inline sqc_result_t
s_default_mainloop(int argc, const char *const argv[],
                   sqc_mainloop_startup_hook_proc_t pre_hook,
                   sqc_mainloop_startup_hook_proc_t post_hook,
                   int ipcfd) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(s_do_abort == false)) {
    ret = s_prologue(argc, argv, pre_hook, post_hook, ipcfd);
    if (likely(ret == SQC_RESULT_OK &&
               s_do_abort == false)) {

      sqc_msg_info("%s is go and entering the main loop.\n",
                      sqc_get_command_name());

      while ((ret = s_default_idle_proc()) == SQC_RESULT_TIMEDOUT) {
        sqc_msg_debug(100, "%s: Wainitg for the shutdown request...\n",
                           sqc_get_command_name());
      }
      if (likely(ret == SQC_RESULT_OK)) {
        (void)global_state_set(GLOBAL_STATE_ACCEPT_SHUTDOWN);
      }

      ret = s_epilogue(s_gl, s_shutdown_timeout);
    }
  }

  return ret;
}





static sqc_result_t
s_callout_idle_proc(void *arg) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  shutdown_grace_level_t *glptr = (shutdown_grace_level_t *)arg;
  shutdown_grace_level_t gl = SHUTDOWN_UNKNOWN;

  sqc_msg_debug(10, "Checking the shutdown request...\n");

  ret = global_state_wait_for_shutdown_request(&gl, 0LL);
  if (unlikely(ret == SQC_RESULT_OK)) {
    sqc_msg_debug(1, "Got the shutdown request.\n");

    if (glptr != NULL) {
      *glptr = gl;
    }
    /*
     * Got a shutdown request. Accept it anyway.
     */
    (void)global_state_set(GLOBAL_STATE_ACCEPT_SHUTDOWN);
    /*
     * And return <0 to stop the callout main loop.
     */
    ret = SQC_RESULT_ANY_FAILURES;
  } else {
    ret = SQC_RESULT_OK;
  }

  return ret;
}


static inline sqc_result_t
s_callout_mainloop(int argc, const char *const argv[],
                   sqc_mainloop_startup_hook_proc_t pre_hook,
                   sqc_mainloop_startup_hook_proc_t post_hook,
                   int ipcfd) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(s_do_abort == false)) {
    bool handler_inited = false;

    /*
     * Initialize the callout handler.
     */
    ret = sqc_callout_initialize_handler(s_n_callout_workers,
                                            s_callout_idle_proc,
                                            (void *)&s_gl,
                                            s_idle_proc_interval,
                                            NULL);
    if (likely(ret == SQC_RESULT_OK)) {
      handler_inited = true;
      if (s_do_abort == false) {
        ret = s_prologue(argc, argv, pre_hook, post_hook, ipcfd);
        if (likely(ret == SQC_RESULT_OK &&
                   s_do_abort == false)) {
          sqc_result_t r1 = SQC_RESULT_ANY_FAILURES;
          sqc_result_t r2 = SQC_RESULT_ANY_FAILURES;


          sqc_msg_info("%s is go and entering the main loop.\n",
                          sqc_get_command_name());

          /*
           * Start the callout handler main loop.
           */
          ret = sqc_callout_start_main_loop();

          /*
           * No matter the ret is, here we have the main loop stopped. To
           * make it sure that the loop stopping call the stop API in case.
           */
          r1 = sqc_callout_stop_main_loop();

          r2 = s_epilogue(s_gl, s_shutdown_timeout);

          if (ret == SQC_RESULT_OK) {
            ret = r1;
          }
          if (ret == SQC_RESULT_OK) {
            ret = r2;
          }
        }
      }

      if (handler_inited == true) {
        sqc_callout_finalize_handler();
      }
    }
  }

  return ret;
}





static inline void
s_set_failsafe_handler(int sig) {
  sighandler_t h = NULL;

  if (sqc_signal(sig, SIG_CUR, &h) == SQC_RESULT_OK) {
    if (h == NULL || h == SIG_DFL || h == SIG_IGN) {
      sqc_result_t r = sqc_signal(sig, s_term_handler, NULL);
      if (r == SQC_RESULT_OK) {
        sqc_msg_warning("The signal %d seems not to be properly handled. "
                           "Set a decent handler.\n", sig);
      } else {
        sqc_perror(r);
        sqc_msg_warning("Can't set failsefa signal handler to "
                           "signal %d.\n", sig);
      }
    }
  }
}


static inline sqc_result_t
s_do_mainloop(mainloop_proc_t mainloopproc,
              int argc, const char *const argv[],
              sqc_mainloop_startup_hook_proc_t pre_hook,
              sqc_mainloop_startup_hook_proc_t post_hook,
              bool do_fork, bool do_pidfile, bool do_thread) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (likely(mainloopproc != NULL)) {

    s_do_pidfile = do_pidfile;
    s_do_fork = do_fork;

    /*
     * failsafe.
     */
    if (do_fork == false) {
      s_set_failsafe_handler(SIGINT);
    }
    s_set_failsafe_handler(SIGTERM);
    s_set_failsafe_handler(SIGQUIT);

    if (do_fork == true) {
      int ipcfds[2];
      int st;

      errno = 0;
      st = pipe(ipcfds);
      if (likely(st == 0)) {
        pid_t pid;

        /*
         * Set logger to lock file for parent/child simultaneous
         * logging forcibly fo now, then reset it to original mode
         * after the parent exits.
         */
        (void)sqc_log_get_multi_process(&s_org_multi_proc_mode);
        (void)sqc_log_set_multi_process(true);

        errno = 0;
        pid = fork();
        if (pid > 0) {
          /*
           * Parent.
           */
          int child_st = -1;
          ssize_t ssz;

          (void)close(ipcfds[1]);

          errno = 0;
          ssz = read(ipcfds[0], &child_st, sizeof(int));
          if (likely(ssz == sizeof(int))) {
            ret = (child_st == 0) ?
                  SQC_RESULT_OK : SQC_RESULT_ANY_FAILURES;
          } else {
            ret = SQC_RESULT_POSIX_API_ERROR;
          }
          (void)close(ipcfds[0]);
        } else if (pid == 0) {
          /*
           * Child.
           */
          (void)close(ipcfds[0]);
          s_daemonize(ipcfds[1]);
          if (sqc_log_get_destination(NULL) == SQC_LOG_EMIT_TO_FILE) {
            (void)sqc_log_reinitialize();
          }
          if (do_thread == false) {
            ret = mainloopproc(argc, argv, pre_hook, post_hook, ipcfds[1]);
          } else {
            ret = s_start_mainloop_thd(mainloopproc, argc, argv,
                                       pre_hook, post_hook, ipcfds[1]);
          }
        } else {
          ret = SQC_RESULT_POSIX_API_ERROR;
        }

      } else {
        ret = SQC_RESULT_POSIX_API_ERROR;
      }

    } else {	/* do_fork == true */

      if (do_thread == false) {
        ret = mainloopproc(argc, argv, pre_hook, post_hook, -1);
      } else {
        ret = s_start_mainloop_thd(mainloopproc, argc, argv,
                                   pre_hook, post_hook, -1);
      }

    }

  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}





void
sqc_abort_before_mainloop(void) {
  s_do_abort = true;
  mbar();
}


bool
sqc_is_abort_before_mainloop(void) {
  return s_do_abort;
}


sqc_result_t
sqc_set_pidfile(const char *file) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (IS_VALID_STRING(file) == true) {
    const char *newfile = strdup(file);
    if (IS_VALID_STRING(newfile) == true) {
      if (IS_VALID_STRING(s_pidfile) == true) {
        free((void *)s_pidfile);
      }
      s_pidfile = newfile;
      ret = SQC_RESULT_OK;
    } else {
      /*
       * Note: scan-build warns a potentialy memory leak here and yes
       * it is intended to be.
       */
      ret = SQC_RESULT_NO_MEMORY;
    }
  } else {
    ret = SQC_RESULT_INVALID_ARGS;
  }

  return ret;
}


void
sqc_create_pidfile(void) {
  s_gen_pidfile();
}


void
sqc_remove_pidfile(void) {
  s_del_pidfile();
}


sqc_result_t
sqc_mainloop_set_callout_workers_number(size_t n) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;

  if (n > 4) {
    ret = SQC_RESULT_TOO_LARGE;
  } else {
    s_n_callout_workers = n;
    ret = SQC_RESULT_OK;
  }

  return ret;
}


sqc_result_t
sqc_mainloop_set_shutdown_check_interval(sqc_chrono_t nsec) {
  if (nsec < ONE_SEC) {
    return SQC_RESULT_TOO_SMALL;
  } else if (nsec >= (10LL * ONE_SEC)) {
    return SQC_RESULT_TOO_LARGE;
  }

  s_idle_proc_interval = nsec;
  return SQC_RESULT_OK;
}


sqc_result_t
sqc_mainloop_set_shutdown_timeout(sqc_chrono_t nsec) {
  if (nsec < ONE_SEC) {
    return SQC_RESULT_TOO_SMALL;
  } else if (nsec >= (30LL * ONE_SEC)) {
    return SQC_RESULT_TOO_LARGE;
  }

  s_shutdown_timeout = nsec;
  return SQC_RESULT_OK;
}





sqc_result_t
sqc_mainloop(int argc, const char *const argv[],
                sqc_mainloop_startup_hook_proc_t pre_hook,
                sqc_mainloop_startup_hook_proc_t post_hook,
                bool do_fork, bool do_pidfile, bool do_thread) {
  return s_do_mainloop(s_default_mainloop, argc, argv,
                       pre_hook, post_hook,
                       do_fork, do_pidfile, do_thread);
}


sqc_result_t
sqc_mainloop_with_callout(int argc, const char *const argv[],
                             sqc_mainloop_startup_hook_proc_t pre_hook,
                             sqc_mainloop_startup_hook_proc_t post_hook,
                             bool do_fork, bool do_pidfile,
                             bool do_thread) {
  return s_do_mainloop(s_callout_mainloop, argc, argv,
                       pre_hook, post_hook,
                       do_fork, do_pidfile, do_thread);
}


void
sqc_mainloop_wait_thread(void) {
  if (s_thd != NULL) {
    s_wait_mainloop_thd();
  }
}


sqc_result_t
sqc_mainloop_prologue(int argc, const char *const argv[],
                         sqc_mainloop_startup_hook_proc_t pre_hook,
                         sqc_mainloop_startup_hook_proc_t post_hook) {
  return s_prologue(argc, argv, pre_hook, post_hook, -1);
}


sqc_result_t
sqc_mainloop_epilogue(shutdown_grace_level_t l, sqc_chrono_t to) {
  return s_epilogue(l, to);
}
