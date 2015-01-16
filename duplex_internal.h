#ifndef DUPLEX_H_INTERNAL
#define DUPLEX_H_INTERNAL

#include "duplex.h"

// Joiner Function Struct
typedef struct {
  void* (*function)(void*);
  void* args;
} duplex_joiner;

// Join the SSH thread on the specified peer to execute a
// function. Returns what the function would return.
void* duplex_peer_join_th(duplex_peer *peer, duplex_joiner *joiner);

#endif
