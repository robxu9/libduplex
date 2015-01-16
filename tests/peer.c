#include <check.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "../duplex_internal.h"

#define NAME "Peer Tests"

START_TEST(test_peer_updown)
{
  duplex_init();

  duplex_peer *peer = duplex_peer_new();

  ck_assert_msg(peer != NULL, "peer is NULL! errno: %s", strerror(errno));

  duplex_err err = duplex_peer_close(peer);
  ck_assert_msg(err == ERR_NONE, "err != ERR_NONE: %d", err);

  // try again to make sure we hit ERR_CLOSED
  err = duplex_peer_close(peer);
  ck_assert_msg(err == ERR_CLOSED, "err != ERR_CLOSED: %d", err);

  // now free
  int result_free = duplex_peer_free(peer);
  ck_assert_msg(result_free == 0, "result_free != 0: %s", strerror(result_free));

  duplex_cleanup();
} END_TEST

Suite*
make_suite()
{
  Suite *s = suite_create(NAME);

  TCase *tc = tcase_create("Testcases");
  tcase_add_test(tc, test_peer_updown);

  suite_add_tcase(s, tc);

  return s;
}

int
main(void)
{
  int number_failed;
  Suite *s = make_suite();

  SRunner *sr = srunner_create(s);
  srunner_run_all(sr, CK_NORMAL);

  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  return number_failed;
}
