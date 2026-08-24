#pragma once

#if (defined(__x86_64__) && !defined(__arm64ec__)) || (defined(_M_X64) && !defined(_M_ARM64EC)) \
    || defined(__i386__) || defined(_M_IX86) || defined(__e2k__)
  #define DXVK_ARCH_X86
  #if defined(__x86_64__) || defined(_M_X64) || defined(__e2k__)
    #define DXVK_ARCH_X86_64
  #endif
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(_M_ARM64EC)
  #define DXVK_ARCH_ARM64
#endif

#ifdef DXVK_ARCH_X86
  #ifndef _MSC_VER
    #if defined(_WIN32) && (defined(__AVX__) || defined(__AVX2__))
      #error "AVX-enabled builds not supported due to stack alignment issues."
    #endif
    #if defined(__WINE__) && defined(__clang__)
      #pragma push_macro("_WIN32")
      #undef _WIN32
    #endif
    #include <x86intrin.h>
    #if defined(__WINE__) && defined(__clang__)
      #pragma pop_macro("_WIN32")
    #endif
  #else
    #include <intrin.h>
  #endif
#endif

#include "util_likely.h"
#include "util_math.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <type_traits>
#include <vector>

namespace dxvk::bit {

  template<typename T, typename J>
  T cast(const J& src) {
    static_assert(sizeof(T) == sizeof(J));
    static_assert(std::is_trivially_copyable<J>::value && std::is_trivial<T>::value);

    T dst;
    std::memcpy(&dst, &src, sizeof(T));
    return dst;
  }
  
  template<typename T>
  T extract(T value, uint32_t fst, uint32_t lst) {
    return (value >> fst) & ~(~T(0) << (lst - fst + 1));
  }

  template<typename T>
  T popcnt(T n) {
    n -= ((n >> 1u) & T(0x5555555555555555ull));
    n = (n & T(0x3333333333333333ull)) + ((n >> 2u) & T(0x3333333333333333ull));
    n = (n + (n >> 4u)) & T(0x0f0f0f0f0f0f0f0full);
    n *= T(0x0101010101010101ull);
    return n >> (8u * (sizeof(T) - 1u));
  }

  inline uint32_t tzcnt(uint32_t n) {
    #if defined(_MSC_VER) && !defined(__clang__)
    if(n == 0)
      return 32;
    return _tzcnt_u32(n);
    #elif defined(__BMI__)
    return __tzcnt_u32(n);
    #elif defined(DXVK_ARCH_X86) && (defined(__GNUC__) || defined(__clang__))
    // tzcnt is encoded as rep bsf, so we can use it on all
    // processors, but the behaviour of zero inputs differs:
    // - bsf:   zf = 1, cf = ?, result = ?
    // - tzcnt: zf = 0, cf = 1, result = 32
    // We'll have to handle this case manually.
    uint32_t res;
    uint32_t tmp;
    asm (
      "tzcnt %2, %0;"
      "mov  $32, %1;"
      "test  %2, %2;"
      "cmovz %1, %0;"
      : "=&r" (res), "=&r" (tmp)
      : "r" (n)
      : "cc");
    return res;
    #elif defined(__GNUC__) || defined(__clang__)
    return n != 0 ? __builtin_ctz(n) : 32;
    #else
    uint32_t r = 31;
    n &= -n;
    r -= (n & 0x0000FFFF) ? 16 : 0;
    r -= (n & 0x00FF00FF) ?  8 : 0;
    r -= (n & 0x0F0F0F0F) ?  4 : 0;
    r -= (n & 0x33333333) ?  2 : 0;
    r -= (n & 0x55555555) ?  1 : 0;
    return n != 0 ? r : 32;
    #endif
  }

  inline uint32_t tzcnt(uint64_t n) {
    #if defined(DXVK_ARCH_X86_64) && defined(_MSC_VER) && !defined(__clang__)
    if(n == 0)
      return 64;
    return (uint32_t)_tzcnt_u64(n);
    #elif defined(DXVK_ARCH_X86_64) && defined(__BMI__)
    return __tzcnt_u64(n);
    #elif defined(DXVK_ARCH_X86_64) && (defined(__GNUC__) || defined(__clang__))
    uint64_t res;
    uint64_t tmp;
    asm (
      "tzcnt %2, %0;"
      "mov  $64, %1;"
      "test  %2, %2;"
      "cmovz %1, %0;"
      : "=&r" (res), "=&r" (tmp)
      : "r" (n)
      : "cc");
    return res;
    #elif defined(__GNUC__) || defined(__clang__)
    return n != 0 ? __builtin_ctzll(n) : 64;
    #else
    uint32_t lo = uint32_t(n);
    if (lo) {
      return tzcnt(lo);
    } else {
      uint32_t hi = uint32_t(n >> 32);
      return tzcnt(hi) + 32;
    }
    #endif
  }

