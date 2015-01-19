#ifndef DUPLEX_GUARD
#error "You should not be including this file directly into your program."
#endif

#ifndef DUPLEX_H_ERR
#define DUPLEX_H_ERR

// For marking functions as deprecated.
#ifdef __GNUC__
#define DPX_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define DPX_DEPRECATED __declspec(deprecated)
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define DPX_DEPRECATED
#endif


// Errors
// All possible errors that can occur within the usage of this library
// are handled here. If, however, it is programmer error and not user
// or other such environment errors, assert() will be used and the library
// WILL crash.
typedef enum {
  ERR_NONE = 0,
  ERR_UNKNOWN,
  ERR_CLOSED,
  ERR_LIBSSH,
  ERR_ALLOC,
  ERR_ARGS,
  ERR_FAIL,
  ERR_AUTH
} duplex_err;

#endif
