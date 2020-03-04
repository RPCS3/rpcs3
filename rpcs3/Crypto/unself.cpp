#include "stdafx.h"
#include "aes.h"
#include "sha1.h"
#include "utils.h"
#include "unself.h"
#include "Emu/VFS.h"
#include "Emu/System.h"

#include <algorithm>
#include <zlib.h>

inline u8 Read8(const fs::file& f)
{
	u8 ret;
	f.read(&ret, sizeof(ret));
	return ret;
}

inline u16 Read16(const fs::file& f)
{
	be_t<u16> ret;
	f.read(&ret, sizeof(ret));
	return ret;
}

inline u32 Read32(const fs::file& f)
{
	be_t<u32> ret;
	f.read(&ret, sizeof(ret));
	return ret;
}

inline u64 Read64(const fs::file& f)
{
	be_t<u64> ret;
	f.read(&ret, sizeof(ret));
	return ret;
}

inline u16 Read16LE(const fs::file& f)
{
	u16 ret;
	f.read(&ret, sizeof(ret));
	return ret;
}

inline u32 Read32LE(const fs::file& f)
{
	u32 ret;
	f.read(&ret, sizeof(ret));
	return ret;
}

inline u64 Read64LE(const fs::file& f)
{
	u64 ret;
	f.read(&ret, sizeof(ret));
	return ret;
}

inline void Write8(const fs::file& f, const u8 data)
{
	f.write(&data, sizeof(data));
}

inline void Write16LE(const fs::file& f, const u16 data)
{
	f.write(&data, sizeof(data));
}

inline void Write32LE(const fs::file& f, const u32 data)
{
	f.write(&data, sizeof(data));
}

inline void Write64LE(const fs::file& f, const u64 data)
{
	f.write(&data, sizeof(data));
}

inline void Write16(const fs::file& f, const be_t<u16> data)
{
	f.write(&data, sizeof(data));
}

inline void Write32(const fs::file& f, const be_t<u32> data)
{
	f.write(&data, sizeof(data));
}

inline void Write64(const fs::file& f, const be_t<u64> data)
{
	f.write(&data, sizeof(data));
}

void WriteEhdr(const fs::file& f, Elf64_Ehdr& ehdr)
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

void WritePhdr(const fs::file& f, Elf64_Phdr& phdr)
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

void WriteShdr(const fs::file& f, Elf64_Shdr& shdr)
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

void WriteEhdr(const fs::file& f, Elf32_Ehdr& ehdr)
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

void WritePhdr(const fs::file& f, Elf32_Phdr& phdr)
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

void WriteShdr(const fs::file& f, Elf32_Shdr& shdr)
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


void AppInfo::Load(const fs::file& f)
{
	authid    = Read64(f);
	vendor_id = Read32(f);
	self_type = Read32(f);
	version   = Read64(f);
	padding   = Read64(f);
}

void AppInfo::Show()
{
	self_log.notice("AuthID: 0x%llx", authid);
	self_log.notice("VendorID: 0x%08x", vendor_id);
	self_log.notice("SELF type: 0x%08x", self_type);
	self_log.notice("Version: 0x%llx", version);
}

void SectionInfo::Load(const fs::file& f)
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
	self_log.notice("Offset: 0x%llx", offset);
	self_log.notice("Size: 0x%llx", size);
	self_log.notice("Compressed: 0x%08x", compressed);
	self_log.notice("Unknown1: 0x%08x", unknown1);
	self_log.notice("Unknown2: 0x%08x", unknown2);
	self_log.notice("Encrypted: 0x%08x", encrypted);
}

void SCEVersionInfo::Load(const fs::file& f)
{
	subheader_type = Read32(f);
	present        = Read32(f);
	size           = Read32(f);
	unknown        = Read32(f);
}

void SCEVersionInfo::Show()
{
	self_log.notice("Sub-header type: 0x%08x", subheader_type);
	self_log.notice("Present: 0x%08x", present);
	self_log.notice("Size: 0x%08x", size);
	self_log.notice("Unknown: 0x%08x", unknown);
}

void ControlInfo::Load(const fs::file& f)
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
			f.read(file_digest_30.digest, 20);
			file_digest_30.unknown = Read64(f);
		}
		else if (size == 0x40)
		{
			f.read(file_digest_40.digest1, 20);
			f.read(file_digest_40.digest2, 20);
			file_digest_40.unknown = Read64(f);
		}
	}
	else if (type == 3)
	{
		npdrm.magic = Read32(f);
		npdrm.unknown1 = Read32(f);
		npdrm.license = Read32(f);
		npdrm.type = Read32(f);
		f.read(npdrm.content_id, 48);
		f.read(npdrm.digest, 16);
		f.read(npdrm.invdigest, 16);
		f.read(npdrm.xordigest, 16);
		npdrm.unknown2 = Read64(f);
		npdrm.unknown3 = Read64(f);
	}
}

