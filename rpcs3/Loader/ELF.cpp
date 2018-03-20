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

		case elf_error::stream: return u8"無效的 stream 或檔案未找到";
		case elf_error::stream_header: return u8"無法讀取 ELF 標題";
		case elf_error::stream_phdrs: return u8"無法讀取 ELF 程式標題";
		case elf_error::stream_shdrs: return u8"無法讀取 ELF 部分標題";
		case elf_error::stream_data: return u8"無法讀取 ELF 程式資料";

		case elf_error::header_magic: return u8"沒有 ELF";
		case elf_error::header_version: return u8"無效或不受支援的 ELF 格式";
		case elf_error::header_class: return u8"ELF 類無效";
		case elf_error::header_machine: return u8"ELF machine 無效";
		case elf_error::header_endianness: return u8"ELF 資料無效 (位元組順序)";
		case elf_error::header_type: return u8"ELF 類型無效";
		case elf_error::header_os: return u8"ELF OS ABI 無效";
		}

		return unknown;
	});
}
