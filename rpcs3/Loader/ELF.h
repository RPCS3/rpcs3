#pragma once

#include "../../Utilities/types.h"
#include "../../Utilities/File.h"

enum class elf_os : u8
{
	none = 0,

	lv2 = 0x66,
};

enum class elf_type : u16
{
	none = 0,
	rel = 1,
	exec = 2,
	dyn = 3,
	core = 4,

	prx = 0xffa4,

	psv1 = 0xfe00, // ET_SCE_EXEC
	psv2 = 0xfe04, // ET_SCE_RELEXEC (vitasdk)
};

enum class elf_machine : u16
{
	ppc64 = 0x15,
	spu = 0x17,
	arm = 0x28,
	mips = 0x08,
};

template<template<typename T> class en_t, typename sz_t>
struct elf_ehdr
{
	nse_t<u32> e_magic;
	u8 e_class;
	u8 e_data;
	u8 e_curver;
	elf_os e_os_abi;
	u8 e_abi_ver;
	u8 e_pad[7];
	en_t<elf_type> e_type;
	en_t<elf_machine> e_machine;
	en_t<u32> e_version;
	en_t<sz_t> e_entry;
	en_t<sz_t> e_phoff;
	en_t<sz_t> e_shoff;
	en_t<u32> e_flags;
	en_t<u16> e_ehsize;
	en_t<u16> e_phentsize;
	en_t<u16> e_phnum;
	en_t<u16> e_shentsize;
	en_t<u16> e_shnum;
	en_t<u16> e_shstrndx;
};

template<template<typename T> class en_t, typename sz_t>
struct elf_phdr
{
	static_assert(!sizeof(sz_t), "Invalid elf size type (must be u32 or u64)");
};

template<template<typename T> class en_t>
struct elf_phdr<en_t, u64>
{
	en_t<u32> p_type;
	en_t<u32> p_flags;
	en_t<u64> p_offset;
	en_t<u64> p_vaddr;
	en_t<u64> p_paddr;
	en_t<u64> p_filesz;
	en_t<u64> p_memsz;
	en_t<u64> p_align;
};

template<template<typename T> class en_t>
struct elf_phdr<en_t, u32>
{
	en_t<u32> p_type;
	en_t<u32> p_offset;
	en_t<u32> p_vaddr;
	en_t<u32> p_paddr;
	en_t<u32> p_filesz;
	en_t<u32> p_memsz;
	en_t<u32> p_flags;
	en_t<u32> p_align;
};

template<template<typename T> class en_t, typename sz_t>
struct elf_prog final : elf_phdr<en_t, sz_t>
{
	std::vector<char> bin;

	using base = elf_phdr<en_t, sz_t>;

	elf_prog() = default;

	elf_prog(u32 type, u32 flags, sz_t vaddr, sz_t memsz, sz_t align, std::vector<char>&& bin)
		: bin(std::move(bin))
	{
		base::p_type = type;
		base::p_flags = flags;
		base::p_vaddr = vaddr;
		base::p_memsz = memsz;
		base::p_align = align;
		base::p_filesz = static_cast<sz_t>(bin.size());
		base::p_paddr = 0;
		base::p_offset = -1;
	}
};

template<template<typename T> class en_t, typename sz_t>
struct elf_shdr
{
	en_t<u32> sh_name;
	en_t<u32> sh_type;
	en_t<sz_t> sh_flags;
	en_t<sz_t> sh_addr;
	en_t<sz_t> sh_offset;
	en_t<sz_t> sh_size;
	en_t<u32> sh_link;
	en_t<u32> sh_info;
	en_t<sz_t> sh_addralign;
	en_t<sz_t> sh_entsize;
};

// Default elf_loader::load() return type, specialized to change.
template<typename T>
struct elf_load_result
{
	using type = void;
};

// ELF loader errors
enum class elf_error
{
	ok = 0,

	stream,
	stream_header,
	stream_phdrs,
	stream_shdrs,
	stream_data,

	header_magic,
	header_version,
	header_class,
	header_machine,
	header_endianness,
	header_type,
	header_os,
};

// ELF loader error information
template<>
struct unveil<elf_error>
{
	static inline const char* get(elf_error error)
	{
		switch (error)
		{
		case elf_error::ok: return "OK";

		case elf_error::stream: return "Invalid stream";
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

		default: throw error;
		}
	}
};

// ELF loader with specified parameters.
// en_t: endianness (specify le_t or be_t)
// sz_t: size (specify u32 for ELF32, u64 for ELF64)
template<template<typename T> class en_t, typename sz_t, elf_machine Machine, elf_os OS, elf_type Type>
class elf_loader
{
	elf_error m_error{};

	elf_error set_error(elf_error e)
	{
		return m_error = e;
	}

public:
	using ehdr_t = elf_ehdr<en_t, sz_t>;
	using phdr_t = elf_phdr<en_t, sz_t>;
	using shdr_t = elf_shdr<en_t, sz_t>;
	using prog_t = elf_prog<en_t, sz_t>;

	ehdr_t header{};

	std::vector<prog_t> progs;
	std::vector<shdr_t> shdrs;

public:
	elf_loader() = default;

