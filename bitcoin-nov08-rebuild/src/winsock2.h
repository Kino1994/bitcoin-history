// nov08-rebuild - Winsock shim.
// On Windows (MinGW/MSVC) it forwards to the system's real winsock2.h.
// On POSIX it maps the Winsock API used by node.cpp to BSD sockets.
#ifndef BITCOIN_WINSOCK2_SHIM_H
#define BITCOIN_WINSOCK2_SHIM_H

#if defined(WIN32) || defined(_WIN32)
#  include_next <winsock2.h>
#else

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

typedef int SOCKET;
#define INVALID_SOCKET   (-1)
#define SOCKET_ERROR     (-1)

inline int closesocket(SOCKET s) { return close(s); }
inline int ioctlsocket(SOCKET s, long cmd, unsigned long* argp) {
    // node.cpp only uses FIONBIO (non-blocking mode)
    int flags = fcntl(s, F_GETFL, 0);
    if (argp && *argp) return fcntl(s, F_SETFL, flags | O_NONBLOCK);
    return fcntl(s, F_SETFL, flags & ~O_NONBLOCK);
}
#ifndef FIONBIO
#define FIONBIO 0x8004667e
#endif

// Errors: Winsock -> errno
inline int WSAGetLastError() { return errno; }
#define WSAEWOULDBLOCK   EWOULDBLOCK
#define WSAEMSGSIZE      EMSGSIZE
#define WSAEINTR         EINTR
#define WSAEINPROGRESS   EINPROGRESS
#define WSAEADDRINUSE    EADDRINUSE
#define WSAENOTSOCK      ENOTSOCK

// Library startup/teardown: no-op on POSIX
typedef struct { int iMaxSockets; } WSADATA;
#define MAKEWORD(a,b) ((unsigned short)(((a)&0xff)|(((b)&0xff)<<8)))
inline int WSAStartup(unsigned short, WSADATA*) { return 0; }
inline int WSACleanup() { return 0; }

#endif // POSIX
#endif // shim
