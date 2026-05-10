#pragma once

#include "key_vault.h"
#include "zlib.h"

#include "util/types.hpp"
#include "Utilities/File.h"
#include "util/logs.hpp"

#include "unedat.h"

LOG_CHANNEL(self_log, "SELF");

// SCE-specific definitions for e_type:
enum
{
	ET_SCE_EXEC         = 0xFE00, // SCE Executable - PRX2
	ET_SCE_RELEXEC      = 0xFE04, // SCE Relocatable Executable - PRX2
	ET_SCE_STUBLIB      = 0xFE0C, // SCE SDK Stubs
	ET_SCE_DYNEXEC      = 0xFE10, // SCE EXEC_ASLR (PS4 Executable with ASLR)
	ET_SCE_DYNAMIC      = 0xFE18, // ?
	ET_SCE_IOPRELEXEC   = 0xFF80, // SCE IOP Relocatable Executable
	ET_SCE_IOPRELEXEC2  = 0xFF81, // SCE IOP Relocatable Executable Version 2
	ET_SCE_EERELEXEC    = 0xFF90, // SCE EE Relocatable Executable
	ET_SCE_EERELEXEC2   = 0xFF91, // SCE EE Relocatable Executable Version 2
	ET_SCE_PSPRELEXEC   = 0xFFA0, // SCE PSP Relocatable Executable
	ET_SCE_PPURELEXEC   = 0xFFA4, // SCE PPU Relocatable Executable
	ET_SCE_ARMRELEXEC   = 0xFFA5, // ?SCE ARM Relocatable Executable (PS Vita System Software earlier or equal 0.931.010)
	ET_SCE_PSPOVERLAY   = 0xFFA8, // ?
};

enum
{
	ELFOSABI_CELL_LV2 = 102 // CELL LV2
};

enum
{
	PT_SCE_RELA         = 0x60000000,
	PT_SCE_LICINFO_1    = 0x60000001,
	PT_SCE_LICINFO_2    = 0x60000002,
	PT_SCE_DYNLIBDATA   = 0x61000000,
	PT_SCE_PROCPARAM    = 0x61000001,
	PT_SCE_UNK_61000010 = 0x61000010,
	PT_SCE_COMMENT      = 0x6FFFFF00,
	PT_SCE_LIBVERSION   = 0x6FFFFF01,
	PT_SCE_UNK_70000001 = 0x70000001,
	PT_SCE_IOPMOD       = 0x70000080,
	PT_SCE_EEMOD        = 0x70000090,
	PT_SCE_PSPRELA      = 0x700000A0,
	PT_SCE_PSPRELA2     = 0x700000A1,
	PT_SCE_PPURELA      = 0x700000A4,
	PT_SCE_SEGSYM       = 0x700000A8,
};

enum
{
	PF_SPU_X = 0x00100000,
	PF_SPU_W = 0x00200000,
	PF_SPU_R = 0x00400000,
	PF_RSX_X = 0x01000000,
	PF_RSX_W = 0x02000000,
	PF_RSX_R = 0x04000000,
};

enum
{
	SHT_SCE_RELA    = 0x60000000,
	SHT_SCE_NID     = 0x61000001,
	SHT_SCE_IOPMOD  = 0x70000080,
	SHT_SCE_EEMOD   = 0x70000090,
	SHT_SCE_PSPRELA = 0x700000A0,
	SHT_SCE_PPURELA = 0x700000A4,
};

struct program_identification_header
{
	u64 program_authority_id;
	u32 program_vendor_id;
	u32 program_type;
	u64 program_sceversion;
	u64 padding;

	void Load(const fs::file& f);
	void Show() const;
};

struct segment_ext_header
{
	u64 offset;      // Offset to data
	u64 size;        // Size of data
	u32 compression; // 1 = plain, 2 = zlib
	u32 unknown;     // Always 0, as far as I know.
	u64 encryption;  // 0 = unrequested, 1 = completed, 2 = requested

	void Load(const fs::file& f);
	void Show() const;
};

struct version_header
{
	u32 subheader_type; // 1 - sceversion
	u32 present;        // 0 = false, 1 = true
	u32 size;           // usually 0x10
	u32 unknown4;

	void Load(const fs::file& f);
	void Show() const;
};

struct supplemental_header
{
	u32 type; // 1=PS3 plaintext_capability; 2=PS3 ELF digest; 3=PS3 NPDRM, 4=PS Vita ELF digest; 5=PS Vita NPDRM; 6=PS Vita boot param; 7=PS Vita shared secret
	u32 size;
	u64 next; // 1 if another Supplemental Header element follows else 0

	union
	{
		// type 1, 0x30 bytes
		struct // 0x20 bytes of data
		{
			u32 ctrl_flag1;
			u32 unknown1;
			u32 unknown2;
			u32 unknown3;
			u32 unknown4;
			u32 unknown5;
			u32 unknown6;
			u32 unknown7;
		} PS3_plaintext_capability_header;

