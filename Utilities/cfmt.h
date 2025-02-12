#pragma once

#include "util/types.hpp"
#include <string>
#include <algorithm>
#include "util/asm.hpp"

/*
C-style format parser. Appends formatted string to `out`, returns number of characters written.
`out`: mutable reference to std::string, std::vector<char> or other compatible container
`fmt`: null-terminated string of `Char` type (char or constructible from char)
`src`: rvalue reference to argument provider.
*/
template<typename Dst, typename Char, typename Src>
usz cfmt_append(Dst& out, const Char* fmt, Src&& src)
{
	ensure(fmt);

	const usz start_pos = out.size();

	struct cfmt_context
	{
		usz size; // Size of current format sequence

		u8 args; // Number of extra args used
		u8 type; // Integral type bytesize
		bool dot; // Precision enabled
		bool left;
		bool sign;
		bool space;
		bool alter;
		bool zeros;

		uint width;
		uint prec;
	};

	cfmt_context ctx{0};

	// Error handling: print untouched sequence, stop further formatting
	const auto drop_sequence = [&]
	{
		out.insert(out.end(), fmt - ctx.size, fmt);
		ctx.size = umax;
	};

	const auto read_decimal = [&](uint result) -> uint
	{
		while (fmt[0] >= '0' && fmt[0] <= '9' && result <= (uint{umax} / 10))
		{
			result = result * 10 + (fmt[0] - '0');
			fmt++, ctx.size++;
		}

		return result;
	};

	const auto write_octal = [&](auto value, u64 min_num)
	{
		if constexpr (sizeof(value) == 16)
		{
			out.resize(out.size() + std::max<u64>(min_num, 129 / 3 - (utils::clz128(value | 1) + 1) / 3), '0');
		}
		else
		{
			out.resize(out.size() + std::max<u64>(min_num, 66 / 3 - (std::countl_zero<u64>(value | 1) + 2) / 3), '0');
		}

		// Write in reversed order
		for (auto i = out.rbegin(); value; i++, value >>= 3)
		{
			*i = static_cast<char>(static_cast<u64>(value) & 7) + '0';
		}
	};

	const auto write_hex = [&](auto value, bool upper, u64 min_num)
	{
		if constexpr (sizeof(value) == 16)
		{
			out.resize(out.size() + std::max<u64>(min_num, 128 / 4 - utils::clz128(value | 1) / 4), '0');
		}
		else
		{
			out.resize(out.size() + std::max<u64>(min_num, 64 / 4 - std::countl_zero<u64>(value | 1) / 4), '0');
		}

		// Write in reversed order
		for (auto i = out.rbegin(); value; i++, value >>= 4)
		{
			*i = (upper ? "0123456789ABCDEF" : "0123456789abcdef")[static_cast<usz>(value & 0xf)];
		}
	};

	const auto write_decimal = [&](auto value, s64 min_size)
	{
		const usz start = out.size();

		do
		{
			if constexpr (sizeof(value) == 16)
			{
				const u128 v0 = value;
				value >>= 1;
				constexpr u128 by_five = 0x3333'3333'3333'3333;
				const u128 v1 = (value >> 64) * by_five;
				const u128 v2 = ((value >> 64) * by_five) >> 64;
				const u128 v3 = (static_cast<u64>(value) * by_five) >> 64;
				value = v1 + v2 + v3;

				out.push_back(static_cast<char>(static_cast<u64>(v0 - (value * 10))) + '0');
			}
			else
			{
				out.push_back(value % 10 + '0');
				value /= 10;
			}
		}
		while (0 < --min_size || value);

		// Revert written characters
		for (usz i = start, j = out.size() - 1; i < j; i++, j--)
		{
			std::swap(out[i], out[j]);
		}
	};

	// Single pass over fmt string (null-terminated), TODO: check correct order
	while (const Char ch = *fmt++) if (ctx.size == 0)
	{
		if (ch == '%')
		{
			ctx.size = 1;
		}
		else
		{
			out.push_back(ch);
		}
	}
	else if (ctx.size == 1 && ch == '%')
	{
		ctx = {0};
		out.push_back(ch);
	}
	else if (ctx.size == umax)
	{
		out.push_back(ch);
	}
	else switch (ctx.size++, ch)
	{
	case '-': ctx.left = true; break;
	case '+': ctx.sign = true; break;
	case ' ': ctx.space = true; break;
	case '#': ctx.alter = true; break;
	case '0': ctx.zeros = true; break;

	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	{
		if (ctx.width) [[unlikely]]
		{
			drop_sequence();
		}
		else
		{
			ctx.width = read_decimal(ch - '0');
		}

		break;
	}

	case '*':
	{
		if (ctx.width || !src.test(ctx.args)) [[unlikely]]
		{
			drop_sequence();
		}
		else
		{
			const int warg = src.template get<int>(ctx.args++);
			ctx.width = std::abs(warg);
			ctx.left |= warg < 0;
		}

		break;
	}

	case '.':
	{
		if (ctx.dot || ctx.prec) [[unlikely]]
		{
			drop_sequence();
		}
		else if (*fmt >= '0' && *fmt <= '9') // TODO: does it allow '0'?
		{
			ctx.prec = read_decimal(0);
			ctx.dot = true;
		}
		else if (*fmt == '*')
		{
			if (!src.test(ctx.args)) [[unlikely]]
			{
				drop_sequence();
			}
			else
			{
				fmt++, ctx.size++;
				const int parg = src.template get<int>(ctx.args++);
				ctx.prec = std::max(parg, 0);
				ctx.dot = parg >= 0;
			}
		}
		else
		{
			ctx.prec = 0;
			ctx.dot = true;
		}

		break;
	}

	case 'h':
	{
		if (ctx.type) [[unlikely]]
		{
			drop_sequence();
		}
		else if (fmt[0] == 'h')
		{
			fmt++, ctx.size++;
			ctx.type = src.size_char;
		}
		else
		{
			ctx.type = src.size_short;
		}

		break;
	}

	case 'l':
	{
		if (ctx.type) [[unlikely]]
		{
			drop_sequence();
		}
		else if (fmt[0] == 'l')
		{
			fmt++, ctx.size++;
			ctx.type = src.size_llong;
		}
		else
		{
			ctx.type = src.size_long;
		}

		break;
	}

	case 'z':
	{
		if (ctx.type) [[unlikely]]
		{
			drop_sequence();
		}
		else
		{
			ctx.type = src.size_size;
		}

		break;
	}

	case 'j':
	{
		if (ctx.type) [[unlikely]]
		{
			drop_sequence();
		}
		else
		{
			ctx.type = src.size_max;
		}

		break;
	}

	case 't':
	{
		if (ctx.type) [[unlikely]]
		{
			drop_sequence();
		}
		else
		{
			ctx.type = src.size_diff;
		}

		break;
	}

	case 'c':
	{
		if (ctx.type || !src.test(ctx.args)) [[unlikely]]
		{
			drop_sequence();
			break;
		}

		const usz start = out.size();
		out.push_back(src.template get<Char>(ctx.args));

		if (1 < ctx.width)
		{
			// Add spaces if necessary
			out.insert(out.begin() + start + ctx.left, ctx.width - 1, ' ');
		}

		src.skip(ctx.args);
		ctx = {0};
		break;
	}

	case 's':
	{
		if (ctx.type || !src.test(ctx.args)) [[unlikely]]
		{
			drop_sequence();
			break;
		}

		const usz start = out.size();
		const usz size1 = src.fmt_string(out, ctx.args);

		if (ctx.dot && size1 > ctx.prec)
		{
			// Shrink if necessary
			out.resize(start + ctx.prec);
		}

		const usz size2 = out.size() - start;

		if (size2 < ctx.width)
		{
			// Add spaces if necessary
			out.insert(ctx.left ? out.end() : out.begin() + start, ctx.width - size2, ' ');
		}

		src.skip(ctx.args);
		ctx = {0};
		break;
	}

	case 'd':
	case 'i':
	{
		if (!src.test(ctx.args)) [[unlikely]]
		{
			drop_sequence();
			break;
		}

		const usz src_type = src.type(ctx.args);

		if (!ctx.type || src_type > 8)
		{
			ctx.type = static_cast<u8>(src_type);

			if (!ctx.type)
			{
				ctx.type = src.size_int;
			}
		}

		// Sign-extended argument expected
		const u64 val = src.template get<u64>(ctx.args);
		bool negative = ctx.type && ctx.type <= 8 && static_cast<s64>(val) < 0;

		if (ctx.type == 16)
		{
			u128 val2 = *reinterpret_cast<const u128*>(val);
			negative = !!(val2 & (u128{1} << 127));
		}

		const usz start = out.size();

		if (!ctx.dot || ctx.prec)
		{
			if (negative)
			{
				out.push_back('-');
			}
			else if (ctx.sign)
			{
				out.push_back('+');
			}
			else if (ctx.space)
			{
				out.push_back(' ');
			}

			if (ctx.type >= 16)
			{
				u128 val2 = *reinterpret_cast<const u128*>(val);
				write_decimal(negative ? u128{} - val2 : val2, ctx.prec);
			}
			else
			{
				write_decimal(negative ? u64{} - val : val, ctx.prec);
			}
		}

		const usz size2 = out.size() - start;

		if (size2 < ctx.width)
		{
			// Add padding if necessary
			if (ctx.zeros && !ctx.left && !ctx.dot)
			{
				out.insert(out.begin() + start + (negative || ctx.sign || ctx.space), ctx.width - size2, '0');
			}
			else
			{
				out.insert(ctx.left ? out.end() : out.begin() + start, ctx.width - size2, ' ');
			}
		}

		src.skip(ctx.args);
		ctx = {0};
		break;
	}

	case 'o':
	{
		if (!src.test(ctx.args)) [[unlikely]]
		{
			drop_sequence();
			break;
		}

		const usz src_type = src.type(ctx.args);

		if (!ctx.type || src_type > 8)
		{
			ctx.type = static_cast<u8>(src_type);

			if (!ctx.type)
			{
				ctx.type = src.size_int;
			}
		}

		const u64 mask =
			ctx.type == 1 ? 0xff :
			ctx.type == 2 ? 0xffff :
			ctx.type == 4 ? 0xffff'ffffu :
			0xffff'ffff'ffff'ffffu;

		// Trunc sign-extended signed types
		const u64 val = src.template get<u64>(ctx.args) & mask;

		const usz start = out.size();

		if (ctx.alter)
		{
			out.push_back('0');

			if (ctx.prec)
			{
				ctx.prec--;
			}
		}

		if ((ctx.alter && val) || !ctx.dot || ctx.prec)
		{
			if (ctx.type >= 16)
			{
				u128 val2 = *reinterpret_cast<const u128*>(val);
				write_octal(val2, ctx.prec);
			}
			else
			{
				write_octal(val, ctx.prec);
			}
		}

		const usz size2 = out.size() - start;

		if (size2 < ctx.width)
		{
			// Add padding if necessary
			out.insert(ctx.left ? out.end() : out.begin() + start, ctx.width - size2, ctx.zeros && !ctx.left && !ctx.dot ? '0' : ' ');
		}

		src.skip(ctx.args);
		ctx = {0};
		break;
	}

	case 'x':
	case 'X':
	{
		if (!src.test(ctx.args)) [[unlikely]]
		{
			drop_sequence();
			break;
		}

		const usz src_type = src.type(ctx.args);

		if (!ctx.type || src_type > 8)
		{
			ctx.type = static_cast<u8>(src_type);

			if (!ctx.type)
			{
				ctx.type = src.size_int;
			}
		}

		const u64 mask =
			ctx.type == 1 ? 0xff :
			ctx.type == 2 ? 0xffff :
			ctx.type == 4 ? 0xffff'ffffu :
			0xffff'ffff'ffff'ffffu;

		// Trunc sign-extended signed types
		const u64 val = src.template get<u64>(ctx.args) & mask;

		const usz start = out.size();

		if (ctx.alter)
		{
			out.push_back('0');

			if (val)
			{
				out.push_back(ch); // Prepend 0x or 0X
			}
		}

		if ((ctx.alter && val) || !ctx.dot || ctx.prec)
		{
			if (ctx.type >= 16)
			{
				u128 val2 = *reinterpret_cast<const u128*>(val);
				write_hex(val2, ch == 'X', ctx.prec);
			}
			else
			{
				write_hex(val, ch == 'X', ctx.prec);
			}
		}

		const usz size2 = out.size() - start;

		if (size2 < ctx.width)
		{
			// Add padding if necessary
			if (ctx.zeros && !ctx.left && !ctx.dot)
			{
				out.insert(out.begin() + start + (ctx.alter && val ? 2 : 0), ctx.width - size2, '0');
			}
			else
			{
				out.insert(ctx.left ? out.end() : out.begin() + start, ctx.width - size2, ' ');
			}
		}

		src.skip(ctx.args);
		ctx = {0};
		break;
	}

	case 'u':
	{
		if (!src.test(ctx.args)) [[unlikely]]
		{
			drop_sequence();
			break;
		}

		const usz src_type = src.type(ctx.args);

		if (!ctx.type || src_type > 8)
		{
			ctx.type = static_cast<u8>(src_type);

			if (!ctx.type)
			{
				ctx.type = src.size_int;
			}
		}

		const u64 mask =
			ctx.type == 1 ? 0xff :
			ctx.type == 2 ? 0xffff :
			ctx.type == 4 ? 0xffff'ffffu :
			0xffff'ffff'ffff'ffffu;

		// Trunc sign-extended signed types
		const u64 val = src.template get<u64>(ctx.args) & mask;

		const usz start = out.size();

		if (!ctx.dot || ctx.prec)
		{
			if (ctx.type >= 16)
			{
				u128 val2 = *reinterpret_cast<const u128*>(val);
				write_decimal(val2, ctx.prec);
			}
			else
			{
				write_decimal(val, ctx.prec);
			}
		}

		const usz size2 = out.size() - start;

		if (size2 < ctx.width)
		{
			// Add padding if necessary
			out.insert(ctx.left ? out.end() : out.begin() + start, ctx.width - size2, ctx.zeros && !ctx.left && !ctx.dot ? '0' : ' ');
		}

		src.skip(ctx.args);
		ctx = {0};
		break;
	}

	case 'p':
	{
		if (!src.test(ctx.args) || ctx.type) [[unlikely]]
		{
			drop_sequence();
			break;
		}

		const u64 val = src.template get<u64>(ctx.args);

		const usz start = out.size();

		write_hex(val, false, sizeof(void*) * 2);

		const usz size2 = out.size() - start;

		if (size2 < ctx.width)
		{
			// Add padding if necessary
			out.insert(ctx.left ? out.end() : out.begin() + start, ctx.width - size2, ' ');
		}

		src.skip(ctx.args);
		ctx = {0};
		break;
	}

	case 'f':
	case 'F':
	case 'e':
	case 'E':
	case 'a':
	case 'A':
	case 'g':
	case 'G':
	{
		if (!src.test(ctx.args) || ctx.type) [[unlikely]]
		{
			drop_sequence();
			break;
		}

		// Fallback (TODO)

		const std::string _fmt(fmt - ctx.size, fmt);

		const f64 arg0 = src.template get<f64>(0);
		const u64 arg1 = ctx.args >= 1 ? src.template get<u64>(1) : 0;
		const u64 arg2 = ctx.args >= 2 ? src.template get<u64>(2) : 0;

		if (const usz _size = std::snprintf(nullptr, 0, _fmt.c_str(), arg0, arg1, arg2))
		{
			out.resize(out.size() + _size);
			std::snprintf(&out.front() + out.size() - _size, _size + 1, _fmt.c_str(), arg0, arg1, arg2);
		}

		src.skip(ctx.args);
		ctx = {0};
		break;
	}

	case 'L': // long double, not supported
	case 'n': // writeback, not supported
	default:
	{
		drop_sequence();
	}
	}

	// Handle unfinished sequence
	if (ctx.size && ctx.size != umax)
	{
		fmt--, drop_sequence();
	}

	return out.size() - start_pos;
}
