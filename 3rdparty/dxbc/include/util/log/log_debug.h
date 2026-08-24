#pragma once

#include <sstream>

#include "log/log.h"

#ifdef _MSC_VER
#define METHOD_NAME __FUNCSIG__
#else
#define METHOD_NAME __PRETTY_FUNCTION__
#endif

#define TRACE_ENABLED

#ifdef TRACE_ENABLED
#define TRACE(...) \
  do { dxvk::debug::trace(METHOD_NAME, ##__VA_ARGS__); } while (0)
#else
#define TRACE(...) \
  do { } while (0)
#endif

namespace dxvk::debug {
  
  std::string methodName(const std::string& prettyName);
  
  inline void traceArgs(std::stringstream& stream) { }
    
  template<typename Arg1>
  void traceArgs(std::stringstream& stream, const Arg1& arg1) {
    stream << arg1;
  }
  
  template<typename Arg1, typename Arg2, typename... Args>
  void traceArgs(std::stringstream& stream, const Arg1& arg1, const Arg2& arg2, const Args&... args) {
    stream << arg1 << ",";
    traceArgs(stream, arg2, args...);
  }
  
  template<typename... Args>
  void trace(const std::string& funcName, const Args&... args) {
    std::stringstream stream;
    stream << methodName(funcName) << "(";
    traceArgs(stream, args...);
    stream << ")";
    Logger::trace(stream.str());
  }
  
}
