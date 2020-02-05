#pragma once

#include "types.h"

#include <exception>
#include <stdexcept>
#include <string>

namespace fmt
{
	template <typename CharT, std::size_t N, typename... Args>
	static std::string format(const CharT(&)[N], const Args&...);
}

template <typename T, typename>
struct fmt_unveil
{
	static_assert(sizeof(T) > 0, "fmt_unveil<> error: incomplete type");

	using type = T;

	static inline u64 get(const T& arg)
	{
		return reinterpret_cast<std::uintptr_t>(&arg);
	}

	// Temporary value container (can possibly be created by other fmt_unveil<> specializations)
	struct u64_wrapper
	{
		T arg;

		// Allow implicit conversion
		operator u64() const
		{
			return reinterpret_cast<std::uintptr_t>(&arg);
		}
	};

	// This overload resolution takes the precedence
	static inline u64_wrapper get(T&& arg)
	{
		return u64_wrapper{std::move(arg)};
	}
};

template <typename T>
struct fmt_unveil<T, std::enable_if_t<std::is_integral<T>::value && sizeof(T) <= 8 && alignof(T) <= 8>>
{
	using type = T;

	static inline u64 get(T arg)
	{
		return static_cast<T>(arg);
	}
};

template <typename T>
struct fmt_unveil<T, std::enable_if_t<std::is_floating_point<T>::value && sizeof(T) <= 8 && alignof(T) <= 8>>
{
	using type = T;

	// Convert FP to f64 and reinterpret as u64
	static inline u64 get(const f64& arg)
	{
		return std::bit_cast<u64>(arg);
	}
};

template <>
struct fmt_unveil<f16, void>
{
	using type = f16;

	static inline u64 get(const f16& arg)
	{
		return fmt_unveil<f64>::get(arg.operator float());
	}
};

template <typename T>
struct fmt_unveil<T, std::enable_if_t<std::is_enum<T>::value>>
{
	using type = T;

	static inline u64 get(T arg)
	{
		return static_cast<std::underlying_type_t<T>>(arg);
	}
};

template <typename T>
struct fmt_unveil<T*, void>
{
	using type = std::add_const_t<T>*;

	static inline u64 get(type arg)
	{
		return reinterpret_cast<std::uintptr_t>(arg);
	}
};

template <typename T, std::size_t N>
struct fmt_unveil<T[N], void>
{
	using type = std::add_const_t<T>*;

	static inline u64 get(type arg)
	{
		return reinterpret_cast<std::uintptr_t>(arg);
	}
};

template <>
struct fmt_unveil<b8, void>
{
	using type = bool;

	static inline u64 get(const b8& value)
	{
		return fmt_unveil<bool>::get(value);
	}
};

// String type format provider, also type classifier (format() called if an argument is formatted as "%s")
template <typename T, typename = void>
struct fmt_class_string
{
	// Formatting function (must be explicitly specialized)
	static void format(std::string& out, u64 arg);

	// Helper typedef (visible in format())
	using type = T;

	// Helper function (converts arg to object reference)
	static SAFE_BUFFERS FORCE_INLINE const T& get_object(u64 arg)
	{
		return *reinterpret_cast<const T*>(static_cast<std::uintptr_t>(arg));
	}

	// Enum -> string function type
	using convert_t = const char*(*)(T value);

	// Helper function (safely converts arg to enum value)
	static SAFE_BUFFERS FORCE_INLINE void format_enum(std::string& out, u64 arg, convert_t convert)
	{
		const auto value = static_cast<std::underlying_type_t<T>>(arg);

		// Check narrowing
		if (static_cast<u64>(value) == arg)
		{
			if (const char* str = convert(static_cast<T>(value)))
			{
				out += str;
				return;
			}
		}

		// Fallback to underlying type formatting
		fmt_class_string<std::underlying_type_t<T>>::format(out, static_cast<u64>(value));
	}

