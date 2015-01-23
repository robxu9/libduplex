#include <assert.h>
#include <check.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../duplex_internal.h"

#include <libssh/buffer.h>
#include <libssh/session.h>
#include <libssh/socket.h>
#include <libssh/libssh.h>
#include <libssh/callbacks.h>

#define NAME "Tests to goduplex"

// gosocket should be in endpoint format
// e.g. unix:///tmp/duplex_socket
char* gosocket = NULL;

int init_called = 0;

// START_TEST(test_peer_duplex_connect)
// {
//   // fork/no-fork guard
//   if (!init_called) {
//     init_called = 1;
//     duplex_init();
//   }
//
//   duplex_peer *peer = duplex_peer_new();
//
//   ck_assert_int_eq(duplex_peer_connect(peer, gosocket), ERR_NONE);
//
//   sleep(10);
//
//   ck_assert_int_eq(duplex_peer_close(peer), ERR_NONE);
//   ck_assert_int_eq(duplex_peer_free(peer), 0);
// } END_TEST

START_TEST(test_peer_libssh_connect)
{
  // fork/no-fork guard
  if (!init_called) {
    init_called = 1;
    duplex_init();
  }

  struct ssh_session_struct* session = ssh_new();
  assert(session != NULL);

  int port = 9999;
  ssh_options_set(session, SSH_OPTIONS_HOST, "127.0.0.1");
  ssh_options_set(session, SSH_OPTIONS_PORT, &port);

  int log_func_level = SSH_LOG_FUNCTIONS;
  ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &log_func_level);

  int rc = ssh_connect(session);
  assert(rc == SSH_OK);

  // authenticate
  int auth = ssh_userauth_publickey_auto(session, NULL, NULL);
  assert(auth == SSH_AUTH_SUCCESS);

  ssh_set_blocking(session, 0);

  sleep(1);

} END_TEST

void
exit_func(void)
{
  if (init_called)
    duplex_cleanup();
}

Suite*
make_suite()
{
  Suite *s = suite_create(NAME);

  TCase *tc = tcase_create("Testcases");
//  tcase_add_test(tc, test_peer_duplex_connect);

  char* libssh = getenv("GODUPLEX_TCP");
  if (libssh != NULL)
    tcase_add_test(tc, test_peer_libssh_connect);
  else
    fprintf(stderr, "skipping libssh direct connect testcase (GODUPLEX_TCP env missing)");

  suite_add_tcase(s, tc);

  return s;
}

int
main(void)
{
  gosocket = getenv("GODUPLEX");
  if (gosocket == NULL) {
    fprintf(stderr, "GODUPLEX env variable not set...");
    fprintf(stderr, "doing nothing.");
    return 1;
  }

  atexit(exit_func);

  int number_failed;
  Suite *s = make_suite();

  SRunner *sr = srunner_create(s);
  srunner_run_all(sr, CK_ENV);

  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  return number_failed;
}
