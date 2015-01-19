#ifndef DUPLEX_H_INTERNAL
#define DUPLEX_H_INTERNAL

#include "duplex.h"

#if defined(DEBUG) | defined(_DEBUG)
#ifndef DEBUG_FUNC
#define DEBUG_FUNC(...) do { __VA_ARGS__; } while(0)
#endif
#else
#ifndef DEBUG_FUNC
#define DEBUG_FUNC(...)
#endif
#endif

// Joiner Function Struct
typedef struct {
  duplex_err (*function)(void*, void**);
  void* args;
  void* result;
  duplex_err error;
} duplex_joiner;

// Join the SSH thread on the specified peer to execute a
// function. If the peer has closed, returns non-zero.
duplex_err _duplex_join(duplex_peer *peer, duplex_joiner *joiner);

// Attempt to parse an endpoint. Return 0 if TCP, 1 if UNIX, and -1 if
// it's unparseable.
// MODIFYS THE ENDPOINT PARAMETER - MAKE SURE TO PASS IN COPY
// AS IT USES IT TO CREATE THE HOSTNAME AND PORT PARTS
// (you still need to free whatever you pass into endpoint anyway)
int _duplex_endpoint_parse(char* endpoint, char** hostname, int* port);

// Try opening a unix connection to the specified path. If success, returns
// an open fd. If fail, returns < 0.
int _duplex_socket_unix_connect(const char* path);

#endif
