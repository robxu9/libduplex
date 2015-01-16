#include "duplex_internal.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

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

  return ERR_NONE;
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
