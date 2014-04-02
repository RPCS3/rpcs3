#pragma once
#include "utils.h"
#include "key_vault.h"
#include "Loader/ELF.h"
#include "Loader/SELF.h"
#include <wx/mstream.h>
#include <wx/zstream.h>

struct AppInfo 
{
  u64 authid;
  u32 vendor_id;
  u32 self_type;
  u64 version;
  u64 padding;

  void Load(vfsStream& f)
  {
		authid		= Read64(f);
		vendor_id	= Read32(f);
		self_type	= Read32(f);
		version		= Read64(f);
		padding		= Read64(f);
  }

  void Show()
  {
	  ConLog.Write("AuthID: 0x%llx",			authid);
	  ConLog.Write("VendorID: 0x%08x",			vendor_id);
	  ConLog.Write("SELF type: 0x%08x",			self_type);
	  ConLog.Write("Version: 0x%llx",		    version);
  }
};

struct SectionInfo
{
  u64 offset;
  u64 size;
  u32 compressed;
  u32 unknown1;
  u32 unknown2;
  u32 encrypted;

  void Load(vfsStream& f)
  {
		offset			= Read64(f);
		size			= Read64(f);
		compressed		= Read32(f);
		unknown1		= Read32(f);
		unknown2		= Read32(f);
		encrypted		= Read32(f);
  }

  void Show()
  {
	  ConLog.Write("Offset: 0x%llx",			offset);
	  ConLog.Write("Size: 0x%llx",				size);
	  ConLog.Write("Compressed: 0x%08x",		compressed);
	  ConLog.Write("Unknown1: 0x%08x",			unknown1);
	  ConLog.Write("Unknown2: 0x%08x",			unknown2);
	  ConLog.Write("Encrypted: 0x%08x",			encrypted);
  }
};

struct SCEVersionInfo
{
  u32 subheader_type;
  u32 present;
  u32 size;
  u32 unknown;

  void Load(vfsStream& f)
  {
	  subheader_type	= Read32(f);
	  present			= Read32(f);
	  size				= Read32(f);
	  unknown			= Read32(f);
  }

  void Show()
  {
	  ConLog.Write("Sub-header type: 0x%08x",			subheader_type);
	  ConLog.Write("Present: 0x%08x",					present);
	  ConLog.Write("Size: 0x%08x",						size);
	  ConLog.Write("Unknown: 0x%08x",					unknown);
  }
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

  void Load(vfsStream& f)
  {
	  type							= Read32(f);
	  size							= Read32(f);
	  next							= Read64(f);

	  if (type == 1)
	  {
		  control_flags.ctrl_flag1		= Read32(f);
		  control_flags.unknown1		= Read32(f);
		  control_flags.unknown2		= Read32(f);
		  control_flags.unknown3		= Read32(f);
		  control_flags.unknown4		= Read32(f);
		  control_flags.unknown5		= Read32(f);
		  control_flags.unknown6		= Read32(f);
		  control_flags.unknown7		= Read32(f);
	  }
	  else if (type == 2)
	  {
		  if (size == 0x30)
		  {
			  f.Read(file_digest_30.digest, 20);
			  file_digest_30.unknown        = Read64(f);
		  }
		  else if (size == 0x40)
		  {
			  f.Read(file_digest_40.digest1, 20);
			  f.Read(file_digest_40.digest2, 20);
			  file_digest_40.unknown        = Read64(f);
		  }
	  }
	  else if (type == 3)
	  {
		  npdrm.magic                   = Read32(f);
		  npdrm.unknown1                = Read32(f);
		  npdrm.license					= Read32(f);
		  npdrm.type					= Read32(f);
		  f.Read(npdrm.content_id, 48);
		  f.Read(npdrm.digest, 16);
		  f.Read(npdrm.invdigest, 16);
		  f.Read(npdrm.xordigest, 16);
		  npdrm.unknown2                = Read64(f);
		  npdrm.unknown3                = Read64(f);
	  }
  }