  inline uint32_t bsf(uint32_t n) {
    #if (defined(__GNUC__) || defined(__clang__)) && !defined(__BMI__) && defined(DXVK_ARCH_X86)
    uint32_t res;
    asm ("tzcnt %1,%0"
    : "=r" (res)
    : "r" (n)
    : "cc");
    return res;
    #else
    return tzcnt(n);
    #endif
  }

  inline uint32_t bsf(uint64_t n) {
    #if (defined(__GNUC__) || defined(__clang__)) && !defined(__BMI__) && defined(DXVK_ARCH_X86_64)
    uint64_t res;
    asm ("tzcnt %1,%0"
    : "=r" (res)
    : "r" (n)
    : "cc");
    return res;
    #else
    return tzcnt(n);
    #endif
  }

  inline uint32_t lzcnt(uint32_t n) {
    #if defined(_MSC_VER) && !defined(__clang__) && !defined(__LZCNT__)
    unsigned long bsr;
    if(n == 0)
      return 32;
    _BitScanReverse(&bsr, n);
    return 31-bsr;
    #elif (defined(_MSC_VER) && !defined(__clang__)) || defined(__LZCNT__)
    return _lzcnt_u32(n);
    #elif defined(__GNUC__) || defined(__clang__)
    return n != 0 ? __builtin_clz(n) : 32;
    #else
    uint32_t r = 0;

    if (n == 0)	return 32;

    if (n <= 0x0000FFFF) { r += 16; n <<= 16; }
    if (n <= 0x00FFFFFF) { r += 8;  n <<= 8; }
    if (n <= 0x0FFFFFFF) { r += 4;  n <<= 4; }
    if (n <= 0x3FFFFFFF) { r += 2;  n <<= 2; }
    if (n <= 0x7FFFFFFF) { r += 1;  n <<= 1; }

    return r;
    #endif
  }

  inline uint32_t lzcnt(uint64_t n) {
    #if defined(_MSC_VER) && !defined(__clang__) && !defined(__LZCNT__) && defined(DXVK_ARCH_X86_64)
    unsigned long bsr;
    if(n == 0)
      return 64;
    _BitScanReverse64(&bsr, n);
    return 63-bsr;
    #elif defined(DXVK_ARCH_X86_64) && ((defined(_MSC_VER) && !defined(__clang__)) && defined(__LZCNT__))
    return _lzcnt_u64(n);
    #elif defined(DXVK_ARCH_X86_64) && (defined(__GNUC__) || defined(__clang__))
    return n != 0 ? __builtin_clzll(n) : 64;
    #else
    uint32_t lo = uint32_t(n);
    uint32_t hi = uint32_t(n >> 32u);
    return hi ? lzcnt(hi) : lzcnt(lo) + 32u;
    #endif
  }

  template<typename T>
  uint32_t pack(T& dst, uint32_t& shift, T src, uint32_t count) {
    constexpr uint32_t Bits = 8 * sizeof(T);
    if (likely(shift < Bits))
      dst |= src << shift;
    shift += count;
    return shift > Bits ? shift - Bits : 0;
  }

  template<typename T>
  uint32_t unpack(T& dst, T src, uint32_t& shift, uint32_t count) {
    constexpr uint32_t Bits = 8 * sizeof(T);
    if (likely(shift < Bits))
      dst = (src >> shift) & ((T(1) << count) - 1);
    shift += count;
    return shift > Bits ? shift - Bits : 0;
  }


