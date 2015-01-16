#include "duplex_internal.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

// Internal.
// Join the SSH thread for SSH session operations.
void* duplex_peer_join_th(duplex_peer *peer, duplex_joiner *joiner) {
  assert(pthread_mutex_lock(&peer->mutex) == 0);
  int socket = peer->socket[0];

  assert(write(socket, &joiner, sizeof(void*)) == sizeof(void*));

  void* result = NULL;

  assert(read(socket, &result, sizeof(void*)) == sizeof(void*));

  assert(pthread_mutex_unlock(&peer->mutex) == 0);
  return result;
}
