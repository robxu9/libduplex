#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <libssh/libssh.h>
#include <libssh/callbacks.h>
#include <libssh/server.h>

#include "duplex_internal.h"
#include "uthash.h"

#ifndef KEYS_FOLDER
  #ifdef _WIN32
    #define KEYS_FOLDER
  #elif __APPLE__
    #define KEYS_FOLDER "/etc/"
  #else
    #define KEYS_FOLDER "/etc/ssh/"
  #endif
#endif

// To be passed into pthread_create.
static void* duplex_peer_handle_joiners(void *arg) {
  duplex_peer *peer = (duplex_peer*) arg;

  int socket = peer->socket[1];

  int nfds = socket + 1;
  struct timeval timeout;
  memset(&timeout, 0, sizeof(struct timeval)); // so secs/msecs ==

  // Listen and handle requests
  while(1) {
    fd_set toread;

    FD_ZERO(&toread);
    FD_SET(socket, &toread);

    int res = select(nfds, &toread, NULL, NULL, &timeout);
    assert(res >= 0);

    if (res != 0) { // we have stuff!
      duplex_joiner *joiner = NULL;

      assert(read(socket, &joiner, sizeof(void*)) == sizeof(void*));

      joiner->error = joiner->function(joiner->args, &joiner->result);

      char ok[] = { 0 };
      assert(write(socket, ok, sizeof(char)) == sizeof(char));

      // if peer->closed is set, break and close our end of the socket
      if (peer->closed)
        break;
    }

    // always initiate polling
    ssh_event_dopoll(peer->monitor, 100);
  }

  // close socket[1] (close function for peer will close socket[0])
  close(socket);

  // and end this thread
  pthread_exit(NULL);
}

// To be used as the global request handler for ssh sessions
void _duplex_peer_global_handle(ssh_session session, ssh_message message, void *userdata) {
  duplex_peer *peer = (duplex_peer*) userdata;

  fprintf(stderr, "got here");

  assert(peer != NULL);
}

duplex_peer* duplex_peer_new() {
  duplex_peer *peer = calloc(1, sizeof(duplex_peer));
  if (peer == NULL)
    return NULL; // errno should still be set

  peer->monitor = ssh_event_new();
  if (peer->monitor == NULL) {
    // fail...
    free(peer);
    return NULL;
  }

  peer->bind = ssh_bind_new();
  if (peer->bind == NULL) {
    // fail...
    ssh_event_free(peer->monitor);
    free(peer);
    return NULL;
  }

  // set defaults
  ssh_bind_options_set(peer->bind, SSH_BIND_OPTIONS_RSAKEY, KEYS_FOLDER "ssh_host_rsa_key");
  ssh_bind_options_set(peer->bind, SSH_BIND_OPTIONS_DSAKEY, KEYS_FOLDER "ssh_host_dsa_key");
  ssh_bind_options_set(peer->bind, SSH_BIND_OPTIONS_ECDSAKEY, KEYS_FOLDER "ssh_host_ecdsa_key");

  // FIXME when ops are implemented, set options, use SSH_BIND_OPTIONS_HOSTKEY to set custom keys

  ssh_bind_set_blocking(peer->bind, 0);

  DEBUG_FUNC(
  int log_func_level = SSH_LOG_FUNCTIONS;
  ssh_bind_options_set(peer->bind, SSH_BIND_OPTIONS_LOG_VERBOSITY, &log_func_level);
  );

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, peer->socket)) {
    // failed! make sure we save errno, cleanup, leave
    int old_errno = errno;
    ssh_bind_free(peer->bind);
    ssh_event_free(peer->monitor);
    free(peer);
    errno = old_errno;
    return NULL;
  }

  int mutex_init = pthread_mutex_init(&peer->mutex, NULL);
  if (mutex_init) {
    // failed! clean up, set errno, die
    close(peer->socket[0]);
    close(peer->socket[1]);

    ssh_bind_free(peer->bind);
    ssh_event_free(peer->monitor);
    free(peer);

    errno = mutex_init;
    return NULL;
  }

  // start the thread to get session
  int thread_init = pthread_create(&peer->thread, NULL,
                    duplex_peer_handle_joiners, peer);
  if (thread_init) {
    // failed! clean up, set errno, die
    assert(pthread_mutex_destroy(&peer->mutex) == 0);

    close(peer->socket[0]);
    close(peer->socket[1]);

    ssh_bind_free(peer->bind);
    ssh_event_free(peer->monitor);
    free(peer);

    errno = thread_init;
    return NULL;
  }

  // cool, we're ready.
  return peer;
}

