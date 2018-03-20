#include "stdafx.h"
#include "ELF.h"

// ELF loading error information
template<>
void fmt_class_string<elf_error>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](elf_error error)
	{
		switch (error)
		{
		case elf_error::ok: return "OK";

		case elf_error::stream: return u8"\u7121\u6548\u7684 stream \u6216\u6A94\u6848\u672A\u627E\u5230";
		case elf_error::stream_header: return u8"\u7121\u6CD5\u8B80\u53D6 ELF \u6A19\u984C";
		case elf_error::stream_phdrs: return u8"\u7121\u6CD5\u8B80\u53D6 ELF \u7A0B\u5F0F\u6A19\u984C";
		case elf_error::stream_shdrs: return u8"\u7121\u6CD5\u8B80\u53D6 ELF \u90E8\u5206\u6A19\u984C";
		case elf_error::stream_data: return u8"\u7121\u6CD5\u8B80\u53D6 ELF \u7A0B\u5F0F\u8CC7\u6599";

		case elf_error::header_magic: return u8"\u6C92\u6709 ELF";
		case elf_error::header_version: return u8"\u7121\u6548\u6216\u4E0D\u53D7\u652F\u63F4\u7684 ELF \u683C\u5F0F";
		case elf_error::header_class: return u8"ELF \u985E\u7121\u6548";
		case elf_error::header_machine: return u8"ELF machine \u7121\u6548";
		case elf_error::header_endianness: return u8"ELF \u8CC7\u6599\u7121\u6548 (\u4F4D\u5143\u7D44\u9806\u5E8F)";
		case elf_error::header_type: return u8"ELF \u985E\u578B\u7121\u6548";
		case elf_error::header_os: return u8"ELF OS ABI \u7121\u6548";
		}

		return unknown;
	});
}
