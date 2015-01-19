#include <check.h>
#include <errno.h>
#include <pthread.h>
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

START_TEST(test_peer_endpoint_parse)
{
  char endpoint_tmp[1024];
  int result;

  char* hostname;
  int port = 0;

  char* endpoints[] = { "unix:///tmp/test_protocol", "tcp://127.0.0.1:333", "tcp://127.0.0.1", "malformed", "unknown://some_protocol" };
  int results[] = { 1, 0, 0, -1, -1 };
  char* hostnames[] = { "/tmp/test_protocol", "127.0.0.1", "127.0.0.1", "", "" };
  int ports[] = { 0, 333, 2259, 0, 0 };

  int i;
  for (i = 0; i < 5; i++) {
    strcpy(endpoint_tmp, endpoints[i]);

    result = _duplex_endpoint_parse(endpoint_tmp, &hostname, &port);
    ck_assert_msg(result == results[i], "result == results[i] failed: result = %d, results[i] = %d, endpoint = %s", result, results[i], endpoints[i]);

    if (result != -1) {
      ck_assert_str_eq(hostname, hostnames[i]);

      if (result != 1) { // if not unix, since unix doesn't touch ports
        ck_assert_int_eq(port, ports[i]);
      }
    }
  }

} END_TEST

Suite*
make_suite()
{
  Suite *s = suite_create(NAME);

  TCase *tc = tcase_create("Testcases");
  tcase_add_test(tc, test_peer_updown);
  tcase_add_test(tc, test_peer_endpoint_parse);

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
