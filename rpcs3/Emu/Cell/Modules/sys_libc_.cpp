#include "stdafx.h"
#include "Emu/Cell/lv2/sys_tty.h"
#include "Emu/Cell/PPUModule.h"
#include "Utilities/cfmt.h"

LOG_CHANNEL(sysPrxForUser);

// cfmt implementation (TODO)

using qsortcmp = s32(vm::cptr<void> e1, vm::cptr<void> e2);

struct ps3_fmt_src
{
	ppu_thread* ctx;
	u32 g_count;

	static bool test(usz)
	{
		return true;
	}

	template <typename T>
	T get(usz index) const
	{
		const u32 i = static_cast<u32>(index) + g_count;
		return ppu_gpr_cast<T>(i < 8 ? ctx->gpr[3 + i] : +*ctx->get_stack_arg(i));
	}

	void skip(usz extra)
	{
		g_count += static_cast<u32>(extra) + 1;
	}

	usz fmt_string(std::string& out, usz extra) const
	{
		const usz start = out.size();
		out += vm::_ptr<const char>(get<u32>(extra));
		return out.size() - start;
	}

	static usz type(usz)
	{
		return 0;
	}

	static constexpr usz size_char  = 1;
	static constexpr usz size_short = 2;
	static constexpr usz size_int   = 4;
	static constexpr usz size_long  = 4;
	static constexpr usz size_llong = 8;
	static constexpr usz size_size  = 4;
	static constexpr usz size_max   = 8;
	static constexpr usz size_diff  = 4;
};

template <>
f64 ps3_fmt_src::get<f64>(usz index) const
{
	return std::bit_cast<f64>(get<u64>(index));
}

static std::string ps3_fmt(ppu_thread& context, vm::cptr<char> fmt, u32 g_count)
{
	std::string result;

	cfmt_append(result, fmt.get_ptr(), ps3_fmt_src{&context, g_count});

	return result;
}

static const std::array<s16, 129> s_ctype_table
{
	0,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x408,
	8, 8, 8, 8,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x18,
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x41, 0x41, 0x41, 0x41, 0x41, 0x41,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	0x10, 0x10, 0x10, 0x10, 0x20,
};

s16 __sys_look_ctype_table(s32 ch)
{
	sysPrxForUser.trace("__sys_look_ctype_table(ch=%d)", ch);

	ensure(ch >= -1 && ch <= 127); // "__sys_look_ctype_table"

	return s_ctype_table[ch + 1];
}

s32 _sys_tolower(s32 ch)
{
	sysPrxForUser.trace("_sys_tolower(ch=%d)", ch);

	ensure(ch >= -1 && ch <= 127); // "_sys_tolower"

	return s_ctype_table[ch + 1] & 1 ? ch + 0x20 : ch;
}

s32 _sys_toupper(s32 ch)
{
	sysPrxForUser.trace("_sys_toupper(ch=%d)", ch);

	ensure(ch >= -1 && ch <= 127); // "_sys_toupper"

	return s_ctype_table[ch + 1] & 2 ? ch - 0x20 : ch;
}

vm::ptr<void> _sys_memset(vm::ptr<void> dst, s32 value, u32 size)
{
	sysPrxForUser.trace("_sys_memset(dst=*0x%x, value=%d, size=0x%x)", dst, value, size);

	std::memset(dst.get_ptr(), value, size);

	return dst;
}

vm::ptr<void> _sys_memcpy(vm::ptr<void> dst, vm::cptr<void> src, u32 size)
{
	sysPrxForUser.trace("_sys_memcpy(dst=*0x%x, src=*0x%x, size=0x%x)", dst, src, size);

	std::memcpy(dst.get_ptr(), src.get_ptr(), size);

	return dst;
}

s32 _sys_memcmp(vm::cptr<u8> buf1, vm::cptr<u8> buf2, u32 size)
{
	sysPrxForUser.trace("_sys_memcmp(buf1=*0x%x, buf2=*0x%x, size=%d)", buf1, buf2, size);

	for (u32 i = 0; i < size; i++)
	{
		const u8 b1 = buf1[i], b2 = buf2[i];

		if (b1 < b2)
		{
			return -1;
		}

		if (b1 > b2)
		{
			return 1;
		}
	}

	return 0;
}

vm::ptr<u8> _sys_memchr(vm::ptr<u8> buf, u8 ch, s32 size)
{
	sysPrxForUser.trace("_sys_memchr(buf=*0x%x, ch=0x%x, size=0x%x)", buf, ch, size);

	if (!buf)
	{
		return vm::null;
	}

	while (size > 0)
	{
		if (*buf == ch)
		{
			return buf;
		}

		buf++;
		size--;
	}

	return vm::null;
}

vm::ptr<void> _sys_memmove(vm::ptr<void> dst, vm::cptr<void> src, u32 size)
{
	sysPrxForUser.trace("_sys_memmove(dst=*0x%x, src=*0x%x, size=%d)", dst, src, size);

	std::memmove(dst.get_ptr(), src.get_ptr(), size);

	return dst;
}

