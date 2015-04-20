#include "stdafx.h"
#include "Utilities/Log.h"
#include "Utilities/rFile.h"
#include "aes.h"
#include "sha1.h"
#include "utils.h"
#include "Emu/FS/vfsLocalFile.h"
#include "unself.h"
#pragma warning(disable : 4996)
#include <wx/mstream.h>
#include <wx/zstream.h>

__forceinline u8 Read8(vfsStream& f)
{
	u8 ret;
	f.Read(&ret, sizeof(ret));
	return ret;
}

__forceinline u16 Read16(vfsStream& f)
{
	be_t<u16> ret;
	f.Read(&ret, sizeof(ret));
	return ret;
}

__forceinline u32 Read32(vfsStream& f)
{
	be_t<u32> ret;
	f.Read(&ret, sizeof(ret));
	return ret;
}

__forceinline u64 Read64(vfsStream& f)
{
	be_t<u64> ret;
	f.Read(&ret, sizeof(ret));
	return ret;
}

__forceinline u16 Read16LE(vfsStream& f)
{
	u16 ret;
	f.Read(&ret, sizeof(ret));
	return ret;
}

__forceinline u32 Read32LE(vfsStream& f)
{
	u32 ret;
	f.Read(&ret, sizeof(ret));
	return ret;
}

__forceinline u64 Read64LE(vfsStream& f)
{
	u64 ret;
	f.Read(&ret, sizeof(ret));
	return ret;
}

__forceinline void Write8(vfsStream& f, const u8 data)
{
	f.Write(&data, sizeof(data));
}

__forceinline void Write8(const rfile_t& f, const u8 data)
{
	f.write(&data, sizeof(data));
}

__forceinline void Write16LE(vfsStream& f, const u16 data)
{
	f.Write(&data, sizeof(data));
}

__forceinline void Write16LE(const rfile_t& f, const u16 data)
{
	f.write(&data, sizeof(data));
}

__forceinline void Write32LE(vfsStream& f, const u32 data)
{
	f.Write(&data, sizeof(data));
}

__forceinline void Write32LE(const rfile_t& f, const u32 data)
{
	f.write(&data, sizeof(data));
}

__forceinline void Write64LE(vfsStream& f, const u64 data)
{
	f.Write(&data, sizeof(data));
}

__forceinline void Write64LE(const rfile_t& f, const u64 data)
{
	f.write(&data, sizeof(data));
}

__forceinline void Write16(vfsStream& f, const u16 data)
{
	Write16LE(f, re16(data));
}

__forceinline void Write16(const rfile_t& f, const u16 data)
{
	Write16LE(f, re16(data));
}

__forceinline void Write32(vfsStream& f, const u32 data)
{
	Write32LE(f, re32(data));
}

__forceinline void Write32(const rfile_t& f, const u32 data)
{
	Write32LE(f, re32(data));
}

__forceinline void Write64(vfsStream& f, const u64 data)
{
	Write64LE(f, re64(data));
}

__forceinline void Write64(const rfile_t& f, const u64 data)
{
	Write64LE(f, re64(data));
}

void WriteEhdr(const rfile_t& f, Elf64_Ehdr& ehdr)
{
	Write32(f, ehdr.e_magic);
	Write8(f, ehdr.e_class);
	Write8(f, ehdr.e_data);
	Write8(f, ehdr.e_curver);
	Write8(f, ehdr.e_os_abi);
	Write64(f, ehdr.e_abi_ver);
	Write16(f, ehdr.e_type);
	Write16(f, ehdr.e_machine);
	Write32(f, ehdr.e_version);
	Write64(f, ehdr.e_entry);
	Write64(f, ehdr.e_phoff);
	Write64(f, ehdr.e_shoff);
	Write32(f, ehdr.e_flags);
	Write16(f, ehdr.e_ehsize);
	Write16(f, ehdr.e_phentsize);
	Write16(f, ehdr.e_phnum);
	Write16(f, ehdr.e_shentsize);
	Write16(f, ehdr.e_shnum);
	Write16(f, ehdr.e_shstrndx);
}

void WritePhdr(const rfile_t& f, Elf64_Phdr& phdr)
{
	Write32(f, phdr.p_type);
	Write32(f, phdr.p_flags);
	Write64(f, phdr.p_offset);
	Write64(f, phdr.p_vaddr);
	Write64(f, phdr.p_paddr);
	Write64(f, phdr.p_filesz);
	Write64(f, phdr.p_memsz);
	Write64(f, phdr.p_align);
}

void WriteShdr(const rfile_t& f, Elf64_Shdr& shdr)
{
	Write32(f, shdr.sh_name);
	Write32(f, shdr.sh_type);
	Write64(f, shdr.sh_flags);
	Write64(f, shdr.sh_addr);
	Write64(f, shdr.sh_offset);
	Write64(f, shdr.sh_size);
	Write32(f, shdr.sh_link);
	Write32(f, shdr.sh_info);
	Write64(f, shdr.sh_addralign);
	Write64(f, shdr.sh_entsize);
}

void WriteEhdr(const rfile_t& f, Elf32_Ehdr& ehdr)
{
	Write32(f, ehdr.e_magic);
	Write8(f, ehdr.e_class);
	Write8(f, ehdr.e_data);
	Write8(f, ehdr.e_curver);
	Write8(f, ehdr.e_os_abi);
	Write64(f, ehdr.e_abi_ver);
	Write16(f, ehdr.e_type);
	Write16(f, ehdr.e_machine);
	Write32(f, ehdr.e_version);
	Write32(f, ehdr.e_entry);
	Write32(f, ehdr.e_phoff);
	Write32(f, ehdr.e_shoff);
	Write32(f, ehdr.e_flags);
	Write16(f, ehdr.e_ehsize);
	Write16(f, ehdr.e_phentsize);
	Write16(f, ehdr.e_phnum);
	Write16(f, ehdr.e_shentsize);
	Write16(f, ehdr.e_shnum);
	Write16(f, ehdr.e_shstrndx);
}

