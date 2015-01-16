#include <check.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include "../duplex_internal.h"

#define NAME "Join Tests"

static void* inside_join(void* args) {
  ck_assert_msg(args != NULL, "args should not be NULL");

  pthread_t main = *(pthread_t*)args;
  pthread_t this = pthread_self();

  ck_assert_msg(pthread_equal(main, this) == 0, "pthreads should be different");

  return NULL;
}

START_TEST(test_join)
{

  duplex_peer *peer = duplex_peer_new();

  ck_assert_msg(peer != NULL, "peer is NULL! errno: %s", strerror(errno));

  pthread_t self = pthread_self();

  duplex_joiner joiner;
  joiner.function = inside_join;
  joiner.args = &self;

  void* result = duplex_peer_join_th(peer, &joiner);

  ck_assert_msg(result == NULL, "result isn't NULL");

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
  srunner_run_all(sr, CK_NORMAL);

  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);

  return number_failed;
}