	// Helper function (bitset formatting)
	static SAFE_BUFFERS FORCE_INLINE void format_bitset(std::string& out, u64 arg, const char* prefix, const char* delim, const char* suffix, void (*fmt)(std::string&, u64))
	{
		// Start from raw value
		fmt_class_string<u64>::format(out, arg);

		out += prefix;

		for (u64 i = 0; i < 63; i++)
		{
			const u64 mask = 1ull << i;

			if (arg & mask)
			{
				fmt(out, i);

				if (arg >> (i + 1))
				{
					out += delim;
				}
			}
		}

		if (arg & (1ull << 63))
		{
			fmt(out, 63);
		}

		out += suffix;
	}

	// Helper constant (may be used in format_enum as lambda return value)
	static constexpr const char* unknown = nullptr;
};

template <>
struct fmt_class_string<const void*, void>
{
	static void format(std::string& out, u64 arg);
};

template <typename T>
struct fmt_class_string<T*, void> : fmt_class_string<const void*, void>
{
	// Classify all pointers as const void*
};

template <>
struct fmt_class_string<const char*, void>
{
	static void format(std::string& out, u64 arg);
};

template <>
struct fmt_class_string<char*, void> : fmt_class_string<const char*>
{
	// Classify char* as const char*
};

struct fmt_type_info
{
	decltype(&fmt_class_string<int>::format) fmt_string;

	template <typename T>
	static constexpr fmt_type_info make()
	{
		return fmt_type_info
		{
			&fmt_class_string<T>::format,
		};
	}
};

// Argument array type (each element generated via fmt_unveil<>)
template <typename... Args>
using fmt_args_t = const u64(&&)[sizeof...(Args) + 1];

namespace fmt
{
	// Base-57 format helper
	struct base57
	{
		const uchar* data;
		std::size_t size;

		template <typename T>
		base57(const T& arg)
			: data(reinterpret_cast<const uchar*>(&arg))
			, size(sizeof(T))
		{
		}

		base57(const uchar* data, std::size_t size)
			: data(data)
			, size(size)
		{
		}
	};

	template <typename... Args>
	SAFE_BUFFERS FORCE_INLINE const fmt_type_info* get_type_info()
	{
		// Constantly initialized null-terminated list of type-specific information
		static constexpr fmt_type_info result[sizeof...(Args) + 1]{fmt_type_info::make<fmt_unveil_t<Args>>()...};

		return result;
	}

	template <typename... Args>
	constexpr const fmt_type_info type_info_v[sizeof...(Args) + 1]{fmt_type_info::make<fmt_unveil_t<Args>>()...};

	// Internal formatting function
	void raw_append(std::string& out, const char*, const fmt_type_info*, const u64*) noexcept;

	// Formatting function
	template <typename CharT, std::size_t N, typename... Args>
	SAFE_BUFFERS FORCE_INLINE void append(std::string& out, const CharT(&fmt)[N], const Args&... args)
	{
		static constexpr fmt_type_info type_list[sizeof...(Args) + 1]{fmt_type_info::make<fmt_unveil_t<Args>>()...};
		raw_append(out, reinterpret_cast<const char*>(fmt), type_list, fmt_args_t<Args...>{fmt_unveil<Args>::get(args)...});
	}

	// Formatting function
	template <typename CharT, std::size_t N, typename... Args>
	SAFE_BUFFERS FORCE_INLINE std::string format(const CharT(&fmt)[N], const Args&... args)
	{
		std::string result;
		append(result, fmt, args...);
		return result;
	}

	// Internal exception message formatting template, must be explicitly specialized or instantiated in cpp to minimize code bloat
	[[noreturn]] void raw_throw_exception(const char*, const fmt_type_info*, const u64*);

	// Throw exception with formatting
	template <typename... Args>
	[[noreturn]] SAFE_BUFFERS FORCE_INLINE void throw_exception(const char* fmt, const Args&... args)
	{
		static constexpr fmt_type_info type_list[sizeof...(Args) + 1]{fmt_type_info::make<fmt_unveil_t<Args>>()...};
		raw_throw_exception(fmt, type_list, fmt_args_t<Args...>{fmt_unveil<Args>::get(args)...});
	}
}
