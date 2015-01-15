#ifndef DUPLEX_GUARD
#error "You should not be including this file directly into your program."
#endif

#ifndef DUPLEX_H_META
#define DUPLEX_H_META

#include <stdlib.h>

#include "channel.h"

// Returns the name of the service, if there is any. It is the client's
// responsibility to free the returned char* pointer.
void duplex_channel_meta_service(duplex_channel *channel, char** service);

// Returns the number of headers.
size_t duplex_channel_meta_headers_len(duplex_channel *channel);

// Fill in the string array with the list of headers.
void duplex_channel_meta_headers(duplex_channel *channel, char* headers[], size_t size);

// Returns the number of trailers.
size_t duplex_channel_meta_trailers_len(duplex_channel *channel);

// Fill in the string array with the list of trailers.
void duplex_channel_meta_trailers(duplex_channel *channel, char* trailers[], size_t size);

// Returns the name of the local peer. It is the client's responsibiity to
// free the returned char* pointer.
void duplex_channel_peer_local(duplex_channel *channel, char** name);

// Returns the name of the remote peer. It is the client's responsibiity to
// free the returned char* pointer.
void duplex_channel_peer_remote(duplex_channel *channel, char** name);

#endif