void ControlInfo::Show()
{
	self_log.notice("Type: 0x%08x", type);
	self_log.notice("Size: 0x%08x", size);
	self_log.notice("Next: 0x%llx", next);

	if (type == 1)
	{
		self_log.notice("Control flag 1: 0x%08x", control_flags.ctrl_flag1);
		self_log.notice("Unknown1: 0x%08x", control_flags.unknown1);
		self_log.notice("Unknown2: 0x%08x", control_flags.unknown2);
		self_log.notice("Unknown3: 0x%08x", control_flags.unknown3);
		self_log.notice("Unknown4: 0x%08x", control_flags.unknown4);
		self_log.notice("Unknown5: 0x%08x", control_flags.unknown5);
		self_log.notice("Unknown6: 0x%08x", control_flags.unknown6);
		self_log.notice("Unknown7: 0x%08x", control_flags.unknown7);
	}
	else if (type == 2)
	{
		if (size == 0x30)
		{
			std::string digest_str;
			for (int i = 0; i < 20; i++)
				digest_str += fmt::format("%02x", file_digest_30.digest[i]);

			self_log.notice("Digest: %s", digest_str.c_str());
			self_log.notice("Unknown: 0x%llx", file_digest_30.unknown);
		}
		else if (size == 0x40)
		{
			std::string digest_str1;
			std::string digest_str2;
			for (int i = 0; i < 20; i++)
			{
				digest_str1 += fmt::format("%02x", file_digest_40.digest1[i]);
				digest_str2 += fmt::format("%02x", file_digest_40.digest2[i]);
			}

			self_log.notice("Digest1: %s", digest_str1.c_str());
			self_log.notice("Digest2: %s", digest_str2.c_str());
			self_log.notice("Unknown: 0x%llx", file_digest_40.unknown);
		}
	}
	else if (type == 3)
	{
		std::string contentid_str;
		std::string digest_str;
		std::string invdigest_str;
		std::string xordigest_str;
		for (int i = 0; i < 48; i++)
			contentid_str += fmt::format("%02x", npdrm.content_id[i]);
		for (int i = 0; i < 16; i++)
		{
			digest_str += fmt::format("%02x", npdrm.digest[i]);
			invdigest_str += fmt::format("%02x", npdrm.invdigest[i]);
			xordigest_str += fmt::format("%02x", npdrm.xordigest[i]);
		}

		self_log.notice("Magic: 0x%08x", npdrm.magic);
		self_log.notice("Unknown1: 0x%08x", npdrm.unknown1);
		self_log.notice("License: 0x%08x", npdrm.license);
		self_log.notice("Type: 0x%08x", npdrm.type);
		self_log.notice("ContentID: %s", contentid_str.c_str());
		self_log.notice("Digest: %s", digest_str.c_str());
		self_log.notice("Inverse digest: %s", invdigest_str.c_str());
		self_log.notice("XOR digest: %s", xordigest_str.c_str());
		self_log.notice("Unknown2: 0x%llx", npdrm.unknown2);
		self_log.notice("Unknown3: 0x%llx", npdrm.unknown3);
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
		key_str += fmt::format("%02x", key[i]);
		key_pad_str += fmt::format("%02x", key_pad[i]);
		iv_str += fmt::format("%02x", iv[i]);
		iv_pad_str += fmt::format("%02x", iv_pad[i]);
	}

	self_log.notice("Key: %s", key_str.c_str());
	self_log.notice("Key pad: %s", key_pad_str.c_str());
	self_log.notice("IV: %s", iv_str.c_str());
	self_log.notice("IV pad: %s", iv_pad_str.c_str());
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
	self_log.notice("Signature input length: 0x%llx", signature_input_length);
	self_log.notice("Unknown1: 0x%08x", unknown1);
	self_log.notice("Section count: 0x%08x", section_count);
	self_log.notice("Key count: 0x%08x", key_count);
	self_log.notice("Optional header size: 0x%08x", opt_header_size);
	self_log.notice("Unknown2: 0x%08x", unknown2);
	self_log.notice("Unknown3: 0x%08x", unknown3);
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
	self_log.notice("Data offset: 0x%llx", data_offset);
	self_log.notice("Data size: 0x%llx", data_size);
	self_log.notice("Type: 0x%08x", type);
	self_log.notice("Program index: 0x%08x", program_idx);
	self_log.notice("Hashed: 0x%08x", hashed);
	self_log.notice("SHA1 index: 0x%08x", sha1_idx);
	self_log.notice("Encrypted: 0x%08x", encrypted);
	self_log.notice("Key index: 0x%08x", key_idx);
	self_log.notice("IV index: 0x%08x", iv_idx);
	self_log.notice("Compressed: 0x%08x", compressed);
}

