#include <iostream>

#include "teststruct.h"
#include "yaml-cpp/eventhandler.h"
#include "yaml-cpp/yaml.h"  // IWYU pragma: keep

namespace Test {
namespace Emitter {
namespace {
void RunGenEmitterTest(TEST (*test)(YAML::Emitter&), const std::string& name,
                       int& passed, int& total) {
  YAML::Emitter out;
  TEST ret;

  try {
    ret = test(out);
  }
  catch (const YAML::Exception& e) {
    ret.ok = false;
    ret.error = std::string("  Exception caught: ") + e.what();
  }

  if (!out.good()) {
    ret.ok = false;
    ret.error = out.GetLastError();
  }

  if (!ret.ok) {
    std::cout << "Generated emitter test failed: " << name << "\n";
    std::cout << "Output:\n";
    std::cout << out.c_str() << "<<<\n";
    std::cout << ret.error << "\n";
  }

  if (ret.ok)
    passed++;
  total++;
}
}
}

#include "genemittertests.h"

bool RunEmitterTests() {
  int passed = 0;
  int total = 0;
  RunGenEmitterTests(passed, total);

  std::cout << "Emitter tests: " << passed << "/" << total << " passed\n";
  return passed == total;
}
}