u32 _sys_strlen(vm::cptr<char> str)
{
	sysPrxForUser.trace("_sys_strlen(str=%s)", str);

	if (!str)
	{
		return 0;
	}

	for (u32 i = 0;; i++)
	{
		if (str[i] == '\0')
		{
			return i;
		}
	}
}

s32 _sys_strcmp(vm::cptr<char> str1, vm::cptr<char> str2)
{
	sysPrxForUser.trace("_sys_strcmp(str1=%s, str2=%s)", str1, str2);

	for (u32 i = 0;; i++)
	{
		const u8 ch1 = str1[i], ch2 = str2[i];
		if (ch1 < ch2)
			return -1;
		if (ch1 > ch2)
			return 1;
		if (ch1 == '\0')
			return 0;
	}
}

s32 _sys_strncmp(vm::cptr<char> str1, vm::cptr<char> str2, u32 max)
{
	sysPrxForUser.trace("_sys_strncmp(str1=%s, str2=%s, max=%d)", str1, str2, max);

	for (u32 i = 0; i < max; i++)
	{
		const u8 ch1 = str1[i], ch2 = str2[i];
		if (ch1 < ch2)
			return -1;
		if (ch1 > ch2)
			return 1;
		if (ch1 == '\0')
			break;
	}

	return 0;
}

vm::ptr<char> _sys_strcat(vm::ptr<char> dst, vm::cptr<char> src)
{
	sysPrxForUser.trace("_sys_strcat(dst=*0x%x %s, src=%s)", dst, dst, src);

	auto str = dst;

	while (*str)
	{
		str++;
	}

	for (u32 i = 0;; i++)
	{
		if (!(str[i] = src[i]))
		{
			return dst;
		}
	}
}

vm::cptr<char> _sys_strchr(vm::cptr<char> str, char ch)
{
	sysPrxForUser.trace("_sys_strchr(str=%s, ch=%d)", str, ch);

	for (u32 i = 0;; i++)
	{
		const char ch1 = str[i];
		if (ch1 == ch)
			return str + i;
		if (ch1 == '\0')
			return vm::null;
	}
}

vm::ptr<char> _sys_strncat(vm::ptr<char> dst, vm::cptr<char> src, u32 max)
{
	sysPrxForUser.trace("_sys_strncat(dst=*0x%x %s, src=%s, max=%u)", dst, dst, src, max);

	auto str = dst;

	while (*str)
	{
		str++;
	}

	for (u32 i = 0; i < max; i++)
	{
		if (!(str[i] = src[i]))
		{
			return dst;
		}
	}

	str[max] = '\0';
	return dst;
}

vm::ptr<char> _sys_strcpy(vm::ptr<char> dst, vm::cptr<char> src)
{
	sysPrxForUser.trace("_sys_strcpy(dst=*0x%x, src=%s)", dst, src);

	for (u32 i = 0;; i++)
	{
		if (!(dst[i] = src[i]))
		{
			return dst;
		}
	}
}

vm::ptr<char> _sys_strncpy(vm::ptr<char> dst, vm::cptr<char> src, s32 len)
{
	sysPrxForUser.trace("_sys_strncpy(dst=*0x%x %s, src=%s, len=%d)", dst, dst, src, len);

	if (!dst || !src)
	{
		return vm::null;
	}

	for (s32 i = 0; i < len; i++)
	{
		if (!(dst[i] = src[i]))
		{
			for (++i; i < len; i++)
			{
				dst[i] = '\0';
			}

			return dst;
		}
	}

	return dst;
}

s32 _sys_strncasecmp(vm::cptr<char> str1, vm::cptr<char> str2, u32 n)
{
	sysPrxForUser.trace("_sys_strncasecmp(str1=%s, str2=%s, n=%d)", str1, str2, n);

	for (u32 i = 0; i < n; i++)
	{
		const int ch1 = _sys_tolower(str1[i]), ch2 = _sys_tolower(str2[i]);
		if (ch1 < ch2)
			return -1;
		if (ch1 > ch2)
			return 1;
		if (ch1 == '\0')
			break;
	}
	return 0;
}

vm::cptr<char> _sys_strrchr(vm::cptr<char> str, char ch)
{
	sysPrxForUser.trace("_sys_strrchr(str=%s, ch=%d)", str, ch);

	vm::cptr<char> res = vm::null;

	for (u32 i = 0;; i++)
	{
		const char ch1 = str[i];
		if (ch1 == ch)
			res = str + i;
		if (ch1 == '\0')
			break;
	}

	return res;
}

u32 _sys_malloc(u32 size)
{
	sysPrxForUser.warning("_sys_malloc(size=0x%x)", size);

	return vm::alloc(size, vm::main);
}

u32 _sys_memalign(u32 align, u32 size)
{
	sysPrxForUser.warning("_sys_memalign(align=0x%x, size=0x%x)", align, size);

	return vm::alloc(size, vm::main, std::max<u32>(align, 0x10000));
}

