#pragma once

#include "Loader/SELF.h"
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

struct SectionHash {
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