void WritePhdr(const rfile_t& f, Elf32_Phdr& phdr)
{
	Write32(f, phdr.p_type);
	Write32(f, phdr.p_offset);
	Write32(f, phdr.p_vaddr);
	Write32(f, phdr.p_paddr);
	Write32(f, phdr.p_filesz);
	Write32(f, phdr.p_memsz);
	Write32(f, phdr.p_flags);
	Write32(f, phdr.p_align);
}

void WriteShdr(const rfile_t& f, Elf32_Shdr& shdr)
{
	Write32(f, shdr.sh_name);
	Write32(f, shdr.sh_type);
	Write32(f, shdr.sh_flags);
	Write32(f, shdr.sh_addr);
	Write32(f, shdr.sh_offset);
	Write32(f, shdr.sh_size);
	Write32(f, shdr.sh_link);
	Write32(f, shdr.sh_info);
	Write32(f, shdr.sh_addralign);
	Write32(f, shdr.sh_entsize);
}


void AppInfo::Load(vfsStream& f)
{
	authid    = Read64(f);
	vendor_id = Read32(f);
	self_type = Read32(f);
	version   = Read64(f);
	padding   = Read64(f);
}

void AppInfo::Show()
{
	LOG_NOTICE(LOADER, "AuthID: 0x%llx", authid);
	LOG_NOTICE(LOADER, "VendorID: 0x%08x", vendor_id);
	LOG_NOTICE(LOADER, "SELF type: 0x%08x", self_type);
	LOG_NOTICE(LOADER, "Version: 0x%llx", version);
}

void SectionInfo::Load(vfsStream& f)
{
	offset     = Read64(f);
	size       = Read64(f);
	compressed = Read32(f);
	unknown1   = Read32(f);
	unknown2   = Read32(f);
	encrypted  = Read32(f);
}

void SectionInfo::Show()
{
	LOG_NOTICE(LOADER, "Offset: 0x%llx", offset);
	LOG_NOTICE(LOADER, "Size: 0x%llx", size);
	LOG_NOTICE(LOADER, "Compressed: 0x%08x", compressed);
	LOG_NOTICE(LOADER, "Unknown1: 0x%08x", unknown1);
	LOG_NOTICE(LOADER, "Unknown2: 0x%08x", unknown2);
	LOG_NOTICE(LOADER, "Encrypted: 0x%08x", encrypted);
}

void SCEVersionInfo::Load(vfsStream& f)
{
	subheader_type = Read32(f);
	present        = Read32(f);
	size           = Read32(f);
	unknown        = Read32(f);
}

void SCEVersionInfo::Show()
{
	LOG_NOTICE(LOADER, "Sub-header type: 0x%08x", subheader_type);
	LOG_NOTICE(LOADER, "Present: 0x%08x", present);
	LOG_NOTICE(LOADER, "Size: 0x%08x", size);
	LOG_NOTICE(LOADER, "Unknown: 0x%08x", unknown);
}

void ControlInfo::Load(vfsStream& f)
{
	type = Read32(f);
	size = Read32(f);
	next = Read64(f);

	if (type == 1)
	{
		control_flags.ctrl_flag1 = Read32(f);
		control_flags.unknown1 = Read32(f);
		control_flags.unknown2 = Read32(f);
		control_flags.unknown3 = Read32(f);
		control_flags.unknown4 = Read32(f);
		control_flags.unknown5 = Read32(f);
		control_flags.unknown6 = Read32(f);
		control_flags.unknown7 = Read32(f);
	}
	else if (type == 2)
	{
		if (size == 0x30)
		{
			f.Read(file_digest_30.digest, 20);
			file_digest_30.unknown = Read64(f);
		}
		else if (size == 0x40)
		{
			f.Read(file_digest_40.digest1, 20);
			f.Read(file_digest_40.digest2, 20);
			file_digest_40.unknown = Read64(f);
		}
	}
	else if (type == 3)
	{
		npdrm.magic = Read32(f);
		npdrm.unknown1 = Read32(f);
		npdrm.license = Read32(f);
		npdrm.type = Read32(f);
		f.Read(npdrm.content_id, 48);
		f.Read(npdrm.digest, 16);
		f.Read(npdrm.invdigest, 16);
		f.Read(npdrm.xordigest, 16);
		npdrm.unknown2 = Read64(f);
		npdrm.unknown3 = Read64(f);
	}
}