void SectionHash::Load(const fs::file& f)
{
	f.read(sha1, 20);
	f.read(padding, 12);
	f.read(hmac_key, 64);
}

void CapabilitiesInfo::Load(const fs::file& f)
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

void Signature::Load(const fs::file& f)
{
	f.read(r, 21);
	f.read(s, 21);
	f.read(padding, 6);
}

void SelfSection::Load(const fs::file& f)
{
	*data = Read32(f);
	size = Read64(f);
	offset = Read64(f);
}

void Elf32_Ehdr::Load(const fs::file& f)
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

void Elf32_Shdr::Load(const fs::file& f)
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

void Elf32_Shdr::LoadLE(const fs::file& f)
{
	f.read(this, sizeof(*this));
}

void Elf32_Phdr::Load(const fs::file& f)
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

void Elf32_Phdr::LoadLE(const fs::file& f)
{
	f.read(this, sizeof(*this));
}

void Elf64_Ehdr::Load(const fs::file& f)
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

void Elf64_Shdr::Load(const fs::file& f)
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

void Elf64_Phdr::Load(const fs::file& f)
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

void SceHeader::Load(const fs::file& f)
{
	se_magic = Read32(f);
	se_hver = Read32(f);
	se_flags = Read16(f);
	se_type = Read16(f);
	se_meta = Read32(f);
	se_hsize = Read64(f);
	se_esize = Read64(f);
}

void SelfHeader::Load(const fs::file& f)
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

SCEDecrypter::SCEDecrypter(const fs::file& s)
	: sce_f(s)
	, data_buf_length(0)
{
}

bool SCEDecrypter::LoadHeaders()
{
	// Read SCE header.
	sce_f.seek(0);
	sce_hdr.Load(sce_f);

	// Check SCE magic.
	if (!sce_hdr.CheckMagic())
	{
		self_log.error("SELF: Not a SELF file!");
		return false;
	}

	return true;
}

bool SCEDecrypter::LoadMetadata(const u8 erk[32], const u8 riv[16])
{
	aes_context aes;
	const auto metadata_info = std::make_unique<u8[]>(sizeof(meta_info));
	const auto metadata_headers_size = sce_hdr.se_hsize - (sizeof(sce_hdr) + sce_hdr.se_meta + sizeof(meta_info));
	const auto metadata_headers = std::make_unique<u8[]>(metadata_headers_size);

	// Locate and read the encrypted metadata info.
	sce_f.seek(sce_hdr.se_meta + sizeof(sce_hdr));
	sce_f.read(metadata_info.get(), sizeof(meta_info));

	// Locate and read the encrypted metadata header and section header.
	sce_f.seek(sce_hdr.se_meta + sizeof(sce_hdr) + sizeof(meta_info));
	sce_f.read(metadata_headers.get(), metadata_headers_size);

	// Copy the necessary parameters.
	u8 metadata_key[0x20];
	u8 metadata_iv[0x10];
	memcpy(metadata_key, erk, 0x20);
	memcpy(metadata_iv, riv, 0x10);

	// Check DEBUG flag.
	if ((sce_hdr.se_flags & 0x8000) != 0x8000)
	{
		// Decrypt the metadata info.
		aes_setkey_dec(&aes, metadata_key, 256);  // AES-256
		aes_crypt_cbc(&aes, AES_DECRYPT, sizeof(meta_info), metadata_iv, metadata_info.get(), metadata_info.get());
	}

	// Load the metadata info.
	meta_info.Load(metadata_info.get());

	// If the padding is not NULL for the key or iv fields, the metadata info
	// is not properly decrypted.
	if ((meta_info.key_pad[0] != 0x00) ||
		(meta_info.iv_pad[0] != 0x00))
	{
		self_log.error("SELF: Failed to decrypt metadata info!");
		return false;
	}

	// Perform AES-CTR encryption on the metadata headers.
	size_t ctr_nc_off = 0;
	u8 ctr_stream_block[0x10];
	aes_setkey_enc(&aes, meta_info.key, 128);
	aes_crypt_ctr(&aes, metadata_headers_size, &ctr_nc_off, meta_info.iv, ctr_stream_block, metadata_headers.get(), metadata_headers.get());

	// Load the metadata header.
	meta_hdr.Load(metadata_headers.get());

	// Load the metadata section headers.
	meta_shdr.clear();
	for (unsigned int i = 0; i < meta_hdr.section_count; i++)
	{
		meta_shdr.emplace_back();
		meta_shdr.back().Load(metadata_headers.get() + sizeof(meta_hdr) + sizeof(MetadataSectionHeader) * i);
	}

	// Copy the decrypted data keys.
	data_keys_length = meta_hdr.key_count * 0x10;
	data_keys = std::make_unique<u8[]>(data_keys_length);
	memcpy(data_keys.get(), metadata_headers.get() + sizeof(meta_hdr) + meta_hdr.section_count * sizeof(MetadataSectionHeader), data_keys_length);

	return true;
}

