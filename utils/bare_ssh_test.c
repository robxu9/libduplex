#include <assert.h>

#include <libssh/libssh.h>

int
main(void)
{
  ssh_threads_set_callbacks(ssh_threads_get_pthread());
  assert(ssh_init() == 0);

  ssh_session session = ssh_new();
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

  // set up ssh_event monitor
  ssh_event monitor = ssh_event_new();
  assert(monitor != NULL);

  assert(ssh_event_add_session(monitor, session) != SSH_ERROR);

  assert(ssh_event_dopoll(monitor, 1000) != SSH_ERROR);

  sleep(1);

  assert(ssh_finalize() == 0);

  return 0;
}