void ControlInfo::Show()
{
	LOG_NOTICE(LOADER, "Type: 0x%08x", type);
	LOG_NOTICE(LOADER, "Size: 0x%08x", size);
	LOG_NOTICE(LOADER, "Next: 0x%llx", next);

	if (type == 1)
	{
		LOG_NOTICE(LOADER, "Control flag 1: 0x%08x", control_flags.ctrl_flag1);
		LOG_NOTICE(LOADER, "Unknown1: 0x%08x", control_flags.unknown1);
		LOG_NOTICE(LOADER, "Unknown2: 0x%08x", control_flags.unknown2);
		LOG_NOTICE(LOADER, "Unknown3: 0x%08x", control_flags.unknown3);
		LOG_NOTICE(LOADER, "Unknown4: 0x%08x", control_flags.unknown4);
		LOG_NOTICE(LOADER, "Unknown5: 0x%08x", control_flags.unknown5);
		LOG_NOTICE(LOADER, "Unknown6: 0x%08x", control_flags.unknown6);
		LOG_NOTICE(LOADER, "Unknown7: 0x%08x", control_flags.unknown7);
	}
	else if (type == 2)
	{
		if (size == 0x30)
		{
			std::string digest_str;
			for (int i = 0; i < 20; i++)
				digest_str += fmt::Format("%02x", file_digest_30.digest[i]);

			LOG_NOTICE(LOADER, "Digest: %s", digest_str.c_str());
			LOG_NOTICE(LOADER, "Unknown: 0x%llx", file_digest_30.unknown);
		}
		else if (size == 0x40)
		{
			std::string digest_str1;
			std::string digest_str2;
			for (int i = 0; i < 20; i++)
			{
				digest_str1 += fmt::Format("%02x", file_digest_40.digest1[i]);
				digest_str2 += fmt::Format("%02x", file_digest_40.digest2[i]);
			}

			LOG_NOTICE(LOADER, "Digest1: %s", digest_str1.c_str());
			LOG_NOTICE(LOADER, "Digest2: %s", digest_str2.c_str());
			LOG_NOTICE(LOADER, "Unknown: 0x%llx", file_digest_40.unknown);
		}
	}
	else if (type == 3)
	{
		std::string contentid_str;
		std::string digest_str;
		std::string invdigest_str;
		std::string xordigest_str;
		for (int i = 0; i < 48; i++)
			contentid_str += fmt::Format("%02x", npdrm.content_id[i]);
		for (int i = 0; i < 16; i++)
		{
			digest_str += fmt::Format("%02x", npdrm.digest[i]);
			invdigest_str += fmt::Format("%02x", npdrm.invdigest[i]);
			xordigest_str += fmt::Format("%02x", npdrm.xordigest[i]);
		}

		LOG_NOTICE(LOADER, "Magic: 0x%08x", npdrm.magic);
		LOG_NOTICE(LOADER, "Unknown1: 0x%08x", npdrm.unknown1);
		LOG_NOTICE(LOADER, "License: 0x%08x", npdrm.license);
		LOG_NOTICE(LOADER, "Type: 0x%08x", npdrm.type);
		LOG_NOTICE(LOADER, "ContentID: %s", contentid_str.c_str());
		LOG_NOTICE(LOADER, "Digest: %s", digest_str.c_str());
		LOG_NOTICE(LOADER, "Inverse digest: %s", invdigest_str.c_str());
		LOG_NOTICE(LOADER, "XOR digest: %s", xordigest_str.c_str());
		LOG_NOTICE(LOADER, "Unknown2: 0x%llx", npdrm.unknown2);
		LOG_NOTICE(LOADER, "Unknown3: 0x%llx", npdrm.unknown3);
	}
}

void MetadataInfo::Load(u8* in)
{
	memcpy(key, in, 0x10);
	memcpy(key_pad, in + 0x10, 0x10);
	memcpy(iv, in + 0x20, 0x10);
	memcpy(iv_pad, in + 0x30, 0x10);
}

void MetadataInfo::Show()
{
	std::string key_str;
	std::string key_pad_str;
	std::string iv_str;
	std::string iv_pad_str;
	for (int i = 0; i < 0x10; i++)
	{
		key_str += fmt::Format("%02x", key[i]);
		key_pad_str += fmt::Format("%02x", key_pad[i]);
		iv_str += fmt::Format("%02x", iv[i]);
		iv_pad_str += fmt::Format("%02x", iv_pad[i]);
	}

	LOG_NOTICE(LOADER, "Key: %s", key_str.c_str());
	LOG_NOTICE(LOADER, "Key pad: %s", key_pad_str.c_str());
	LOG_NOTICE(LOADER, "IV: %s", iv_str.c_str());
	LOG_NOTICE(LOADER, "IV pad: %s", iv_pad_str.c_str());
}

void MetadataHeader::Load(u8* in)
{
	memcpy(&signature_input_length, in, 8);
	memcpy(&unknown1, in + 8, 4);
	memcpy(&section_count, in + 12, 4);
	memcpy(&key_count, in + 16, 4);
	memcpy(&opt_header_size, in + 20, 4);
	memcpy(&unknown2, in + 24, 4);
	memcpy(&unknown3, in + 28, 4);

	// Endian swap.
	signature_input_length = swap64(signature_input_length);
	unknown1 = swap32(unknown1);
	section_count = swap32(section_count);
	key_count = swap32(key_count);
	opt_header_size = swap32(opt_header_size);
	unknown2 = swap32(unknown2);
	unknown3 = swap32(unknown3);
}

void MetadataHeader::Show()
{
	LOG_NOTICE(LOADER, "Signature input length: 0x%llx", signature_input_length);
	LOG_NOTICE(LOADER, "Unknown1: 0x%08x", unknown1);
	LOG_NOTICE(LOADER, "Section count: 0x%08x", section_count);
	LOG_NOTICE(LOADER, "Key count: 0x%08x", key_count);
	LOG_NOTICE(LOADER, "Optional header size: 0x%08x", opt_header_size);
	LOG_NOTICE(LOADER, "Unknown2: 0x%08x", unknown2);
	LOG_NOTICE(LOADER, "Unknown3: 0x%08x", unknown3);
}

void MetadataSectionHeader::Load(u8* in)
{
	memcpy(&data_offset, in, 8);
	memcpy(&data_size, in + 8, 8);
	memcpy(&type, in + 16, 4);
	memcpy(&program_idx, in + 20, 4);
	memcpy(&hashed, in + 24, 4);
	memcpy(&sha1_idx, in + 28, 4);
	memcpy(&encrypted, in + 32, 4);
	memcpy(&key_idx, in + 36, 4);
	memcpy(&iv_idx, in + 40, 4);
	memcpy(&compressed, in + 44, 4);

	// Endian swap.
	data_offset = swap64(data_offset);
	data_size = swap64(data_size);
	type = swap32(type);
	program_idx = swap32(program_idx);
	hashed = swap32(hashed);
	sha1_idx = swap32(sha1_idx);
	encrypted = swap32(encrypted);
	key_idx = swap32(key_idx);
	iv_idx = swap32(iv_idx);
	compressed = swap32(compressed);
}

