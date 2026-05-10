#pragma once

#include "util/types.hpp"
#include "../../Utilities/File.h"
#include "../../Utilities/bit_set.h"

#include <span>

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

enum class sec_type : u32
{
	sht_null = 0,
	sht_progbits = 1,
	sht_symtab = 2,
	sht_strtab = 3,
	sht_rela = 4,
	sht_hash = 5,
	sht_dynamic = 6,
	sht_note = 7,
	sht_nobits = 8,
	sht_rel = 9,
};

enum class sh_flag : u32
{
	shf_write,
	shf_alloc,
	shf_execinstr,

	__bitset_enum_max
};

constexpr bool is_memorizable_section(sec_type type, bs_t<sh_flag> flags)
{
	switch (type)
	{
	case sec_type::sht_null:
	case sec_type::sht_nobits:
	{
		return false;
	}
	case sec_type::sht_progbits:
	{
		return flags.all_of(sh_flag::shf_alloc);
	}
	default:
	{
		if (type > sec_type::sht_rel)
		{
			return false;
		}

		return true;
	}
	}
}

template<typename T>
using elf_be = be_t<T>;

template<typename T>
using elf_le = le_t<T>;

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
	std::vector<uchar> bin{};

	using base = elf_phdr<en_t, sz_t>;

	elf_prog() = default;

	elf_prog(u32 type, u32 flags, sz_t vaddr, sz_t memsz, sz_t align, std::vector<uchar>&& bin)
		: bin(std::move(bin))
	{
		base::p_type = type;
		base::p_flags = flags;
		base::p_vaddr = vaddr;
		base::p_memsz = memsz;
		base::p_align = align;
		base::p_filesz = static_cast<sz_t>(this->bin.size());
		base::p_paddr = 0;
		base::p_offset = -1;
	}
};

template<template<typename T> class en_t, typename sz_t>
struct elf_shdr
{
	en_t<u32> sh_name;
	en_t<sec_type> sh_type;
	en_t<sz_t> _sh_flags;
	en_t<sz_t> sh_addr;
	en_t<sz_t> sh_offset;
	en_t<sz_t> sh_size;
	en_t<u32> sh_link;
	en_t<u32> sh_info;
	en_t<sz_t> sh_addralign;
	en_t<sz_t> sh_entsize;

	bs_t<sh_flag> sh_flags() const
	{
		return std::bit_cast<bs_t<sh_flag>>(static_cast<u32>(+_sh_flags));
	}
};

template<template<typename T> class en_t, typename sz_t>
struct elf_shdata final : elf_shdr<en_t, sz_t>
{
	std::vector<uchar> bin{};
	std::span<const uchar> bin_view{};

	using base = elf_shdr<en_t, sz_t>;

	elf_shdata() = default;

	std::span<const uchar> get_bin() const
	{
		if (!bin_view.empty())
		{
			return bin_view;
		}

		return {bin.data(), bin.size()};
	}
};

// ELF loading options
enum class elf_opt : u32
{
	no_programs, // Don't load phdrs, implies no_data
	no_sections, // Don't load shdrs
	no_data,     // Load phdrs without data

	__bitset_enum_max
};

// ELF loading error
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

// ELF object with specified parameters.
// en_t: endianness (elf_le or elf_be)
// sz_t: size (u32 for ELF32, u64 for ELF64)
template<template<typename T> class en_t, typename sz_t, elf_machine Machine, elf_os OS, elf_type Type>
class elf_object
{
	elf_error m_error = elf_error::stream; // Set initial error to "file not found" error

public:
	using ehdr_t = elf_ehdr<en_t, sz_t>;
	using phdr_t = elf_phdr<en_t, sz_t>;
	using shdr_t = elf_shdr<en_t, sz_t>;
	using prog_t = elf_prog<en_t, sz_t>;
	using shdata_t = elf_shdata<en_t, sz_t>;

	ehdr_t header{};

	std::vector<prog_t> progs{};
	std::vector<shdata_t> shdrs{};

	usz highest_offset = 0;

public:
	elf_object() = default;

	elf_object(const fs::file& stream, u64 offset = 0, bs_t<elf_opt> opts = {})
	{
		open(stream, offset, opts);
	}

