#ifndef DUPLEX_H_INTERNAL
#define DUPLEX_H_INTERNAL

#include "duplex.h"

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

#endif
