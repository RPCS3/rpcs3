#pragma once

#include "types.h"
#include <string>
#include <vector>

/*
C-style format parser. Appends formatted string to `out`, returns number of characters written.
Arguments are provided via `src`. TODO
*/
template<typename Src, typename Size = std::size_t>
Size cfmt_append(std::string& out, const char* fmt, Src&& src)
{
	const std::size_t old_size = out.size();

	out.reserve(old_size + 992);

	struct cfmt_context
	{
		std::size_t size; // Size of current format sequence

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
		out.append(fmt - ctx.size, ctx.size);
		ctx.size = -1;
	};

	// TODO: check overflow
	const auto read_decimal = [&](uint result) -> uint
	{
		while (fmt[0] >= '0' && fmt[0] <= '9')
		{
			result = result * 10 + (fmt[0] - '0');
			fmt++, ctx.size++;
		}

		return result;
	};

	// TODO: remove this
	const auto fallback = [&]()
	{
		const std::string _fmt(fmt - ctx.size, fmt);

		const u64 arg0 = src.template get<u64>();
		const int arg1 = ctx.args >= 1 ? src.template get<int>(1) : 0;
		const int arg2 = ctx.args >= 2 ? src.template get<int>(2) : 0;

		if (const std::size_t _size = std::snprintf(0, 0, _fmt.c_str(), arg0, arg1, arg2))
		{
			out.resize(out.size() + _size);
			std::snprintf(&out.front() + out.size() - _size, _size + 1, _fmt.c_str(), arg0, arg1, arg2);
		}
	};

	// Single pass over fmt string (null-terminated), TODO: check correct order
	while (const char ch = *fmt++) if (ctx.size == 0)
	{
		if (ch == '%')
		{
			ctx.size = 1;
		}
		else
		{
			out += ch;
		}
	}
	else if (ctx.size == 1 && ch == '%')
	{
		ctx = {0};
		out += ch;
	}
	else if (ctx.size == -1)
	{
		out += ch;
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
		ctx.width = read_decimal(ch - '0');
		break;
	}

	case '*':
	{
		if (!src.test(++ctx.args))
		{
			drop_sequence();
			break;
		}

		const int warg = src.template get<int>(ctx.args);
		ctx.width = std::abs(warg);
		ctx.left |= warg < 0;
		break;
	}

	case '.':
	{
		if (*fmt >= '0' && *fmt <= '9') // TODO: does it allow '0'?
		{
			ctx.prec = read_decimal(0);
			ctx.dot = true;
		}
		else if (*fmt == '*')
		{
			if (!src.test(++ctx.args))
			{
				drop_sequence();
				break;
			}

			fmt++, ctx.size++;
			const int parg = src.template get<int>(ctx.args);
			ctx.prec = parg;
			ctx.dot = parg >= 0;
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
		if (ctx.type)
		{
			drop_sequence();
		}
		else if (fmt[0] == 'h')
		{
			fmt++, ctx.size++;
			ctx.type = sizeof(char);
		}
		else
		{
			ctx.type = sizeof(short);
		}

		break;
	}

	case 'l':
	{
		if (ctx.type)
		{
			drop_sequence();
		}
		else if (fmt[0] == 'l')
		{
			fmt++, ctx.size++;
			ctx.type = sizeof(llong);
		}
		else
		{
			ctx.type = sizeof(long);
		}

		break;
	}

	case 'z':
	{
		if (ctx.type)
		{
			drop_sequence();
		}
		else
		{
			ctx.type = sizeof(std::size_t);
		}

		break;
	}

	case 'j':
	{
		if (ctx.type)
		{
			drop_sequence();
		}
		else
		{
			ctx.type = sizeof(std::intmax_t);
		}

		break;
	}

	case 't':
	{
		if (ctx.type)
		{
			drop_sequence();
		}
		else
		{
			ctx.type = sizeof(std::ptrdiff_t);
		}

		break;
	}

	case 'c':
	{
		if (ctx.type || !src.test())
		{
			drop_sequence();
			break;
		}

		const std::size_t start = out.size();
		out += src.template get<char>(0);

		if (1 < ctx.width)
		{
			// Add spaces if necessary
			out.insert(start + ctx.left, ctx.width - 1, ' ');
		}

		src.skip(ctx.args);
		ctx = {0};
		break;
	}

	case 's':
	{
		if (ctx.type || !src.test())
		{
			drop_sequence();
			break;
		}

		const std::size_t start = out.size();
		const std::size_t size1 = src.fmt_string(out);
		
		if (ctx.dot && size1 > ctx.prec)
		{
			// Shrink if necessary
			out.resize(start + ctx.prec);
		}

		// TODO: how it works if precision and width specified simultaneously?
		const std::size_t size2 = out.size() - start;

		if (size2 < ctx.width)
		{
			// Add spaces if necessary
			out.insert(ctx.left ? out.size() : start, ctx.width - size2, ' ');
		}

		src.skip(ctx.args);
		ctx = {0};
		break;
	}

	case 'd':
	case 'i':
	{
		if (!src.test())
		{
			drop_sequence();
			break;
		}

		if (!ctx.type)
		{
			ctx.type = sizeof(int);
		}

		fallback(); // TODO
		src.skip(ctx.args);
		ctx = {0};
		break;
	}

	case 'o':
	{
		if (!src.test())
		{
			drop_sequence();
			break;
		}

		if (!ctx.type)
		{
			ctx.type = sizeof(int);
		}

		fallback(); // TODO
		src.skip(ctx.args);
		ctx = {0};
		break;
	}

	case 'x':
	case 'X':
	{
		if (!src.test())
		{
			drop_sequence();
			break;
		}

		if (!ctx.type)
		{
			ctx.type = sizeof(int);
		}

		fallback(); // TODO
		src.skip(ctx.args);
		ctx = {0};
		break;
	}

	case 'u':
	{
		if (!src.test())
		{
			drop_sequence();
			break;
		}

		if (!ctx.type)
		{
			ctx.type = sizeof(int);
		}

		fallback(); // TODO
		src.skip(ctx.args);
		ctx = {0};
		break;
	}

	case 'p':
	{
		if (!src.test())
		{
			drop_sequence();
			break;
		}

		if (ctx.type)
		{
			drop_sequence();
			break;
		}

		fallback(); // TODO
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
		if (!src.test())
		{
			drop_sequence();
			break;
		}

		if (ctx.type)
		{
			drop_sequence();
			break;
		}

		fallback(); // TODO
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
	if (ctx.size && ctx.size != -1)
	{
		fmt--, drop_sequence();
	}

	return static_cast<Size>(out.size() - old_size);
}
