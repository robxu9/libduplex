#include <assert.h>
#include <check.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../duplex_internal.h"

#include <libssh/libssh.h>
#include <libssh/callbacks.h>

#define NAME "Tests to goduplex"

// gosocket should be in endpoint format
// e.g. unix:///tmp/duplex_socket
char* gosocket = NULL;

int init_called = 0;

START_TEST(test_peer_duplex_connect)
{
  // fork/no-fork guard
  if (!init_called) {
    init_called = 1;
    duplex_init();
  }

  duplex_peer *peer = duplex_peer_new();

  ck_assert_int_eq(duplex_peer_connect(peer, gosocket), ERR_NONE);

  sleep(5);

  ck_assert_int_eq(duplex_peer_close(peer), ERR_NONE);
  ck_assert_int_eq(duplex_peer_free(peer), 0);
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
  tcase_add_test(tc, test_peer_duplex_connect);

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