error_code _sys_free(u32 addr)
{
	sysPrxForUser.warning("_sys_free(addr=0x%x)", addr);

	vm::dealloc(addr, vm::main);

	return CELL_OK;
}

s32 _sys_snprintf(ppu_thread& ppu, vm::ptr<char> dst, u32 count, vm::cptr<char> fmt, ppu_va_args_t va_args)
{
	sysPrxForUser.warning("_sys_snprintf(dst=*0x%x, count=%d, fmt=%s, ...)", dst, count, fmt);

	std::string result = ps3_fmt(ppu, fmt, va_args.count);

	if (!count)
	{
		return 0; // ???
	}
	else
	{
		count = static_cast<u32>(std::min<usz>(count - 1, result.size()));

		std::memcpy(dst.get_ptr(), result.c_str(), count);
		dst[count] = 0;
		return count;
	}
}

error_code _sys_printf(ppu_thread& ppu, vm::cptr<char> fmt, ppu_va_args_t va_args)
{
	sysPrxForUser.warning("_sys_printf(fmt=%s, ...)", fmt);

	const auto buf = vm::make_str(ps3_fmt(ppu, fmt, va_args.count));

	sys_tty_write(ppu, 0, buf, buf.get_count() - 1, vm::var<u32>{});

	return CELL_OK;
}

s32 _sys_sprintf(ppu_thread& ppu, vm::ptr<char> buffer, vm::cptr<char> fmt, ppu_va_args_t va_args)
{
	sysPrxForUser.warning("_sys_sprintf(buffer=*0x%x, fmt=%s, ...)", buffer, fmt);

	std::string result = ps3_fmt(ppu, fmt, va_args.count);

	std::memcpy(buffer.get_ptr(), result.c_str(), result.size() + 1);

	return static_cast<s32>(result.size());
}

error_code _sys_vprintf()
{
	sysPrxForUser.todo("_sys_vprintf()");
	return CELL_OK;
}

error_code _sys_vsnprintf()
{
	sysPrxForUser.todo("_sys_vsnprintf()");
	return CELL_OK;
}

error_code _sys_vsprintf()
{
	sysPrxForUser.todo("_sys_vsprintf()");
	return CELL_OK;
}

void _sys_qsort(vm::ptr<void> base, u32 nelem, u32 size, vm::ptr<qsortcmp> cmp)
{
	sysPrxForUser.warning("_sys_qsort(base=*0x%x, nelem=%d, size=0x%x, cmp=*0x%x)", base, nelem, size, cmp);

	static thread_local decltype(cmp) g_tls_cmp;
	g_tls_cmp = cmp;

	std::qsort(base.get_ptr(), nelem, size, [](const void* a, const void* b) -> s32
	{
		return g_tls_cmp(static_cast<ppu_thread&>(*get_current_cpu_thread()), vm::get_addr(a), vm::get_addr(b));
	});
}

void sysPrxForUser_sys_libc_init()
{
	REG_FUNC(sysPrxForUser, __sys_look_ctype_table);
	REG_FUNC(sysPrxForUser, _sys_tolower);
	REG_FUNC(sysPrxForUser, _sys_toupper);

	REG_FUNC(sysPrxForUser, _sys_memset);
	REG_FUNC(sysPrxForUser, _sys_memcpy);
	REG_FUNC(sysPrxForUser, _sys_memcmp);
	REG_FUNC(sysPrxForUser, _sys_memchr);
	REG_FUNC(sysPrxForUser, _sys_memmove);

	REG_FUNC(sysPrxForUser, _sys_strlen);
	REG_FUNC(sysPrxForUser, _sys_strcmp);
	REG_FUNC(sysPrxForUser, _sys_strncmp);
	REG_FUNC(sysPrxForUser, _sys_strcat);
	REG_FUNC(sysPrxForUser, _sys_strchr);
	REG_FUNC(sysPrxForUser, _sys_strncat);
	REG_FUNC(sysPrxForUser, _sys_strcpy);
	REG_FUNC(sysPrxForUser, _sys_strncpy);
	REG_FUNC(sysPrxForUser, _sys_strncasecmp);
	REG_FUNC(sysPrxForUser, _sys_strrchr);

	REG_FUNC(sysPrxForUser, _sys_malloc);
	REG_FUNC(sysPrxForUser, _sys_memalign);
	REG_FUNC(sysPrxForUser, _sys_free);

	REG_FUNC(sysPrxForUser, _sys_snprintf);
	REG_FUNC(sysPrxForUser, _sys_printf);
	REG_FUNC(sysPrxForUser, _sys_sprintf);
	REG_FUNC(sysPrxForUser, _sys_vprintf);
	REG_FUNC(sysPrxForUser, _sys_vsnprintf);
	REG_FUNC(sysPrxForUser, _sys_vsprintf);

	REG_FUNC(sysPrxForUser, _sys_qsort);
}
