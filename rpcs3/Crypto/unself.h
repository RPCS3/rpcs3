#pragma once

#include "Loader/ELF64.h"
#include "Loader/ELF32.h"
#include "key_vault.h"

struct AppInfo 
{
  u64 authid;
  u32 vendor_id;
  u32 self_type;
  u64 version;
  u64 padding;

  void Load(vfsStream& f);

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

  void Load(vfsStream& f);

  void Show();
};

struct SCEVersionInfo
{
  u32 subheader_type;
  u32 present;
  u32 size;
  u32 unknown;

  void Load(vfsStream& f);

  void Show();
};

struct ControlInfo
{
  u32 type;
  u32 size;
  u64 next;

  union {
    // type 1 0x30 bytes
    struct {
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
	struct {
		u8 digest[20];
		u64 unknown;
	} file_digest_30;

	// type 2 0x40 bytes
    struct {
      u8 digest1[20];
      u8 digest2[20];
      u64 unknown;
    } file_digest_40;

	// type 3 0x90 bytes
    struct {
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

  void Load(vfsStream& f);

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

  void Load(vfsStream& f);
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

  void Load(vfsStream& f);
};

struct Signature
{
  u8 r[21];
  u8 s[21];
  u8 padding[6];

  void Load(vfsStream& f);
};

struct SelfSection
{
  u8 *data;
  u64 size;
  u64 offset;

  void Load(vfsStream& f);
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
	void Show() {}
	bool IsLittleEndian() const
	{
		return e_data == 1;
	}

	void Load(vfsStream& f)
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
	void Load(vfsStream& f)
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
	void LoadLE(vfsStream& f)
	{
		f.Read(this, sizeof(*this));
	}
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
	void Load(vfsStream& f)
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
	void LoadLE(vfsStream& f)
	{
		f.Read(this, sizeof(*this));
	}
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
	void Load(vfsStream& f)
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
	void Load(vfsStream& f)
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
	void Load(vfsStream& f)
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
	void Load(vfsStream& f)
	{
		se_magic = Read32(f);
		se_hver = Read32(f);
		se_flags = Read16(f);
		se_type = Read16(f);
		se_meta = Read32(f);
		se_hsize = Read64(f);
		se_esize = Read64(f);
	}
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
	void Load(vfsStream& f)
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
	void Show(){}
};


class SELFDecrypter
{
	// Main SELF file stream.
	vfsStream& self_f;

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
	u8 *data_keys;
	u32 data_keys_length;
	u8 *data_buf;
	u32 data_buf_length;

	// Main key vault instance.
	KeyVault key_v;

public:
	SELFDecrypter(vfsStream& s);
	bool MakeElf(const std::string& elf, bool isElf32);
	bool LoadHeaders(bool isElf32);
	void ShowHeaders(bool isElf32);
	bool LoadMetadata();
	bool DecryptData();
	bool DecryptNPDRM(u8 *metadata, u32 metadata_size);
	bool GetKeyFromRap(u8 *content_id, u8 *npdrm_key);
};

extern bool IsSelf(const std::string& path);
extern bool IsSelfElf32(const std::string& path);
extern bool CheckDebugSelf(const std::string& self, const std::string& elf);
extern bool DecryptSelf(const std::string& elf, const std::string& self);
