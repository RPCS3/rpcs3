#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Utilities/cfmt.h"
#include <string.h>
#include <ctype.h>

namespace vm { using namespace ps3; }

extern logs::channel sysPrxForUser;

extern fs::file g_tty;

// cfmt implementation (TODO)

struct ps3_fmt_src
{
	ppu_thread* ctx;
	u32 g_count;

	bool test(std::size_t index) const
	{
		return true;
	}

	template <typename T>
	T get(std::size_t index) const
	{
		const u32 i = (u32)index + g_count;
		return ppu_gpr_cast<T>(i < 8 ? ctx->gpr[3 + i] : +*ctx->get_stack_arg(i));
	}

	void skip(std::size_t extra)
	{
		++g_count += (u32)extra;
	}

	std::size_t fmt_string(std::string& out, std::size_t extra) const
	{
		const std::size_t start = out.size();
		out += vm::ps3::_ptr<const char>(get<u32>(extra));
		return out.size() - start;
	}

	std::size_t type(std::size_t extra) const
	{
		return 0;
	}

	static constexpr std::size_t size_char  = 1;
	static constexpr std::size_t size_short = 2;
	static constexpr std::size_t size_int   = 4;
	static constexpr std::size_t size_long  = 4;
	static constexpr std::size_t size_llong = 8;
	static constexpr std::size_t size_size  = 4;
	static constexpr std::size_t size_max   = 8;
	static constexpr std::size_t size_diff  = 4;
};

template <>
f64 ps3_fmt_src::get<f64>(std::size_t index) const
{
	const u64 value = get<u64>(index);
	return *reinterpret_cast<const f64*>(reinterpret_cast<const u8*>(&value));
}

