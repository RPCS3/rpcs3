#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "nodetests.h"
#include "tests.h"

namespace Test {
void RunAll() {
  bool passed = true;
  if (!RunNodeTests())
    passed = false;

  if (passed)
    std::cout << "All tests passed!\n";
}
}