		// type 2, 0x40 bytes
		struct // 0x30 bytes of data
		{
			u8 constant[0x14]; // same for every PS3/PS Vita SELF, hardcoded in make_fself.exe: 627CB1808AB938E32C8C091708726A579E2586E4
			u8 elf_digest[0x14]; // SHA-1. Hash F2C552BF716ED24759CBE8A0A9A6DB9965F3811C is blacklisted by appldr
			u64 required_system_version; // filled on Sony authentication server, contains decimal PS3_SYSTEM_VER value from PARAM.SFO
		} PS3_elf_digest_header_40;

		// type 2, 0x30 bytes
		struct // 0x20 bytes of data
		{
			u8 constant_or_elf_digest[0x14];
			u8 padding[0xC];
		} PS3_elf_digest_header_30;

		// type 3, 0x90 bytes
		struct // 0x80 bytes of data
		{
			NPD_HEADER npd;
		} PS3_npdrm_header;
	};

	void Load(const fs::file& f);
	void Show() const;
};


struct MetadataInfo
{
	u8 key[0x10];
	u8 key_pad[0x10];
	u8 iv[0x10];
	u8 iv_pad[0x10];

	void Load(u8* in);
	void Show() const;
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
	void Show() const;
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
	void Show() const;
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
	u8* data;
	u64 size;
	u64 offset;

	void Load(const fs::file& f);
};

struct Elf32_Ehdr
{
	//u8 e_ident[16];      // ELF identification
	u32 e_magic;
	u8 e_class;
	u8 e_data;
	u8 e_curver;
	u8 e_os_abi;
	u64 e_abi_ver;
	u16 e_type;          // object file type
	u16 e_machine;       // machine type
	u32 e_version;       // object file version
	u32 e_entry;         // entry point address
	u32 e_phoff;         // program header offset
	u32 e_shoff;         // section header offset
	u32 e_flags;         // processor-specific flags
	u16 e_ehsize;        // ELF header size
	u16 e_phentsize;     // size of program header entry
	u16 e_phnum;         // number of program header entries
	u16 e_shentsize;     // size of section header entry
	u16 e_shnum;         // number of section header entries
	u16 e_shstrndx;      // section name string table index

	void Load(const fs::file& f);
	static void Show() {}
	bool IsLittleEndian() const { return e_data == 1; }
	bool CheckMagic() const { return e_magic == 0x7F454C46; }
	u32 GetEntry() const { return e_entry; }
};

struct Elf32_Shdr
{
	u32 sh_name;      // section name
	u32 sh_type;      // section type
	u32 sh_flags;     // section attributes
	u32 sh_addr;      // virtual address in memory
	u32 sh_offset;    // offset in file
	u32 sh_size;      // size of section
	u32 sh_link;      // link to other section
	u32 sh_info;      // miscellaneous information
	u32 sh_addralign; // address alignment boundary
	u32 sh_entsize;   // size of entries, if section has table

	void Load(const fs::file& f);
	void LoadLE(const fs::file& f);
	static void Show() {}
};

struct Elf32_Phdr
{
	u32 p_type;   // Segment type
	u32 p_offset; // Segment file offset
	u32 p_vaddr;  // Segment virtual address
	u32 p_paddr;  // Segment physical address
	u32 p_filesz; // Segment size in file
	u32 p_memsz;  // Segment size in memory
	u32 p_flags;  // Segment flags
	u32 p_align;  // Segment alignment

	void Load(const fs::file& f);
	void LoadLE(const fs::file& f);
	static void Show() {}
};

struct Elf64_Ehdr
{
	//u8 e_ident[16];      // ELF identification
	u32 e_magic;
	u8 e_class;
	u8 e_data;
	u8 e_curver;
	u8 e_os_abi;
	u64 e_abi_ver;
	u16 e_type;          // object file type
	u16 e_machine;       // machine type
	u32 e_version;       // object file version
	u64 e_entry;         // entry point address
	u64 e_phoff;         // program header offset
	u64 e_shoff;         // section header offset
	u32 e_flags;         // processor-specific flags
	u16 e_ehsize;        // ELF header size
	u16 e_phentsize;     // size of program header entry
	u16 e_phnum;         // number of program header entries
	u16 e_shentsize;     // size of section header entry
	u16 e_shnum;         // number of section header entries
	u16 e_shstrndx;      // section name string table index

	void Load(const fs::file& f);
	static void Show() {}
	bool CheckMagic() const { return e_magic == 0x7F454C46; }
	u64 GetEntry() const { return e_entry; }
};

