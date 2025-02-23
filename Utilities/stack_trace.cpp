#include "stdafx.h"
#include "stack_trace.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define DBGHELP_TRANSLATE_TCHAR
#include <DbgHelp.h>
#include <codecvt>
#else
#include <execinfo.h>
#endif

namespace utils
{
#ifdef _WIN32
	std::string wstr_to_utf8(LPWSTR data, int str_len)
	{
		if (!str_len)
		{
			return {};
		}

		// Calculate size
		const auto length = WideCharToMultiByte(CP_UTF8, 0, data, str_len, NULL, 0, NULL, NULL);

		// Convert
		std::vector<char> out(length + 1, 0);
		WideCharToMultiByte(CP_UTF8, 0, data, str_len, out.data(), length, NULL, NULL);
		return out.data();
	}

	std::vector<void*> get_backtrace(int max_depth)
	{
		std::vector<void*> result = {};

		const auto hProcess = ::GetCurrentProcess();
		const auto hThread = ::GetCurrentThread();

		CONTEXT context{};
		RtlCaptureContext(&context);

		STACKFRAME64 stack = {};
		stack.AddrPC.Mode = AddrModeFlat;
		stack.AddrStack.Mode = AddrModeFlat;
		stack.AddrFrame.Mode = AddrModeFlat;
#if defined(ARCH_X64)
		stack.AddrPC.Offset = context.Rip;
		stack.AddrStack.Offset = context.Rsp;
		stack.AddrFrame.Offset = context.Rbp;
#elif defined(ARCH_ARM64)
		stack.AddrPC.Offset = context.Pc;
		stack.AddrStack.Offset = context.Sp;
		stack.AddrFrame.Offset = context.Fp;
#endif

		while (max_depth--)
		{
			if (!StackWalk64(
				IMAGE_FILE_MACHINE_AMD64,
				hProcess,
				hThread,
				&stack,
				&context,
				NULL,
				SymFunctionTableAccess64,
				SymGetModuleBase64,
				NULL))
			{
				break;
			}

			result.push_back(reinterpret_cast<void*>(stack.AddrPC.Offset));
		}

		return result;
	}

	std::vector<std::string> get_backtrace_symbols(const std::vector<void*>& stack)
	{
		std::vector<std::string> result = {};
		std::vector<u8> symbol_buf(sizeof(SYMBOL_INFOW) + sizeof(TCHAR) * 256);

		const auto hProcess = ::GetCurrentProcess();

		auto sym = reinterpret_cast<SYMBOL_INFOW*>(symbol_buf.data());
		sym->SizeOfStruct = sizeof(SYMBOL_INFOW);
		sym->MaxNameLen = 256;

		IMAGEHLP_LINEW64 line_info{};
		line_info.SizeOfStruct = sizeof(IMAGEHLP_LINEW64);

		SymInitialize(hProcess, NULL, TRUE);
		SymSetOptions(SYMOPT_LOAD_LINES);

		for (const auto& pointer : stack)
		{
			DWORD64 unused;
			SymFromAddrW(hProcess, reinterpret_cast<DWORD64>(pointer), &unused, sym);

			if (sym->NameLen)
			{
				const auto function_name = wstr_to_utf8(sym->Name, static_cast<int>(sym->NameLen));

				// Attempt to get file and line information if available
				DWORD unused2;
				if (SymGetLineFromAddrW64(hProcess, reinterpret_cast<DWORD64>(pointer), &unused2, &line_info))
				{
					const auto full_path = fmt::format("%s:%u %s", wstr_to_utf8(line_info.FileName, -1), line_info.LineNumber, function_name);
					result.push_back(full_path);
				}
				else
				{
					result.push_back(function_name);
				}
			}
			else
			{
				result.push_back(fmt::format("rpcs3@0xp", pointer));
			}
		}

		return result;
	}
#else
	std::vector<void*> get_backtrace(int max_depth)
	{
		std::vector<void*> result(max_depth);
#ifndef ANDROID
		int depth = backtrace(result.data(), max_depth);
		result.resize(depth);
#endif
		return result;
	}

	std::vector<std::string> get_backtrace_symbols(const std::vector<void*>& stack)
	{
		std::vector<std::string> result;
#ifndef ANDROID
		result.reserve(stack.size());

		const auto symbols = backtrace_symbols(stack.data(), static_cast<int>(stack.size()));
		for (usz i = 0; i < stack.size(); ++i)
		{
			result.push_back(symbols[i]);
		}

		free(symbols);
#endif
		return result;
	}
#endif
}
