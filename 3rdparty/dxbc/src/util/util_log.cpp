#include "log/log_debug.h"

namespace dxvk::debug {
  
  std::string methodName(const std::string& prettyName) {
    size_t end = prettyName.find("(");
    size_t begin = prettyName.substr(0, end).rfind(" ") + 1;
    return prettyName.substr(begin,end - begin);
  }
  
}