// joiner function
static duplex_err _duplex_peer_close(void* args, void** result) {
  duplex_peer *peer = (duplex_peer*) args;

  // clean up sessions here

  // now set closed on the peer
  peer->closed = 1;

  return ERR_NONE;
}

duplex_err duplex_peer_close(duplex_peer *peer) {
  duplex_joiner joiner;
  joiner.function = _duplex_peer_close;
  joiner.args = peer;

  return _duplex_join(peer, &joiner);
}

int duplex_peer_free(duplex_peer *peer) {
  if (!peer->closed) {
    return EBUSY;
  }

  assert(pthread_mutex_destroy(&peer->mutex) == 0);
  free(peer);
  return 0;
}

// duplex_peer_option_set
// duplex_peer_option_get_str
// duplex_peer_option_get_int

struct _duplex_peer_endpoint_s {
  duplex_peer *peer;
  const char* endpoint;
};

static duplex_err _duplex_peer_connect(void* args, void** result) {
  struct _duplex_peer_endpoint_s *s = (struct _duplex_peer_endpoint_s*) args;

  duplex_peer *peer = s->peer;
  const char* endpoint = s->endpoint;

  ssh_session session = ssh_new();
  if (session == NULL)
    return ERR_LIBSSH;

  // parse the endpoint
  char* endpointcpy = malloc(strlen(endpoint) + 1);
  if (endpointcpy == NULL) {
    ssh_free(session);
    return ERR_ALLOC;
  }

  strcpy(endpointcpy, endpoint);

  char* hostname;
  int port;
  switch (_duplex_endpoint_parse(endpointcpy, &hostname, &port)) {
  case 0: // tcp
    ssh_options_set(session, SSH_OPTIONS_HOST, hostname);
    ssh_options_set(session, SSH_OPTIONS_PORT, &port);
    break;
  case 1: // unix
  {
    // path is in hostname
    int fd = _duplex_socket_unix_connect(hostname);
    if (fd < 0) // guess what, we couldn't connect to this path
      goto connect_bad_endpoint;

    char host[1023];
    assert(gethostname(host, 1023) == 0);

    char* full_endpoint = malloc(strlen(host) + strlen(hostname) + 2);
    assert(full_endpoint != NULL);

    strcpy(full_endpoint, host);
    strcat(full_endpoint, ":");
    strcat(full_endpoint, hostname);

    ssh_options_set(session, SSH_OPTIONS_HOST, full_endpoint);
    ssh_options_set(session, SSH_OPTIONS_FD, &fd);

    free(full_endpoint);
    break;
  }
  default:
    goto connect_bad_endpoint;
  }

  DEBUG_FUNC(
  int log_func_level = SSH_LOG_FUNCTIONS;
  ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &log_func_level);
  );

  // set callbacks
  struct ssh_callbacks_struct *cb = calloc(1, sizeof(struct ssh_callbacks_struct));

  cb->userdata = peer;
  cb->global_request_function = _duplex_peer_global_handle;

  ssh_callbacks_init(cb);
  assert(ssh_set_callbacks(session, cb) == SSH_OK);

  // moment of truth
  int rc = ssh_connect(session);
  if (rc != SSH_OK) {
    // that didn't work
    // FIXME: if unix socket, ssh_get_fd and close fd? LEAK?
    free(endpointcpy);
    ssh_free(session);
    return ERR_FAIL;
  }

  // FIXME touch known_hosts?

  // authenticate
  int auth = ssh_userauth_publickey_auto(session, NULL, NULL);
  if (auth != SSH_AUTH_SUCCESS) {
    // failed to authenticate
    // FIXME: if unix socket, ssh_get_fd and close fd? LEAK?
    ssh_disconnect(session);
    ssh_free(session);
    free(endpointcpy);
    return ERR_AUTH;
  }

  ssh_set_blocking(session, 0);

  assert(ssh_event_add_session(peer->monitor, session) == SSH_OK);

  // FIXME add callback for disgraceful disconnects

  // FIXME select on the following conditions:
  // ~> get global request from "@duplex-greeting" with name, reply TRUE
  // ~> hit 5 second timeout and kill, return ERR_FAIL

  // insert into peer
  duplex_peer_session *peer_session = malloc(sizeof(duplex_peer_session));
  assert(peer_session != NULL); // I don't even if it's NULL

  strcpy(peer_session->endpoint, endpoint);
  peer_session->session = session;

  HASH_ADD_STR(peer->sessions, endpoint, peer_session);

  free(endpointcpy);

  return ERR_NONE;

