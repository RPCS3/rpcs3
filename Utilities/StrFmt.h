#pragma once

#include <cstdarg>
#include <exception>
#include <string>

#include "Platform.h"
#include "types.h"

namespace fmt
{
	std::string unsafe_format(const char* fmt...) noexcept;
	std::string unsafe_vformat(const char*, va_list) noexcept;

	// Formatting function
	template<typename... Args>
	inline std::string format(const char* fmt, const Args&... args)
	{
		return unsafe_format(fmt, ::unveil<Args>::get(args)...);
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
}
