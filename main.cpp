/**
 * Irfan G. (c) 2025.
 */

#include <iostream>
#include "kdb.h"

auto main (int argc, char *argv[]) -> int {
#ifdef WIN32
    WSADATA wsaData = {0};
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
#ifndef NDEBUG
  std::cout << "running tests.. ";

  kdb::Crypto::Test();
  kdb::Net::Test();
  kdb::kdbms::Test();

  std::cout << "done.\n";
#endif
#ifdef WIN32
    WSACleanup();
#endif

}

