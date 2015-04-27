#ifndef PTR_STACK_H_62B23520_7C8E_11DE_8A39_0800200C9A66
#define PTR_STACK_H_62B23520_7C8E_11DE_8A39_0800200C9A66

#if defined(_MSC_VER) ||                                            \
    (defined(__GNUC__) && (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || \
     (__GNUC__ >= 4))  // GCC supports "pragma once" correctly since 3.4
#pragma once
#endif

#include <cstddef>
#include <cstdlib>
#include <memory>
#include <vector>

#include "yaml-cpp/noncopyable.h"

// TODO: This class is no longer needed
template <typename T>
class ptr_stack : private YAML::noncopyable {
 public:
  ptr_stack() {}

  void clear() {
    m_data.clear();
  }

  std::size_t size() const { return m_data.size(); }
  bool empty() const { return m_data.empty(); }

  void push(std::unique_ptr<T>&& t) {
    m_data.push_back(std::move(t));
  }
  std::unique_ptr<T> pop() {
   std::unique_ptr<T> t(std::move(m_data.back()));
    m_data.pop_back();
    return t;
  }

  T& top() {
    return *(m_data.back().get());
  }
  const T& top() const {
    return *(m_data.back().get());
  }

  T& top(std::ptrdiff_t diff) {
    return *((m_data.end() - 1 + diff)->get());
  }

  const T& top(std::ptrdiff_t diff) const {
    return *((m_data.end() - 1 + diff)->get());
  }

 private:
    std::vector<std::unique_ptr<T>> m_data;
};

#endif  // PTR_STACK_H_62B23520_7C8E_11DE_8A39_0800200C9A66
