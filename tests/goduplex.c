#include <check.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../duplex_internal.h"

#include <libssh/libssh.h>

#define NAME "Tests to goduplex"

// gosocket should be in endpoint format
// e.g. unix:///tmp/duplex_socket
char* gosocket = NULL;

START_TEST(test_peer_connect)
{
  duplex_init();

  duplex_peer *peer = duplex_peer_new();

  ck_assert_int_eq(duplex_peer_connect(peer, gosocket), ERR_NONE);

  ck_assert(duplex_peer_close(peer) == ERR_NONE);
  ck_assert_int_eq(duplex_peer_free(peer), 0);
  duplex_cleanup();

  fflush(stdout);
} END_TEST

Suite*
make_suite()
{
  Suite *s = suite_create(NAME);

  TCase *tc = tcase_create("Testcases");
  tcase_add_test(tc, test_peer_connect);

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

  int number_failed;
  Suite *s = make_suite();

  SRunner *sr = srunner_create(s);
  srunner_run_all(sr, CK_ENV);

  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  return number_failed;
}
