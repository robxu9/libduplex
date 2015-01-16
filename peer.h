#ifndef DUPLEX_GUARD
#error "You should not be including this file directly into your program."
#endif

#ifndef DUPLEX_H_PEER
#define DUPLEX_H_PEER

#include <stdlib.h>
#include <pthread.h>

#include <libssh/libssh.h>

#include "error.h"
#include "uthash.h"

// Endpoint -> Peer Hashing
typedef struct {
  char* endpoint;
  ssh_session *session;
  UT_hash_handle hh;
} duplex_peer_active;

// Duplex Peer
typedef struct {
  int socket[2]; // sockets to communicate with ssh thread
                 // clients use [0], ssh session uses [1]
  pthread_t thread;
  pthread_mutex_t mutex; // ssh thread mutex

  duplex_peer_active *sessions; // ssh session (should only be touched by ssh thread)
} duplex_peer;

// Create a new duplex peer. If there is an error, this returns NULL and
// errno will probably be set.
duplex_peer *duplex_peer_new();

// Gracefully close the peer. This _is_ blocking, and ensures that the peer
// is not usable once this method returns.
duplex_err duplex_peer_close(duplex_peer *peer);

// Free the peer and all internal data structures. If there is an error, an
// error is returned annd errno will probably be set.
duplex_err duplex_peer_free(duplex_peer *peer);

// Peer options
typedef enum {
  OP_UNKNOWN = 0
} duplex_peer_option;

// Set an option to its corresponding value. The value depends on
// the option provided. If the value provided is not the type
// that libduplex expects, the library will return an error.
duplex_err duplex_peer_option_set(duplex_peer *peer, duplex_peer_option option, const void* value);

// Retrieve a string option. If the option is not a string, an error is
// returned. It is the client's responsibility to free the returned
// char* pointer.
duplex_err duplex_peer_option_get_str(duplex_peer *peer, duplex_peer_option option, char** value);

// Connect to another peer at the specified endpoint. The endpoint is parsed
// and a connection is attempted. This _is_ blocking.
// Endpoints are structured as such:
//     tcp://location:port
//     unix:///path/to/socket
duplex_err duplex_peer_connect(duplex_peer *peer, const char* endpoint);

// Disconnect from another peer at the specified endpoint. If the peer was
// not connected, this returns an error.
duplex_err duplex_peer_disconnect(duplex_peer *peer, const char* endpoint);

// Bind to an endpoint. This _is_ blocking.
// Endpoints are structured as such:
//     tcp://location:port (use 0.0.0.0 for global access)
//     unix:///path/to/socket (to create the socket file)
duplex_err duplex_peer_bind(duplex_peer *peer, const char* endpoint);

// Unbind from an endpoint. If the endpoint was not bound, this returns
// an error.
duplex_err duplex_peer_unbind(duplex_peer *peer, const char* endpoint);

// Get the number of remote peers
size_t duplex_peer_remote_len(duplex_peer *peer);

// Fill in the string array with the list of remote peers.
void duplex_peer_remote(duplex_peer *peer, char* remotes[], size_t size);

// Drop a specific peer. If the peer is not connected, this returns an error.
duplex_err duplex_peer_remote_drop(duplex_peer *peer, const char* remote);

// Round-robin and return the next peer. It is the client's responsibility to
// free the returned char* pointer. Returns an error if there is no peer.
duplex_err duplex_peer_remote_next(duplex_peer *peer, char** next_remote);

#endif