  /**
   * \brief Clears cache lines of memory
   *
   * Uses non-temporal stores. The memory region offset
   * and size are assumed to be aligned to 64 bytes.
   * \param [in] mem Memory region to clear
   * \param [in] size Number of bytes to clear
   */
  inline void bclear(void* mem, size_t size) {
    #if defined(DXVK_ARCH_X86) && (defined(__GNUC__) || defined(__clang__) || defined(_MSC_VER))
    auto zero = _mm_setzero_si128();

    #if defined(__clang__)
    #pragma nounroll
    #elif defined(__GNUC__)
    #pragma GCC unroll 0
    #endif
    for (size_t i = 0; i < size; i += 64u) {
      auto* ptr = reinterpret_cast<__m128i*>(mem) + i / sizeof(zero);
      _mm_stream_si128(ptr + 0u, zero);
      _mm_stream_si128(ptr + 1u, zero);
      _mm_stream_si128(ptr + 2u, zero);
      _mm_stream_si128(ptr + 3u, zero);
    }
    #else
    std::memset(mem, 0, size);
    #endif
  }


  /**
   * \brief Compares two aligned structs bit by bit
   *
   * \param [in] a First struct
   * \param [in] b Second struct
   * \returns \c true if the structs are equal
   */
  template<typename T>
  bool bcmpeq(const T* a, const T* b) {
    static_assert(alignof(T) >= 16);
    #if defined(DXVK_ARCH_X86) && (defined(__GNUC__) || defined(__clang__) || defined(_MSC_VER))
    auto ai = reinterpret_cast<const __m128i*>(a);
    auto bi = reinterpret_cast<const __m128i*>(b);

    size_t i = 0;

    #if defined(__clang__)
    #pragma nounroll
    #elif defined(__GNUC__)
    #pragma GCC unroll 0
    #endif

    for ( ; i < 2 * (sizeof(T) / 32); i += 2) {
      __m128i eq0 = _mm_cmpeq_epi8(
        _mm_load_si128(ai + i),
        _mm_load_si128(bi + i));
      __m128i eq1 = _mm_cmpeq_epi8(
        _mm_load_si128(ai + i + 1),
        _mm_load_si128(bi + i + 1));
      __m128i eq = _mm_and_si128(eq0, eq1);

      int mask = _mm_movemask_epi8(eq);
      if (mask != 0xFFFF)
        return false;
    }

    for ( ; i < sizeof(T) / 16; i++) {
      __m128i eq = _mm_cmpeq_epi8(
        _mm_load_si128(ai + i),
        _mm_load_si128(bi + i));

      int mask = _mm_movemask_epi8(eq);
      if (mask != 0xFFFF)
        return false;
    }

    return true;
    #else
    return !std::memcmp(a, b, sizeof(T));
    #endif
  }

  template <size_t Bits>
  class bitset {
    static constexpr size_t Dwords = align(Bits, 32) / 32;
  public:

    constexpr bitset()
      : m_dwords() {

    }

    constexpr bool get(uint32_t idx) const {
      uint32_t dword = 0;
      uint32_t bit   = idx;

      // Compiler doesn't remove this otherwise.
      if constexpr (Dwords > 1) {
        dword = idx / 32;
        bit   = idx % 32;
      }

      return m_dwords[dword] & (1u << bit);
    }

    constexpr void set(uint32_t idx, bool value) {
      uint32_t dword = 0;
      uint32_t bit   = idx;

      // Compiler doesn't remove this otherwise.
      if constexpr (Dwords > 1) {
        dword = idx / 32;
        bit   = idx % 32;
      }

      if (value)
        m_dwords[dword] |= 1u << bit;
      else
        m_dwords[dword] &= ~(1u << bit);
    }

    constexpr bool exchange(uint32_t idx, bool value) {
      bool oldValue = get(idx);
      set(idx, value);
      return oldValue;
    }

    constexpr void flip(uint32_t idx) {
      uint32_t dword = 0;
      uint32_t bit   = idx;

      // Compiler doesn't remove this otherwise.
      if constexpr (Dwords > 1) {
        dword = idx / 32;
        bit   = idx % 32;
      }

      m_dwords[dword] ^= 1u << bit;
    }

    constexpr void setAll() {
      if constexpr (Bits % 32 == 0) {
        for (size_t i = 0; i < Dwords; i++)
          m_dwords[i] = std::numeric_limits<uint32_t>::max();
      }
      else {
        for (size_t i = 0; i < Dwords - 1; i++)
          m_dwords[i] = std::numeric_limits<uint32_t>::max();

        m_dwords[Dwords - 1] = (1u << (Bits % 32)) - 1;
      }
    }

