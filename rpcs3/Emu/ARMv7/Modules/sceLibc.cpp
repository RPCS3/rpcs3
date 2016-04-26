﻿#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/ARMv7Module.h"
#include "Emu/ARMv7/ARMv7Thread.h"
#include "Emu/ARMv7/ARMv7Callback.h"

#include "sceLibc.h"

LOG_CHANNEL(sceLibc);

extern fs::file g_tty;

// TODO
vm::ptr<void> g_dso;

std::vector<std::function<void(ARMv7Thread&)>> g_atexit;

std::mutex g_atexit_mutex;

std::string arm_fmt(ARMv7Thread& cpu, vm::cptr<char> fmt, u32 g_count)
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
			// TODO:: Syphurith: Sorry i can not classify/understand these lines exactly..
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
					return cpu.get_next_gpr_arg(g_count);
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
					return cpu.get_next_gpr_arg(g_count);
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
				const s64 value = cpu.get_next_gpr_arg(g_count);

				if (plus_sign || minus_sign || space_sign || number_sign || zero_padding || width || prec) break;

				result += fmt::format("%lld", value);
				continue;
			}
			case 'x':
			case 'X':
			{
				// hexadecimal
				const u64 value = cpu.get_next_gpr_arg(g_count);

				if (plus_sign || minus_sign || space_sign || prec) break;

				if (number_sign && value)
				{
					result += cf == 'x' ? "0x" : "0X";
				}

				const std::string& hex = fmt::format(cf == 'x' ? "%llx" : "%llX", value);

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
				const vm::cptr<char> string{ cpu.get_next_gpr_arg(g_count), vm::addr };

				if (plus_sign || minus_sign || space_sign || number_sign || zero_padding || width || prec) break;

				result += string.get_ptr();
				continue;
			}
			}

			throw EXCEPTION("Unknown formatting '%s'", start.get_ptr());
		}
		}

		result += c;
	}

	return result;
}

namespace sce_libc_func
{
	void __cxa_atexit(vm::ptr<atexit_func_t> func, vm::ptr<void> arg, vm::ptr<void> dso)
	{
		sceLibc.warning("__cxa_atexit(func=*0x%x, arg=*0x%x, dso=*0x%x)", func, arg, dso);
		
		std::lock_guard<std::mutex> lock(g_atexit_mutex);

		g_atexit.insert(g_atexit.begin(), [func, arg, dso](ARMv7Thread& cpu)
		{
			func(cpu, arg);
		});
	}

	void __aeabi_atexit(vm::ptr<void> arg, vm::ptr<atexit_func_t> func, vm::ptr<void> dso)
	{
		sceLibc.warning("__aeabi_atexit(arg=*0x%x, func=*0x%x, dso=*0x%x)", arg, func, dso);

		std::lock_guard<std::mutex> lock(g_atexit_mutex);

		g_atexit.insert(g_atexit.begin(), [func, arg, dso](ARMv7Thread& cpu)
		{
			func(cpu, arg);
		});
	}

	void exit(ARMv7Thread& cpu)
	{
		sceLibc.warning("exit()");

		std::lock_guard<std::mutex> lock(g_atexit_mutex);

		CHECK_EMU_STATUS;

		for (auto& func : decltype(g_atexit)(std::move(g_atexit)))
		{
			func(cpu);
		}

		sceLibc.success("Process finished");

		Emu.CallAfter([]()
		{
			Emu.Stop();
		});

		while (true)
		{
			CHECK_EMU_STATUS;

			std::this_thread::sleep_for(1ms);
		}
	}

	void printf(ARMv7Thread& cpu, vm::cptr<char> fmt, arm_va_args_t va_args)
	{
		sceLibc.warning("printf(fmt=*0x%x)", fmt);
		sceLibc.trace("*** *fmt = '%s'", fmt.get_ptr());

		const std::string& result = arm_fmt(cpu, fmt, va_args.count);
		sceLibc.trace("***     -> '%s'", result);

		if (g_tty)
		{
			g_tty.write(result);
		}
	}

