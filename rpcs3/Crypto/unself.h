#pragma once

#include "key_vault.h"
#include "zlib.h"

#include "Utilities/types.h"
#include "Utilities/File.h"
#include "util/logs.hpp"

LOG_CHANNEL(self_log, "SELF");

struct AppInfo
{
	u64 authid;
	u32 vendor_id;
	u32 self_type;
	u64 version;
	u64 padding;

	void Load(const fs::file& f);
	void Show();
};

struct SectionInfo
{
	u64 offset;
	u64 size;
	u32 compressed;
	u32 unknown1;
	u32 unknown2;
	u32 encrypted;

	void Load(const fs::file& f);
	void Show();
};

struct SCEVersionInfo
{
	u32 subheader_type;
	u32 present;
	u32 size;
	u32 unknown;

	void Load(const fs::file& f);
	void Show();
};

struct ControlInfo
{
	u32 type;
	u32 size;
	u64 next;

	union
	{
		// type 1 0x30 bytes
		struct
		{
			u32 ctrl_flag1;
			u32 unknown1;
			u32 unknown2;
			u32 unknown3;
			u32 unknown4;
			u32 unknown5;
			u32 unknown6;
			u32 unknown7;

		} control_flags;

		// type 2 0x30 bytes
		struct
		{
			u8 digest[20];
			u64 unknown;

		} file_digest_30;

		// type 2 0x40 bytes
		struct
		{
			u8 digest1[20];
			u8 digest2[20];
			u64 unknown;

		} file_digest_40;

		// type 3 0x90 bytes
		struct
		{
			u32 magic;
			u32 unknown1;
			u32 license;
			u32 type;
			u8 content_id[48];
			u8 digest[16];
			u8 invdigest[16];
			u8 xordigest[16];
			u64 unknown2;
			u64 unknown3;

		} npdrm;
	};

	void Load(const fs::file& f);
	void Show();
};


struct MetadataInfo
{
	u8 key[0x10];
	u8 key_pad[0x10];
	u8 iv[0x10];
	u8 iv_pad[0x10];

	void Load(u8* in);
	void Show();
};

struct MetadataHeader
{
	u64 signature_input_length;
	u32 unknown1;
	u32 section_count;
	u32 key_count;
	u32 opt_header_size;
	u32 unknown2;
	u32 unknown3;

	void Load(u8* in);
	void Show();
};

struct MetadataSectionHeader
{
	u64 data_offset;
	u64 data_size;
	u32 type;
	u32 program_idx;
	u32 hashed;
	u32 sha1_idx;
	u32 encrypted;
	u32 key_idx;
	u32 iv_idx;
	u32 compressed;

	void Load(u8* in);
	void Show();
};

struct SectionHash
{
	u8 sha1[20];
	u8 padding[12];
	u8 hmac_key[64];

	void Load(const fs::file& f);
};

struct CapabilitiesInfo
{
	u32 type;
	u32 capabilities_size;
	u32 next;
	u32 unknown1;
	u64 unknown2;
	u64 unknown3;
	u64 flags;
	u32 unknown4;
	u32 unknown5;

	void Load(const fs::file& f);
};

struct Signature
{
	u8 r[21];
	u8 s[21];
	u8 padding[6];

	void Load(const fs::file& f);
};

struct SelfSection
{
	u8 *data;
	u64 size;
	u64 offset;

	void Load(const fs::file& f);
};

struct Elf32_Ehdr
{
	u32 e_magic;
	u8 e_class;
	u8 e_data;
	u8 e_curver;
	u8 e_os_abi;
	u64 e_abi_ver;
	u16 e_type;
	u16 e_machine;
	u32 e_version;
	u32 e_entry;
	u32 e_phoff;
	u32 e_shoff;
	u32 e_flags;
	u16 e_ehsize;
	u16 e_phentsize;
	u16 e_phnum;
	u16 e_shentsize;
	u16 e_shnum;
	u16 e_shstrndx;

	void Load(const fs::file& f);
	void Show() {}
	bool IsLittleEndian() const { return e_data == 1; }
	bool CheckMagic() const { return e_magic == 0x7F454C46; }
	u32 GetEntry() const { return e_entry; }
};

