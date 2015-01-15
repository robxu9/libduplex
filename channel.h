#ifndef DUPLEX_GUARD
#error "You should not be including this file directly into your program."
#endif

#ifndef DUPLEX_H_CHANNEL
#define DUPLEX_H_CHANNEL

#include <stdlib.h>

#include "error.h"
#include "peer.h"

// Duplex Channel
typedef struct {

} duplex_channel;

// Open a channel to a remote peer for the specified service. Returns the
// opened channel, or if an error is returned, the channel is NULL. All headers
// are to be NULL-terminated. This _is_ blocking.
duplex_err duplex_peer_open(duplex_peer *peer, const char* remote,
                            const char* service, const char* headers[],
                            size_t headers_len, duplex_channel **opened);

// Accepts a channel from a remote peer. Returns the accepted channel, or if
// there is an error, the channel is NULL. This _is_ blocking.
duplex_err duplex_peer_accept(duplex_peer *peer, duplex_channel **accepted);

// Open a channel based on another channel for the specified service. Returns
// the opened channel, or if an error is returned, the channel is NULL. All
// headers are to be NULL-terminated. This _is_ blocking.
duplex_err duplex_channel_open(duplex_channel *channel, const char* service,
                               const char* headers[], size_t headers_len,
                               duplex_channel **opened);

// Accepts a channel from a remote channel. Returns the accepted channel, or if
// there is an error, the channel is NULL. This _is_ blocking.
duplex_err duplex_channel_accept(duplex_channel *channel, duplex_channel** accepted);

// Closes the channel for writing only, and sends EOF to the remote channel.
// Returns an error ONLY if the channel could not be closed for writing (not if
// it is already closed).
duplex_err duplex_channel_write_close(duplex_channel *channel);

// Closes the channel entirely. This returns an error ONLY if the channel could
// not be closed. This _is_ blocking, and the channel will be unusable for
// I/O after this operation.
duplex_err duplex_channel_close(duplex_channel *channel);

// Frees the channel and all internal data structures. If there is an error, an
// error is returned and errno will probably be set.
duplex_err duplex_channel_free(duplex_channel *channel);

// Write to a channel. Returns the number of bytes written, or < 0 if errored.
size_t duplex_channel_write(duplex_channel *channel, const void* data, size_t size);

// Read from a channel into the buffer provided. Returns the number of bytes
// read, or < 0 if an error occurred.
size_t duplex_channel_read(duplex_channel *channel, void* buffer, size_t size);

// Write a frame to a channel. Returns the number of bytes written, or <0 if
// an error occurred.
size_t duplex_channel_frame_write(duplex_channel *channel, const void* data, size_t size);

// Read a frame from a channel into the buffer provided. Returns the number of
// bytes read, or < 0 if an error occurred.
size_t duplex_channel_frame_read(duplex_channel *channel, void* buffer, size_t size);

// Returns the length of the next frame from the channel. This _is_ blocking if
// there are currently no frames queued.
size_t duplex_channel_frame_next(duplex_channel *channel);

// Write an error to a channel. Returns the number of bytes written, or < 0 if
// an error occurred.
size_t duplex_channel_error_write(duplex_channel *channel, const void* data, size_t size);

// Read an error from a channel into the buffer provided. Returns the number of
// bytes read, or < 0 if an error occure
size_t duplex_channel_error_read(duplex_channel *channel, void* buffer, size_t size);

// Returns the length of the next error from the channel. This _is_ blocking if
// there are currently no errors queued.
size_t duplex_channel_error_next(duplex_channel *channel);

// Write a list of trailers to the channel. These trailers will be duplicated on
// both ends of the channel. No race-checking is done.
duplex_err duplex_channel_trailers_write(duplex_channel *channel,
                            const char* trailers[], size_t trailers_len);

// Join to an existing socket. The channel will read and write to the socket,
// acting as a proxy. This _is_ blocking (obviously), and ends when the socket
// has returned EOF and the remote channel has EOF'ed as well.
duplex_err duplex_channel_join(int socket);

#endif