bool SCEDecrypter::DecryptData()
{
	aes_context aes;

	// Calculate the total data size.
	for (unsigned int i = 0; i < meta_hdr.section_count; i++)
	{
		data_buf_length += ::narrow<u32>(meta_shdr[i].data_size, HERE);
	}

	// Allocate a buffer to store decrypted data.
	data_buf = std::make_unique<u8[]>(data_buf_length);

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
			if ((meta_shdr[i].key_idx <= meta_hdr.key_count - 1) && (meta_shdr[i].iv_idx <= meta_hdr.key_count))
			{
				// Get the key and iv from the previously stored key buffer.
				memcpy(data_key, data_keys.get() + meta_shdr[i].key_idx * 0x10, 0x10);
				memcpy(data_iv, data_keys.get() + meta_shdr[i].iv_idx * 0x10, 0x10);

				// Allocate a buffer to hold the data.
				auto buf = std::make_unique<u8[]>(meta_shdr[i].data_size);

				// Seek to the section data offset and read the encrypted data.
				sce_f.seek(meta_shdr[i].data_offset);
				sce_f.read(buf.get(), meta_shdr[i].data_size);

				// Zero out our ctr nonce.
				memset(ctr_stream_block, 0, sizeof(ctr_stream_block));

				// Perform AES-CTR encryption on the data blocks.
				aes_setkey_enc(&aes, data_key, 128);
				aes_crypt_ctr(&aes, meta_shdr[i].data_size, &ctr_nc_off, data_iv, ctr_stream_block, buf.get(), buf.get());

				// Copy the decrypted data.
				memcpy(data_buf.get() + data_buf_offset, buf.get(), meta_shdr[i].data_size);
			}
		}
		else
		{
			auto buf = std::make_unique<u8[]>(meta_shdr[i].data_size);
			sce_f.seek(meta_shdr[i].data_offset);
			sce_f.read(buf.get(), meta_shdr[i].data_size);
			memcpy(data_buf.get() + data_buf_offset, buf.get(), meta_shdr[i].data_size);
		}

		// Advance the buffer's offset.
		data_buf_offset += ::narrow<u32>(meta_shdr[i].data_size, HERE);
	}

	return true;
}

// Each section gets put into its own file.
std::vector<fs::file> SCEDecrypter::MakeFile()
{
	std::vector<fs::file> vec;

	// Set initial offset.
	u32 data_buf_offset = 0;

	// Write data.
	for (unsigned int i = 0; i < meta_hdr.section_count; i++)
	{
		fs::file out_f = fs::make_stream<std::vector<u8>>();

		bool isValid = true;

		// Decompress if necessary.
		if (meta_shdr[i].compressed == 2)
		{
			const size_t BUFSIZE = 32 * 1024;
			u8 tempbuf[BUFSIZE];
			z_stream strm;
			strm.zalloc = Z_NULL;
			strm.zfree = Z_NULL;
			strm.opaque = Z_NULL;
			strm.avail_in = ::narrow<uInt>(meta_shdr[i].data_size, HERE);
			strm.avail_out = BUFSIZE;
			strm.next_in = data_buf.get()+data_buf_offset;
			strm.next_out = tempbuf;
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
			int ret = inflateInit(&strm);
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif
			while (strm.avail_in)
			{
				ret = inflate(&strm, Z_NO_FLUSH);
				if (ret == Z_STREAM_END)
					break;
				if (ret != Z_OK)
					isValid = false;

				if (!strm.avail_out) {
					out_f.write(tempbuf, BUFSIZE);
					strm.next_out = tempbuf;
					strm.avail_out = BUFSIZE;
				}
				else
					break;
			}

			int inflate_res = Z_OK;
			inflate_res = inflate(&strm, Z_FINISH);

			if (inflate_res != Z_STREAM_END)
				isValid = false;

			out_f.write(tempbuf, BUFSIZE - strm.avail_out);
			inflateEnd(&strm);
		}
		else
		{
			// Write the data.
			out_f.write(data_buf.get()+data_buf_offset, meta_shdr[i].data_size);
		}

		// Advance the data buffer offset by data size.
		data_buf_offset += ::narrow<u32>(meta_shdr[i].data_size, HERE);

		if (out_f.pos() != out_f.size())
			fmt::throw_exception("MakeELF written bytes (%llu) does not equal buffer size (%llu).", out_f.pos(), out_f.size());

		if (isValid) vec.push_back(std::move(out_f));
	}

	return vec;
}