void MetadataSectionHeader::Show()
{
	LOG_NOTICE(LOADER, "Data offset: 0x%llx", data_offset);
	LOG_NOTICE(LOADER, "Data size: 0x%llx", data_size);
	LOG_NOTICE(LOADER, "Type: 0x%08x", type);
	LOG_NOTICE(LOADER, "Program index: 0x%08x", program_idx);
	LOG_NOTICE(LOADER, "Hashed: 0x%08x", hashed);
	LOG_NOTICE(LOADER, "SHA1 index: 0x%08x", sha1_idx);
	LOG_NOTICE(LOADER, "Encrypted: 0x%08x", encrypted);
	LOG_NOTICE(LOADER, "Key index: 0x%08x", key_idx);
	LOG_NOTICE(LOADER, "IV index: 0x%08x", iv_idx);
	LOG_NOTICE(LOADER, "Compressed: 0x%08x", compressed);
}

void SectionHash::Load(vfsStream& f)
{
	f.Read(sha1, 20);
	f.Read(padding, 12);
	f.Read(hmac_key, 64);
}

void CapabilitiesInfo::Load(vfsStream& f)
{
	type     = Read32(f);
	capabilities_size = Read32(f);
	next     = Read32(f);
	unknown1 = Read32(f);
	unknown2 = Read64(f);
	unknown3 = Read64(f);
	flags    = Read64(f);
	unknown4 = Read32(f);
	unknown5 = Read32(f);
}

void Signature::Load(vfsStream& f)
{
	f.Read(r, 21);
	f.Read(s, 21);
	f.Read(padding, 6);
}

void SelfSection::Load(vfsStream& f)
{
	*data = Read32(f);
	size = Read64(f);
	offset = Read64(f);
}

void Elf32_Ehdr::Load(vfsStream& f)
{
	e_magic = Read32(f);
	e_class = Read8(f);
	e_data = Read8(f);
	e_curver = Read8(f);
	e_os_abi = Read8(f);

	if (IsLittleEndian())
	{
		e_abi_ver = Read64LE(f);
		e_type = Read16LE(f);
		e_machine = Read16LE(f);
		e_version = Read32LE(f);
		e_entry = Read32LE(f);
		e_phoff = Read32LE(f);
		e_shoff = Read32LE(f);
		e_flags = Read32LE(f);
		e_ehsize = Read16LE(f);
		e_phentsize = Read16LE(f);
		e_phnum = Read16LE(f);
		e_shentsize = Read16LE(f);
		e_shnum = Read16LE(f);
		e_shstrndx = Read16LE(f);
	}
	else
	{
		e_abi_ver = Read64(f);
		e_type = Read16(f);
		e_machine = Read16(f);
		e_version = Read32(f);
		e_entry = Read32(f);
		e_phoff = Read32(f);
		e_shoff = Read32(f);
		e_flags = Read32(f);
		e_ehsize = Read16(f);
		e_phentsize = Read16(f);
		e_phnum = Read16(f);
		e_shentsize = Read16(f);
		e_shnum = Read16(f);
		e_shstrndx = Read16(f);
	}
}

void Elf32_Shdr::Load(vfsStream& f)
{
	sh_name = Read32(f);
	sh_type = Read32(f);
	sh_flags = Read32(f);
	sh_addr = Read32(f);
	sh_offset = Read32(f);
	sh_size = Read32(f);
	sh_link = Read32(f);
	sh_info = Read32(f);
	sh_addralign = Read32(f);
	sh_entsize = Read32(f);
}

void Elf32_Shdr::LoadLE(vfsStream& f)
{
	f.Read(this, sizeof(*this));
}

void Elf32_Phdr::Load(vfsStream& f)
{
	p_type = Read32(f);
	p_offset = Read32(f);
	p_vaddr = Read32(f);
	p_paddr = Read32(f);
	p_filesz = Read32(f);
	p_memsz = Read32(f);
	p_flags = Read32(f);
	p_align = Read32(f);
}

void Elf32_Phdr::LoadLE(vfsStream& f)
{
	f.Read(this, sizeof(*this));
}

void Elf64_Ehdr::Load(vfsStream& f)
{
	e_magic = Read32(f);
	e_class = Read8(f);
	e_data = Read8(f);
	e_curver = Read8(f);
	e_os_abi = Read8(f);
	e_abi_ver = Read64(f);
	e_type = Read16(f);
	e_machine = Read16(f);
	e_version = Read32(f);
	e_entry = Read64(f);
	e_phoff = Read64(f);
	e_shoff = Read64(f);
	e_flags = Read32(f);
	e_ehsize = Read16(f);
	e_phentsize = Read16(f);
	e_phnum = Read16(f);
	e_shentsize = Read16(f);
	e_shnum = Read16(f);
	e_shstrndx = Read16(f);
}

void Elf64_Shdr::Load(vfsStream& f)
{
	sh_name = Read32(f);
	sh_type = Read32(f);
	sh_flags = Read64(f);
	sh_addr = Read64(f);
	sh_offset = Read64(f);
	sh_size = Read64(f);
	sh_link = Read32(f);
	sh_info = Read32(f);
	sh_addralign = Read64(f);
	sh_entsize = Read64(f);
}

void Elf64_Phdr::Load(vfsStream& f)
{
	p_type = Read32(f);
	p_flags = Read32(f);
	p_offset = Read64(f);
	p_vaddr = Read64(f);
	p_paddr = Read64(f);
	p_filesz = Read64(f);
	p_memsz = Read64(f);
	p_align = Read64(f);
}

void SceHeader::Load(vfsStream& f)
{
	se_magic = Read32(f);
	se_hver = Read32(f);
	se_flags = Read16(f);
	se_type = Read16(f);
	se_meta = Read32(f);
	se_hsize = Read64(f);
	se_esize = Read64(f);
}

