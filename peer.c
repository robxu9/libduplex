#include "duplex_internal.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

// To be passed into pthread_create.
void* duplex_peer_handle_joiners(void *arg) {
  duplex_peer *peer = (duplex_peer*) arg;

  int socket = peer->socket[1];

  // Listen and handle requests
  while(1) {
    duplex_joiner *joiner = NULL;

    assert(read(socket, &joiner, sizeof(void*)) == sizeof(void*));

    void* result = joiner->function(joiner->args);

    assert(write(socket, &result, sizeof(void*)) == sizeof(void*));
    // if the session is closed, set closed and break
  }

  // close socket[1] (close function for peer will close socket[0])
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
    goto cleanup;
  }

  // start the thread to get session
  int thread_init = pthread_create(&peer->thread, NULL,
                    duplex_peer_handle_joiners, peer);
  if (thread_init) {
    // failed! clean up, set errno, die
    goto cleanup;
  }

  // cool, we're ready.
  return peer;

cleanup:
  close(peer->socket[0]);
  close(peer->socket[1]);

  free(peer);

  errno = mutex_init;
  return NULL;
}