  void Show()
  {
	  ConLog.Write("Type: 0x%08x",			type);
	  ConLog.Write("Size: 0x%08x",			size);
	  ConLog.Write("Next: 0x%llx",			next);

	  if (type == 1)
	  {
		  ConLog.Write("Control flag 1: 0x%08x",			control_flags.ctrl_flag1);
		  ConLog.Write("Unknown1: 0x%08x",					control_flags.unknown1);
		  ConLog.Write("Unknown2: 0x%08x",					control_flags.unknown2);
		  ConLog.Write("Unknown3: 0x%08x",					control_flags.unknown3);
		  ConLog.Write("Unknown4: 0x%08x",					control_flags.unknown4);
		  ConLog.Write("Unknown5: 0x%08x",					control_flags.unknown5);
		  ConLog.Write("Unknown6: 0x%08x",					control_flags.unknown6);
		  ConLog.Write("Unknown7: 0x%08x",					control_flags.unknown7);
	  }
	  else if (type == 2)
	  {
		  if (size == 0x30)
		  {
			  std::string digest_str;
			  for (int i = 0; i < 20; i++)
				  digest_str += fmt::Format("%02x", file_digest_30.digest[i]);

			  ConLog.Write("Digest: %s",						digest_str.c_str());
			  ConLog.Write("Unknown: 0x%llx",					file_digest_30.unknown);
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
			  
			  ConLog.Write("Digest1: %s",						digest_str1.c_str());
			  ConLog.Write("Digest2: %s",						digest_str2.c_str());
			  ConLog.Write("Unknown: 0x%llx",					file_digest_40.unknown);
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

		  ConLog.Write("Magic: 0x%08x",							npdrm.magic);
		  ConLog.Write("Unknown1: 0x%08x",						npdrm.unknown1);
		  ConLog.Write("License: 0x%08x",						npdrm.license);
		  ConLog.Write("Type: 0x%08x",							npdrm.type);
		  ConLog.Write("ContentID: %s",							contentid_str.c_str());
		  ConLog.Write("Digest: %s",							digest_str.c_str());
		  ConLog.Write("Inverse digest: %s",					invdigest_str.c_str());
		  ConLog.Write("XOR digest: %s",						xordigest_str.c_str());
		  ConLog.Write("Unknown2: 0x%llx",						npdrm.unknown2);
		  ConLog.Write("Unknown3: 0x%llx",						npdrm.unknown3);
	  }
  }
};


struct MetadataInfo
{
  u8 key[0x10];
  u8 key_pad[0x10];
  u8 iv[0x10];
  u8 iv_pad[0x10];

  void Load(u8* in)
  {
	  memcpy(key, in, 0x10);
	  memcpy(key_pad, in + 0x10, 0x10);
	  memcpy(iv, in + 0x20, 0x10);
	  memcpy(iv_pad, in + 0x30, 0x10);
  }

  void Show()
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
	  
	  ConLog.Write("Key: %s", key_str.c_str());
	  ConLog.Write("Key pad: %s", key_pad_str.c_str());
	  ConLog.Write("IV: %s", iv_str.c_str());
	  ConLog.Write("IV pad: %s", iv_pad_str.c_str());
  }
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

  void Load(u8* in)
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

  void Show()
  {
	  ConLog.Write("Signature input length: 0x%llx",			signature_input_length);
	  ConLog.Write("Unknown1: 0x%08x",							unknown1);
	  ConLog.Write("Section count: 0x%08x",						section_count);
	  ConLog.Write("Key count: 0x%08x",							key_count);
	  ConLog.Write("Optional header size: 0x%08x",				opt_header_size);
	  ConLog.Write("Unknown2: 0x%08x",							unknown2);
	  ConLog.Write("Unknown3: 0x%08x",							unknown3);
  }
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

  void Load(u8* in)
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

  void Show()
  {
	  ConLog.Write("Data offset: 0x%llx",						data_offset);
	  ConLog.Write("Data size: 0x%llx",							data_size);
	  ConLog.Write("Type: 0x%08x",								type);
	  ConLog.Write("Program index: 0x%08x",						program_idx);
	  ConLog.Write("Hashed: 0x%08x",							hashed);
	  ConLog.Write("SHA1 index: 0x%08x",						sha1_idx);
	  ConLog.Write("Encrypted: 0x%08x",							encrypted);
	  ConLog.Write("Key index: 0x%08x",							key_idx);
	  ConLog.Write("IV index: 0x%08x",							iv_idx);
	  ConLog.Write("Compressed: 0x%08x",						compressed);
  }
};

struct SectionHash {
  u8 sha1[20];
  u8 padding[12];
  u8 hmac_key[64];

  void Load(vfsStream& f)
  {
	  f.Read(sha1, 20);
	  f.Read(padding, 12);
	  f.Read(hmac_key, 64);
  }
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

  void Load(vfsStream& f)
  {
	  type					= Read32(f);
	  capabilities_size		= Read32(f);
	  next					= Read32(f);
	  unknown1				= Read32(f);
	  unknown2				= Read64(f);
	  unknown3				= Read64(f);
	  flags					= Read64(f);
	  unknown4				= Read32(f);
	  unknown5				= Read32(f);
  }
};

struct Signature
{
  u8 r[21];
  u8 s[21];
  u8 padding[6];

  void Load(vfsStream& f)
  {
	  f.Read(r, 21);
	  f.Read(s, 21);
	  f.Read(padding, 6);
  }
};

struct SelfSection
{
  u8 *data;
  u64 size;
  u64 offset;

  void Load(vfsStream& f)
  {
	  *data				= Read32(f);
	  size				= Read64(f);
	  offset			= Read64(f);
  }
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
	Array<Elf64_Shdr> shdr64_arr;
	Array<Elf64_Phdr> phdr64_arr;

	// ELF32 header and program header/section header arrays.
	Elf32_Ehdr elf32_hdr;
	Array<Elf32_Shdr> shdr32_arr;
	Array<Elf32_Phdr> phdr32_arr;

	// Decryption info structs.
	Array<SectionInfo> secinfo_arr;
	SCEVersionInfo scev_info;
	Array<ControlInfo> ctrlinfo_arr;

	// Metadata structs.
	MetadataInfo meta_info;
	MetadataHeader meta_hdr;
	Array<MetadataSectionHeader> meta_shdr;

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