void SelfHeader::Load(vfsStream& f)
{
	se_htype = Read64(f);
	se_appinfooff = Read64(f);
	se_elfoff = Read64(f);
	se_phdroff = Read64(f);
	se_shdroff = Read64(f);
	se_secinfoff = Read64(f);
	se_sceveroff = Read64(f);
	se_controloff = Read64(f);
	se_controlsize = Read64(f);
	pad = Read64(f);
}

SELFDecrypter::SELFDecrypter(vfsStream& s)
	: self_f(s)
	, key_v()
	, data_buf_length(0)
{
}

bool SELFDecrypter::LoadHeaders(bool isElf32)
{
	// Read SCE header.
	self_f.Seek(0);
	sce_hdr.Load(self_f);

	// Check SCE magic.
	if (!sce_hdr.CheckMagic())
	{
		LOG_ERROR(LOADER, "SELF: Not a SELF file!");
		return false;
	}

	// Read SELF header.
	self_hdr.Load(self_f);

	// Read the APP INFO.
	self_f.Seek(self_hdr.se_appinfooff);
	app_info.Load(self_f);

	// Read ELF header.
	self_f.Seek(self_hdr.se_elfoff);
	if (isElf32)
		elf32_hdr.Load(self_f);
	else
		elf64_hdr.Load(self_f);

	// Read ELF program headers.
	if (isElf32)
	{
		phdr32_arr.clear();
		if(elf32_hdr.e_phoff == 0 && elf32_hdr.e_phnum)
		{
			LOG_ERROR(LOADER, "SELF: ELF program header offset is null!");
			return false;
		}
		self_f.Seek(self_hdr.se_phdroff);
		for(u32 i = 0; i < elf32_hdr.e_phnum; ++i)
		{
			phdr32_arr.emplace_back();
			phdr32_arr.back().Load(self_f);
		}
	}
	else
	{
		phdr64_arr.clear();
		if(elf64_hdr.e_phoff == 0 && elf64_hdr.e_phnum)
		{
			LOG_ERROR(LOADER, "SELF: ELF program header offset is null!");
			return false;
		}
		self_f.Seek(self_hdr.se_phdroff);
		for(u32 i = 0; i < elf64_hdr.e_phnum; ++i)
		{
			phdr64_arr.emplace_back();
			phdr64_arr.back().Load(self_f);
		}
	}


	// Read section info.
	secinfo_arr.clear();
	self_f.Seek(self_hdr.se_secinfoff);

	for(u32 i = 0; i < ((isElf32) ? elf32_hdr.e_phnum : elf64_hdr.e_phnum); ++i)
	{
		secinfo_arr.emplace_back();
		secinfo_arr.back().Load(self_f);
	}

	// Read SCE version info.
	self_f.Seek(self_hdr.se_sceveroff);
	scev_info.Load(self_f);

	// Read control info.
	ctrlinfo_arr.clear();
	self_f.Seek(self_hdr.se_controloff);

	u32 i = 0;
	while(i < self_hdr.se_controlsize)
	{
		ctrlinfo_arr.emplace_back();
		ControlInfo &cinfo = ctrlinfo_arr.back();
		cinfo.Load(self_f);
		i += cinfo.size;
	}

	// Read ELF section headers.
	if (isElf32)
	{
		shdr32_arr.clear();
		if(elf32_hdr.e_shoff == 0 && elf32_hdr.e_shnum)
		{
			LOG_WARNING(LOADER, "SELF: ELF section header offset is null!");
			return true;
		}
		self_f.Seek(self_hdr.se_shdroff);
		for(u32 i = 0; i < elf32_hdr.e_shnum; ++i)
		{
			shdr32_arr.emplace_back();
			shdr32_arr.back().Load(self_f);
		}
	}
	else
	{
		shdr64_arr.clear();
		if(elf64_hdr.e_shoff == 0 && elf64_hdr.e_shnum)
		{
			LOG_WARNING(LOADER, "SELF: ELF section header offset is null!");
			return true;
		}
		self_f.Seek(self_hdr.se_shdroff);
		for(u32 i = 0; i < elf64_hdr.e_shnum; ++i)
		{
			shdr64_arr.emplace_back();
			shdr64_arr.back().Load(self_f);
		}
	}

	return true;
}

void SELFDecrypter::ShowHeaders(bool isElf32)
{
	LOG_NOTICE(LOADER, "SCE header");
	LOG_NOTICE(LOADER, "----------------------------------------------------");
	sce_hdr.Show();
	LOG_NOTICE(LOADER, "----------------------------------------------------");
	LOG_NOTICE(LOADER, "SELF header");
	LOG_NOTICE(LOADER, "----------------------------------------------------");
	self_hdr.Show();
	LOG_NOTICE(LOADER, "----------------------------------------------------");
	LOG_NOTICE(LOADER, "APP INFO");
	LOG_NOTICE(LOADER, "----------------------------------------------------");
	app_info.Show();
	LOG_NOTICE(LOADER, "----------------------------------------------------");
	LOG_NOTICE(LOADER, "ELF header");
	LOG_NOTICE(LOADER, "----------------------------------------------------");
	isElf32 ? elf32_hdr.Show() : elf64_hdr.Show();
	LOG_NOTICE(LOADER, "----------------------------------------------------");
	LOG_NOTICE(LOADER, "ELF program headers");
	LOG_NOTICE(LOADER, "----------------------------------------------------");
	for(unsigned int i = 0; i < ((isElf32) ? phdr32_arr.size() : phdr64_arr.size()); i++)
		isElf32 ? phdr32_arr[i].Show() : phdr64_arr[i].Show();
	LOG_NOTICE(LOADER, "----------------------------------------------------");
	LOG_NOTICE(LOADER, "Section info");
	LOG_NOTICE(LOADER, "----------------------------------------------------");
	for(unsigned int i = 0; i < secinfo_arr.size(); i++)
		secinfo_arr[i].Show();
	LOG_NOTICE(LOADER, "----------------------------------------------------");
	LOG_NOTICE(LOADER, "SCE version info");
	LOG_NOTICE(LOADER, "----------------------------------------------------");
	scev_info.Show();
	LOG_NOTICE(LOADER, "----------------------------------------------------");
	LOG_NOTICE(LOADER, "Control info");
	LOG_NOTICE(LOADER, "----------------------------------------------------");
	for(unsigned int i = 0; i < ctrlinfo_arr.size(); i++)
		ctrlinfo_arr[i].Show();
	LOG_NOTICE(LOADER, "----------------------------------------------------");
	LOG_NOTICE(LOADER, "ELF section headers");
	LOG_NOTICE(LOADER, "----------------------------------------------------");
	for(unsigned int i = 0; i < ((isElf32) ? shdr32_arr.size() : shdr64_arr.size()); i++)
		isElf32 ? shdr32_arr[i].Show() : shdr64_arr[i].Show();
	LOG_NOTICE(LOADER, "----------------------------------------------------");
}