struct Elf64_Shdr
{
	u32 sh_name;      // section name
	u32 sh_type;      // section type
	u64 sh_flags;     // section attributes
	u64 sh_addr;      // virtual address in memory
	u64 sh_offset;    // offset in file
	u64 sh_size;      // size of section
	u32 sh_link;      // link to other section
	u32 sh_info;      // miscellaneous information
	u64 sh_addralign; // address alignment boundary
	u64 sh_entsize;   // size of entries, if section has table

	void Load(const fs::file& f);
	static void Show(){}
};

struct Elf64_Phdr
{
	u32 p_type;   // Segment type
	u32 p_flags;  // Segment flags
	u64 p_offset; // Segment file offset
	u64 p_vaddr;  // Segment virtual address
	u64 p_paddr;  // Segment physical address
	u64 p_filesz; // Segment size in file
	u64 p_memsz;  // Segment size in memory
	u64 p_align;  // Segment alignment

	void Load(const fs::file& f);
	static void Show(){}
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
	static void Show(){}
	bool CheckMagic() const { return se_magic == 0x53434500; }
};

struct ext_hdr
{
	u64 ext_hdr_version;
	u64 program_identification_hdr_offset;
	u64 ehdr_offset;
	u64 phdr_offset;
	u64 shdr_offset;
	u64 segment_ext_hdr_offset;
	u64 version_hdr_offset;
	u64 supplemental_hdr_offset;
	u64 supplemental_hdr_size;
	u64 padding;

	void Load(const fs::file& f);
	static void Show(){}
};

struct SelfAdditionalInfo
{
	bool valid = false;
	std::vector<supplemental_header> supplemental_hdr;
	program_identification_header prog_id_hdr;
};

class SCEDecrypter
{
protected:
	// Main SELF file stream.
	const fs::file& sce_f;

	// SCE headers.
	SceHeader sce_hdr{};

	// Metadata structs.
	MetadataInfo meta_info{};
	MetadataHeader meta_hdr{};
	std::vector<MetadataSectionHeader> meta_shdr{};

	// Internal data buffers.
	std::unique_ptr<u8[]> data_keys{};
	u32 data_keys_length{};
	std::unique_ptr<u8[]> data_buf{};
	u32 data_buf_length{};

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
	SceHeader sce_hdr{};
	ext_hdr m_ext_hdr{};
	program_identification_header m_prog_id_hdr{};

	// ELF64 header and program header/section header arrays.
	Elf64_Ehdr elf64_hdr{};
	std::vector<Elf64_Shdr> shdr64_arr{};
	std::vector<Elf64_Phdr> phdr64_arr{};

	// ELF32 header and program header/section header arrays.
	Elf32_Ehdr elf32_hdr{};
	std::vector<Elf32_Shdr> shdr32_arr{};
	std::vector<Elf32_Phdr> phdr32_arr{};

	// Decryption info structs.
	std::vector<segment_ext_header> m_seg_ext_hdr{};
	version_header m_version_hdr{};
	std::vector<supplemental_header> m_supplemental_hdr_arr{};

	// Metadata structs.
	MetadataInfo meta_info{};
	MetadataHeader meta_hdr{};
	std::vector<MetadataSectionHeader> meta_shdr{};

	// Internal data buffers.
	std::unique_ptr<u8[]> data_keys{};
	u32 data_keys_length{};
	std::unique_ptr<u8[]> data_buf{};
	u32 data_buf_length{};

	// Main key vault instance.
	KeyVault key_v{};

public:
	SELFDecrypter(const fs::file& s);
	fs::file MakeElf(bool isElf32);
	bool LoadHeaders(bool isElf32, SelfAdditionalInfo* out_info = nullptr);
	void ShowHeaders(bool isElf32);
	bool LoadMetadata(const u8* klic_key);
	bool DecryptData();
	bool DecryptNPDRM(u8 *metadata, u32 metadata_size);
	const NPD_HEADER* GetNPDHeader() const;
	static bool GetKeyFromRap(const char *content_id, u8 *npdrm_key);

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
					const int rv = uncompress(decomp_buf.get(), &decomp_buf_length, zlib_buf.get() + data_buf_offset, data_buf_length);

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
				data_buf_offset += ::narrow<u32>(meta_shdr[i].data_size);
			}
		}

		// Write section headers.
		if (m_ext_hdr.shdr_offset != 0)
		{
			e.seek(ehdr.e_shoff);

			for (u32 i = 0; i < ehdr.e_shnum; ++i)
			{
				WriteShdr(e, shdr[i]);
			}
		}
	}
};

fs::file decrypt_self(const fs::file& elf_or_self, const u8* klic_key = nullptr, SelfAdditionalInfo* additional_info = nullptr);
bool verify_npdrm_self_headers(const fs::file& self, u8* klic_key = nullptr, NPD_HEADER* npd_out = nullptr);
bool get_npdrm_self_header(const fs::file& self, NPD_HEADER& npd);

u128 get_default_self_klic();
