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

		case elf_error::stream: return u8"�L�Ī� stream ���ɮץ����";
		case elf_error::stream_header: return u8"�L�kŪ�� ELF ���D";
		case elf_error::stream_phdrs: return u8"�L�kŪ�� ELF �{�����D";
		case elf_error::stream_shdrs: return u8"�L�kŪ�� ELF �������D";
		case elf_error::stream_data: return u8"�L�kŪ�� ELF �{�����";

		case elf_error::header_magic: return u8"�S�� ELF";
		case elf_error::header_version: return u8"�L�ĩΤ����䴩�� ELF �榡";
		case elf_error::header_class: return u8"ELF ���L��";
		case elf_error::header_machine: return u8"ELF machine �L��";
		case elf_error::header_endianness: return u8"ELF ��ƵL�� (�줸�ն���)";
		case elf_error::header_type: return u8"ELF �����L��";
		case elf_error::header_os: return u8"ELF OS ABI �L��";
		}

		return unknown;
	});
}