bool SELFDecrypter::DecryptNPDRM(u8 *metadata, u32 metadata_size)
{
	aes_context aes;
	ControlInfo *ctrl = NULL;
	u8 npdrm_key[0x10];
	u8 npdrm_iv[0x10];

	// Parse the control info structures to find the NPDRM control info.
	for(unsigned int i = 0; i < ctrlinfo_arr.size(); i++)
	{
		if (ctrlinfo_arr[i].type == 3)
		{
			ctrl = &ctrlinfo_arr[i];
			break;
		}
	}

	// Check if we have a valid NPDRM control info structure.
	// If not, the data has no NPDRM layer.
	if (!ctrl)
	{
		LOG_WARNING(LOADER, "SELF: No NPDRM control info found!");
		return true;
	}

	u8 klicensee_key[0x10];
	memcpy(klicensee_key, key_v.GetKlicenseeKey(), 0x10);

	// Use klicensee if available.
	if (klicensee_key != NULL)
		memcpy(npdrm_key, klicensee_key, 0x10);

	if (ctrl->npdrm.license == 1)  // Network license.
	{
		LOG_ERROR(LOADER, "SELF: Can't decrypt network NPDRM!");
		return false;
	}
	else if (ctrl->npdrm.license == 2)  // Local license.
	{
		// Try to find a RAP file to get the key.
		if (!GetKeyFromRap(ctrl->npdrm.content_id, npdrm_key))
		{
			LOG_ERROR(LOADER, "SELF: Can't find RAP file for NPDRM decryption!");
			return false;
		}
	}
	else if (ctrl->npdrm.license == 3)  // Free license.
	{
		// Use the NP_KLIC_FREE.
		memcpy(npdrm_key, NP_KLIC_FREE, 0x10);
	}
	else
	{
		LOG_ERROR(LOADER, "SELF: Invalid NPDRM license type!");
		return false;
	}

	// Decrypt our key with NP_KLIC_KEY.
	aes_setkey_dec(&aes, NP_KLIC_KEY, 128);
	aes_crypt_ecb(&aes, AES_DECRYPT, npdrm_key, npdrm_key);

	// IV is empty.
	memset(npdrm_iv, 0, 0x10);

	// Use our final key to decrypt the NPDRM layer.
	aes_setkey_dec(&aes, npdrm_key, 128);
	aes_crypt_cbc(&aes, AES_DECRYPT, metadata_size, npdrm_iv, metadata, metadata);

	return true;
}

bool SELFDecrypter::LoadMetadata()
{
	aes_context aes;
	u32 metadata_info_size = sizeof(meta_info);
	u8 *metadata_info = (u8 *)malloc(metadata_info_size);
	u32 metadata_headers_size = sce_hdr.se_hsize - (sizeof(sce_hdr) + sce_hdr.se_meta + sizeof(meta_info));
	u8 *metadata_headers = (u8 *)malloc(metadata_headers_size);

	// Locate and read the encrypted metadata info.
	self_f.Seek(sce_hdr.se_meta + sizeof(sce_hdr));
	self_f.Read(metadata_info, metadata_info_size);

	// Locate and read the encrypted metadata header and section header.
	self_f.Seek(sce_hdr.se_meta + sizeof(sce_hdr) + metadata_info_size);
	self_f.Read(metadata_headers, metadata_headers_size);

	// Find the right keyset from the key vault.
	SELF_KEY keyset = key_v.FindSelfKey(app_info.self_type, sce_hdr.se_flags, app_info.version);

	// Copy the necessary parameters.
	u8 metadata_key[0x20];
	u8 metadata_iv[0x10];
	memcpy(metadata_key, keyset.erk, 0x20);
	memcpy(metadata_iv, keyset.riv, 0x10);

	// Check DEBUG flag.
	if ((sce_hdr.se_flags & 0x8000) != 0x8000)
	{
		// Decrypt the NPDRM layer.
		if (!DecryptNPDRM(metadata_info, metadata_info_size))
			return false;

		// Decrypt the metadata info.
		aes_setkey_dec(&aes, metadata_key, 256);  // AES-256
		aes_crypt_cbc(&aes, AES_DECRYPT, metadata_info_size, metadata_iv, metadata_info, metadata_info);
	}

	// Load the metadata info.
	meta_info.Load(metadata_info);

	// If the padding is not NULL for the key or iv fields, the metadata info
	// is not properly decrypted.
	if ((meta_info.key_pad[0] != 0x00) ||
		(meta_info.iv_pad[0] != 0x00))
	{
		LOG_ERROR(LOADER, "SELF: Failed to decrypt metadata info!");
		return false;
	}

	// Perform AES-CTR encryption on the metadata headers.
	size_t ctr_nc_off = 0;
	u8 ctr_stream_block[0x10];
	aes_setkey_enc(&aes, meta_info.key, 128);
	aes_crypt_ctr(&aes, metadata_headers_size, &ctr_nc_off, meta_info.iv, ctr_stream_block, metadata_headers, metadata_headers);

	// Load the metadata header.
	meta_hdr.Load(metadata_headers);

	// Load the metadata section headers.
	meta_shdr.clear();
	for (unsigned int i = 0; i < meta_hdr.section_count; i++)
	{
		meta_shdr.emplace_back();
		meta_shdr.back().Load(metadata_headers + sizeof(meta_hdr) + sizeof(MetadataSectionHeader) * i);
	}

	// Copy the decrypted data keys.
	data_keys_length = meta_hdr.key_count * 0x10;
	data_keys = (u8 *) malloc (data_keys_length);
	memcpy(data_keys, metadata_headers + sizeof(meta_hdr) + meta_hdr.section_count * sizeof(MetadataSectionHeader), data_keys_length);

	return true;
}

