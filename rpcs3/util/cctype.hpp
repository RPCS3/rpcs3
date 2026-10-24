#pragma once

#include "types.hpp"

#include <cctype>
#include <concepts>

namespace utils
{
	template <typename T>
	concept cctype_char = std::integral<T> && (std::is_same_v<T, char> || std::is_same_v<T, s8> || std::is_unsigned_v<T>);

	template <cctype_char T>
	constexpr bool check_cctype_arg(T c)
	{
		if constexpr (std::is_same_v<T, char>)
		{
			return true;
		}
		else
		{
			return c <= std::numeric_limits<unsigned char>::max();
		}
	}

	template <cctype_char T> constexpr bool isalnum(T c) { return check_cctype_arg(c) && ::isalnum(static_cast<unsigned char>(c)); }
	template <cctype_char T> constexpr bool isalpha(T c) { return check_cctype_arg(c) && ::isalpha(static_cast<unsigned char>(c)); }
	template <cctype_char T> constexpr bool iscntrl(T c) { return check_cctype_arg(c) && ::iscntrl(static_cast<unsigned char>(c)); }
	template <cctype_char T> constexpr bool isdigit(T c) { return check_cctype_arg(c) && ::isdigit(static_cast<unsigned char>(c)); }
	template <cctype_char T> constexpr bool isgraph(T c) { return check_cctype_arg(c) && ::isgraph(static_cast<unsigned char>(c)); }
	template <cctype_char T> constexpr bool islower(T c) { return check_cctype_arg(c) && ::islower(static_cast<unsigned char>(c)); }
	template <cctype_char T> constexpr bool isupper(T c) { return check_cctype_arg(c) && ::isupper(static_cast<unsigned char>(c)); }
	template <cctype_char T> constexpr bool isprint(T c) { return check_cctype_arg(c) && ::isprint(static_cast<unsigned char>(c)); }
	template <cctype_char T> constexpr bool ispunct(T c) { return check_cctype_arg(c) && ::ispunct(static_cast<unsigned char>(c)); }
	template <cctype_char T> constexpr bool isspace(T c) { return check_cctype_arg(c) && ::isspace(static_cast<unsigned char>(c)); }
	template <cctype_char T> constexpr bool isblank(T c) { return check_cctype_arg(c) && ::isblank(static_cast<unsigned char>(c)); }
	template <cctype_char T> constexpr bool isxdigit(T c) { return check_cctype_arg(c) && ::isxdigit(static_cast<unsigned char>(c)); }

	template <cctype_char T>
	constexpr int tolower(T c)
	{
		if (!check_cctype_arg(c)) return EOF;

		return ::tolower(static_cast<unsigned char>(c));
	}

	template <cctype_char T>
	constexpr int toupper(T c)
	{
		if (!check_cctype_arg(c)) return EOF;

		return ::toupper(static_cast<unsigned char>(c));
	}
}
