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

#endif
