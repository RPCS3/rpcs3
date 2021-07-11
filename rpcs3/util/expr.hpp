#pragma once

#include "util/types.hpp"

template <usz Size>
struct utf32_expr
{
	static constexpr usz size = Size;

	static_assert(Size > 0);

	char32_t data[Size]{};

	template <usz N>
	constexpr utf32_expr(const char32_t(&str)[N])
	{
		for (usz i = 0; i < Size; i++)
		{
			data[i] = str[i];
		}
	}

	template <typename CharT, usz N> requires (sizeof(CharT) == 1)
	constexpr utf32_expr(const CharT(&str)[N])
	{
		for (usz i = 0, j = 0; i < Size && j < Size; j++)
		{
			const uchar ch = static_cast<uchar>(str[j]);

			if (ch >= 0 && ch <= 0x7f)
			{
				// Write single ASCII character
				data[i++] = ch;
			}
			else if ((ch & 0xc0) == 0x80)
			{
				// Append UTF-8 encoded tail
				data[i] = (data[i] << 6) | ch;
			}
			else
			{
				// Write UTF-8 head
				const uint sh = std::countl_one(ch);
				data[i++] = ((ch << sh) & 0xff) >> sh;
			}
		}
	}
};

template <usz Size>
utf32_expr(const char32_t(&a)[Size]) -> utf32_expr<Size>;

template <typename CharT, usz Size> requires (sizeof(CharT) == 1)
utf32_expr(const CharT(&a)[Size]) -> utf32_expr<Size>;

template <char32_t Cmd, utf32_expr S, usz I = 1>
struct utf32_cmd
{
	static_assert(!Cmd || (Cmd >= U'0' && Cmd <= U'9') || (Cmd >= U'i' && Cmd <= U'n'), "Unsupported command");

	template <typename S0, typename... SS>
	static FORCE_INLINE constexpr SAFE_BUFFERS(auto) eval(u64 ctrs[], S0&& s0, SS&&... stack)
	{
		if constexpr (!Cmd)
		{
			// Null terminator
			if constexpr (sizeof...(SS) == 0)
			{
				return std::forward<S0>(s0);
			}
			else
			{
				return std::forward_as_tuple<S0, SS...>(s0, stack...);
			}
		}
		else if constexpr (Cmd >= U'0' && Cmd <= U'9')
		{
			// Members 0..9 of tuples or other structural objects
			return utf32_cmd<S.data[I], S, I + 1>::eval(ctrs, std::get<Cmd - U'0'>(s0), std::forward<SS>(stack)...);
		}
		else if constexpr (Cmd >= U'i' && Cmd <= U'n')
		{
			// Variables i, j, k, l, m, n (six @ loop counters)
			return utf32_cmd<S.data[I], S, I + 1>::eval(ctrs, ctrs[Cmd - U'i'], std::forward<S0>(s0), std::forward<SS>(stack)...);
		}
	}
};

// Loop start
template <utf32_expr S, usz I>
struct utf32_cmd<U'@', S, I>
{
	template <typename S0, typename... SS>
	static FORCE_INLINE constexpr SAFE_BUFFERS(auto) eval(u64 ctrs[], S0&& s0, SS&&... stack)
	{
		ctrs[0] = 0;

		while (ctrs[0] != umax)
		{
			utf32_cmd<S.data[I], S, I + 1>::eval(ctrs + 1, std::forward<S0>(s0), std::forward<SS>(stack)...);

			ctrs[0]++;
		}

		// TODO
		return;
	}
};

// Read current level loop counter
template <utf32_expr S, usz I>
struct utf32_cmd<U'o', S, I>
{
	template <typename S0, typename... SS>
	static FORCE_INLINE constexpr SAFE_BUFFERS(auto) eval(u64 ctrs[], S0&& s0, SS&&... stack)
	{
		return utf32_cmd<S.data[I], S, I + 1>::eval(ctrs, ctrs[-1], std::forward<S0>(s0), std::forward<SS>(stack)...);
	}
};

// Loop unconditional terminator
template <utf32_expr S, usz I>
struct utf32_cmd<U';', S, I>
{
	template <typename... SS>
	static FORCE_INLINE constexpr SAFE_BUFFERS(auto) eval(u64 ctrs[], SS&&... stack)
	{
		ctrs[-1] = -1;
		return utf32_cmd<S.data[I], S, I + 1>::eval(ctrs - 1, std::forward<SS>(stack)...);
	}
};

// Loop continue-else
template <utf32_expr S, usz I>
struct utf32_cmd<U'\\', S, I>
{
	template <typename... SS>
	static FORCE_INLINE constexpr SAFE_BUFFERS(void) eval(u64 /*ctrs[]*/, SS&&... /*stack*/)
	{
		return;
	}
};