connect_bad_endpoint:
  free(endpointcpy);
  ssh_free(session);
  return ERR_ARGS;
}

duplex_err duplex_peer_connect(duplex_peer *peer, const char* endpoint) {
  duplex_joiner joiner;
  joiner.function = _duplex_peer_connect;

  struct _duplex_peer_endpoint_s args;
  args.peer = peer;
  args.endpoint = endpoint;

  joiner.args = &args;

  duplex_err err = _duplex_join(peer, &joiner);
  if (err != ERR_NONE) // something went wrong?
    return err;

  return joiner.error;
}

// duplex_peer_disconnect
// duplex_peer_connected_len
// duplex_peer_connected

// - only one instance of ssh_bind is needed
// - manage fds individually
// - call ssh_bind_accept_fd instead
// - makes it easier to select() on bound fds.
static duplex_err _duplex_peer_bind(void* args, void** result) {
  struct _duplex_peer_endpoint_s *s = (struct _duplex_peer_endpoint_s*) args;

  duplex_peer *peer = s->peer;
  const char* endpoint = s->endpoint;

  duplex_err err = ERR_NONE;

  char* endpointcpy = malloc(strlen(endpoint) + 1);
  if (endpointcpy == NULL)
    return ERR_ALLOC;

  strcpy(endpointcpy, endpoint);

  char* hostname;
  int port;

  int type = _duplex_endpoint_parse(endpointcpy, &hostname, &port);
  int fd;

  switch(type) {
  case 0: // tcp
    fd = _duplex_socket_tcp_bind(hostname, port);
  case 1: // unix
    fd = _duplex_socket_unix_bind(hostname);
  default: // ??
    goto bind_bad_cleanup;
  }

  if (fd < 0) // couldn't make
    goto bind_bad_cleanup;

  // set O_NONBLOCK
  int flags = fcntl(fd, F_GETFL, 0);
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK)) {
    // why.
    close(fd);
    goto bind_bad_cleanup;
  }

  // mark it as ready for listening
  // FIXME: let listening queue be adjustable!
  if (listen(fd, 10) < 0) {
    // fail.
    close(fd);
    goto bind_bad_cleanup;
  }

  // add to peer->servers (duplex_peer_fd)
  duplex_peer_fd *peer_fd = malloc(sizeof(duplex_peer_fd));
  assert(peer_fd != NULL);

  strcpy(peer_fd->endpoint, endpoint);
  peer_fd->fd = fd;

  HASH_ADD_STR(peer->servers, endpoint, peer_fd);

  goto bind_cleanup;

bind_bad_cleanup:
  err = ERR_ARGS;
bind_cleanup:
  free(endpointcpy);
  return err;
}

duplex_err duplex_peer_bind(duplex_peer *peer, const char* endpoint) {
  duplex_joiner joiner;
  joiner.function = _duplex_peer_bind;

  struct _duplex_peer_endpoint_s args;
  args.peer = peer;
  args.endpoint = endpoint;

  joiner.args = &args;

  duplex_err err = _duplex_join(peer, &joiner);
  if (err != ERR_NONE)
    return err;

  return joiner.error;
}
