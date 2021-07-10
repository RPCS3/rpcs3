#pragma once // No BOM and only basic ASCII in this header, or a neko will die

#include "util/types.hpp"

#include <string>

namespace fmt
{
	template <typename CharT, usz N, typename... Args>
	static std::string format(const CharT(&)[N], const Args&...);
}

template <typename T, typename = void>
struct fmt_unveil
{
	static_assert(sizeof(T) > 0, "fmt_unveil<> error: incomplete type");

	using type = T;

	static inline u64 get(const T& arg)
	{
		return reinterpret_cast<uptr>(&arg);
	}

	// Temporary value container (can possibly be created by other fmt_unveil<> specializations)
	struct u64_wrapper
	{
		T arg;

		// Allow implicit conversion
		operator u64() const
		{
			return reinterpret_cast<uptr>(&arg);
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
		return reinterpret_cast<uptr>(arg);
	}
};

template <typename T, usz N>
struct fmt_unveil<T[N], void>
{
	using type = std::add_const_t<T>*;

	static inline u64 get(type arg)
	{
		return reinterpret_cast<uptr>(arg);
	}
};

template <typename T, bool Se, usz Align>
struct fmt_unveil<se_t<T, Se, Align>, void>
{
	using type = typename fmt_unveil<T>::type;

	static inline auto get(const se_t<T, Se, Align>& arg)
	{
		return fmt_unveil<T>::get(arg);
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
	static FORCE_INLINE SAFE_BUFFERS(const T&) get_object(u64 arg)
	{
		return *reinterpret_cast<const T*>(static_cast<uptr>(arg));
	}

	// Enum -> string function type
	using convert_t = const char*(*)(T value);

	// Helper function (safely converts arg to enum value)
	static FORCE_INLINE SAFE_BUFFERS(void) format_enum(std::string& out, u64 arg, convert_t convert)
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
	static FORCE_INLINE SAFE_BUFFERS(void) format_bitset(std::string& out, u64 arg, const char* prefix, const char* delim, const char* suffix, void (*fmt)(std::string&, u64))
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

template <>
struct fmt_class_string<const char8_t*, void> : fmt_class_string<const char*>
{
};

template <>
struct fmt_class_string<char8_t*, void> : fmt_class_string<const char8_t*>
{
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

template <typename Arg>
using fmt_unveil_t = typename fmt_unveil<Arg>::type;

namespace fmt
{
	// Base-57 format helper
	struct base57
	{
		const uchar* data;
		usz size;

		template <typename T>
		base57(const T& arg)
			: data(reinterpret_cast<const uchar*>(&arg))
			, size(sizeof(T))
		{
		}

		base57(const uchar* data, usz size)
			: data(data)
			, size(size)
		{
		}
	};

	template <typename... Args>
	constexpr const fmt_type_info type_info_v[sizeof...(Args) + 1]{fmt_type_info::make<fmt_unveil_t<Args>>()...};

	// Internal formatting function
	void raw_append(std::string& out, const char*, const fmt_type_info*, const u64*) noexcept;

	// Formatting function
	template <typename CharT, usz N, typename... Args>
	FORCE_INLINE SAFE_BUFFERS(void) append(std::string& out, const CharT(&fmt)[N], const Args&... args)
	{
		raw_append(out, reinterpret_cast<const char*>(fmt), type_info_v<Args...>, fmt_args_t<Args...>{fmt_unveil<Args>::get(args)...});
	}

	// Formatting function
	template <typename CharT, usz N, typename... Args>
	FORCE_INLINE SAFE_BUFFERS(std::string) format(const CharT(&fmt)[N], const Args&... args)
	{
		std::string result;
		append(result, fmt, args...);
		return result;
	}

	// Internal exception message formatting template, must be explicitly specialized or instantiated in cpp to minimize code bloat
	[[noreturn]] void raw_throw_exception(const src_loc&, const char*, const fmt_type_info*, const u64*);

	// Throw exception with formatting
	template <typename CharT, usz N, typename... Args>
	struct throw_exception
	{
		[[noreturn]] FORCE_INLINE SAFE_BUFFERS() throw_exception(const CharT(&fmt)[N], const Args&... args,
			u32 line = __builtin_LINE(),
			u32 col = __builtin_COLUMN(),
			const char* file = __builtin_FILE(),
			const char* func = __builtin_FUNCTION())
		{
			raw_throw_exception({line, col, file, func}, reinterpret_cast<const char*>(fmt), type_info_v<Args...>, fmt_args_t<Args...>{fmt_unveil<Args>::get(args)...});
		}

#ifndef _MSC_VER
		[[noreturn]] ~throw_exception();
#endif
	};

	template <typename CharT, usz N, typename... Args>
	throw_exception(const CharT(&)[N], const Args&...) -> throw_exception<CharT, N, Args...>;

	// Helper template: pack format variables
	template <typename Arg = void, typename... Args>
	struct tie
	{
		// Universal reference
		std::add_rvalue_reference_t<Arg> arg;

		tie<Args...> next;

		// Store only references, unveil op is postponed
		tie(Arg&& arg, Args&&... args) noexcept
			: arg(std::forward<Arg>(arg))
			, next(std::forward<Args>(args)...)
		{
		}

		using type = std::remove_cvref_t<Arg>;

		// Storage for fmt_unveil (deferred initialization)
		decltype(fmt_unveil<type>::get(std::declval<Arg>())) value;

		void init(u64 to[])
		{
			value = fmt_unveil<type>::get(arg);
			to[0] = value;
			next.init(to + 1);
		}
	};

	template <>
	struct tie<void>
	{
		void init(u64 to[]) const
		{
			// Isn't really null terminated, this value has no meaning
			to[0] = 0;
		}
	};

	template <typename... Args>
	tie(Args&&... args) -> tie<Args...>;

	// Ensure with formatting
	template <typename T, typename CharT, usz N, typename... Args>
	decltype(auto) ensure(T&& arg, const CharT(&fmt)[N], tie<Args...> args,
		u32 line = __builtin_LINE(),
		u32 col = __builtin_COLUMN(),
		const char* file = __builtin_FILE(),
		const char* func = __builtin_FUNCTION()) noexcept
	{
		if (std::forward<T>(arg)) [[likely]]
		{
			return std::forward<T>(arg);
		}

		// Prepare u64 array
		u64 data[sizeof...(Args) + 1];
		args.init(data);

		raw_throw_exception({line, col, file, func}, reinterpret_cast<const char*>(fmt), type_info_v<std::remove_cvref_t<Args>...>, +data);
	}
}
