#ifndef DUPLEX_GUARD
#error "You should not be including this file directly into your program."
#endif

#ifndef DUPLEX_H_PEER
#define DUPLEX_H_PEER

#include <stdlib.h>
#include <pthread.h>

#include <libssh/libssh.h>
#include <libssh/server.h>

#include "error.h"
#include "uthash.h"

// Peer options
typedef enum {
  OP_UNKNOWN = 0
} duplex_peer_option;

typedef union {
  int i;
  char* str;
} duplex_peer_option_value;

typedef struct {
  const duplex_peer_option option;
  duplex_peer_option_value value;
} duplex_peer_option_map;

// Endpoint -> Peer Hashing
typedef struct {
  const char endpoint[8192];
  ssh_session session;
  UT_hash_handle hh;
} duplex_peer_session;

typedef struct {
  const char endpoint[8192];
  ssh_bind bind;
  UT_hash_handle hh;
} duplex_peer_server;

// Duplex Peer
typedef struct {
  int socket[2]; // sockets to communicate with ssh thread
                 // clients use [0], ssh session uses [1]
  pthread_t thread;
  pthread_mutex_t mutex; // ssh thread mutex

  int closed; // if the peer is closed

  duplex_peer_option_map *options; // options for the peer

  duplex_peer_session *sessions; // active connections (should only be touched by ssh thread)
  duplex_peer_server *servers; // ssh servers (should only be touched by ssh thread)
} duplex_peer;

// Create a new duplex peer. If there is an error, this returns NULL and
// errno will probably be set.
duplex_peer *duplex_peer_new();

// Gracefully close the peer. This _is_ blocking, and ensures that the peer
// is not usable once this method returns.
duplex_err duplex_peer_close(duplex_peer *peer);

// Free the peer and all internal data structures. If there is an error, a
// non-zero value is returned, following errno values.
int duplex_peer_free(duplex_peer *peer);

// Set an option to its corresponding value. The value depends on
// the option provided. If the value provided is not the type that libduplex
// expects, the library will probably crash.
// The char* value passed in is copied by the library.
void duplex_peer_option_set_str(duplex_peer *peer, duplex_peer_option option, const char* value);

// Set an option to its corresponding value. The value depends on
// the option provided. If the value provided is not the type that libduplex
// expects, the library will probably crash.
void duplex_peer_option_set_int(duplex_peer *peer, duplex_peer_option option, const int value);

// Retrieve a peer option.
duplex_peer_option_value duplex_peer_option_get(duplex_peer *peer, duplex_peer_option option);

// Connect to another peer at the specified endpoint. The endpoint is parsed
// and a connection is attempted. This _is_ blocking.
// Supported endpoints:
//     tcp://location:port - if a port is not specified it defaults to 2259
//     unix:///path/to/socket
duplex_err duplex_peer_connect(duplex_peer *peer, const char* endpoint);

// Disconnect from another peer at the specified endpoint. If the peer was
// not connected, this returns an error.
duplex_err duplex_peer_disconnect(duplex_peer *peer, const char* endpoint);

// Get the number of active connections made to remote hosts.
size_t duplex_peer_connected_len(duplex_peer *peer);

// Fill in the string array with the list of active endpoints.
void duplex_peer_connected(duplex_peer *peer, char* endpoints[], size_t size);

// Bind to an endpoint. This _is_ blocking.
// Endpoints are structured as such:
//     tcp://location:port (use 0.0.0.0 for global access)
//     unix:///path/to/socket (to create the socket file)
duplex_err duplex_peer_bind(duplex_peer *peer, const char* endpoint);

// Unbind from an endpoint. If the endpoint was not bound, this returns
// an error.
duplex_err duplex_peer_unbind(duplex_peer *peer, const char* endpoint);

// Get the number of bound sockets open.
size_t duplex_peer_bound_len(duplex_peer *peer);

// Fill in the string array with the list of endpoints currently bound.
void duplex_peer_bound(duplex_peer *peer, char* endpoints[], size_t size);

// Get the number of remote peers
size_t duplex_peer_remote_len(duplex_peer *peer);

// Fill in the string array with the list of remote peers.
void duplex_peer_remote(duplex_peer *peer, char* remotes[], size_t size);

// Drop a specific peer. If the peer is not connected, this returns an error.
// This is basically a helper method for duplex_peer_disconnect, except
// referencing the remote peer name instead of its connected endpoint.
duplex_err duplex_peer_remote_drop(duplex_peer *peer, const char* remote);

// Round-robin and return the next peer. It is the client's responsibility to
// free the returned char* pointer. Returns an error if there is no peer.
duplex_err duplex_peer_remote_next(duplex_peer *peer, char** next_remote);

#endif