	void sprintf(ARMv7Thread& cpu, vm::ptr<char> str, vm::cptr<char> fmt, arm_va_args_t va_args)
	{
		sceLibc.warning("sprintf(str=*0x%x, fmt=*0x%x)", str, fmt);
		sceLibc.trace("*** *fmt = '%s'", fmt.get_ptr());

		const std::string& result = arm_fmt(cpu, fmt, va_args.count);
		sceLibc.trace("***     -> '%s'", result);

		::memcpy(str.get_ptr(), result.c_str(), result.size() + 1);
	}

	void __cxa_set_dso_handle_main(vm::ptr<void> dso)
	{
		sceLibc.warning("__cxa_set_dso_handle_main(dso=*0x%x)", dso);
		
		g_dso = dso;
		g_atexit.clear();
	}

	void memcpy(vm::ptr<void> dst, vm::cptr<void> src, u32 size)
	{
		sceLibc.warning("memcpy(dst=*0x%x, src=*0x%x, size=0x%x)", dst, src, size);

		::memcpy(dst.get_ptr(), src.get_ptr(), size);
	}

	void memset(vm::ptr<void> dst, s32 value, u32 size)
	{
		sceLibc.warning("memset(dst=*0x%x, value=%d, size=0x%x)", dst, value, size);

		::memset(dst.get_ptr(), value, size);
	}

	void _Assert(vm::cptr<char> text, vm::cptr<char> func)
	{
		sceLibc.error("_Assert(text=*0x%x, func=*0x%x)", text, func);

		LOG_FATAL(HLE, "%s : %s\n", func.get_ptr(), text.get_ptr());
		Emu.Pause();
	}
}

#define REG_FUNC(nid, name) REG_FNID(SceLibc, nid, sce_libc_func::name)