	elf_error open(const fs::file& stream, u64 offset = 0, bs_t<elf_opt> opts = {})
	{
		highest_offset = 0;

		// Check stream
		if (!stream)
			return set_error(elf_error::stream);

		// Read ELF header
		if (sizeof(header) != stream.read_at(offset, &header, sizeof(header)))
			return set_error(elf_error::stream_header);

		// Check magic
		if (header.e_magic != "\177ELF"_u32)
			return set_error(elf_error::header_magic);

		// Check class
		if (header.e_class != (std::is_same_v<sz_t, u32> ? 1 : 2))
			return set_error(elf_error::header_class);

		// Check endianness
		if (header.e_data != (std::is_same_v<en_t<u32>, le_t<u32>> ? 1 : 2))
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
		if (header.e_curver != 1 || header.e_version != 1u || header.e_ehsize != u16{sizeof(ehdr_t)})
			return set_error(elf_error::header_version);

		if (header.e_phnum && header.e_phentsize != u16{sizeof(phdr_t)})
			return set_error(elf_error::header_version);

		if (header.e_shnum && header.e_shentsize != u16{sizeof(shdr_t)})
			return set_error(elf_error::header_version);

		highest_offset = sizeof(header);

		// Load program headers
		std::vector<phdr_t> _phdrs;
		std::vector<shdr_t> _shdrs;

		u64 seek_pos = 0;

		if (!(opts & elf_opt::no_programs))
		{
			seek_pos = offset + header.e_phoff;
			highest_offset = std::max<usz>(highest_offset, seek_pos);

			if (!stream.read(_phdrs, header.e_phnum, true, seek_pos))
				return set_error(elf_error::stream_phdrs);
		}

		if (!(opts & elf_opt::no_sections))
		{
			seek_pos = offset + header.e_shoff;
			highest_offset = std::max<usz>(highest_offset, seek_pos);

			if (!stream.read(_shdrs, header.e_shnum, true, seek_pos))
				return set_error(elf_error::stream_shdrs);
		}

		progs.clear();
		progs.reserve(_phdrs.size());
		for (const auto& hdr : _phdrs)
		{
			static_cast<phdr_t&>(progs.emplace_back()) = hdr;

			if (!(opts & elf_opt::no_data))
			{
				seek_pos = offset + hdr.p_offset;
				highest_offset = std::max<usz>(highest_offset, seek_pos);

				if (!stream.read(progs.back().bin, hdr.p_filesz, true, seek_pos))
					return set_error(elf_error::stream_data);
			}
		}

		shdrs.clear();
		shdrs.reserve(_shdrs.size());
		for (const auto& shdr : _shdrs)
		{
			static_cast<shdr_t&>(shdrs.emplace_back()) = shdr;

			if (!(opts & elf_opt::no_data) && is_memorizable_section(shdr.sh_type, shdr.sh_flags()))
			{
				usz p_index = umax;

				for (const auto& hdr : _phdrs)
				{
					// Try to find it in phdr data instead of allocating new section
					p_index++;

					if (hdr.p_offset <= shdr.sh_offset && shdr.sh_offset + shdr.sh_size - 1 <= hdr.p_offset + hdr.p_filesz - 1)
					{
						const auto& prog = ::at32(progs, p_index);
						shdrs.back().bin_view = {prog.bin.data() + shdr.sh_offset - hdr.p_offset, shdr.sh_size};
					}
				}

				if (!shdrs.back().bin_view.empty())
				{
					// Optimized
					continue;
				}

				seek_pos = offset + shdr.sh_offset;
				highest_offset = std::max<usz>(highest_offset, seek_pos);

				if (!stream.read(shdrs.back().bin, shdr.sh_size, true, seek_pos))
					return set_error(elf_error::stream_data);
			}
		}

		shdrs.shrink_to_fit();
		progs.shrink_to_fit();

		return m_error = elf_error::ok;
	}