	elf_loader(const fs::file& stream, u64 offset = 0)
	{
		open(stream, offset);
	}

	elf_error open(const fs::file& stream, u64 offset = 0)
	{
		// Check stream
		if (!stream)
			return set_error(elf_error::stream);

		// Read ELF header
		stream.seek(offset);
		if (!stream.read(header))
			return set_error(elf_error::stream_header);

		// Check magic
		if (header.e_magic != "\177ELF"_u32)
			return set_error(elf_error::header_magic);

		// Check class
		if (header.e_class != (std::is_same<sz_t, u32>::value ? 1 : 2))
			return set_error(elf_error::header_class);

		// Check endianness
		if (header.e_data != (std::is_same<en_t<u32>, le_t<u32>>::value ? 1 : 2))
			return set_error(elf_error::header_endianness);

		// Check machine
		if (header.e_machine != Machine)
			return set_error(elf_error::header_machine);

		// Check OS only if specified (hack)
		if (OS != elf_os::none && header.e_os_abi != OS)
			return set_error(elf_error::header_os);

		// Check type only if specified (hack)
		if (Type != elf_type::none && header.e_type != Type)
			return set_error(elf_error::header_type);

		// Check version and other params
		if (header.e_curver != 1 || header.e_version != 1 || header.e_ehsize != sizeof(ehdr_t))
			return set_error(elf_error::header_version);

		if (header.e_phnum && header.e_phentsize != sizeof(phdr_t))
			return set_error(elf_error::header_version);

		if (header.e_shnum && header.e_shentsize != sizeof(shdr_t))
			return set_error(elf_error::header_version);

		// Load program headers
		std::vector<phdr_t> _phdrs(header.e_phnum);
		stream.seek(offset + header.e_phoff);
		if (!stream.read(_phdrs))
			return set_error(elf_error::stream_phdrs);

		shdrs.resize(header.e_shnum);
		stream.seek(offset + header.e_shoff);
		if (!stream.read(shdrs))
			return set_error(elf_error::stream_shdrs);

		progs.clear();
		progs.reserve(_phdrs.size());
		for (const auto& hdr : _phdrs)
		{
			progs.emplace_back();

			static_cast<phdr_t&>(progs.back()) = hdr;
			progs.back().bin.resize(hdr.p_filesz);
			stream.seek(offset + hdr.p_offset);
			if (!stream.read(progs.back().bin))
				return set_error(elf_error::stream_data);
		}

		shdrs.shrink_to_fit();
		progs.shrink_to_fit();

		return m_error = elf_error::ok;
	}

	void save(const fs::file& stream) const
	{
		// Write header
		ehdr_t header{};
		header.e_magic = "\177ELF"_u32;
		header.e_class = std::is_same<sz_t, u32>::value ? 1 : 2;
		header.e_data = std::is_same<en_t<u32>, le_t<u32>>::value ? 1 : 2;
		header.e_curver = 1;
		header.e_os_abi = OS != elf_os::none ? OS : this->header.e_os_abi;
		header.e_abi_ver = this->header.e_abi_ver;
		header.e_type = Type != elf_type::none ? Type : this->header.e_type;
		header.e_machine = Machine;
		header.e_version = 1;
		header.e_entry = this->header.e_entry;
		header.e_phoff = SIZE_32(ehdr_t);
		header.e_shoff = SIZE_32(ehdr_t) + SIZE_32(phdr_t) * ::size32(progs);
		header.e_flags = this->header.e_flags;
		header.e_ehsize = SIZE_32(ehdr_t);
		header.e_phentsize = SIZE_32(phdr_t);
		header.e_phnum = ::size32(progs);
		header.e_shentsize = SIZE_32(shdr_t);
		header.e_shnum = ::size32(shdrs);
		header.e_shstrndx = this->header.e_shstrndx;
		stream.write(header);

		sz_t off = header.e_shoff + SIZE_32(shdr_t) * ::size32(shdrs);

		for (phdr_t phdr : progs)
		{
			phdr.p_offset = std::exchange(off, off + phdr.p_filesz);
			stream.write(phdr);
		}

		for (shdr_t shdr : shdrs)
		{
			// TODO?
			stream.write(shdr);
		}

		// Write data
		for (const auto& prog : progs)
		{
			stream.write(prog.bin);
		}
	}

	// Return error code
	operator elf_error() const
	{
		return m_error;
	}

	elf_error get_error() const
	{
		return m_error;
	}

	// Format-specific loader function (must be specialized)
	typename elf_load_result<elf_loader>::type load() const;
};

using ppu_exec_loader = elf_loader<be_t, u64, elf_machine::ppc64, elf_os::none, elf_type::exec>;
using ppu_prx_loader = elf_loader<be_t, u64, elf_machine::ppc64, elf_os::lv2, elf_type::prx>;
using spu_exec_loader = elf_loader<be_t, u32, elf_machine::spu, elf_os::none, elf_type::exec>;
using arm_exec_loader = elf_loader<le_t, u32, elf_machine::arm, elf_os::none, elf_type::none>;

template<> struct elf_load_result<ppu_prx_loader> { using type = std::shared_ptr<struct lv2_prx_t>; };