bool SELFDecrypter::DecryptData()
{
	aes_context aes;

	// Calculate the total data size.
	for (unsigned int i = 0; i < meta_hdr.section_count; i++)
	{
		if (meta_shdr[i].encrypted == 3)
		{
			if ((meta_shdr[i].key_idx <= meta_hdr.key_count - 1) && (meta_shdr[i].iv_idx <= meta_hdr.key_count))
				data_buf_length += meta_shdr[i].data_size;
		}
	}

	// Allocate a buffer to store decrypted data.
	data_buf = (u8*)malloc(data_buf_length);

	// Set initial offset.
	u32 data_buf_offset = 0;

	// Parse the metadata section headers to find the offsets of encrypted data.
	for (unsigned int i = 0; i < meta_hdr.section_count; i++)
	{
		size_t ctr_nc_off = 0;
		u8 ctr_stream_block[0x10];
		u8 data_key[0x10];
		u8 data_iv[0x10];

		// Check if this is an encrypted section.
		if (meta_shdr[i].encrypted == 3)
		{
			// Make sure the key and iv are not out of boundaries.
			if((meta_shdr[i].key_idx <= meta_hdr.key_count - 1) && (meta_shdr[i].iv_idx <= meta_hdr.key_count))
			{
				// Get the key and iv from the previously stored key buffer.
				memcpy(data_key, data_keys + meta_shdr[i].key_idx * 0x10, 0x10);
				memcpy(data_iv, data_keys + meta_shdr[i].iv_idx * 0x10, 0x10);

				// Allocate a buffer to hold the data.
				u8 *buf = (u8 *)malloc(meta_shdr[i].data_size);

				// Seek to the section data offset and read the encrypted data.
				self_f.Seek(meta_shdr[i].data_offset);
				self_f.Read(buf, meta_shdr[i].data_size);

				// Zero out our ctr nonce.
				memset(ctr_stream_block, 0, sizeof(ctr_stream_block));

				// Perform AES-CTR encryption on the data blocks.
				aes_setkey_enc(&aes, data_key, 128);
				aes_crypt_ctr(&aes, meta_shdr[i].data_size, &ctr_nc_off, data_iv, ctr_stream_block, buf, buf);

				// Copy the decrypted data.
				memcpy(data_buf + data_buf_offset, buf, meta_shdr[i].data_size);

				// Advance the buffer's offset.
				data_buf_offset += meta_shdr[i].data_size;

				// Release the temporary buffer.
				free(buf);
			}
		}
	}

	return true;
}

bool SELFDecrypter::MakeElf(const std::string& elf, bool isElf32)
{
	// Create a new ELF file.
	rfile_t e(elf, o_write | o_create | o_trunc);
	if(!e)
	{
		LOG_ERROR(LOADER, "Could not create ELF file! (%s)", elf.c_str());
		return false;
	}

	// Set initial offset.
	u32 data_buf_offset = 0;

	if (isElf32)
	{
		// Write ELF header.
		WriteEhdr(e, elf32_hdr);

		// Write program headers.
		for(u32 i = 0; i < elf32_hdr.e_phnum; ++i)
			WritePhdr(e, phdr32_arr[i]);

		for (unsigned int i = 0; i < meta_hdr.section_count; i++)
		{
			// PHDR type.
			if (meta_shdr[i].type == 2)
			{
				// Seek to the program header data offset and write the data.
				e.seek(phdr32_arr[meta_shdr[i].program_idx].p_offset);
				e.write(data_buf + data_buf_offset, meta_shdr[i].data_size);

				// Advance the data buffer offset by data size.
				data_buf_offset += meta_shdr[i].data_size;
			}
		}

		// Write section headers.
		if(self_hdr.se_shdroff != 0)
		{
			e.seek(elf32_hdr.e_shoff);

			for(u32 i = 0; i < elf32_hdr.e_shnum; ++i)
				WriteShdr(e, shdr32_arr[i]);
		}
	}
	else
	{
		// Write ELF header.
		WriteEhdr(e, elf64_hdr);

		// Write program headers.
		for(u32 i = 0; i < elf64_hdr.e_phnum; ++i)
			WritePhdr(e, phdr64_arr[i]);

		// Write data.
		for (unsigned int i = 0; i < meta_hdr.section_count; i++)
		{
			// PHDR type.
			if (meta_shdr[i].type == 2)
			{
				// Decompress if necessary.
				if (meta_shdr[i].compressed == 2)
				{
					// Allocate a buffer for decompression.
					u8 *decomp_buf = (u8 *)malloc(phdr64_arr[meta_shdr[i].program_idx].p_filesz);

					// Set up memory streams for input/output.
					wxMemoryInputStream decomp_stream_in(data_buf + data_buf_offset, meta_shdr[i].data_size);
					wxMemoryOutputStream decomp_stream_out;

					// Create a Zlib stream, read the data and flush the stream.
					wxZlibInputStream* z_stream = new wxZlibInputStream(decomp_stream_in);
					z_stream->Read(decomp_stream_out);
					delete z_stream;

					// Copy the decompressed result from the stream.
					decomp_stream_out.CopyTo(decomp_buf, phdr64_arr[meta_shdr[i].program_idx].p_filesz);

					// Seek to the program header data offset and write the data.
					e.seek(phdr64_arr[meta_shdr[i].program_idx].p_offset);
					e.write(decomp_buf, phdr64_arr[meta_shdr[i].program_idx].p_filesz);

					// Release the decompression buffer.
					free(decomp_buf);
				}
				else
				{
					// Seek to the program header data offset and write the data.
					e.seek(phdr64_arr[meta_shdr[i].program_idx].p_offset);
					e.write(data_buf + data_buf_offset, meta_shdr[i].data_size);
				}

				// Advance the data buffer offset by data size.
				data_buf_offset += meta_shdr[i].data_size;
			}
		}

		// Write section headers.
		if(self_hdr.se_shdroff != 0)
		{
			e.seek(elf64_hdr.e_shoff);

			for(u32 i = 0; i < elf64_hdr.e_shnum; ++i)
				WriteShdr(e, shdr64_arr[i]);
		}
	}

	return true;
}