// Loop "if-else" block
template <utf32_expr S, usz I>
struct utf32_cmd<U'?', S, I>
{
	template <char32_t Cmd, usz J, usz L = 0, typename... SS>
	static FORCE_INLINE constexpr SAFE_BUFFERS(auto) skip(u64 ctrs[], SS&&... stack)
	{
		if constexpr (Cmd == U'\\' && L == 0)
		{
			return utf32_cmd<S.data[J], S, J + 1>::eval(ctrs, std::forward<SS>(stack)...);
		}

		if constexpr (Cmd == U'@')
		{
			return skip<S.data[J], J + 1, L + 1>(ctrs, std::forward<SS>(stack)...);
		}

		if constexpr (Cmd == U';')
		{
			if constexpr (L > 0)
			{
				return skip<S.data[J], J + 1, L - 1>(ctrs, std::forward<SS>(stack)...);
			}
			else
			{
				return;
			}
		}

		skip<S.data[J], J + 1, L>(ctrs, std::forward<SS>(stack)...);
	}

	template <typename S0, typename... SS>
	static FORCE_INLINE constexpr SAFE_BUFFERS(auto) eval(u64 ctrs[], S0&& s0, SS&&... stack)
	{
		if (s0)
		{
			utf32_cmd<S.data[I], S, I + 1>::eval(ctrs, std::forward<SS>(stack)...);
		}
		else
		{
			skip<S.data[I], S, I + 1>(std::forward<SS>(stack)...);
		}
	}
};

// Duplicate top stack value
template <utf32_expr S, usz I>
struct utf32_cmd<U':', S, I>
{
	template <typename S0, typename... SS>
	static FORCE_INLINE constexpr SAFE_BUFFERS(auto) eval(u64 ctrs[], S0&& s0, SS&&... stack)
	{
		return utf32_cmd<S.data[I], S, I + 1>::eval(ctrs, std::forward<S0>(s0), std::forward<S0>(s0), std::forward<SS>(stack)...);
	}
};

// Drop top stack value
template <utf32_expr S, usz I>
struct utf32_cmd<U'.', S, I>
{
	template <typename S0, typename... SS>
	static FORCE_INLINE constexpr SAFE_BUFFERS(auto) eval(u64 ctrs[], S0&& s0, SS&&... stack)
	{
		return utf32_cmd<S.data[I], S, I + 1>::eval(ctrs, std::forward<SS>(stack)...);
	}
};

// Duplicate two top stack values
template <utf32_expr S, usz I>
struct utf32_cmd<U'#', S, I>
{
	template <typename S0, typename S1, typename... SS>
	static FORCE_INLINE constexpr SAFE_BUFFERS(auto) eval(u64 ctrs[], S0&& s0, S1&& s1, SS&&... stack)
	{
		return utf32_cmd<S.data[I], S, I + 1>::eval(ctrs, std::forward<S0>(s0), std::forward<S0>(s1), std::forward<S0>(s0), std::forward<S1>(s1), std::forward<SS>(stack)...);
	}
};

// Swap two top stack values
template <utf32_expr S, usz I>
struct utf32_cmd<U's', S, I>
{
	template <typename S0, typename S1, typename... SS>
	static FORCE_INLINE constexpr SAFE_BUFFERS(auto) eval(u64 ctrs[], S0&& s0, S1&& s1, SS&&... stack)
	{
		return utf32_cmd<S.data[I], S, I + 1>::eval(ctrs, std::forward<S1>(s1), std::forward<S0>(s0), std::forward<SS>(stack)...);
	}
};

// Rotate stack so the top is moved to the bottom
template <utf32_expr S, usz I>
struct utf32_cmd<U',', S, I>
{
	template <typename S0, typename... SS>
	static FORCE_INLINE constexpr SAFE_BUFFERS(auto) eval(u64 ctrs[], S0&& s0, SS&&... stack)
	{
		return utf32_cmd<S.data[I], S, I + 1>::eval(ctrs, std::forward<SS>(stack)..., std::forward<S0>(s0));
	}
};

// Negate top stack value
template <utf32_expr S, usz I>
struct utf32_cmd<U'!', S, I>
{
	template <typename S0, typename... SS>
	static FORCE_INLINE constexpr SAFE_BUFFERS(auto) eval(u64 ctrs[], S0&& s0, SS&&... stack)
	{
		return utf32_cmd<S.data[I], S, I + 1>::eval(ctrs, std::forward<S0>(s0) ? false : true, std::forward<SS>(stack)...);
	}
};

// Bitwise negate
template <utf32_expr S, usz I>
struct utf32_cmd<U'~', S, I>
{
	template <typename S0, typename... SS> requires requires (S0&& v) { ~v; }
	static FORCE_INLINE constexpr SAFE_BUFFERS(auto) eval(u64 ctrs[], S0&& s0, SS&&... stack)
	{
		// Prefer existing operator ~
		using type = std::remove_cvref_t<S0>;

		if constexpr (std::is_same_v<type, bool>)
		{
			// Fallback to logic negation
			return utf32_cmd<S.data[I], S, I + 1>::eval(ctrs, !std::forward<S0>(s0), std::forward<SS>(stack)...);
		}
		else if constexpr (std::is_integral_v<type>)
		{
			// Disable integer promotion
			return utf32_cmd<S.data[I], S, I + 1>::eval(ctrs, static_cast<type>(~std::forward<S0>(s0)), std::forward<SS>(stack)...);
		}
		else
		{
			return utf32_cmd<S.data[I], S, I + 1>::eval(ctrs, ~std::forward<S0>(s0), std::forward<SS>(stack)...);
		}
	}

