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

		case elf_error::stream: return "File not found";
		case elf_error::stream_header: return "Failed to read ELF header";
		case elf_error::stream_phdrs: return "Failed to read ELF program headers";
		case elf_error::stream_shdrs: return "Failed to read ELF section headers";
		case elf_error::stream_data: return "Failed to read ELF program data";

		case elf_error::header_magic: return "Not an ELF";
		case elf_error::header_version: return "Invalid or unsupported ELF format";
		case elf_error::header_class: return "Invalid ELF class";
		case elf_error::header_machine: return "Invalid ELF machine";
		case elf_error::header_endianness: return "Invalid ELF data (endianness)";
		case elf_error::header_type: return "Invalid ELF type";
		case elf_error::header_os: return "Invalid ELF OS ABI";
		}

		return unknown;
	});
}
