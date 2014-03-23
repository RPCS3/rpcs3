#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "emittertests.h"
#include "nodetests.h"
#include "parsertests.h"
#include "spectests.h"
#include "tests.h"

namespace Test {
void RunAll() {
  bool passed = true;
  if (!RunParserTests())
    passed = false;

  if (!RunEmitterTests())
    passed = false;

  if (!RunSpecTests())
    passed = false;

  if (!RunNodeTests())
    passed = false;

  if (passed)
    std::cout << "All tests passed!\n";
}
}