	template <typename S0, typename... SS>
	static FORCE_INLINE constexpr SAFE_BUFFERS(auto) eval(u64 ctrs[], S0&& s0, SS&&... stack)
	{
		// Synthesize operator ~ for pointers, floats, and all trivially copyable types
		using type = std::remove_cvref_t<S0>;

		static_assert(std::is_trivially_copyable_v<type>);

		if constexpr (sizeof(type) == 8 || sizeof(type) == 4 || sizeof(type) == 2)
		{
			using itype = get_uint_t<sizeof(type)>;

			return utf32_cmd<S.data[I], S, I + 1>::eval(ctrs, std::bit_cast<type, itype>(~std::bit_cast<itype>(std::forward<S0>(s0))), std::forward<SS>(stack)...);
		}
		else
		{
			uchar buf[sizeof(type)];

			for (usz i = 0; i < sizeof(type); i++)
			{
				buf[i] = ~reinterpret_cast<const uchar*>(&s0)[i];
			}

			return utf32_cmd<S.data[I], S, I + 1>::eval(ctrs, std::bit_cast<type>(buf), std::forward<SS>(stack)...);
		}
	}
};

// Following operators are big TODO
#define REG_BIN_OP(ch, _func)\
template <utf32_expr S, usz I>\
struct utf32_cmd<ch, S, I>\
{\
	template <typename S0, typename S1, typename... SS>\
	static FORCE_INLINE constexpr SAFE_BUFFERS(auto) eval(u64 ctrs[], S0&& s0, S1&& s1, SS&&... stack)\
	{\
		return utf32_cmd<S.data[I], S, I + 1>::eval(ctrs, (_func)(std::forward<S0>(s0), std::forward<S1>(s1)), std::forward<SS>(stack)...);\
	}\
};\

REG_BIN_OP(U'+', [](const auto& a, const auto& b){ return a + b; })
REG_BIN_OP(U'-', [](const auto& a, const auto& b){ return a - b; })
REG_BIN_OP(U'*', [](const auto& a, const auto& b){ return a * b; })
REG_BIN_OP(U'/', [](const auto& a, const auto& b){ return a / b; })
REG_BIN_OP(U'&', [](const auto& a, const auto& b){ return a & b; })
REG_BIN_OP(U'|', [](const auto& a, const auto& b){ return a | b; })
REG_BIN_OP(U'^', [](const auto& a, const auto& b){ return a ^ b; })
REG_BIN_OP(U'«', [](const auto& a, const auto& b){ return a << b; })
REG_BIN_OP(U'»', [](const auto& a, const auto& b){ return a >> b; })
REG_BIN_OP(U'<', [](const auto& a, const auto& b){ return a < b; })
REG_BIN_OP(U'>', [](const auto& a, const auto& b){ return a > b; })
REG_BIN_OP(U'=', [](const auto& a, const auto& b){ return a == b; })
REG_BIN_OP(U'≠', [](const auto& a, const auto& b){ return a != b; })
REG_BIN_OP(U'≥', [](const auto& a, const auto& b){ return a >= b; })
REG_BIN_OP(U'≤', [](const auto& a, const auto& b){ return a <= b; })
#undef REG_BIN_OP


template <utf32_expr S>
struct utf32_eval
{
	// Initial state machine
	template <typename... Args>
	FORCE_INLINE constexpr SAFE_BUFFERS(auto) operator()(Args&&... args) const
	{
		// Could possibly be extended
		static_assert(sizeof...(args) <= 10, "Only operands 0..9 are supported.");

		// Loop counters (safe size up to the length of expr plus ';' handling)
		u64 counters[S.size + 1]{};

		// Put the args on top of stack
		return utf32_cmd<S.data[0], S>::eval(counters + 1, std::forward<Args>(args)...);
	}
};

#ifdef __INTELLISENSE__
constexpr std::true_type operator""_x(const char32_t* str, usz);
constexpr std::true_type operator""_x(const char* str, usz);
#else
template <utf32_expr S>
constexpr auto operator""_x()
{
	return utf32_eval<S>();
}
#endif

template <utf32_expr P, typename T>
constexpr decltype(auto) ensure(T&& arg, const_str msg = const_str(),
	u32 line = __builtin_LINE(),
	u32 col = __builtin_COLUMN(),
	const char* file = __builtin_FILE(),
	const char* func = __builtin_FUNCTION()) noexcept
{
	if (utf32_eval<P>()(std::forward<T>(arg))) [[likely]]
	{
		return std::forward<T>(arg);
	}

	fmt::raw_verify_error({line, col, file, func}, msg);
}
