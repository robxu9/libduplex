#ifndef DUPLEX_H
#define DUPLEX_H

// libduplex interface
//
// The libduplex interface is the main entry point with which
// all calls to duplex are made.
//
// You should include this header only into your project. Do not
// include any of the other individual headers; they will be automatically
// pulled into your project with this header.

#define DUPLEX_GUARD
#include "error.h"
#include "peer.h"
#include "channel.h"
#include "meta.h"
#undef DUPLEX_GUARD

// This sets up libssh threads and calls it to initialise all internal
// structures, as well as any structures that duplex may utilize globally.
void duplex_init();

#endif
