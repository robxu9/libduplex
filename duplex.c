#include "duplex_internal.h"

#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <libssh/libssh.h>
#include <libssh/callbacks.h>

void duplex_init() {
  ssh_threads_set_callbacks(ssh_threads_get_pthread());
  assert(ssh_init() == 0);
}

void duplex_cleanup() {
  assert(ssh_finalize() == 0);
}

// Internal.
// Join the SSH thread for SSH session operations.
// Returns non-zero if the peer has closed.
duplex_err _duplex_join(duplex_peer *peer, duplex_joiner *joiner) {
  assert(pthread_mutex_lock(&peer->mutex) == 0);

  int socket = peer->socket[0];

  if (peer->closed) {
    // we're already closed.

    // return non-zero
    assert(pthread_mutex_unlock(&peer->mutex) == 0);

    return ERR_CLOSED;
  }


  assert(write(socket, &joiner, sizeof(void*)) == sizeof(void*));

  char result[1];

  assert(read(socket, result, sizeof(char)) == sizeof(char));

  assert(result[0] == 0);

  if (peer->closed) {
    // close our end of the socket
    close(socket);

    // join the thread
    assert(pthread_join(peer->thread, NULL) == 0);
  }

  assert(pthread_mutex_unlock(&peer->mutex) == 0);
  return ERR_NONE;
}

int _duplex_endpoint_parse(char* endpoint, char** hostname, int* port) {
  // see if we can grab the protocol
  char* tok = strstr(endpoint, "://");
  if (tok == NULL) // there is none
    return -1;

  *tok = '\0'; // split the string there
  char* protocol = endpoint;
  char* after = tok + 3;

  // handle based on the protocol
  if (!strcmp(protocol, "tcp")) {
    // after contains location:port

    *hostname = after;
    *port = 2259;

    // if : exists, then chomp
    char* colon = strchr(after, ':');
    if (colon != NULL) {
      *colon = '\0';
      char* port_str = colon + 1;

      if ((*port = atoi(port_str)) == 0)
        return -1; // 0 isn't a valid port - don't use a random one!
    }

    return 0;
  }

  if (!strcmp(protocol, "unix")) {
    // after contains /tmp/to/location
    *hostname = after;
    return 1;
  }

  return -1;
}

int _duplex_socket_unix_connect(const char* path) {
  // try opening a path to this socket.
  int fd;

  if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    // we can't even make the fd /sigh
    return -1;
  }

  struct sockaddr_un sa; // socket address
  memset(&sa, 0, sizeof(struct sockaddr_un)); // clear the stack alloc

  sa.sun_family = AF_UNIX;
  strcpy(sa.sun_path, path);

  int connect_len = SUN_LEN(&sa);

  if (connect(fd, (struct sockaddr*) &sa, connect_len)) {
    // we can't connect... so time to exit
    assert(close(fd) == 0);
    return -1;
  }

  return fd;
}

int _duplex_socket_unix_bind(const char* path) {
  // try binding a path to this socket.
  int fd;

  if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    // can't even make the fd...
    return -1;
  }

  int reuseval = 1;
  assert(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseval, sizeof(reuseval)) == 0);

  struct sockaddr_un sa;
  memset(&sa, 0, sizeof(struct sockaddr_un));

  sa.sun_family = AF_UNIX;
  strcpy(sa.sun_path, path);

  int bind_len = SUN_LEN(&sa);

  if (bind(fd, (struct sockaddr*) &sa, bind_len)) {
    // bind failed...
    assert(close(fd) == 0);
    return -1;
  }

  return fd;
}

int _duplex_socket_tcp_bind(const char* hostname, const int port) {
  // call getaddrinfo
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo *res, *i;

  char port_digits[6]; // largest port is 65535<NUL>
  assert(sprintf(port_digits, "%d", port) >= 0);

  if (getaddrinfo(hostname, port_digits, &hints, &res) != 0) {
    // can't get the addr info...
    return -1;
  }

  int fd;

  for (i = res; i != NULL; i = i->ai_next) {
    if ((fd = socket(i->ai_family, i->ai_socktype, i->ai_protocol)) < 0) {
      // guess this doesn't work
      continue;
    }

    int reuseval = 1;
    assert(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuseval, sizeof(reuseval)) == 0);

    if (bind(fd, i->ai_addr, i->ai_addrlen)) {
      // that didn't work.
      close(fd);
      continue;
    }

    break;
  }

  if (i == NULL) {// couldn't bind at all
    freeaddrinfo(res);
    return -1;
  }

  freeaddrinfo(res);
  return fd;
}