    constexpr void clearAll() {
      for (size_t i = 0; i < Dwords; i++)
        m_dwords[i] = 0;
    }

    constexpr bool any() const {
      for (size_t i = 0; i < Dwords; i++) {
        if (m_dwords[i] != 0)
          return true;
      }

      return false;
    }

    constexpr uint32_t& dword(uint32_t idx) {
      return m_dwords[idx];
    }

    constexpr size_t bitCount() {
      return Bits;
    }

    constexpr size_t dwordCount() {
      return Dwords;
    }

    constexpr bool operator [] (uint32_t idx) const {
      return get(idx);
    }

    constexpr void setN(uint32_t bits) {
      uint32_t fullDwords = bits / 32;
      uint32_t offset = bits % 32;

      for (size_t i = 0; i < fullDwords; i++)
        m_dwords[i] = std::numeric_limits<uint32_t>::max();
     
      if (offset > 0)
        m_dwords[fullDwords] = (1u << offset) - 1;
    }

  private:

    uint32_t m_dwords[Dwords];

  };

  class bitvector {
  public:

    bool get(uint32_t idx) const {
      uint32_t dword = idx / 32;
      uint32_t bit   = idx % 32;

      return m_dwords[dword] & (1u << bit);
    }

    void ensureSize(uint32_t bitCount) {
      uint32_t dword = bitCount / 32;
      if (unlikely(dword >= m_dwords.size())) {
        m_dwords.resize(dword + 1);
      }
      m_bitCount = std::max(m_bitCount, bitCount);
    }

    void set(uint32_t idx, bool value) {
      ensureSize(idx + 1);

      uint32_t dword = 0;
      uint32_t bit   = idx;

      if (value)
        m_dwords[dword] |= 1u << bit;
      else
        m_dwords[dword] &= ~(1u << bit);
    }

    bool exchange(uint32_t idx, bool value) {
      ensureSize(idx + 1);

      bool oldValue = get(idx);
      set(idx, value);
      return oldValue;
    }

    void flip(uint32_t idx) {
      ensureSize(idx + 1);

      uint32_t dword = idx / 32;
      uint32_t bit   = idx % 32;

      m_dwords[dword] ^= 1u << bit;
    }

    void setAll() {
      if (m_bitCount % 32 == 0) {
        for (size_t i = 0; i < m_dwords.size(); i++)
          m_dwords[i] = std::numeric_limits<uint32_t>::max();
      }
      else {
        for (size_t i = 0; i < m_dwords.size() - 1; i++)
          m_dwords[i] = std::numeric_limits<uint32_t>::max();

        m_dwords[m_dwords.size() - 1] = (1u << (m_bitCount % 32)) - 1;
      }
    }

    void clearAll() {
      for (size_t i = 0; i < m_dwords.size(); i++)
        m_dwords[i] = 0;
    }

    bool any() const {
      for (size_t i = 0; i < m_dwords.size(); i++) {
        if (m_dwords[i] != 0)
          return true;
      }

      return false;
    }

    uint32_t& dword(uint32_t idx) {
      return m_dwords[idx];
    }

    size_t bitCount() const {
      return m_bitCount;
    }

    size_t dwordCount() const {
      return m_dwords.size();
    }

    bool operator [] (uint32_t idx) const {
      return get(idx);
    }

    void setN(uint32_t bits) {
      ensureSize(bits);

      uint32_t fullDwords = bits / 32;
      uint32_t offset = bits % 32;

      for (size_t i = 0; i < fullDwords; i++)
        m_dwords[i] = std::numeric_limits<uint32_t>::max();

      if (offset > 0)
        m_dwords[fullDwords] = (1u << offset) - 1;
    }

  private:

    std::vector<uint32_t> m_dwords;
    uint32_t              m_bitCount = 0;

  };

  template<typename T>
  class BitMask {

  public:

    class iterator {
    public:
      using iterator_category = std::input_iterator_tag;
      using value_type = T;
      using difference_type = T;
      using pointer = const T*;
      using reference = T;

      explicit iterator(T flags)
        : m_mask(flags) { }

      iterator& operator ++ () {
        m_mask &= m_mask - 1;
        return *this;
      }

