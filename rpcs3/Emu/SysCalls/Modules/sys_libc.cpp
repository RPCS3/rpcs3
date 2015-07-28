#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/Cell/PPUInstrTable.h"

extern Module sys_libc;

std::string ps3_fmt(PPUThread& context, vm::cptr<char> fmt, u32 g_count, u32 f_count, u32 v_count)
{
	std::string result;

	for (char c = *fmt++; c; c = *fmt++)
	{
		switch (c)
		{
		case '%':
		{
			const auto start = fmt - 1;

			// read flags
			const bool plus_sign = *fmt == '+' ? fmt++, true : false;
			const bool minus_sign = *fmt == '-' ? fmt++, true : false;
			const bool space_sign = *fmt == ' ' ? fmt++, true : false;
			const bool number_sign = *fmt == '#' ? fmt++, true : false;
			const bool zero_padding = *fmt == '0' ? fmt++, true : false;

			// read width
			const u32 width = [&]() -> u32
			{
				u32 width = 0;

				if (*fmt == '*')
				{
					fmt++;
					return context.get_next_gpr_arg(g_count, f_count, v_count);
				}

				while (*fmt - '0' < 10)
				{
					width = width * 10 + (*fmt++ - '0');
				}

				return width;
			}();

			// read precision
			const u32 prec = [&]() -> u32
			{
				u32 prec = 0;

				if (*fmt != '.')
				{
					return 0;
				}

				if (*++fmt == '*')
				{
					fmt++;
					return context.get_next_gpr_arg(g_count, f_count, v_count);
				}

				while (*fmt - '0' < 10)
				{
					prec = prec * 10 + (*fmt++ - '0');
				}

				return prec;
			}();

			switch (char cf = *fmt++)
			{
			case '%':
			{
				if (plus_sign || minus_sign || space_sign || number_sign || zero_padding || width || prec) break;

				result += '%';
				continue;
			}
			case 'd':
			case 'i':
			{
				// signed decimal
				const s64 value = context.get_next_gpr_arg(g_count, f_count, v_count);

				if (plus_sign || minus_sign || space_sign || number_sign || zero_padding || width || prec) break;

				result += fmt::to_sdec(value);
				continue;
			}
			case 'x':
			case 'X':
			{
				// hexadecimal
				const u64 value = context.get_next_gpr_arg(g_count, f_count, v_count);

				if (plus_sign || minus_sign || space_sign || prec) break;

				if (number_sign && value)
				{
					result += cf == 'x' ? "0x" : "0X";
				}

				const std::string& hex = cf == 'x' ? fmt::to_hex(value) : fmt::toupper(fmt::to_hex(value));

				if (hex.length() >= width)
				{
					result += hex;
				}
				else if (zero_padding)
				{
					result += std::string(width - hex.length(), '0') + hex;
				}
				else
				{
					result += hex + std::string(width - hex.length(), ' ');
				}
				continue;
			}
			case 's':
			{
				// string
				auto string = vm::cptr<char, u64>::make(context.get_next_gpr_arg(g_count, f_count, v_count));

				if (plus_sign || minus_sign || space_sign || number_sign || zero_padding || width || prec) break;

				result += string.get_ptr();
				continue;
			}
			case 'u':
			{
				// unsigned decimal
				const u64 value = context.get_next_gpr_arg(g_count, f_count, v_count);

				if (plus_sign || minus_sign || space_sign || number_sign || zero_padding || width || prec) break;

				result += fmt::to_udec(value);
				continue;
			}
			}

			throw EXCEPTION("Unknown formatting: '%s'", start.get_ptr());
		}
		}

		result += c;
	}

	return result;
}

namespace sys_libc_func
{
	void memcpy(vm::ptr<void> dst, vm::cptr<void> src, u32 size)
	{
		sys_libc.Log("memcpy(dst=*0x%x, src=*0x%x, size=0x%x)", dst, src, size);

		::memcpy(dst.get_ptr(), src.get_ptr(), size);
	}
}

extern Module sysPrxForUser;

vm::ptr<void> _sys_memset(vm::ptr<void> dst, s32 value, u32 size)
{
	sysPrxForUser.Log("_sys_memset(dst=*0x%x, value=%d, size=0x%x)", dst, value, size);

	memset(dst.get_ptr(), value, size);

	return dst;
}

vm::ptr<void> _sys_memcpy(vm::ptr<void> dst, vm::cptr<void> src, u32 size)
{
	sysPrxForUser.Log("_sys_memcpy(dst=*0x%x, src=*0x%x, size=0x%x)", dst, src, size);

	memcpy(dst.get_ptr(), src.get_ptr(), size);

	return dst;
}

s32 _sys_memcmp(vm::cptr<void> buf1, vm::cptr<void> buf2, u32 size)
{
	sysPrxForUser.Log("_sys_memcmp(buf1=*0x%x, buf2=*0x%x, size=%d)", buf1, buf2, size);

	return memcmp(buf1.get_ptr(), buf2.get_ptr(), size);
}

s32 _sys_memchr()
{
	throw EXCEPTION("");
}

s32 _sys_memmove()
{
	throw EXCEPTION("");
}

s64 _sys_strlen(vm::cptr<char> str)
{
	sysPrxForUser.Log("_sys_strlen(str=*0x%x)", str);

	return strlen(str.get_ptr());
}

s32 _sys_strcmp(vm::cptr<char> str1, vm::cptr<char> str2)
{
	sysPrxForUser.Log("_sys_strcmp(str1=*0x%x, str2=*0x%x)", str1, str2);

	return strcmp(str1.get_ptr(), str2.get_ptr());
}

