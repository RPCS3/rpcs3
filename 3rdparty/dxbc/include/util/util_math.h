#pragma once

#include <cmath>

namespace dxvk {
  
  constexpr size_t CACHE_LINE_SIZE = 64;
  constexpr double pi = 3.14159265359;

  template<typename T>
  constexpr T clamp(T n, T lo, T hi) {
    if (n < lo) return lo;
    if (n > hi) return hi;
    return n;
  }
  
  template<typename T, typename U = T>
  constexpr T align(T what, U to) {
    return (what + to - 1) & ~(to - 1);
  }

  template<typename T, typename U = T>
  constexpr T alignDown(T what, U to) {
    return (what / to) * to;
  }

  // Equivalent of std::clamp for use with floating point numbers
  // Handles (-){INFINITY,NAN} cases.
  // Will return min in cases of NAN, etc.
  inline float fclamp(float value, float min, float max) {
    return std::fmin(
      std::fmax(value, min), max);
  }

  template<typename T>
  inline T divCeil(T dividend, T divisor) {
    return (dividend + divisor - 1) / divisor;
  }
  
}
