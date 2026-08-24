#pragma once

#include <cstring>
#include <string>
#include <sstream>
#include <vector>

#include "util_bit.h"
#include "util_likely.h"

namespace dxvk::str {

  template<size_t S> struct UnicodeChar { };
  template<> struct UnicodeChar<1> { using type = uint8_t;  };
  template<> struct UnicodeChar<2> { using type = uint16_t; };
  template<> struct UnicodeChar<4> { using type = uint32_t; };

  template<typename T>
  using UnicodeCharType = typename UnicodeChar<sizeof(T)>::type;

  const uint8_t* decodeTypedChar(
    const uint8_t*  begin,
    const uint8_t*  end,
          uint32_t& ch);

  const uint16_t* decodeTypedChar(
    const uint16_t* begin,
    const uint16_t* end,
          uint32_t& ch);

  const uint32_t* decodeTypedChar(
    const uint32_t* begin,
    const uint32_t* end,
          uint32_t& ch);

  size_t encodeTypedChar(
          uint8_t*  begin,
          uint8_t*  end,
          uint32_t  ch);

  size_t encodeTypedChar(
          uint16_t* begin,
          uint16_t* end,
          uint32_t  ch);

  size_t encodeTypedChar(
          uint32_t* begin,
          uint32_t* end,
          uint32_t  ch);

  /**
   * \brief Decodes a single character
   *
   * Note that \c begin and \c end must not be equal.
   * \param [in] begin Pointer to current position within the input string
   * \param [in] end Pointer to the end of the input string
   * \param [out] ch Pointer to the decoded character code
   * \returns Pointer to next character in the input string
   */
  template<typename T>
  const T* decodeChar(
    const T*        begin,
    const T*        end,
          uint32_t& ch) {
    using CharType = UnicodeCharType<T>;

    const CharType* result = decodeTypedChar(
      reinterpret_cast<const CharType*>(begin),
      reinterpret_cast<const CharType*>(end),
      ch);

    return reinterpret_cast<const T*>(result);
  }

  /**
   * \brief Encodes a character
   *
   * Note that \c begin and \c end may be both be \c nullptr or equal, in
   * which case only the length of the encoded character will be returned.
   * \param [in] begin Pointer to current position within the output string
   * \param [in] end Pointer to the end of the output string
   * \param [in] ch Character to encode
   * \returns If begin is \c nullptr , the number of units required to encode
   *    the character. Otherwise, the number of units written to the output.
   *    This may return \c 0 for characters that cannot be written or encoded.
   */
  template<typename T>
  size_t encodeChar(
          T*        begin,
          T*        end,
          uint32_t  ch) {
    using CharType = UnicodeCharType<T>;

    return encodeTypedChar(
      reinterpret_cast<CharType*>(begin),
      reinterpret_cast<CharType*>(end),
      ch);
  }

  /**
   * \brief Computes length of a null-terminated string
   *
   * \param [in] begin Start of input string
   * \returns Number of characters in input string,
   *    excluding the terminating null character
   */
  template<typename S>
  size_t length(const S* string) {
    size_t result = 0;

    while (string[result])
      result += 1;

    return result;
  }

  /**
   * \brief Converts string from one encoding to another
   *
   * The output string arguments may be \c nullptr. In that case, the
   * total length of the transcoded string will be returned, in units
   * of the output character type. The output string will only be
   * null-terminated if the input string is also null-terminated.
   * \tparam D Output character type
   * \tparam S Input character type
   * \param [in] dstBegin Start of output string
   * \param [in] dstLength Length of output string
   * \param [in] srcBegin Start of input string
   * \param [in] srcLength Length of input string
   * \returns If \c dstBegin is \c nullptr , the total number of output
   *    characters required to store the output string. Otherwise, the
   *    total number of characters written to the output string.
   */
  template<typename D, typename S>
  size_t transcodeString(
          D*      dstBegin,
          size_t  dstLength,
    const S*      srcBegin,
          size_t  srcLength) {
    size_t totalLength = 0;

    auto dstEnd = dstBegin + dstLength;
    auto srcEnd = srcBegin + srcLength;

    while (srcBegin < srcEnd) {
      uint32_t ch;

      srcBegin = decodeChar<S>(srcBegin, srcEnd, ch);

      if (dstBegin)
        totalLength += encodeChar<D>(dstBegin + totalLength, dstEnd, ch);
      else
        totalLength += encodeChar<D>(nullptr, nullptr, ch);

      if (!ch)
        break;
    }

    return totalLength;
  }

  /**
   * \brief Creates string object from wide char array
   *
   * \param [in] ws Null-terminated wide string
   * \returns Regular string object
   */
  std::string fromws(const wchar_t* ws);

  /**
   * \brief Creates wide string object from char array
   *
   * \param [in] mbs Null-terminated string
   * \returns Wide string object
   */
  std::wstring tows(const char* mbs);

#ifdef _WIN32
  using path_string = std::wstring;
  inline path_string topath(const char* mbs) { return tows(mbs); }
#else
  using path_string = std::string;
  inline path_string topath(const char* mbs) { return std::string(mbs); }
#endif
  
  inline void format1(std::stringstream&) { }

  template<typename... Tx>
  void format1(std::stringstream& str, const wchar_t *arg, const Tx&... args) {
    str << fromws(arg);
    format1(str, args...);
  }

  template<typename T, typename... Tx>
  void format1(std::stringstream& str, const T& arg, const Tx&... args) {
    str << arg;
    format1(str, args...);
  }
  
  template<typename... Args>
  std::string format(const Args&... args) {
    std::stringstream stream;
    format1(stream, args...);
    return stream.str();
  }

  inline void strlcpy(char* dst, const char* src, size_t count) {
    if (count > 0) {
      std::strncpy(dst, src, count - 1);
      dst[count - 1] = '\0';
    }
  }

  /**
   * \brief Split string at one or more delimiters characters
   * 
   * \param [in] string String to split
   * \param [in] delims Delimiter characters
   * \returns Vector of substring views
  */
  inline std::vector<std::string_view> split(std::string_view string, std::string_view delims = " ") {
    std::vector<std::string_view> tokens;

    for (size_t start = 0; start < string.size(); ) {
      // Find first delimiter
      const auto end = string.find_first_of(delims, start);

      // Add non-empty tokens
      if (start != end)
        tokens.emplace_back(string.substr(start, end-start));

      // Break at the end of string
      if (end == std::string_view::npos)
        break;

      start = end + 1;
    }
    return tokens;
  }
}