s32 _sys_strncmp(vm::cptr<char> str1, vm::cptr<char> str2, s32 max)
{
	sysPrxForUser.Log("_sys_strncmp(str1=*0x%x, str2=*0x%x, max=%d)", str1, str2, max);

	return strncmp(str1.get_ptr(), str2.get_ptr(), max);
}

vm::ptr<char> _sys_strcat(vm::ptr<char> dest, vm::cptr<char> source)
{
	sysPrxForUser.Log("_sys_strcat(dest=*0x%x, source=*0x%x)", dest, source);

	if (strcat(dest.get_ptr(), source.get_ptr()) != dest.get_ptr())
	{
		throw EXCEPTION("Unexpected strcat() result");
	}

	return dest;
}

vm::cptr<char> _sys_strchr(vm::cptr<char> str, s32 ch)
{
	sysPrxForUser.Log("_sys_strchr(str=*0x%x, ch=0x%x)", str, ch);

	return vm::cptr<char>::make(vm::get_addr(strchr(str.get_ptr(), ch)));
}

vm::ptr<char> _sys_strncat(vm::ptr<char> dest, vm::cptr<char> source, u32 len)
{
	sysPrxForUser.Log("_sys_strncat(dest=*0x%x, source=*0x%x, len=%d)", dest, source, len);

	if (strncat(dest.get_ptr(), source.get_ptr(), len) != dest.get_ptr())
	{
		throw EXCEPTION("Unexpected strncat() result");
	}

	return dest;
}

vm::ptr<char> _sys_strcpy(vm::ptr<char> dest, vm::cptr<char> source)
{
	sysPrxForUser.Log("_sys_strcpy(dest=*0x%x, source=*0x%x)", dest, source);

	if (strcpy(dest.get_ptr(), source.get_ptr()) != dest.get_ptr())
	{
		throw EXCEPTION("Unexpected strcpy() result");
	}

	return dest;
}

vm::ptr<char> _sys_strncpy(vm::ptr<char> dest, vm::cptr<char> source, u32 len)
{
	sysPrxForUser.Log("_sys_strncpy(dest=*0x%x, source=*0x%x, len=%d)", dest, source, len);

	if (!dest || !source)
	{
		return vm::null;
	}

	if (strncpy(dest.get_ptr(), source.get_ptr(), len) != dest.get_ptr())
	{
		throw EXCEPTION("Unexpected strncpy() result");
	}

	return dest;
}

s32 _sys_strncasecmp()
{
	throw EXCEPTION("");
}

s32 _sys_strrchr()
{
	throw EXCEPTION("");
}

s32 _sys_tolower()
{
	throw EXCEPTION("");
}

s32 _sys_toupper()
{
	throw EXCEPTION("");
}

u32 _sys_malloc(u32 size)
{
	sysPrxForUser.Warning("_sys_malloc(size=0x%x)", size);

	return vm::alloc(size, vm::main);
}

u32 _sys_memalign(u32 align, u32 size)
{
	sysPrxForUser.Warning("_sys_memalign(align=0x%x, size=0x%x)", align, size);

	return vm::alloc(size, vm::main, std::max<u32>(align, 4096));
}

s32 _sys_free(u32 addr)
{
	sysPrxForUser.Warning("_sys_free(addr=0x%x)", addr);

	vm::dealloc(addr, vm::main);

	return CELL_OK;
}

s32 _sys_snprintf(PPUThread& ppu, vm::ptr<char> dst, u32 count, vm::cptr<char> fmt, ppu_va_args_t va_args)
{
	sysPrxForUser.Warning("_sys_snprintf(dst=*0x%x, count=%d, fmt=*0x%x, ...)", dst, count, fmt);

	std::string result = ps3_fmt(ppu, fmt, va_args.g_count, va_args.f_count, va_args.v_count);

	sysPrxForUser.Warning("*** '%s' -> '%s'", fmt.get_ptr(), result);

	if (!count)
	{
		return 0; // ???
	}
	else
	{
		count = (u32)std::min<size_t>(count - 1, result.size());

		memcpy(dst.get_ptr(), result.c_str(), count);
		dst[count] = 0;
		return count;
	}
}

s32 _sys_printf(PPUThread& ppu, vm::cptr<char> fmt, ppu_va_args_t va_args)
{
	sysPrxForUser.Warning("_sys_printf(fmt=*0x%x, ...)", fmt);
	std::string result = ps3_fmt(ppu, fmt, va_args.g_count, va_args.f_count, va_args.v_count);

	LOG_ERROR(TTY, result);

	return CELL_OK;
}

s32 _sys_sprintf()
{
	throw EXCEPTION("");
}

s32 _sys_vprintf()
{
	throw EXCEPTION("");
}

s32 _sys_vsnprintf()
{
	throw EXCEPTION("");
}

s32 _sys_vsprintf()
{
	throw EXCEPTION("");
}

s32 _sys_qsort()
{
	throw EXCEPTION("");
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

Module sys_libc("sys_libc", []()
{
	using namespace PPU_instr;

	REG_SUB(sys_libc, sys_libc_func, memcpy,
		SP_I(CMPLDI(cr7, r5, 7)),
		SP_I(CLRLDI(r3, r3, 32)),
		SP_I(CLRLDI(r4, r4, 32)),
		SP_I(MR(r11, r3)),
		SP_I(BGT(cr7, XXX & 0xff)),
		SP_I(CMPDI(r5, 0)),
		OPT_SP_I(MR(r9, r3)),
		{ SPET_MASKED_OPCODE, 0x4d820020, 0xffffffff },
	);
});
