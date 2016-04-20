#pragma once

#include <cstdarg>
#include <string>
#include <vector>
#include <functional>

#include "Platform.h"
#include "types.h"

// Copy null-terminated string from std::string to char array with truncation
template<std::size_t N>
force_inline void strcpy_trunc(char(&dst)[N], const std::string& src)
{
	const std::size_t count = src.size() >= N ? N - 1 : src.size();
	std::memcpy(dst, src.c_str(), count);
	dst[count] = '\0';
}

// Copy null-terminated string from char array to another char array with truncation
template<std::size_t N, std::size_t N2>
force_inline void strcpy_trunc(char(&dst)[N], const char(&src)[N2])
{
	const std::size_t count = N2 >= N ? N - 1 : N2;
	std::memcpy(dst, src, count);
	dst[count] = '\0';
}

// Formatting helper, type-specific preprocessing for improving safety and functionality
template<typename T, typename>
struct unveil
{
	// TODO
	static inline const T& get(const T& arg)
	{
		return arg;
	}
};

template<>
struct unveil<std::string, void>
{
	static inline const char* get(const std::string& arg)
	{
		return arg.c_str();
	}
};

namespace fmt
{
	std::string replace_first(const std::string& src, const std::string& from, const std::string& to);
	std::string replace_all(const std::string &src, const std::string& from, const std::string& to);

	template<size_t list_size>
	std::string replace_all(std::string src, const std::pair<std::string, std::string>(&list)[list_size])
	{
		for (size_t pos = 0; pos < src.length(); ++pos)
		{
			for (size_t i = 0; i < list_size; ++i)
			{
				const size_t comp_length = list[i].first.length();

				if (src.length() - pos < comp_length)
					continue;

				if (src.substr(pos, comp_length) == list[i].first)
				{
					src = (pos ? src.substr(0, pos) + list[i].second : list[i].second) + src.substr(pos + comp_length);
					pos += list[i].second.length() - 1;
					break;
				}
			}
		}

		return src;
	}

	template<size_t list_size>
	std::string replace_all(std::string src, const std::pair<std::string, std::function<std::string()>>(&list)[list_size])
	{
		for (size_t pos = 0; pos < src.length(); ++pos)
		{
			for (size_t i = 0; i < list_size; ++i)
			{
				const size_t comp_length = list[i].first.length();

				if (src.length() - pos < comp_length)
					continue;

				if (src.substr(pos, comp_length) == list[i].first)
				{
					src = (pos ? src.substr(0, pos) + list[i].second() : list[i].second()) + src.substr(pos + comp_length);
					pos += list[i].second().length() - 1;
					break;
				}
			}
		}

		return src;
	}

	std::string to_hex(u64 value, u64 count = 1);
	std::string to_udec(u64 value);
	std::string to_sdec(s64 value);
	std::string _format(const char* fmt...) noexcept;
	std::string _vformat(const char*, va_list) noexcept;

	// Formatting function with special functionality (fmt::unveil)
	template<typename... Args>
	force_inline std::string format(const char* fmt, const Args&... args) noexcept
	{
		return _format(fmt, ::unveil<Args>::get(args)...);
	}

	// Helper class
	class exception_base : public std::runtime_error
	{
		// Helper (there is no other room)
		va_list m_args;

	protected:
		// Internal formatting constructor
		exception_base(const char* fmt...);
	};

	// Exception type derived from std::runtime_error with formatting constructor
	class exception : public exception_base
	{
	public:
		// Formatting constructor
		template<typename... Args>
		exception(const char* fmt, const Args&... args)
			: exception_base(fmt, ::unveil<Args>::get(args)...)
		{
		}
	};

	// Narrow cast (similar to gsl::narrow) with exception message formatting
	template<typename To, typename From, typename... Args>
	inline auto narrow(const char* format_str, const From& value, const Args&... args) -> decltype(static_cast<To>(static_cast<From>(std::declval<To>())))
	{
		const auto result = static_cast<To>(value);
		if (static_cast<From>(result) != value) throw fmt::exception(format_str, value, args...);
		return result;
	}

	std::vector<std::string> split(const std::string& source, std::initializer_list<std::string> separators, bool is_skip_empty = true);
	std::string trim(const std::string& source, const std::string& values = " \t");

	template<typename T>
	std::string merge(const T& source, const std::string& separator)
	{
		if (!source.size())
		{
			return{};
		}

		std::string result;

		auto it = source.begin();
		auto end = source.end();
		for (--end; it != end; ++it)
		{
			result += *it + separator;
		}

		return result + source.back();
	}

	template<typename T>
	std::string merge(std::initializer_list<T> sources, const std::string& separator)
	{
		if (!sources.size())
		{
			return{};
		}

		std::string result;
		bool first = true;

		for (auto &v : sources)
		{
			if (first)
			{
				result = fmt::merge(v, separator);
				first = false;
			}
			else
			{
				result += separator + fmt::merge(v, separator);
			}
		}

		return result;
	}

	template<typename IT>
	std::string to_lower(IT _begin, IT _end)
	{
		std::string result; result.resize(_end - _begin);
		std::transform(_begin, _end, result.begin(), ::tolower);
		return result;
	}

	template<typename T>
	std::string to_lower(const T& string)
	{
		return to_lower(std::begin(string), std::end(string));
	}

	template<typename IT>
	std::string to_upper(IT _begin, IT _end)
	{
		std::string result; result.resize(_end - _begin);
		std::transform(_begin, _end, result.begin(), ::toupper);
		return result;
	}

	template<typename T>
	std::string to_upper(const T& string)
	{
		return to_upper(std::begin(string), std::end(string));
	}

	std::string escape(const std::string& source, std::initializer_list<char> more = {});
	std::string unescape(const std::string& source);
	bool match(const std::string &source, const std::string &mask);
}