struct Elf32_Shdr
{
	u32 sh_name;
	u32 sh_type;
	u32 sh_flags;
	u32 sh_addr;
	u32 sh_offset;
	u32 sh_size;
	u32 sh_link;
	u32 sh_info;
	u32 sh_addralign;
	u32 sh_entsize;

	void Load(const fs::file& f);
	void LoadLE(const fs::file& f);
	void Show() {}
};

struct Elf32_Phdr
{
	u32 p_type;
	u32 p_offset;
	u32 p_vaddr;
	u32 p_paddr;
	u32 p_filesz;
	u32 p_memsz;
	u32 p_flags;
	u32 p_align;

	void Load(const fs::file& f);
	void LoadLE(const fs::file& f);
	void Show() {}
};

struct Elf64_Ehdr
{
	u32 e_magic;
	u8 e_class;
	u8 e_data;
	u8 e_curver;
	u8 e_os_abi;
	u64 e_abi_ver;
	u16 e_type;
	u16 e_machine;
	u32 e_version;
	u64 e_entry;
	u64 e_phoff;
	u64 e_shoff;
	u32 e_flags;
	u16 e_ehsize;
	u16 e_phentsize;
	u16 e_phnum;
	u16 e_shentsize;
	u16 e_shnum;
	u16 e_shstrndx;

	void Load(const fs::file& f);
	void Show() {}
	bool CheckMagic() const { return e_magic == 0x7F454C46; }
	u64 GetEntry() const { return e_entry; }
};

struct Elf64_Shdr
{
	u32 sh_name;
	u32 sh_type;
	u64 sh_flags;
	u64 sh_addr;
	u64 sh_offset;
	u64 sh_size;
	u32 sh_link;
	u32 sh_info;
	u64 sh_addralign;
	u64 sh_entsize;

	void Load(const fs::file& f);
	void Show(){}
};

struct Elf64_Phdr
{
	u32 p_type;
	u32 p_flags;
	u64 p_offset;
	u64 p_vaddr;
	u64 p_paddr;
	u64 p_filesz;
	u64 p_memsz;
	u64 p_align;

	void Load(const fs::file& f);
	void Show(){}
};

struct SceHeader
{
	u32 se_magic;
	u32 se_hver;
	u16 se_flags;
	u16 se_type;
	u32 se_meta;
	u64 se_hsize;
	u64 se_esize;

	void Load(const fs::file& f);
	void Show(){}
	bool CheckMagic() const { return se_magic == 0x53434500; }
};

struct SelfHeader
{
	u64 se_htype;
	u64 se_appinfooff;
	u64 se_elfoff;
	u64 se_phdroff;
	u64 se_shdroff;
	u64 se_secinfoff;
	u64 se_sceveroff;
	u64 se_controloff;
	u64 se_controlsize;
	u64 pad;

	void Load(const fs::file& f);
	void Show(){}
};

struct SelfAdditionalInfo
{
	bool valid = false;
	std::vector<ControlInfo> ctrl_info;
	AppInfo app_info;
};

class SCEDecrypter
{
protected:
	// Main SELF file stream.
	const fs::file& sce_f;

	// SCE headers.
	SceHeader sce_hdr;

	// Metadata structs.
	MetadataInfo meta_info;
	MetadataHeader meta_hdr;
	std::vector<MetadataSectionHeader> meta_shdr;

	// Internal data buffers.
	std::unique_ptr<u8[]> data_keys;
	u32 data_keys_length;
	std::unique_ptr<u8[]> data_buf;
	u32 data_buf_length;

public:
	SCEDecrypter(const fs::file& s);
	std::vector<fs::file> MakeFile();
	bool LoadHeaders();
	bool LoadMetadata(const u8 erk[32], const u8 riv[16]);
	bool DecryptData();
};

class SELFDecrypter
{
	// Main SELF file stream.
	const fs::file& self_f;

	// SCE, SELF and APP headers.
	SceHeader sce_hdr;
	SelfHeader self_hdr;
	AppInfo app_info;

	// ELF64 header and program header/section header arrays.
	Elf64_Ehdr elf64_hdr;
	std::vector<Elf64_Shdr> shdr64_arr;
	std::vector<Elf64_Phdr> phdr64_arr;

	// ELF32 header and program header/section header arrays.
	Elf32_Ehdr elf32_hdr;
	std::vector<Elf32_Shdr> shdr32_arr;
	std::vector<Elf32_Phdr> phdr32_arr;

