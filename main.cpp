/**
 * Irfan G. (c) 2025.
 */

#include <iostream>
#include "kdb.h"

auto main (int argc, char *argv[]) -> int {

#ifndef NDEBUG
  std::cout << "running tests.. ";

  assert(kdb::kdbms::Test() == true);

  std::cout << "done.\n";
#endif

}