      iterator operator ++ (int) {
        iterator retval = *this;
        m_mask &= m_mask - 1;
        return retval;
      }

      T operator * () const {
        return bsf(m_mask);
      }

      bool operator == (iterator other) const { return m_mask == other.m_mask; }
      bool operator != (iterator other) const { return m_mask != other.m_mask; }

    private:

      T m_mask;

    };

    BitMask()
      : m_mask(0) { }

    explicit BitMask(T n)
      : m_mask(n) { }

    iterator begin() {
      return iterator(m_mask);
    }

    iterator end() {
      return iterator(0);
    }

  private:

    T m_mask;

  };


  /**
   * \brief Encodes float as fixed point
   *
   * Rounds away from zero. If this is not suitable for
   * certain use cases, implement round to nearest even.
   * \tparam T Integer type, may be signed
   * \tparam I Integer bits
   * \tparam F Fractional bits
   * \param n Float to encode
   * \returns Encoded fixed-point value
   */
  template<typename T, int32_t I, int32_t F>
  T encodeFixed(float n) {
    if (n != n)
      return 0u;

    n *= float(1u << F);

    if constexpr (std::is_signed_v<T>) {
      n = std::max(n, -float(1u << (I + F - 1u)));
      n = std::min(n,  float(1u << (I + F - 1u)) - 1.0f);
      n += n < 0.0f ? -0.5f : 0.5f;
    } else {
      n = std::max(n, 0.0f);
      n = std::min(n, float(1u << (I + F)) - 1.0f);
      n += 0.5f;
    }

    T result = T(n);

    if constexpr (std::is_signed_v<T>)
      result &= ((T(1u) << (I + F)) - 1u);

    return result;
  }


  /**
   * \brief Decodes fixed-point integer to float
   *
   * \tparam T Integer type, may be signed
   * \tparam I Integer bits
   * \tparam F Fractional bits
   * \param n Number to decode
   * \returns Decoded  number
   */
  template<typename T, int32_t I, int32_t F>
  float decodeFixed(T n) {
    // Sign-extend as necessary
    if constexpr (std::is_signed_v<T>)
      n -= (n & (T(1u) << (I + F - 1u))) << 1u;

    return float(n) / float(1u << F);
  }


  /**
   * \brief Inserts one null bit after each bit
   */
  inline uint32_t split2(uint32_t c) {
    c = (c ^ (c << 8u)) & 0x00ff00ffu;
    c = (c ^ (c << 4u)) & 0x0f0f0f0fu;
    c = (c ^ (c << 2u)) & 0x33333333u;
    c = (c ^ (c << 1u)) & 0x55555555u;
    return c;
  }


  /**
   * \brief Inserts two null bits after each bit
   */
  inline uint64_t split3(uint64_t c) {
    c = (c | c << 32u) & 0x001f00000000ffffull;
    c = (c | c << 16u) & 0x001f0000ff0000ffull;
    c = (c | c <<  8u) & 0x100f00f00f00f00full;
    c = (c | c <<  4u) & 0x10c30c30c30c30c3ull;
    c = (c | c <<  2u) & 0x1249249249249249ull;
    return c;
  }


  /**
   * \brief Interleaves bits from two integers
   *
   * Both numbers must fit into 16 bits.
   * \param [in] x X coordinate
   * \param [in] y Y coordinate
   * \returns Morton code of x and y
   */
  inline uint32_t interleave(uint16_t x, uint16_t y) {
    return split2(x) | (split2(y) << 1u);
  }


  /**
   * \brief Interleaves bits from three integers
   *
   * All three numbers must fit into 16 bits.
   */
  inline uint64_t interleave(uint16_t x, uint16_t y, uint16_t z) {
    return split3(x) | (split3(y) << 1u) | (split3(z) << 2u);
  }


  /**
   * \brief 48-bit integer storage type
   */
  struct uint48_t {
    explicit uint48_t(uint64_t n)
    : a(uint16_t(n)), b(uint16_t(n >> 16)), c(uint16_t(n >> 32)) { }

    uint16_t a;
    uint16_t b;
    uint16_t c;

    explicit operator uint64_t () const {
      // GCC generates worse code if we promote to uint64 directly
      uint32_t lo = uint32_t(a) | (uint32_t(b) << 16);
      return uint64_t(lo) | (uint64_t(c) << 32);
    }
  };

}