SELFDecrypter::SELFDecrypter(const fs::file& s)
	: self_f(s)
	, key_v()
{
}

bool SELFDecrypter::LoadHeaders(bool isElf32, SelfAdditionalInfo* out_info)
{
	// Read SCE header.
	self_f.seek(0);
	sce_hdr.Load(self_f);

	if (out_info)
	{
		out_info->valid = false;
	}

	// Check SCE magic.
	if (!sce_hdr.CheckMagic())
	{
		self_log.error("SELF: Not a SELF file!");
		return false;
	}

	// Read SELF header.
	self_hdr.Load(self_f);

	// Read the APP INFO.
	self_f.seek(self_hdr.se_appinfooff);
	app_info.Load(self_f);

	if (out_info)
	{
		out_info->app_info = app_info;
	}

	// Read ELF header.
	self_f.seek(self_hdr.se_elfoff);

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
			self_log.error("SELF: ELF program header offset is null!");
			return false;
		}
		self_f.seek(self_hdr.se_phdroff);
		for(u32 i = 0; i < elf32_hdr.e_phnum; ++i)
		{
			phdr32_arr.emplace_back();
			phdr32_arr.back().Load(self_f);
		}
	}
	else
	{
		phdr64_arr.clear();

		if (elf64_hdr.e_phoff == 0 && elf64_hdr.e_phnum)
		{
			self_log.error("SELF: ELF program header offset is null!");
			return false;
		}

		self_f.seek(self_hdr.se_phdroff);

		for (u32 i = 0; i < elf64_hdr.e_phnum; ++i)
		{
			phdr64_arr.emplace_back();
			phdr64_arr.back().Load(self_f);
		}
	}


	// Read section info.
	secinfo_arr.clear();
	self_f.seek(self_hdr.se_secinfoff);

	for(u32 i = 0; i < ((isElf32) ? elf32_hdr.e_phnum : elf64_hdr.e_phnum); ++i)
	{
		secinfo_arr.emplace_back();
		secinfo_arr.back().Load(self_f);
	}

	// Read SCE version info.
	self_f.seek(self_hdr.se_sceveroff);
	scev_info.Load(self_f);

	// Read control info.
	ctrlinfo_arr.clear();
	self_f.seek(self_hdr.se_controloff);

	for (u64 i = 0; i < self_hdr.se_controlsize;)
	{
		ctrlinfo_arr.emplace_back();
		ControlInfo &cinfo = ctrlinfo_arr.back();
		cinfo.Load(self_f);
		i += cinfo.size;
	}

	if (out_info)
	{
		out_info->ctrl_info = ctrlinfo_arr;
	}

	// Read ELF section headers.
	if (isElf32)
	{
		shdr32_arr.clear();

		if (elf32_hdr.e_shoff == 0 && elf32_hdr.e_shnum)
		{
			self_log.warning("SELF: ELF section header offset is null!");
			return true;
		}

		self_f.seek(self_hdr.se_shdroff);

		for(u32 i = 0; i < elf32_hdr.e_shnum; ++i)
		{
			shdr32_arr.emplace_back();
			shdr32_arr.back().Load(self_f);
		}
	}
	else
	{
		shdr64_arr.clear();
		if (elf64_hdr.e_shoff == 0 && elf64_hdr.e_shnum)
		{
			self_log.warning("SELF: ELF section header offset is null!");
			return true;
		}

		self_f.seek(self_hdr.se_shdroff);

		for(u32 i = 0; i < elf64_hdr.e_shnum; ++i)
		{
			shdr64_arr.emplace_back();
			shdr64_arr.back().Load(self_f);
		}
	}

	if (out_info)
	{
		out_info->valid = true;
	}

	return true;
}