	// Decryption info structs.
	std::vector<SectionInfo> secinfo_arr;
	SCEVersionInfo scev_info;
	std::vector<ControlInfo> ctrlinfo_arr;

	// Metadata structs.
	MetadataInfo meta_info;
	MetadataHeader meta_hdr;
	std::vector<MetadataSectionHeader> meta_shdr;

	// Internal data buffers.
	std::unique_ptr<u8[]> data_keys;
	u32 data_keys_length;
	std::unique_ptr<u8[]> data_buf;
	u32 data_buf_length = 0;

	// Main key vault instance.
	KeyVault key_v;

public:
	SELFDecrypter(const fs::file& s);
	fs::file MakeElf(bool isElf32);
	bool LoadHeaders(bool isElf32, SelfAdditionalInfo* out_info = nullptr);
	void ShowHeaders(bool isElf32);
	bool LoadMetadata(u8* klic_key);
	bool DecryptData();
	bool DecryptNPDRM(u8 *metadata, u32 metadata_size);
	bool GetKeyFromRap(u8 *content_id, u8 *npdrm_key);

private:
	template<typename EHdr, typename SHdr, typename PHdr>
	void WriteElf(fs::file& e, EHdr ehdr, SHdr shdr, PHdr phdr)
	{
		// Set initial offset.
		u32 data_buf_offset = 0;

		// Write ELF header.
		WriteEhdr(e, ehdr);

		// Write program headers.
		for (u32 i = 0; i < ehdr.e_phnum; ++i)
		{
			WritePhdr(e, phdr[i]);
		}

		for (unsigned int i = 0; i < meta_hdr.section_count; i++)
		{
			// PHDR type.
			if (meta_shdr[i].type == 2)
			{
				// Decompress if necessary.
				if (meta_shdr[i].compressed == 2)
				{
					const auto filesz = phdr[meta_shdr[i].program_idx].p_filesz;

					// Create a pointer to a buffer for decompression.
					std::unique_ptr<u8[]> decomp_buf(new u8[filesz]);

					// Create a buffer separate from data_buf to uncompress.
					std::unique_ptr<u8[]> zlib_buf(new u8[data_buf_length]);
					memcpy(zlib_buf.get(), data_buf.get(), data_buf_length);

					uLongf decomp_buf_length = ::narrow<uLongf>(filesz);

					// Use zlib uncompress on the new buffer.
					// decomp_buf_length changes inside the call to uncompress
					int rv = uncompress(decomp_buf.get(), &decomp_buf_length, zlib_buf.get() + data_buf_offset, data_buf_length);

					// Check for errors (TODO: Probably safe to remove this once these changes have passed testing.)
					switch (rv)
					{
					case Z_MEM_ERROR: self_log.error("MakeELF encountered a Z_MEM_ERROR!"); break;
					case Z_BUF_ERROR: self_log.error("MakeELF encountered a Z_BUF_ERROR!"); break;
					case Z_DATA_ERROR: self_log.error("MakeELF encountered a Z_DATA_ERROR!"); break;
					default: break;
					}

					// Seek to the program header data offset and write the data.
					e.seek(phdr[meta_shdr[i].program_idx].p_offset);
					e.write(decomp_buf.get(), filesz);
				}
				else
				{
					// Seek to the program header data offset and write the data.
					e.seek(phdr[meta_shdr[i].program_idx].p_offset);
					e.write(data_buf.get() + data_buf_offset, meta_shdr[i].data_size);
				}

				// Advance the data buffer offset by data size.
				data_buf_offset += ::narrow<u32>(meta_shdr[i].data_size, HERE);
			}
		}

		// Write section headers.
		if (self_hdr.se_shdroff != 0)
		{
			e.seek(ehdr.e_shoff);

			for (u32 i = 0; i < ehdr.e_shnum; ++i)
			{
				WriteShdr(e, shdr[i]);
			}
		}
	}
};

fs::file decrypt_self(fs::file elf_or_self, u8* klic_key = nullptr, SelfAdditionalInfo* additional_info = nullptr);
bool verify_npdrm_self_headers(const fs::file& self, u8* klic_key = nullptr);
std::array<u8, 0x10> get_default_self_klic();
