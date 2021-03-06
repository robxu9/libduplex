#include <check.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "../duplex_internal.h"

#define NAME "Join Tests"

static duplex_err inside_join(void* args, void** result) {
  ck_assert_msg(args != NULL, "args should not be NULL");

  pthread_t main = *(pthread_t*)args;
  pthread_t this = pthread_self();

  ck_assert_msg(pthread_equal(main, this) == 0, "pthreads should be different");

  *result = "OK";
  return ERR_NONE;
}

START_TEST(test_join)
{
  duplex_init();

  duplex_peer *peer = duplex_peer_new();

  ck_assert_msg(peer != NULL, "peer is NULL! errno: %s", strerror(errno));

  pthread_t self = pthread_self();

  duplex_joiner joiner;
  joiner.function = inside_join;
  joiner.args = &self;

  duplex_err err = _duplex_join(peer, &joiner);

  ck_assert_msg(err == ERR_NONE, "err != ERR_NONE: %d", err);

  ck_assert_str_eq(joiner.result, "OK");

  // again for posterity's sake
  err = _duplex_join(peer, &joiner);

  ck_assert_msg(err == ERR_NONE, "err != ERR_NONE: %d", err);

  ck_assert_str_eq(joiner.result, "OK");

  duplex_cleanup();
} END_TEST

Suite*
make_suite()
{
  Suite *s = suite_create(NAME);

  TCase *tc = tcase_create("Testcases");
  tcase_add_test(tc, test_join);

  suite_add_tcase(s, tc);

  return s;
}

int
main(void)
{
  int number_failed;
  Suite *s = make_suite();

  SRunner *sr = srunner_create(s);
  srunner_run_all(sr, CK_ENV);

  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  return number_failed;
}