void SELFDecrypter::ShowHeaders(bool isElf32)
{
	self_log.notice("SCE header");
	self_log.notice("----------------------------------------------------");
	sce_hdr.Show();
	self_log.notice("----------------------------------------------------");
	self_log.notice("SELF header");
	self_log.notice("----------------------------------------------------");
	self_hdr.Show();
	self_log.notice("----------------------------------------------------");
	self_log.notice("APP INFO");
	self_log.notice("----------------------------------------------------");
	app_info.Show();
	self_log.notice("----------------------------------------------------");
	self_log.notice("ELF header");
	self_log.notice("----------------------------------------------------");
	isElf32 ? elf32_hdr.Show() : elf64_hdr.Show();
	self_log.notice("----------------------------------------------------");
	self_log.notice("ELF program headers");
	self_log.notice("----------------------------------------------------");
	for(unsigned int i = 0; i < ((isElf32) ? phdr32_arr.size() : phdr64_arr.size()); i++)
		isElf32 ? phdr32_arr[i].Show() : phdr64_arr[i].Show();
	self_log.notice("----------------------------------------------------");
	self_log.notice("Section info");
	self_log.notice("----------------------------------------------------");
	for(unsigned int i = 0; i < secinfo_arr.size(); i++)
		secinfo_arr[i].Show();
	self_log.notice("----------------------------------------------------");
	self_log.notice("SCE version info");
	self_log.notice("----------------------------------------------------");
	scev_info.Show();
	self_log.notice("----------------------------------------------------");
	self_log.notice("Control info");
	self_log.notice("----------------------------------------------------");
	for(unsigned int i = 0; i < ctrlinfo_arr.size(); i++)
		ctrlinfo_arr[i].Show();
	self_log.notice("----------------------------------------------------");
	self_log.notice("ELF section headers");
	self_log.notice("----------------------------------------------------");
	for(unsigned int i = 0; i < ((isElf32) ? shdr32_arr.size() : shdr64_arr.size()); i++)
		isElf32 ? shdr32_arr[i].Show() : shdr64_arr[i].Show();
	self_log.notice("----------------------------------------------------");
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
		self_log.trace("SELF: No NPDRM control info found!");
		return true;
	}

	if (ctrl->npdrm.license == 1)  // Network license.
	{
		self_log.error("SELF: Can't decrypt network NPDRM!");
		return false;
	}
	else if (ctrl->npdrm.license == 2)  // Local license.
	{
		// Try to find a RAP file to get the key.
		if (!GetKeyFromRap(ctrl->npdrm.content_id, npdrm_key))
		{
			self_log.error("SELF: Can't find RAP file for NPDRM decryption!");
			return false;
		}
	}
	else if (ctrl->npdrm.license == 3)  // Free license.
	{
		// Use klicensee if available.
		if (key_v.GetKlicenseeKey() != nullptr)
			memcpy(npdrm_key, key_v.GetKlicenseeKey(), 0x10);
		else
			memcpy(npdrm_key, NP_KLIC_FREE, 0x10);
	}
	else
	{
		self_log.error("SELF: Invalid NPDRM license type!");
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

bool SELFDecrypter::LoadMetadata(u8* klic_key)
{
	aes_context aes;
	const auto metadata_info = std::make_unique<u8[]>(sizeof(meta_info));
	const auto metadata_headers_size = sce_hdr.se_hsize - (sizeof(sce_hdr) + sce_hdr.se_meta + sizeof(meta_info));
	const auto metadata_headers = std::make_unique<u8[]>(metadata_headers_size);

	// Locate and read the encrypted metadata info.
	self_f.seek(sce_hdr.se_meta + sizeof(sce_hdr));
	self_f.read(metadata_info.get(), sizeof(meta_info));

	// Locate and read the encrypted metadata header and section header.
	self_f.seek(sce_hdr.se_meta + sizeof(sce_hdr) + sizeof(meta_info));
	self_f.read(metadata_headers.get(), metadata_headers_size);

	// Find the right keyset from the key vault.
	SELF_KEY keyset = key_v.FindSelfKey(app_info.self_type, sce_hdr.se_flags, app_info.version);

	// Set klic if given
	if (klic_key)
		key_v.SetKlicenseeKey(klic_key);

	// Copy the necessary parameters.
	u8 metadata_key[0x20];
	u8 metadata_iv[0x10];
	memcpy(metadata_key, keyset.erk, 0x20);
	memcpy(metadata_iv, keyset.riv, 0x10);

	// Check DEBUG flag.
	if ((sce_hdr.se_flags & 0x8000) != 0x8000)
	{
		// Decrypt the NPDRM layer.
		if (!DecryptNPDRM(metadata_info.get(), sizeof(meta_info)))
			return false;

		// Decrypt the metadata info.
		aes_setkey_dec(&aes, metadata_key, 256);  // AES-256
		aes_crypt_cbc(&aes, AES_DECRYPT, sizeof(meta_info), metadata_iv, metadata_info.get(), metadata_info.get());
	}

	// Load the metadata info.
	meta_info.Load(metadata_info.get());

	// If the padding is not NULL for the key or iv fields, the metadata info
	// is not properly decrypted.
	if ((meta_info.key_pad[0] != 0x00) ||
		(meta_info.iv_pad[0] != 0x00))
	{
		self_log.error("SELF: Failed to decrypt metadata info!");
		return false;
	}

	// Perform AES-CTR encryption on the metadata headers.
	size_t ctr_nc_off = 0;
	u8 ctr_stream_block[0x10];
	aes_setkey_enc(&aes, meta_info.key, 128);
	aes_crypt_ctr(&aes, metadata_headers_size, &ctr_nc_off, meta_info.iv, ctr_stream_block, metadata_headers.get(), metadata_headers.get());

	// Load the metadata header.
	meta_hdr.Load(metadata_headers.get());

	// Load the metadata section headers.
	meta_shdr.clear();
	for (unsigned int i = 0; i < meta_hdr.section_count; i++)
	{
		meta_shdr.emplace_back();
		meta_shdr.back().Load(metadata_headers.get() + sizeof(meta_hdr) + sizeof(MetadataSectionHeader) * i);
	}

	// Copy the decrypted data keys.
	data_keys_length = meta_hdr.key_count * 0x10;
	data_keys = std::make_unique<u8[]>(data_keys_length);
	memcpy(data_keys.get(), metadata_headers.get() + sizeof(meta_hdr) + meta_hdr.section_count * sizeof(MetadataSectionHeader), data_keys_length);

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
				data_buf_length += ::narrow<u32>(meta_shdr[i].data_size, HERE);
		}
	}

	// Allocate a buffer to store decrypted data.
	data_buf = std::make_unique<u8[]>(data_buf_length);

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
				memcpy(data_key, data_keys.get() + meta_shdr[i].key_idx * 0x10, 0x10);
				memcpy(data_iv, data_keys.get() + meta_shdr[i].iv_idx * 0x10, 0x10);

				// Allocate a buffer to hold the data.
				auto buf = std::make_unique<u8[]>(meta_shdr[i].data_size);

				// Seek to the section data offset and read the encrypted data.
				self_f.seek(meta_shdr[i].data_offset);
				self_f.read(buf.get(), meta_shdr[i].data_size);

				// Zero out our ctr nonce.
				memset(ctr_stream_block, 0, sizeof(ctr_stream_block));

				// Perform AES-CTR encryption on the data blocks.
				aes_setkey_enc(&aes, data_key, 128);
				aes_crypt_ctr(&aes, meta_shdr[i].data_size, &ctr_nc_off, data_iv, ctr_stream_block, buf.get(), buf.get());

				// Copy the decrypted data.
				memcpy(data_buf.get() + data_buf_offset, buf.get(), meta_shdr[i].data_size);

				// Advance the buffer's offset.
				data_buf_offset += ::narrow<u32>(meta_shdr[i].data_size, HERE);
			}
		}
	}

	return true;
}