	std::vector<u8> save(std::vector<u8>&& init = std::vector<u8>{}) const
	{
		if (get_error() != elf_error::ok)
		{
			return std::move(init);
		}

		fs::file stream = fs::make_stream<std::vector<u8>>(std::move(init));

		const bool fixup_shdrs = shdrs.empty() || shdrs[0].sh_type != sec_type::sht_null;

		// Write header
		ehdr_t header{};
		header.e_magic = "\177ELF"_u32;
		header.e_class = std::is_same_v<sz_t, u32> ? 1 : 2;
		header.e_data = std::is_same_v<en_t<u32>, le_t<u32>> ? 1 : 2;
		header.e_curver = 1;
		header.e_os_abi = OS != elf_os::none ? OS : this->header.e_os_abi;
		header.e_abi_ver = this->header.e_abi_ver;
		header.e_type = Type != elf_type::none ? Type : static_cast<elf_type>(this->header.e_type);
		header.e_machine = Machine;
		header.e_version = 1;
		header.e_entry = this->header.e_entry;
		header.e_phoff = u32{sizeof(ehdr_t)};
		header.e_shoff = u32{sizeof(ehdr_t)} + u32{sizeof(phdr_t)} * ::size32(progs);
		header.e_flags = this->header.e_flags;
		header.e_ehsize = u32{sizeof(ehdr_t)};
		header.e_phentsize = u32{sizeof(phdr_t)};
		header.e_phnum = ::size32(progs);
		header.e_shentsize = u32{sizeof(shdr_t)};
		header.e_shnum = ::size32(shdrs) + u32{fixup_shdrs};
		header.e_shstrndx = this->header.e_shstrndx;
		stream.write(header);

		sz_t off = header.e_shoff + u32{sizeof(shdr_t)} * ::size32(shdrs);

		for (phdr_t phdr : progs)
		{
			phdr.p_offset = std::exchange(off, off + phdr.p_filesz);
			stream.write(phdr);
		}

		if (fixup_shdrs)
		{
			// Insert a must-have empty section at the start
			stream.write(shdr_t{});
		}

		for (const auto& shdr : shdrs)
		{
			shdr_t out = static_cast<shdr_t>(shdr);

			if (is_memorizable_section(shdr.sh_type, shdr.sh_flags()))
			{
				usz p_index = umax;
				usz data_base = header.e_shoff + u32{sizeof(shdr_t)} * ::size32(shdrs);
				bool result = false;

				for (const auto& hdr : progs)
				{
					if (shdr.bin_view.empty())
					{
						break;
					}

					// Try to find it in phdr data instead of writing new section
					p_index++;

					// Rely on previous sh_offset value!
					if (hdr.p_offset <= shdr.sh_offset && shdr.sh_offset + shdr.sh_size - 1 <= hdr.p_offset + hdr.p_filesz - 1)
					{
						out.sh_offset = ::narrow<sz_t>(data_base + static_cast<usz>(shdr.sh_offset - hdr.p_offset));
						result = true;
						break;
					}

					data_base += ::at32(progs, p_index).p_filesz;
				}

				if (result)
				{
					// Optimized
				}
				else
				{
					out.sh_offset = std::exchange(off, off + shdr.sh_size);
				}
			}

			stream.write(static_cast<shdr_t>(out));
		}

		// Write data
		for (const auto& prog : progs)
		{
			stream.write(prog.bin);
		}

		for (const auto& shdr : shdrs)
		{
			if (!is_memorizable_section(shdr.sh_type, shdr.sh_flags()) || !shdr.bin_view.empty())
			{
				continue;
			}

			stream.write(shdr.bin);
		}

		return std::move(static_cast<fs::container_stream<std::vector<u8>>*>(stream.release().get())->obj);
	}

	elf_object& clear()
	{
		// Do not use clear() in order to dealloc memory
		progs = {};
		shdrs = {};
		header.e_magic = 0;
		m_error = elf_error::stream;
		return *this;
	}

	elf_object& set_error(elf_error error)
	{
		// Setting an error causes the state to clear if there was no error before
		if (m_error == elf_error::ok && error != elf_error::ok)
			clear();

		m_error = error;
		highest_offset = 0;
		return *this;
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
};

using ppu_exec_object = elf_object<elf_be, u64, elf_machine::ppc64, elf_os::none, elf_type::exec>;
using ppu_prx_object  = elf_object<elf_be, u64, elf_machine::ppc64, elf_os::lv2, elf_type::prx>;
using ppu_rel_object  = elf_object<elf_be, u64, elf_machine::ppc64, elf_os::lv2, elf_type::rel>;
using spu_exec_object = elf_object<elf_be, u32, elf_machine::spu, elf_os::none, elf_type::exec>;
using spu_rel_object  = elf_object<elf_be, u32, elf_machine::spu, elf_os::none, elf_type::rel>;
using arm_exec_object = elf_object<elf_le, u32, elf_machine::arm, elf_os::none, elf_type::none>;
