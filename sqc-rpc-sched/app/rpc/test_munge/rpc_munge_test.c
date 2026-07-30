#include "unity.h"

#include "rpc.pb-c.c"
#include "rpc_msg_util.c"
#include "rpc_session_internal.c"
#include "rpc_munge.c"

void setUp(void) {
}

void tearDown(void) {
}

static int
get_listening_socket(uint16_t port) {
  int ret, fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in sin = {0};

  if (fd < 0) {
    return -1;
  }
  sin.sin_port = htons(port);
  ret = bind(fd, &sin, sizeof(sin));
  if (ret < 0) {
    return -1;
  }
  ret = listen(fd, SOMAXCONN);
  if (ret < 0) {
    return -1;
  }

  return fd;
}

static int
do_accept(int sock1, struct sockaddr_storage *ss, socklen_t *ss_len) {
  int sock2 = accept(sock1, (struct sockaddr *) ss, ss_len);

  return sock2;
}

void test_rpc_munge_client_get_cred(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  rpc_session_client_t tmp_rpc_session = NULL;
  char *auth_data = NULL;
  size_t auth_datalen = 0u;
  int sock;
  uid_t uid;
  gid_t gid;
  char *cred_addr_port = NULL;

  tmp_rpc_session = malloc(sizeof(struct rpc_session_client));
  tmp_rpc_session->session_ = NULL;
  tmp_rpc_session->tls_conf_ = NULL;
  tmp_rpc_session->jwt_ctx_ = NULL;

  sock = get_listening_socket(33375);
  TEST_ASSERT_NOT_EQUAL(-1, sock);

  ret = sqc_session_create_client(&(tmp_rpc_session->session_), 0u, "127.0.0.1:33375", true);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, ret);
  ret = sqc_endpoint_connect((sqc_endpoint_t *)&(tmp_rpc_session->session_));
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, ret);
  ret = rpc_munge_client_get_cred(&tmp_rpc_session, &auth_data, &auth_datalen);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, ret);

  fprintf(stderr, "cred: %s\n", auth_data);
  ret = s_munge_decode(auth_data, &uid, &gid, &cred_addr_port);
  TEST_ASSERT_EQUAL(SQC_RESULT_OK, ret);
  fprintf(stderr, "cred: %s\n", cred_addr_port);
  close(sock);
}

void test_rpc_munge_server_validate_cred(void) {
  sqc_result_t ret = SQC_RESULT_ANY_FAILURES;
  rpc_session_client_t session_client = NULL;
  rpc_session_server_t session_server = NULL;
  int sock1, sock2, status;
  struct sockaddr_storage ss;
  socklen_t ss_len;

  session_client = malloc(sizeof(struct rpc_session_client));
  session_client->session_ = NULL;
  session_client->tls_conf_ = NULL;
  session_client->jwt_ctx_ = NULL;

  session_server = malloc(sizeof(struct rpc_session_server));
  session_client->session_ = NULL;
  session_client->tls_conf_ = NULL;
  session_client->jwt_ctx_ = NULL;

  if (fork() == 0) {
    ret = rpc_session_client_create_from_conf_dir(&session_client, "127.0.0.1:33375", true, RPC_AUTH_METHOD_MUNGE, ".sqc_rpc_sched");
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, ret);

    rpc_session_client_destroy(&session_client);
    exit(0);
  } else {
    sock1 = get_listening_socket(33375);
    TEST_ASSERT_NOT_EQUAL(-1, sock1);
    fprintf(stderr, "do_accept() in\n");
    sock2 = do_accept(sock1, &ss, &ss_len);
    fprintf(stderr, "do_accept() %d out\n", sock2);
    TEST_ASSERT_NOT_EQUAL(-1, sock2);
    fprintf(stderr, "rpc_session_server_create_from_conf_dir() in\n");
    ret = rpc_session_server_create_from_conf_dir(&session_server, sock2, &ss, ss_len, "sqc_rpc_sched");
    fprintf(stderr, "rpc_session_server_create_from_conf_dir() out\n");
    TEST_ASSERT_EQUAL(SQC_RESULT_OK, ret);
    do {
      fprintf(stderr, "rpc_session_server_process_request() in\n");
      ret = rpc_session_server_process_request(&session_server);
      fprintf(stderr, "rpc_session_server_process_request() out\n");
      TEST_ASSERT_EQUAL(SQC_RESULT_OK, ret);
    } while (ret == SQC_RESULT_OK);
    wait(&status);
    fprintf(stderr, "wait done %d\n", status);
    close(sock2);
    close(sock1);
    rpc_session_server_destroy(&session_server);
  }
}