fs::file SELFDecrypter::MakeElf(bool isElf32)
{
	// Create a new ELF file.
	fs::file e = fs::make_stream<std::vector<u8>>();

	if (isElf32)
	{
		WriteElf(e, elf32_hdr, shdr32_arr, phdr32_arr);
	}
	else
	{
		WriteElf(e, elf64_hdr, shdr64_arr, phdr64_arr);
	}

	return e;
}

bool SELFDecrypter::GetKeyFromRap(u8* content_id, u8* npdrm_key)
{
	// Set empty RAP key.
	u8 rap_key[0x10];
	memset(rap_key, 0, 0x10);

	// Try to find a matching RAP file under exdata folder.
	const std::string ci_str = reinterpret_cast<const char*>(content_id);
	const std::string rap_path = Emulator::GetHddDir() + "/home/" + Emu.GetUsr() + "/exdata/" + ci_str + ".rap";

	// Open the RAP file and read the key.
	const fs::file rap_file(rap_path);

	if (!rap_file)
	{
		self_log.fatal("Failed to locate the game license file: %s."
				  "\nEnsure the .rap license file is placed in the dev_hdd0/home/00000001/exdata folder with a lowercase file extension."
				  "\nIf you need assistance on dumping the license file from your PS3, read our quickstart guide: https://rpcs3.net/quickstart", rap_path);
		return false;
	}

	self_log.notice("Loading RAP file %s.rap", ci_str);
	rap_file.read(rap_key, 0x10);

	// Convert the RAP key.
	rap_to_rif(rap_key, npdrm_key);

	return true;
}