bool SELFDecrypter::GetKeyFromRap(u8 *content_id, u8 *npdrm_key)
{
	// Set empty RAP key.
	u8 rap_key[0x10];
	memset(rap_key, 0, 0x10);

	// Try to find a matching RAP file under exdata folder.
	std::string ci_str((const char *)content_id);
	std::string pf_str("00000001");  // TODO: Allow multiple profiles. Use default for now.
	std::string rap_path("dev_hdd0/home/" + pf_str + "/exdata/" + ci_str + ".rap");

	// Check if we have a valid RAP file.
	if (!rIsFile(rap_path))
	{
		LOG_ERROR(LOADER, "This application requires a valid RAP file for decryption!");
		return false;
	}

	// Open the RAP file and read the key.
	rfile_t rap_file(rap_path);

	if (!rap_file)
	{
		LOG_ERROR(LOADER, "Failed to load RAP file!");
		return false;
	}

	LOG_NOTICE(LOADER, "Loading RAP file %s.rap", ci_str);
	rap_file.read(rap_key, 0x10);

	// Convert the RAP key.
	rap_to_rif(rap_key, npdrm_key);

	return true;
}

bool IsSelf(const std::string& path)
{
	vfsLocalFile f(nullptr);

	if(!f.Open(path))
		return false;

	SceHeader hdr;
	hdr.Load(f);

	return hdr.CheckMagic();
}

bool IsSelfElf32(const std::string& path)
{
	vfsLocalFile f(nullptr);

	if(!f.Open(path))
		return false;

	SceHeader hdr;
	SelfHeader sh;
	hdr.Load(f);
	sh.Load(f);
	
	// Locate the class byte and check it.
	u8 elf_class[0x8];
	f.Seek(sh.se_elfoff);
	f.Read(elf_class, 0x8);

	return (elf_class[4] == 1);
}

bool CheckDebugSelf(const std::string& self, const std::string& elf)
{
	// Open the SELF file.
	rfile_t s(self);

	if(!s)
	{
		LOG_ERROR(LOADER, "Could not open SELF file! (%s)", self.c_str());
		return false;
	}

	// Get the key version.
	s.seek(0x08);
	u16 key_version;
	s.read(&key_version, sizeof(key_version));

	// Check for DEBUG version.
	if(swap16(key_version) == 0x8000)
	{
		LOG_WARNING(LOADER, "Debug SELF detected! Removing fake header...");

		// Get the real elf offset.
		s.seek(0x10);
		u64 elf_offset;
		s.read(&elf_offset, sizeof(elf_offset));

		// Start at the real elf offset.
		elf_offset = swap64(elf_offset);
		s.seek(elf_offset);

		// Write the real ELF file back.
		rfile_t e(elf, o_write | o_create | o_trunc);
		if(!e)
		{
			LOG_ERROR(LOADER, "Could not create ELF file! (%s)", elf.c_str());
			return false;
		}

		// Copy the data.
		char buf[2048];
		while (ssize_t size = s.read(buf, 2048))
			e.write(buf, size);

		return true;
	}

	// Leave the file untouched.
	return false;
}

bool DecryptSelf(const std::string& elf, const std::string& self)
{
	// Check for a debug SELF first.
	if (!CheckDebugSelf(self, elf))
	{
		// Set a virtual pointer to the SELF file.
		vfsLocalFile self_vf(nullptr);

		if (!self_vf.Open(self))
			return false;

		// Check the ELF file class (32 or 64 bit).
		bool isElf32 = IsSelfElf32(self);

		// Start the decrypter on this SELF file.
		SELFDecrypter self_dec(self_vf);

		// Load the SELF file headers.
		if (!self_dec.LoadHeaders(isElf32))
		{
			LOG_ERROR(LOADER, "SELF: Failed to load SELF file headers!");
			return false;
		}
		
		// Load and decrypt the SELF file metadata.
		if (!self_dec.LoadMetadata())
		{
			LOG_ERROR(LOADER, "SELF: Failed to load SELF file metadata!");
			return false;
		}
		
		// Decrypt the SELF file data.
		if (!self_dec.DecryptData())
		{
			LOG_ERROR(LOADER, "SELF: Failed to decrypt SELF file data!");
			return false;
		}
		
		// Make a new ELF file from this SELF.
		if (!self_dec.MakeElf(elf, isElf32))
		{
			LOG_ERROR(LOADER, "SELF: Failed to make ELF file from SELF!");
			return false;
		}
	}

	return true;
}