static std::string ps3_fmt(ppu_thread& context, vm::cptr<char> fmt, u32 g_count)
{
	std::string result;

	cfmt_append(result, fmt.get_ptr(), ps3_fmt_src{&context, g_count});

	return result;
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

s32 _sys_memcmp(vm::cptr<void> buf1, vm::cptr<void> buf2, u32 size)
{
	sysPrxForUser.trace("_sys_memcmp(buf1=*0x%x, buf2=*0x%x, size=%d)", buf1, buf2, size);

	return std::memcmp(buf1.get_ptr(), buf2.get_ptr(), size);
}

s32 _sys_memchr()
{
	fmt::throw_exception("Unimplemented" HERE);
}

vm::ptr<void> _sys_memmove(vm::ptr<void> dst, vm::cptr<void> src, u32 size)
{
	sysPrxForUser.trace("_sys_memmove(dst=*0x%x, src=*0x%x, size=%d)", dst, src, size);

	std::memmove(dst.get_ptr(), src.get_ptr(), size);

	return dst;
}

s64 _sys_strlen(vm::cptr<char> str)
{
	sysPrxForUser.trace("_sys_strlen(str=%s)", str);

	return std::strlen(str.get_ptr());
}

s32 _sys_strcmp(vm::cptr<char> str1, vm::cptr<char> str2)
{
	sysPrxForUser.trace("_sys_strcmp(str1=%s, str2=%s)", str1, str2);

	return std::strcmp(str1.get_ptr(), str2.get_ptr());
}

s32 _sys_strncmp(vm::cptr<char> str1, vm::cptr<char> str2, s32 max)
{
	sysPrxForUser.trace("_sys_strncmp(str1=%s, str2=%s, max=%d)", str1, str2, max);

	return std::strncmp(str1.get_ptr(), str2.get_ptr(), max);
}

vm::ptr<char> _sys_strcat(vm::ptr<char> dest, vm::cptr<char> source)
{
	sysPrxForUser.trace("_sys_strcat(dest=*0x%x, source=%s)", dest, source);

	verify(HERE), std::strcat(dest.get_ptr(), source.get_ptr()) == dest.get_ptr();

	return dest;
}

vm::cptr<char> _sys_strchr(vm::cptr<char> str, s32 ch)
{
	sysPrxForUser.trace("_sys_strchr(str=%s, ch=0x%x)", str, ch);

	return vm::cptr<char>::make(vm::get_addr(strchr(str.get_ptr(), ch)));
}

vm::ptr<char> _sys_strncat(vm::ptr<char> dest, vm::cptr<char> source, u32 len)
{
	sysPrxForUser.trace("_sys_strncat(dest=*0x%x, source=%s, len=%d)", dest, source, len);

	verify(HERE), std::strncat(dest.get_ptr(), source.get_ptr(), len) == dest.get_ptr();

	return dest;
}

vm::ptr<char> _sys_strcpy(vm::ptr<char> dest, vm::cptr<char> source)
{
	sysPrxForUser.trace("_sys_strcpy(dest=*0x%x, source=%s)", dest, source);

	verify(HERE), std::strcpy(dest.get_ptr(), source.get_ptr()) == dest.get_ptr();

	return dest;
}

vm::ptr<char> _sys_strncpy(vm::ptr<char> dest, vm::cptr<char> source, u32 len)
{
	sysPrxForUser.trace("_sys_strncpy(dest=*0x%x, source=%s, len=%d)", dest, source, len);

	if (!dest || !source)
	{
		return vm::null;
	}

	verify(HERE), std::strncpy(dest.get_ptr(), source.get_ptr(), len) == dest.get_ptr();

	return dest;
}

s32 _sys_strncasecmp(vm::cptr<char> str1, vm::cptr<char> str2, s32 n)
{
	sysPrxForUser.trace("_sys_strncasecmp(str1=%s, str2=%s, n=%d)", str1, str2, n);
	
	for (u32 i = 0; i < n; i++)
	{
		const int ch1 = tolower(str1[i]), ch2 = tolower(str2[i]);
		if (ch1 < ch2)
			return -1;
		if (ch1 > ch2)
			return 1;
		if (ch1 == '\0')
			break;
	}
	return 0;
}

vm::ptr<char> _sys_strrchr(vm::cptr<char> str, s32 character)
{
	sysPrxForUser.trace("_sys_strrchr(str=%s, character=%c)", str, (char)character);
	
	return vm::ptr<char>::make(vm::get_addr(strrchr(str.get_ptr(), character)));
}

s32 _sys_tolower()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 _sys_toupper()
{
	fmt::throw_exception("Unimplemented" HERE);
}

u32 _sys_malloc(u32 size)
{
	sysPrxForUser.warning("_sys_malloc(size=0x%x)", size);

	return vm::alloc(size, vm::main);
}

u32 _sys_memalign(u32 align, u32 size)
{
	sysPrxForUser.warning("_sys_memalign(align=0x%x, size=0x%x)", align, size);

	return vm::alloc(size, vm::main, std::max<u32>(align, 4096));
}

s32 _sys_free(u32 addr)
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
		count = (u32)std::min<size_t>(count - 1, result.size());

		std::memcpy(dst.get_ptr(), result.c_str(), count);
		dst[count] = 0;
		return count;
	}
}

s32 _sys_printf(ppu_thread& ppu, vm::cptr<char> fmt, ppu_va_args_t va_args)
{
	sysPrxForUser.warning("_sys_printf(fmt=%s, ...)", fmt);

	if (g_tty)
	{
		g_tty.write(ps3_fmt(ppu, fmt, va_args.count));
	}

	return CELL_OK;
}

s32 _sys_sprintf(ppu_thread& ppu, vm::ptr<char> buffer, vm::cptr<char> fmt, ppu_va_args_t va_args)
{
	sysPrxForUser.warning("_sys_sprintf(buffer=*0x%x, fmt=%s, ...)", buffer, fmt);

	std::string result = ps3_fmt(ppu, fmt, va_args.count);

	std::memcpy(buffer.get_ptr(), result.c_str(), result.size() + 1);

	return static_cast<s32>(result.size());
}

s32 _sys_vprintf()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 _sys_vsnprintf()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 _sys_vsprintf()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 _sys_qsort()
{
	fmt::throw_exception("Unimplemented" HERE);
}

void sysPrxForUser_sys_libc_init()
{
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
	REG_FUNC(sysPrxForUser, _sys_tolower);
	REG_FUNC(sysPrxForUser, _sys_toupper);

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
