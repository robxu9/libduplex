#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <libssh/libssh.h>

#include "duplex_internal.h"
#include "uthash.h"

// To be passed into pthread_create.
static void* duplex_peer_handle_joiners(void *arg) {
  duplex_peer *peer = (duplex_peer*) arg;

  int socket = peer->socket[1];

  // Listen and handle requests
  while(1) {
    duplex_joiner *joiner = NULL;

    assert(read(socket, &joiner, sizeof(void*)) == sizeof(void*));

    joiner->error = joiner->function(joiner->args, &joiner->result);

    char ok[] = { 0 };
    assert(write(socket, ok, sizeof(char)) == sizeof(char));

    // if peer->closed is set, break and close our end of the socket
    if (peer->closed)
      break;
  }

  // close socket[1] (close function for peer will close socket[0])
  close(socket);

  // and end this thread
  pthread_exit(NULL);
}

duplex_peer* duplex_peer_new() {
  duplex_peer *peer = calloc(1, sizeof(duplex_peer));
  if (peer == NULL)
    return NULL; // errno should still be set

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, peer->socket)) {
    // failed! make sure we save errno, cleanup, leave
    int old_errno = errno;
    free(peer);
    errno = old_errno;
    return NULL;
  }

  int mutex_init = pthread_mutex_init(&peer->mutex, NULL);
  if (mutex_init) {
    // failed! clean up, set errno, die
    close(peer->socket[0]);
    close(peer->socket[1]);

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

  return duplex_peer_join_th(peer, &joiner);
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

struct _duplex_peer_connect_s {
  duplex_peer *peer;
  const char* endpoint;
};

static duplex_err _duplex_peer_connect(void* args, void** result) {
  struct _duplex_peer_connect_s *s = (struct _duplex_peer_connect_s*) args;

  duplex_peer *peer = s->peer;
  const char* endpoint = s->endpoint;

  // parse the endpoint
  char* endpointcpy = malloc(strlen(endpoint) + 1);
  if (endpointcpy == NULL)
    return ERR_ALLOC;

  // see if we can grab the protocol
  char* tok = strtok(endpointcpy, "://");
  if (tok == NULL) {// there is none
    free(endpointcpy);

    return ERR_ARGS;
  }

  ssh_session session = ssh_new();
  if (session == NULL) {
    free(endpointcpy);

    return ERR_LIBSSH;
  }

  // handle based on the protocol
  if (!strcmp(tok, "tcp")) {
    // tcp://location:port
    tok = strtok(NULL, "://");
    if (tok == NULL)
      goto connect_bad_endpoint;
    // location:port

    // now split by :
    tok = strtok(tok, ":");
    if (tok == NULL)
      goto connect_bad_endpoint;
    // location
    char* location = tok;

    // port
    char* port = "2259";
    tok = strtok(NULL, ":");
    if (tok != NULL)
      port = tok;

    ssh_options_set(session, SSH_OPTIONS_HOST, location);
    ssh_options_set(session, SSH_OPTIONS_PORT_STR, port);

  } else if (!strcmp(tok, "unix")) {
    // unix:///path/to/socket
    tok = strtok(NULL, "://");
    if (tok == NULL)
      goto connect_bad_endpoint;
    // /path/to/socket

    // try opening a path to this socket

    int fd;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0) < 0) {
      // we can't even make the fd /sigh
      goto connect_bad_endpoint;
    }

    struct sockaddr_un sa; // socket address
    memset(&sa, 0, sizeof(sa)); // clear the stack alloc

    sa.sun_family = AF_UNIX;
    strcpy(sa.sun_path, tok);

    int connect_len = strlen(sa.sun_path) + sizeof(sa.sun_family);

    if (connect(fd, (struct sockaddr*)sa, connect_len)) {
      // we can't connect... so time to exit
      goto connect_bad_endpoint;
    }

    char hostname[1023];
    assert(gethostname(hostname, 1023) == 0);

    char* full_endpoint = malloc(1023 + strlen(tok) + 2);
    strcpy(full_endpoint, hostname);
    strcat(full_endpoint, ":");
    strcat(full_endpoint, tok);

    ssh_options_set(session, SSH_OPTIONS_HOST, full_endpoint);
    ssh_options_set(session, SSH_OPTIONS_FD, fd);
    free(full_endpoint);
  } else {
    goto connect_bad_endpoint;
  }

  // moment of truth
  int rc = ssh_connect(session);
  if (rc != SSH_OK) {
    // that didn't work
    free(endpointcpy);
    ssh_free(session);
    return ERR_FAIL;
  }

  // FIXME send greeting?

  ssh_set_blocking(session, 0);

  // insert into peer
  duplex_peer_session peer_session = malloc(sizeof(duplex_peer_session));
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

  struct _duplex_peer_connect_s args;
  args.peer = peer;
  args.endpoint = endpoint;

  joiner.args = &args;

  duplex_err err = duplex_peer_join_th(peer, &joiner);
  if (err != ERR_NONE) // something went wrong?
    return err;

  return joiner.error;
}
