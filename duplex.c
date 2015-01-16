#include "duplex_internal.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

// Internal.
// Join the SSH thread for SSH session operations.
// Returns non-zero if the peer has closed.
duplex_err duplex_peer_join_th(duplex_peer *peer, duplex_joiner *joiner) {
  assert(pthread_mutex_lock(&peer->mutex) == 0);

  int socket = peer->socket[0];

  if (peer->closed) {
    // we're already closed.

    // return non-zero
    assert(pthread_mutex_unlock(&peer->mutex) == 0);

    return ERR_CLOSED;
  }


  assert(write(socket, &joiner, sizeof(void*)) == sizeof(void*));

  char result[1];

  assert(read(socket, result, sizeof(char)) == sizeof(char));

  assert(result[0] == 0);

  if (peer->closed) {
    // close our end of the socket
    close(socket);

    // join the thread
    assert(pthread_join(peer->thread, NULL) == 0);
  }

  assert(pthread_mutex_unlock(&peer->mutex) == 0);
  return ERR_NONE;
}