static bool IsSelfElf32(const fs::file& f)
{
	if (!f) return false;

	f.seek(0);

	SceHeader hdr;
	SelfHeader sh;
	hdr.Load(f);
	sh.Load(f);

	// Locate the class byte and check it.
	u8 elf_class[0x8];

	f.seek(sh.se_elfoff);
	f.read(elf_class, 0x8);

	return (elf_class[4] == 1);
}

static bool CheckDebugSelf(fs::file& s)
{
	if (s.size() < 0x18)
	{
		return false;
	}

	// Get the key version.
	s.seek(0x08);

	const u16 key_version = s.read<le_t<u16>>();

	// Check for DEBUG version.
	if (key_version == 0x80 || key_version == 0xc0)
	{
		self_log.warning("Debug SELF detected! Removing fake header...");

		// Get the real elf offset.
		s.seek(0x10);

		// Start at the real elf offset.
		s.seek(key_version == 0x80 ? +s.read<be_t<u64>>() : +s.read<le_t<u64>>());

		// Write the real ELF file back.
		fs::file e = fs::make_stream<std::vector<u8>>();

		// Copy the data.
		char buf[2048];
		while (u64 size = s.read(buf, 2048))
		{
			e.write(buf, size);
		}

		s = std::move(e);
		return true;
	}

	// Leave the file untouched.
	return false;
}

fs::file decrypt_self(fs::file elf_or_self, u8* klic_key, SelfAdditionalInfo* out_info)
{
	if (out_info)
	{
		out_info->valid = false;
	}

	if (!elf_or_self)
	{
		return fs::file{};
	}

	elf_or_self.seek(0);

	// Check SELF header first. Check for a debug SELF.
	if (elf_or_self.size() >= 4 && elf_or_self.read<u32>() == "SCE\0"_u32 && !CheckDebugSelf(elf_or_self))
	{
		// Check the ELF file class (32 or 64 bit).
		bool isElf32 = IsSelfElf32(elf_or_self);

		// Start the decrypter on this SELF file.
		SELFDecrypter self_dec(elf_or_self);

		// Load the SELF file headers.
		if (!self_dec.LoadHeaders(isElf32, out_info))
		{
			self_log.error("SELF: Failed to load SELF file headers!");
			return fs::file{};
		}

		// Load and decrypt the SELF file metadata.
		if (!self_dec.LoadMetadata(klic_key))
		{
			self_log.error("SELF: Failed to load SELF file metadata!");
			return fs::file{};
		}

		// Decrypt the SELF file data.
		if (!self_dec.DecryptData())
		{
			self_log.error("SELF: Failed to decrypt SELF file data!");
			return fs::file{};
		}

		// Make a new ELF file from this SELF.
		return self_dec.MakeElf(isElf32);
	}

	return elf_or_self;
}

bool verify_npdrm_self_headers(const fs::file& self, u8* klic_key)
{
	if (!self)
		return false;

	self.seek(0);

	if (self.size() >= 4 && self.read<u32>() == "SCE\0"_u32)
	{
		// Check the ELF file class (32 or 64 bit).
		bool isElf32 = IsSelfElf32(self);

		// Start the decrypter on this SELF file.
		SELFDecrypter self_dec(self);

		// Load the SELF file headers.
		if (!self_dec.LoadHeaders(isElf32))
		{
			self_log.error("SELF: Failed to load SELF file headers!");
			return false;
		}

		// Load and decrypt the SELF file metadata.
		if (!self_dec.LoadMetadata(klic_key))
		{
			self_log.error("SELF: Failed to load SELF file metadata!");
			return false;
		}
	}
	return true;
}

std::array<u8, 0x10> get_default_self_klic()
{
	std::array<u8, 0x10> key;
	std::copy(std::begin(NP_KLIC_FREE), std::end(NP_KLIC_FREE), std::begin(key));
	return key;
}
