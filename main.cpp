/**
 * Irfan G. (c) 2025.
 */

#include <iostream>
#include "kdb.h"

auto main (int argc, char *argv[]) -> int {

#ifndef NDEBUG
  std::cout << "running tests.. ";

  kdb::kdbms::Test();

  std::cout << "done.\n";
#endif

}

