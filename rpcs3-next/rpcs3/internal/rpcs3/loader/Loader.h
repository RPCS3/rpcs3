#pragma once
#include "Emu/Memory/vm.h"

struct vfsFileBase;
struct vfsStream;

#ifdef _DEBUG	
	//#define LOADER_DEBUG
#endif

enum Elf_Machine
{
	MACHINE_Unknown,
	MACHINE_MIPS = 0x08,
	MACHINE_PPC64 = 0x15,
	MACHINE_SPU = 0x17,
	MACHINE_ARM = 0x28
};

enum ShdrType
{
	SHT_NULL,
	SHT_PROGBITS,
	SHT_SYMTAB,
	SHT_STRTAB,
	SHT_RELA,
	SHT_HASH,
	SHT_DYNAMIC,
	SHT_NOTE,
	SHT_NOBITS,
	SHT_REL,
	SHT_SHLIB,
	SHT_DYNSYM
};

enum ShdrFlag
{
	SHF_WRITE     = 0x1,
	SHF_ALLOC     = 0x2,
	SHF_EXECINSTR = 0x4,
	SHF_MASKPROC  = 0xf0000000
};

const std::string Ehdr_DataToString(const u8 data);
const std::string Ehdr_TypeToString(const u16 type);
const std::string Ehdr_OS_ABIToString(const u8 os_abi);
const std::string Ehdr_MachineToString(const u16 machine);
const std::string Phdr_FlagsToString(u32 flags);
const std::string Phdr_TypeToString(const u32 type);

namespace loader
{
	class handler
	{
		u64 m_stream_offset;

	protected:
		vfsStream* m_stream;

	public:
		enum error_code
		{
			bad_version = -1,
			bad_file = -2,
			broken_file = -3,
			loading_error = -4,
			bad_relocation_type = -5,
			ok = 0
		};

		virtual ~handler() = default;

		virtual error_code init(vfsStream& stream);
		virtual error_code load() = 0;
		u64 get_stream_offset() const
		{
			return m_stream_offset;
		}

		void set_status(const error_code& code)
		{
			m_status = code;
		}

		error_code get_status() const
		{
			return m_status;
		}

		const std::string get_error_code() const
		{
			switch (m_status)
			{
			case bad_version: return "Bad version";
			case bad_file: return "Bad file";
			case broken_file: return "Broken file";
			case loading_error: return "Loading error";
			case bad_relocation_type: return "Bad relocation type";
			case ok: return "Ok";

			default: return "Unknown error code";
			}
		}

	protected:
		error_code m_status;
	};

	class loader
	{
		std::vector<handler*> m_handlers;

	public:
		~loader()
		{
			for (auto &h : m_handlers)
			{
				delete h;
			}
		}

		void register_handler(handler* handler)
		{
			m_handlers.push_back(handler);
		}

		bool load(vfsStream& stream);
	};

	using namespace vm;
}
