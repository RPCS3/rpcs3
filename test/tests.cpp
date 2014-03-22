#include "tests.h"
#include "emittertests.h"
#include "parsertests.h"
#include "spectests.h"
#include "yaml-cpp/yaml.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <iostream>

namespace Test {
void RunAll() {
  bool passed = true;
  if (!RunEmitterTests())
    passed = false;

  if (!RunSpecTests())
    passed = false;

  if (!RunParserTests())
    passed = false;

  if (passed)
    std::cout << "All tests passed!\n";
}
}
