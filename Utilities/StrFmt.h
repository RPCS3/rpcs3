#pragma once

#include <array>
#include <string>
#include <vector>
#include <functional>
#include <memory>

#include "Platform.h"
#include "types.h"

#if defined(_MSC_VER) && _MSC_VER <= 1800
#define snprintf _snprintf
#endif

// Copy null-terminated string from std::string to char array with truncation
template<std::size_t N>
inline void strcpy_trunc(char(&dst)[N], const std::string& src)
{
	const std::size_t count = src.size() >= N ? N - 1 : src.size();
	std::memcpy(dst, src.c_str(), count);
	dst[count] = '\0';
}

// Copy null-terminated string from char array to another char array with truncation
template<std::size_t N, std::size_t N2>
inline void strcpy_trunc(char(&dst)[N], const char(&src)[N2])
{
	const std::size_t count = N2 >= N ? N - 1 : N2;
	std::memcpy(dst, src, count);
	dst[count] = '\0';
}

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

	template<typename T, typename>
	struct unveil
	{
		using result_type = T;

		force_inline static result_type get_value(const T& arg)
		{
			return arg;
		}
	};

	template<>
	struct unveil<const char*, void>
	{
		using result_type = const char* const;

		static result_type get_value(const char* const& arg)
		{
			return arg;
		}
	};

	template<std::size_t N>
	struct unveil<char[N], void>
	{
		using result_type = const char* const;

		static result_type get_value(const char(&arg)[N])
		{
			return arg;
		}
	};

	template<>
	struct unveil<std::string, void>
	{
		using result_type = const char*;

		static result_type get_value(const std::string& arg)
		{
			return arg.c_str();
		}
	};

	template<typename T>
	struct unveil<T, std::enable_if_t<std::is_enum<T>::value>>
	{
		using result_type = std::underlying_type_t<T>;

		force_inline static result_type get_value(const T& arg)
		{
			return static_cast<result_type>(arg);
		}
	};

	template<typename T>
	force_inline typename unveil<T>::result_type do_unveil(const T& arg)
	{
		return unveil<T>::get_value(arg);
	}

	// Formatting function with special functionality (fmt::unveil)
	template<typename... Args>
	safe_buffers std::string format(const char* fmt, const Args&... args)
	{
		// fixed stack buffer for the first attempt
		std::array<char, 4096> fixed_buf;

		// possibly dynamically allocated buffer for the second attempt
		std::unique_ptr<char[]> buf;

		// pointer to the current buffer
		char* buf_addr = fixed_buf.data();

		for (std::size_t buf_size = fixed_buf.size();;)
		{
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif
			const std::size_t len = std::snprintf(buf_addr, buf_size, fmt, do_unveil(args)...);
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
			if (len > INT_MAX)
			{
				throw std::runtime_error("std::snprintf() failed");
			}

			if (len < buf_size)
			{
				return{ buf_addr, len };
			}

			buf.reset(buf_addr = new char[buf_size = len + 1]);
		}
	}

	// Create exception of type T (std::runtime_error by default) with formatting
	template<typename T = std::runtime_error, typename... Args>
	never_inline safe_buffers T exception(const char* fmt, const Args&... args) noexcept(noexcept(T{ fmt }))
	{
		return T{ format(fmt, do_unveil(args)...).c_str() };
	}

	// Create exception of type T (std::runtime_error by default) without formatting
	template<typename T = std::runtime_error>
	safe_buffers T exception(const char* msg) noexcept(noexcept(T{ msg }))
	{
		return T{ msg };
	}

	// Narrow cast (similar to gsl::narrow) with exception message formatting
	template<typename To, typename From, typename... Args>
	inline auto narrow(const char* format_str, const From& value, const Args&... args) -> decltype(static_cast<To>(static_cast<From>(std::declval<To>())))
	{
		const auto result = static_cast<To>(value);
		if (static_cast<From>(result) != value) throw fmt::exception(format_str, fmt::do_unveil(value), fmt::do_unveil(args)...);
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