DECLARE(arm_module_manager::SceLibc)("SceLibc", []()
{
	REG_FUNC(0xE4531F85, _Assert);
	//REG_FUNC(0xE71C5CDE, _Stoul);
	//REG_FUNC(0x7A5CA6A3, _Stoulx);
	//REG_FUNC(0x6794B3C6, _Stoull);
	//REG_FUNC(0xD00F68FD, _Stoullx);
	//REG_FUNC(0xBF94193B, _Stod);
	//REG_FUNC(0x6798AA28, _Stodx);
	//REG_FUNC(0x784D8D95, _Stof);
	//REG_FUNC(0x99A49A62, _Stofx);
	//REG_FUNC(0x5AE9FFD8, _Stold);
	//REG_FUNC(0x448A3CBE, _Stoldx);
	//REG_FUNC(0x6B9E23FE, _Stoll);
	//REG_FUNC(0x0D9E3B1C, _Stollx);
	//REG_FUNC(0x52DDCDAF, _Stolx);
	//REG_FUNC(0x76904D60, _Wctomb);
	//REG_FUNC(0x5E56EA4E, _Mbtowc);
	//REG_FUNC(0xDEC462AB, _FCbuild);
	//REG_FUNC(0x9E248B76, _sceLibcErrnoLoc);
	//REG_FUNC(0xA2F50E9E, _Fltrounds);
	//REG_FUNC(0x7CC1B964, isalnum);
	//REG_FUNC(0x94A89A00, isalpha);
	//REG_FUNC(0xF894ECCB, isblank);
	//REG_FUNC(0x13F4A8C8, iscntrl);
	//REG_FUNC(0xEB93BC93, isdigit);
	//REG_FUNC(0xDFEEFB1A, isgraph);
	//REG_FUNC(0x465B93F1, islower);
	//REG_FUNC(0xB7F87C4D, isprint);
	//REG_FUNC(0x82C1E3FD, ispunct);
	//REG_FUNC(0x18F2A715, isspace);
	//REG_FUNC(0xF2012814, isupper);
	//REG_FUNC(0x3B561695, isxdigit);
	//REG_FUNC(0x83F73C88, tolower);
	//REG_FUNC(0x1218642B, toupper);
	//REG_FUNC(0x1118B49F, imaxabs);
	//REG_FUNC(0x5766B4A8, imaxdiv);
	//REG_FUNC(0xB45FD61E, strtoimax);
	//REG_FUNC(0xB318952F, strtoumax);
	//REG_FUNC(0x3F2D104F, wcstoimax);
	//REG_FUNC(0xB9E511B4, wcstoumax);
	//REG_FUNC(0xCEF7C575, mspace_create);
	//REG_FUNC(0x30CBBC66, mspace_destroy);
	//REG_FUNC(0xE080B96E, mspace_malloc);
	//REG_FUNC(0x3CDFD2A3, mspace_free);
	//REG_FUNC(0x30470BBA, mspace_calloc);
	//REG_FUNC(0x52F780DD, mspace_memalign);
	//REG_FUNC(0x4633134A, mspace_realloc);
	//REG_FUNC(0x915DA59E, mspace_reallocalign);
	//REG_FUNC(0x7AD7A737, mspace_malloc_usable_size);
	//REG_FUNC(0x8D2A14C4, mspace_malloc_stats);
	//REG_FUNC(0x2FF5D5BB, mspace_malloc_stats_fast);
	//REG_FUNC(0xF355F381, mspace_is_heap_empty);
	//REG_FUNC(0x50B326CE, setjmp);
	//REG_FUNC(0x2D81C8C8, longjmp);
	//REG_FUNC(0x72BA4468, clearerr);
	//REG_FUNC(0xEC97321C, fclose);
	//REG_FUNC(0xB2F318FE, fdopen);
	//REG_FUNC(0xBF96AD71, feof);
	//REG_FUNC(0xB724BFC1, ferror);
	//REG_FUNC(0x5AAD2996, fflush);
	//REG_FUNC(0x672C58E0, fgetc);
	//REG_FUNC(0x3CDA3118, fgetpos);
	//REG_FUNC(0xBA14322F, fgets);
	//REG_FUNC(0xFEC1502E, fileno);
	//REG_FUNC(0xFFFBE239, fopen);
	//REG_FUNC(0xE0C79764, fprintf);
	//REG_FUNC(0x7E6A6108, fputc);
	//REG_FUNC(0xC8FF13E5, fputs);
	//REG_FUNC(0xB31C73A9, fread);
	//REG_FUNC(0x715C4395, freopen);
	//REG_FUNC(0x505601C6, fscanf);
	//REG_FUNC(0xC3A7CDE1, fseek);
	//REG_FUNC(0xDC1BDBD7, fsetpos);
	//REG_FUNC(0x41C2AF95, ftell);
	//REG_FUNC(0x8BCDCC4E, fwrite);
	//REG_FUNC(0x4BD5212E, getc);
	//REG_FUNC(0x4790BF1E, getchar);
	//REG_FUNC(0xF97B8CA3, gets);
	//REG_FUNC(0x4696E7BE, perror);
	REG_FUNC(0x9a004680, printf);
	//REG_FUNC(0x995708A6, putc);
	//REG_FUNC(0x7CDAC89C, putchar);
	//REG_FUNC(0x59C3E171, puts);
	//REG_FUNC(0x40293B75, remove);
	//REG_FUNC(0x6FE983A3, rename);
	//REG_FUNC(0x6CA5BAB9, rewind);
	//REG_FUNC(0x9CB9D899, scanf);
	//REG_FUNC(0x395490DA, setbuf);
	//REG_FUNC(0x2CA980A0, setvbuf);
	//REG_FUNC(0xA1BFF606, snprintf);
	REG_FUNC(0x7449B359, sprintf);
	//REG_FUNC(0xEC585241, sscanf);
	//REG_FUNC(0x2BCB3F01, ungetc);
	//REG_FUNC(0xF7915685, vfprintf);
	//REG_FUNC(0xF137771A, vfscanf);
	//REG_FUNC(0xE7B5E23E, vprintf);
	//REG_FUNC(0x0E9BD318, vscanf);
	//REG_FUNC(0xFE83F2E4, vsnprintf);
	//REG_FUNC(0x802FDDF9, vsprintf);
	//REG_FUNC(0xA9889307, vsscanf);
	//REG_FUNC(0x20FE0FFF, abort);
	//REG_FUNC(0x8E5A06C5, abs);
	//REG_FUNC(0xD500DE27, atof);
	//REG_FUNC(0x21493BE7, atoi);
	//REG_FUNC(0xA1DBEE9F, atol);
	//REG_FUNC(0x875994F3, atoll);
	//REG_FUNC(0xD1BC28E7, bsearch);
	//REG_FUNC(0xE9F823C0, div);
	REG_FUNC(0x826bbbaf, exit);
	//REG_FUNC(0xB53B345B, _Exit);
	//REG_FUNC(0xBCEA304B, labs);
	//REG_FUNC(0x9D2D17CD, llabs);
	//REG_FUNC(0xD63330DA, ldiv);
	//REG_FUNC(0x3F887699, lldiv);
	//REG_FUNC(0x3E347849, mblen);
	//REG_FUNC(0x2F75CF9B, mbstowcs);
	//REG_FUNC(0xD791A952, mbtowc);
	//REG_FUNC(0xA7CBE4A6, qsort);
	//REG_FUNC(0x6CA88B08, strtod);
	//REG_FUNC(0x6CB8540E, strtof);
	//REG_FUNC(0x181827ED, strtol);
	//REG_FUNC(0x48C684B2, strtold);
	//REG_FUNC(0x39B7E681, strtoll);
	//REG_FUNC(0xF34AE312, strtoul);
	//REG_FUNC(0xE0E12333, strtoull);
	//REG_FUNC(0x9A8F7FC0, wcstombs);
	//REG_FUNC(0x6489B5E4, wctomb);
	//REG_FUNC(0xC0883865, rand);
	//REG_FUNC(0x3AAD41B0, srand);
	//REG_FUNC(0x962097AA, rand_r);
	//REG_FUNC(0x775A0CB2, malloc);
	//REG_FUNC(0xE7EC3D0B, calloc);
	//REG_FUNC(0x006B54BA, realloc);
	//REG_FUNC(0x5B9BB802, free);
	//REG_FUNC(0xA9363E6B, memalign);
	//REG_FUNC(0x608AC135, reallocalign);
	//REG_FUNC(0x57A729DB, malloc_stats);
	//REG_FUNC(0xB3D29DE1, malloc_stats_fast);
	//REG_FUNC(0x54A54EB1, malloc_usable_size);
	//REG_FUNC(0x2F3E5B16, memchr);
	//REG_FUNC(0x7747F6D7, memcmp);
	REG_FUNC(0x7205BFDB, memcpy);
	//REG_FUNC(0xAF5C218D, memmove);
	REG_FUNC(0x6DC1F0D8, memset);
	//REG_FUNC(0x1434FA46, strcat);
	//REG_FUNC(0xB9336E16, strchr);
	//REG_FUNC(0x1B58FA3B, strcmp);
	//REG_FUNC(0x46AE2311, strcoll);
	//REG_FUNC(0x85B924B7, strcpy);
	//REG_FUNC(0x0E29D27A, strcspn);
	//REG_FUNC(0x1E9D6335, strerror);
	//REG_FUNC(0x8AECC873, strlen);
	//REG_FUNC(0xFBA69BC2, strncat);
	//REG_FUNC(0xE4299DCB, strncmp);
	//REG_FUNC(0x9F87712D, strncpy);
	//REG_FUNC(0x68C307B6, strpbrk);
	//REG_FUNC(0xCEFDD143, strrchr);
	//REG_FUNC(0x4203B663, strspn);
	//REG_FUNC(0x0D5200CB, strstr);
	//REG_FUNC(0x0289B8B3, strtok);
	//REG_FUNC(0x4D023DE9, strxfrm);
	//REG_FUNC(0xEB31926D, strtok_r);
	//REG_FUNC(0xFF6F77C7, strdup);
	//REG_FUNC(0x184C4B07, strcasecmp);
	//REG_FUNC(0xAF1CA2F1, strncasecmp);
	//REG_FUNC(0xC94AE948, asctime);
	//REG_FUNC(0xC082CA03, clock);
	//REG_FUNC(0x1EA1CA8D, ctime);
	//REG_FUNC(0x33AD70A0, difftime);
	//REG_FUNC(0xF283CFE3, gmtime);
	//REG_FUNC(0xF4A2E0BF, localtime);
	//REG_FUNC(0xD1A2DFC3, mktime);
	//REG_FUNC(0xEEB76FED, strftime);
	//REG_FUNC(0xDAE8D60F, time);
	//REG_FUNC(0xEFB3BC61, btowc);
	//REG_FUNC(0x1D1DA5AD, _Btowc);
	//REG_FUNC(0x89541CA5, fgetwc);
	//REG_FUNC(0x982AFA4D, fgetws);
	//REG_FUNC(0xA597CDC8, fputwc);
	//REG_FUNC(0xB755927C, fputws);
	//REG_FUNC(0xA77327D2, fwide);
	//REG_FUNC(0xE52278E8, fwprintf);
	//REG_FUNC(0x7BFC75C6, fwscanf);
	//REG_FUNC(0x23E0F442, getwc);
	//REG_FUNC(0x55DB4E32, getwchar);
	//REG_FUNC(0x1927CAE8, mbrlen);
	//REG_FUNC(0x910664C3, mbrtowc);
	//REG_FUNC(0x961D12F8, mbsinit);
	//REG_FUNC(0x9C14D58E, mbsrtowcs);
	//REG_FUNC(0x247C71A6, putwc);
	//REG_FUNC(0x3E04AB1C, putwchar);
	//REG_FUNC(0x1B581BEB, swprintf);
	//REG_FUNC(0xE1D2AE42, swscanf);
	//REG_FUNC(0x39334D9C, ungetwc);
	//REG_FUNC(0x36BF1E06, vfwprintf);
	//REG_FUNC(0x37A563BE, vfwscanf);
	//REG_FUNC(0x572DAB57, vswprintf);
	//REG_FUNC(0x9451EE20, vswscanf);
	//REG_FUNC(0x0A451B11, vwprintf);
	//REG_FUNC(0xAD0C43DC, vwscanf);
	//REG_FUNC(0xD9FF289D, wcrtomb);
	//REG_FUNC(0x2F990FF9, wcscat);
	//REG_FUNC(0xC1587971, wcschr);
	//REG_FUNC(0xF42128B9, wcscmp);
	//REG_FUNC(0x8EC70609, wcscoll);
	//REG_FUNC(0x8AAADD56, wcscpy);
	//REG_FUNC(0x25F7E46A, wcscspn);
	//REG_FUNC(0x74136BC1, wcsftime);
	//REG_FUNC(0xA778A14B, wcslen);
	//REG_FUNC(0x196AB9F2, wcsncat);
	//REG_FUNC(0xAAA6AAA2, wcsncmp);
	//REG_FUNC(0x62E9B2D5, wcsncpy);
	//REG_FUNC(0x07F229DB, wcspbrk);
	//REG_FUNC(0xDF806521, wcsrchr);
	//REG_FUNC(0xD8889FC8, wcsrtombs);
	//REG_FUNC(0x5F5AA692, wcsspn);
	//REG_FUNC(0x5BE328EE, wcsstr);
	//REG_FUNC(0x35D7F1B1, wcstod);
	//REG_FUNC(0x322243A8, _WStod);
	//REG_FUNC(0x64123137, wcstof);
	//REG_FUNC(0x340AF0F7, _WStof);
	//REG_FUNC(0xA17C24A3, wcstok);
	//REG_FUNC(0xFBEB657E, wcstol);
	//REG_FUNC(0x2D7C3A7A, wcstold);
	//REG_FUNC(0x1EDA8F09, _WStold);
	//REG_FUNC(0x6EEFB7D7, wcstoll);
	//REG_FUNC(0xACF13D54, wcstoul);
	//REG_FUNC(0x87C94271, _WStoul);
	//REG_FUNC(0xCBFF8200, wcstoull);
	//REG_FUNC(0xF6069AFD, wcsxfrm);
	//REG_FUNC(0x704321CC, wctob);
	//REG_FUNC(0x2F0C81A6, _Wctob);
	//REG_FUNC(0x7A08BE70, wmemchr);
	//REG_FUNC(0x9864C99F, wmemcmp);
	//REG_FUNC(0xD9F9DDCD, wmemcpy);
	//REG_FUNC(0x53F7EB4B, wmemmove);
	//REG_FUNC(0x4D04A480, wmemset);
	//REG_FUNC(0xBF2F5FCE, wprintf);
	//REG_FUNC(0xADC32204, wscanf);
	//REG_FUNC(0x7E811AF2, iswalnum);
	//REG_FUNC(0xC5ECB7B6, iswalpha);
	//REG_FUNC(0xC8FC4BBE, iswblank);
	//REG_FUNC(0xC30AE3C7, iswcntrl);
	//REG_FUNC(0x9E348712, iswctype);
	//REG_FUNC(0xC37DB2C2, iswdigit);
	//REG_FUNC(0x70632234, iswgraph);
	//REG_FUNC(0x40F84B7D, iswlower);
	//REG_FUNC(0xF52F9241, iswprint);
	//REG_FUNC(0x3922B91A, iswpunct);
	//REG_FUNC(0x2BDA4905, iswspace);
	//REG_FUNC(0x9939E1AD, iswupper);
	//REG_FUNC(0x82FCEFA4, iswxdigit);
	//REG_FUNC(0x09C38DE4, towlower);
	//REG_FUNC(0xCF77D465, towctrans);
	//REG_FUNC(0xCACE34B9, towupper);
	//REG_FUNC(0xE8270951, wctrans);
	//REG_FUNC(0xA967B88D, wctype);
	//REG_FUNC(0x9D885076, _Towctrans);
	//REG_FUNC(0xE980110A, _Iswctype);
	REG_FUNC(0x33b83b70, __cxa_atexit);
	REG_FUNC(0xEDC939E1, __aeabi_atexit);
	//REG_FUNC(0xB538BF48, __cxa_finalize);
	//REG_FUNC(0xD0310E31, __cxa_guard_acquire);
	//REG_FUNC(0x4ED1056F, __cxa_guard_release);
	//REG_FUNC(0xD18E461D, __cxa_guard_abort);
	REG_FUNC(0xbfe02b3a, __cxa_set_dso_handle_main);
	//REG_FUNC(0x64DA2C47, _Unlocksyslock);
	//REG_FUNC(0x7DBC0575, _Locksyslock);
	//REG_FUNC(0x5044FC32, _Lockfilelock);
	//REG_FUNC(0xFD5DD98C, _Unlockfilelock);
	//REG_FUNC(0x1EFFBAC2, __set_exidx_main);
	//REG_FUNC(0x855FC605, _Files);
	//REG_FUNC(0x66B7406C, _Stdin);
	//REG_FUNC(0x5D8C1282, _Stdout);
	//REG_FUNC(0xDDF9BB09, _Stderr);
	//REG_FUNC(0x3CE6109D, _Ctype);
	//REG_FUNC(0x36878958, _Touptab);
	//REG_FUNC(0xD662E07C, _Tolotab);
	//REG_FUNC(0xE5620A03, _Flt);
	//REG_FUNC(0x338FE545, _Dbl);
	//REG_FUNC(0x94CE931C, _Ldbl);
	//REG_FUNC(0xF708906E, _Denorm);
	//REG_FUNC(0x01B05132, _FDenorm);
	//REG_FUNC(0x2C8DBEB7, _LDenorm);
	//REG_FUNC(0x710B5F33, _Inf);
	//REG_FUNC(0x8A0F308C, _FInf);
	//REG_FUNC(0x5289BBC0, _LInf);
	//REG_FUNC(0x116F3DA9, _Nan);
	//REG_FUNC(0x415162DD, _FNan);
	//REG_FUNC(0x036D0F07, _LNan);
	//REG_FUNC(0x677CDE35, _Snan);
	//REG_FUNC(0x7D35108B, _FSnan);
	//REG_FUNC(0x48AEEF2A, _LSnan);
});
