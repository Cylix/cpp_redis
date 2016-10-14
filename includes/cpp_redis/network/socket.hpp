#pragma once

namespace cpp_redis {

namespace network {

#ifdef _WIN32
#include <WinSock2.h>
typedef SOCKET _sock_t;
#else
typedef int _sock_t;
#endif /* _WIN32 */

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif /* INVALID_SOCKET */

} //! network

} //! cpp_redis
