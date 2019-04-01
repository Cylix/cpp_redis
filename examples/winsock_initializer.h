#pragma once

#ifdef _WIN32
#include <Winsock2.h>
#include <stdexcept>

class winsock_initializer {
public:
  winsock_initializer() {
    //! Windows netword DLL init
    WORD version = MAKEWORD(2, 2);
    WSADATA data;

    if (WSAStartup(version, &data) != 0) {
      throw std::runtime_error("WSAStartup() failure");
    }
  }
  ~winsock_initializer() { WSACleanup(); }
};
#else
class winsock_initializer {};
#endif /* _WIN32 */
