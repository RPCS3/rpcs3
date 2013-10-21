/*
* Copyright (c) 2011-2013 by naehrwert
* This file is released under the GPLv2.
*/

#ifndef _SCE_H_
#define _SCE_H_

#include <stdio.h>
#include <string.h>

#include "types.h"
#include "list.h"

/*! SCE file align. */
#define SCE_ALIGN 0x10
/*! Header align. */
#define HEADER_ALIGN 0x80

/*! SCE header magic value ("SCE\0"). */
#define SCE_HEADER_MAGIC 0x53434500

/*! SCE header versions. */
/*! Header version 2. */
#define SCE_HEADER_VERSION_2 2

/*! Key revisions. */
#define KEY_REVISION_0 0x00
#define KEY_REVISION_092_330 0x01
#define KEY_REVISION_1 0x02
//#define KEY_REVISION_ 0x03
#define KEY_REVISION_340_342 0x04
//#define KEY_REVISION_ 0x05
//#define KEY_REVISION_ 0x06
#define KEY_REVISION_350 0x07
//#define KEY_REVISION_ 0x08
//#define KEY_REVISION_ 0x09
#define KEY_REVISION_355 0x0a
//#define KEY_REVISION_ 0x0b
//#define KEY_REVISION_ 0x0c
#define KEY_REVISION_356 0x0d
//#define KEY_REVISION_ 0x0e
//#define KEY_REVISION_ 0x0f
#define KEY_REVISION_360_361 0x10
//#define KEY_REVISION_ 0x11
//#define KEY_REVISION_ 0x12
#define KEY_REVISION_365 0x13
//#define KEY_REVISION_ 0x14
//#define KEY_REVISION_ 0x15
#define KEY_REVISION_370_373 0x16
//#define KEY_REVISION_ 0x17
//#define KEY_REVISION_ 0x18
#define KEY_REVISION_DEBUG 0x8000

/*! SCE header types. */
/*! SELF header. */
#define SCE_HEADER_TYPE_SELF 1
/*! RVK header. */
#define SCE_HEADER_TYPE_RVK 2
/*! PKG header. */
#define SCE_HEADER_TYPE_PKG 3
/*! SPP header. */
#define SCE_HEADER_TYPE_SPP 4

/*! Sub header types. */
/*! SCE version header. */
#define SUB_HEADER_TYPE_SCEVERSION 1
/*! SELF header. */
#define SUB_HEADER_TYPE_SELF 3

/*! Control info types. */
/*! Control flags. */
#define CONTROL_INFO_TYPE_FLAGS 1
/*! Digest. */
#define CONTROL_INFO_TYPE_DIGEST 2
/*! NPDRM block. */
#define CONTROL_INFO_TYPE_NPDRM 3

/*! Optional header types. */
/*! Capability flags header. */
#define OPT_HEADER_TYPE_CAP_FLAGS 1
/*! Individuals seed header. */
#define OPT_HEADER_TYPE_INDIV_SEED 2

/*! Metadata key/iv lengths. */
#define METADATA_INFO_KEYBITS 128
#define METADATA_INFO_KEY_LEN 16
#define METADATA_INFO_KEYPAD_LEN 16
#define METADATA_INFO_IV_LEN 16
#define METADATA_INFO_IVPAD_LEN 16

/*! Metadata section types. */
/*! Segment header. */
#define METADATA_SECTION_TYPE_SHDR 1
/*! Program header. */
#define METADATA_SECTION_TYPE_PHDR 2
/*! Unknown header type 3. */
#define METADATA_SECTION_TYPE_UNK_3 3

/*! Section is hashed. */
#define METADATA_SECTION_HASHED 2
/*! Section is not encrypted. */
#define METADATA_SECTION_NOT_ENCRYPTED 1
/*! Section is encrypted. */
#define METADATA_SECTION_ENCRYPTED 3
/*! Section is not compressed. */
#define METADATA_SECTION_NOT_COMPRESSED 1
/*! Section is compressed. */
#define METADATA_SECTION_COMPRESSED 2

/*! Signature sizes. */
/*! Signature S part size. */
#define SIGNATURE_S_SIZE 21
/*! Signature R part size. */
#define SIGNATURE_R_SIZE 21

/*! Compressed. */
#define SECTION_INFO_COMPRESSED 2
/*! Not compressed. */
#define SECTION_INFO_NOT_COMPRESSED 1

/*! SCE version not present. */
#define SCE_VERSION_NOT_PRESENT 0
/*! SCE version present. */
#define SCE_VERSION_PRESENT 1

/*! SELF types. */
/*! lv0. */
#define SELF_TYPE_LV0 1
/*! lv1. */
#define SELF_TYPE_LV1 2
/*! lv2. */
#define SELF_TYPE_LV2 3
/*! Application. */
#define SELF_TYPE_APP 4
/*! Isolated SPU module. */
#define SELF_TYPE_ISO 5
/*! Secure loader. */
#define SELF_TYPE_LDR 6
/*! Unknown type 7. */
#define SELF_TYPE_UNK_7 7
/*! NPDRM application. */
#define SELF_TYPE_NPDRM 8

/*! NPDRM control info magic value ("NPD\0"). */
#define NP_CI_MAGIC 0x4E504400

/*! NPDRM license types. */
#define NP_LICENSE_NETWORK 1
#define NP_LICENSE_LOCAL 2
#define NP_LICENSE_FREE 3

/*! NPDRM application types. */
#define NP_TYPE_UPDATE 0x20
#define NP_TYPE_SPRX 0
#define NP_TYPE_EXEC 1
#define NP_TYPE_USPRX (NP_TYPE_UPDATE | NP_TYPE_SPRX)
#define NP_TYPE_UEXEC (NP_TYPE_UPDATE | NP_TYPE_EXEC)

/*! SCE header. */
typedef struct _sce_header
{
	/*! Magic value. */
	scetool::u32 magic;
	/*! Header version .*/
	scetool::u32 version;
	/*! Key revision. */
	scetool::u16 key_revision;
	/*! Header type. */
	scetool::u16 header_type;
	/*! Metadata offset. */
	scetool::u32 metadata_offset;
	/*! Header length. */
	scetool::u64 header_len;
	/*! Length of encapsulated data. */
	scetool::u64 data_len;
} sce_header_t;

/*! SELF header. */
typedef struct _self_header
{
	/*! Header type. */
	scetool::u64 header_type;
	/*! Application info offset. */
	scetool::u64 app_info_offset;
	/*! ELF offset. */
	scetool::u64 elf_offset;
	/*! Program headers offset. */
	scetool::u64 phdr_offset;
	/*! Section headers offset. */
	scetool::u64 shdr_offset;
	/*! Section info offset. */
	scetool::u64 section_info_offset;
	/*! SCE version offset. */
	scetool::u64 sce_version_offset;
	/*! Control info offset. */
	scetool::u64 control_info_offset;
	/*! Control info size. */
	scetool::u64 control_info_size;
	/*! Padding. */
	scetool::u64 padding;
} self_header_t;

/*! Metadata info. */
typedef struct _metadata_info
{
	/*! Key. */
	scetool::u8 key[METADATA_INFO_KEY_LEN];
	/*! Key padding. */
	scetool::u8 key_pad[METADATA_INFO_KEYPAD_LEN];
	/*! IV. */
	scetool::u8 iv[METADATA_INFO_IV_LEN];
	/*! IV padding. */
	scetool::u8 iv_pad[METADATA_INFO_IVPAD_LEN];
} metadata_info_t;

typedef struct _metadata_header
{
	/*! Signature input length. */
	scetool::u64 sig_input_length;
	scetool::u32 unknown_0;
	/*! Section count. */
	scetool::u32 section_count;
	/*! Key count. */
	scetool::u32 key_count;
	/*! Optional header size. */
	scetool::u32 opt_header_size;
	scetool::u32 unknown_1;
	scetool::u32 unknown_2;
} metadata_header_t;

/*! Metadata section header. */
typedef struct _metadata_section_header
{
	/*! Data offset. */
	scetool::u64 data_offset;
	/*! Data size. */
	scetool::u64 data_size;
	/*! Type. */
	scetool::u32 type;
	/*! Index. */
	scetool::u32 index;
	/*! Hashed. */
	scetool::u32 hashed;
	/*! SHA1 index. */
	scetool::u32 sha1_index;
	/*! Encrypted. */
	scetool::u32 encrypted;
	/*! Key index. */
	scetool::u32 key_index;
	/*! IV index. */
	scetool::u32 iv_index;
	/*! Compressed. */
	scetool::u32 compressed;
} metadata_section_header_t;

/*! SCE file signature. */
typedef struct _signature
{
	scetool::u8 r[SIGNATURE_R_SIZE];
	scetool::u8 s[SIGNATURE_S_SIZE];
	scetool::u8 padding[6];
} signature_t;

/*! Section info. */
typedef struct _section_info
{
	scetool::u64 offset;
	scetool::u64 size;
	scetool::u32 compressed;
	scetool::u32 unknown_0;
	scetool::u32 unknown_1;
	scetool::u32 encrypted;
} section_info_t;

/*! SCE version. */
typedef struct _sce_version
{
	/*! Header type. */
	scetool::u32 header_type;
	/*! SCE version section present? */
	scetool::u32 present;
	/*! Size. */
	scetool::u32 size;
	scetool::u32 unknown_3;
} sce_version_t;

/*! SCE version data 0x30. */
typedef struct _sce_version_data_30
{
	scetool::u16 unknown_1; //Dunno.
	scetool::u16 unknown_2; //0x0001
	scetool::u32 unknown_3; //Padding?
	scetool::u32 unknown_4; //Number of sections?
	scetool::u32 unknown_5; //Padding?
	/*! Data offset. */
	scetool::u64 offset;
	/*! Data size. */
	scetool::u64 size;
} sce_version_data_30_t;

//(auth_id & AUTH_ONE_MASK) has to be 0x1000000000000000
#define AUTH_ONE_MASK 0xF000000000000000
#define AUTH_TERRITORY_MASK 0x0FF0000000000000
#define VENDOR_TERRITORY_MASK 0xFF000000
#define VENDOR_ID_MASK 0x00FFFFFF

/*! Application info. */
typedef struct _app_info
{
	/*! Auth ID. */
	scetool::u64 auth_id;
	/*! Vendor ID. */
	scetool::u32 vendor_id;
	/*! SELF type. */
	scetool::u32 self_type;
	/*! Version. */
	scetool::u64 version;
	/*! Padding. */
	scetool::u64 padding;
} app_info_t;

/*! Control info. */
typedef struct _control_info
{
	/*! Control info type. */
	scetool::u32 type;
	/*! Size of following data. */
	scetool::u32 size;
	/*! Next flag (1 if another info follows). */
	scetool::u64 next;
} control_info_t;

#define CI_FLAG_00_80 0x80
#define CI_FLAG_00_40 0x40 //root access
#define CI_FLAG_00_20 0x20 //kernel access

#define CI_FLAG_17_01 0x01
#define CI_FLAG_17_02 0x02
#define CI_FLAG_17_04 0x04
#define CI_FLAG_17_08 0x08
#define CI_FLAG_17_10 0x10

//1B:
//bdj 0x01, 0x09
//psp_emu 0x08
//psp_transl 0x0C
#define CI_FLAG_1B_01 0x01 //may use shared mem?
#define CI_FLAG_1B_02 0x02
#define CI_FLAG_1B_04 0x04
#define CI_FLAG_1B_08 0x08 //ss

#define CI_FLAG_1F_SHAREABLE 0x01
#define CI_FLAG_1F_02 0x02 //internal?
#define CI_FLAG_1F_FACTORY 0x04
#define CI_FLAG_1F_08 0x08 //???

/*! Control info data flags. */
typedef struct _ci_data_flags
{
	scetool::u8 data[0x20];
} ci_data_flags_t;

/*! Control info data digest 0x30. */
typedef struct _ci_data_digest_30
{
	scetool::u8 digest[20];
	scetool::u64 unknown_0;
} ci_data_digest_30_t;

/*! Control info data digest 0x40. */
typedef struct _ci_data_digest_40
{
	scetool::u8 digest1[20];
	scetool::u8 digest2[20];
	scetool::u64 fw_version;
} ci_data_digest_40_t;

/*! Control info data NPDRM. */
typedef struct _ci_data_npdrm
{
	/*! Magic. */
	scetool::u32 magic;
	scetool::u32 unknown_0;
	/*! License type. */
	scetool::u32 license_type;
	/*! Application type. */
	scetool::u32 app_type;
	/*! Content ID. */
	scetool::u8 content_id[0x30];
	/*! Random padding. */
	scetool::u8 rndpad[0x10];
	/*! ContentID_FileName hash. */
	scetool::u8 hash_cid_fname[0x10];
	/*! Control info hash. */
	scetool::u8 hash_ci[0x10];
	scetool::u64 unknown_1;
	scetool::u64 unknown_2;
} ci_data_npdrm_t;

/*! Optional header. */
typedef struct _opt_header
{
	/*! Type. */
	scetool::u32 type;
	/*! Size. */
	scetool::u32 size;
	/*! Next flag (1 if another header follows). */
	scetool::u64 next;
} opt_header_t;

/*! Capability flags. */
#define CAP_FLAG_1 0x01 //only seen in PPU selfs
#define CAP_FLAG_2 0x02 //only seen in PPU selfs
#define CAP_FLAG_4 0x04 //only seen in bdj PPU self
#define CAP_FLAG_REFTOOL 0x08
#define CAP_FLAG_DEBUG 0x10
#define CAP_FLAG_RETAIL 0x20
#define CAP_FLAG_SYSDBG 0x40

#define UNK7_2000 0x2000 //hddbind?
#define UNK7_20000 0x20000 //flashbind?
#define UNK7_40000 0x40000 //discbind?
#define UNK7_80000 0x80000

#define UNK7_PS3SWU 0x116000 //dunno...

/*! SCE file capability flags. */
typedef struct _oh_data_cap_flags
{
	scetool::u64 unk3; //0
	scetool::u64 unk4; //0
	/*! Flags. */
	scetool::u64 flags;
	scetool::u32 unk6;
	scetool::u32 unk7;
} oh_data_cap_flags_t;

/*! Section context. */
typedef struct _sce_section_ctxt
{
	/*! Data buffer. */
	void *buffer;
	/*! Size. */
	scetool::u32 size;
	/*! Offset. */
	scetool::u32 offset;
	/*! May be compressed. */
	BOOL may_compr;
} sce_section_ctxt_t;

typedef struct _makeself_ctxt
{
	/*! ELF file buffer (for ELF -> SELF). */
	scetool::u8 *elf;
	/*! ELF file length. */
	scetool::u32 elf_len;
	/*! ELF header. */
	void *ehdr;
	/*! ELF header size. */
	scetool::u32 ehsize;
	/*! Program headers. */
	void *phdrs;
	/*! Program headers size. */
	scetool::u32 phsize;
	/*! Section headers. */
	void *shdrs;
	/*! Section headers size. */
	scetool::u32 shsize;
	/*! Section info count. */
	scetool::u32 si_cnt;
	/*! Number of section infos that are present as data sections. */
	scetool::u32 si_sec_cnt;
} makeself_ctxt_t;

/*! SCE file buffer context. */
typedef struct _sce_buffer_ctxt
{
	/*! SCE file buffer. */
	scetool::u8 *scebuffer;

	/*! SCE header. */
	sce_header_t *sceh;
	/*! File type dependent header. */
	union
	{
		struct
		{
			/*! SELF header. */
			self_header_t *selfh;
			/*! Application info. */
			app_info_t *ai;
			/*! Section info. */
			section_info_t *si;
			/*! SCE version. */
			sce_version_t *sv;
			/*! Control infos. */
			list_t *cis;
			/*! Optional headers. */
			list_t *ohs;
		} self;
	};
	/*! Metadata info. */
	metadata_info_t *metai;
	/*! Metadata header. */
	metadata_header_t *metah;
	/*! Metadata section headers. */
	metadata_section_header_t *metash;
	/*! SCE file keys. */
	scetool::u8 *keys;
	/*! Keys length. */
	scetool::u32 keys_len;
	/*! Signature. */
	signature_t *sig;

	/*! Metadata decrypted? */
	BOOL mdec;

	/*! Data layout. */
	/*! SCE header offset. */
	scetool::u32 off_sceh;
	union
	{
		struct
		{
			/*! SELF header offset. */
			scetool::u32 off_selfh;
			/*! Application info offset. */
			scetool::u32 off_ai;
			/*! ELF header offset. */
			scetool::u32 off_ehdr;
			/*! Program header offset. */
			scetool::u32 off_phdr;
			/*! Section info offset. */
			scetool::u32 off_si;
			/*! SCE version offset. */
			scetool::u32 off_sv;
			/*! Control infos offset. */
			scetool::u32 off_cis;
			/*! Optional headers offset. */
			scetool::u32 off_ohs;
		} off_self;
	};
	/*! Metadata info offset. */
	scetool::u32 off_metai;
	/*! Metadata header offset. */
	scetool::u32 off_metah;
	/*! Metadata section headers offset. */
	scetool::u32 off_metash;
	/*! Keys offset. */
	scetool::u32 off_keys;
	/*! Signature offset. */
	scetool::u32 off_sig;
	/*! Header padding end offset. */
	scetool::u32 off_hdrpad;

	/*! File creation type dependent data. */
	union
	{
		/*! ELF -> SELF. */
		makeself_ctxt_t *makeself;
	};

	/*! Data sections. */
	list_t *secs;
} sce_buffer_ctxt_t;

/*! Create SCE file context from SCE file buffer. */
sce_buffer_ctxt_t *sce_create_ctxt_from_buffer(scetool::u8 *scebuffer);

/*! Create SCE file context for SELF creation. */
sce_buffer_ctxt_t *sce_create_ctxt_build_self(scetool::u8 *elf, scetool::u32 elf_len);

/*! Add data section to SCE context. */
void sce_add_data_section(sce_buffer_ctxt_t *ctxt, void *buffer, scetool::u32 size, BOOL may_compr);

/*! Set metadata section header. */
void sce_set_metash(sce_buffer_ctxt_t *ctxt, scetool::u32 type, BOOL encrypted, scetool::u32 idx);

/*! Compress data. */
void sce_compress_data(sce_buffer_ctxt_t *ctxt);

/*! Layout offsets for SCE file creation. */
void sce_layout_ctxt(sce_buffer_ctxt_t *ctxt);

/*! Encrypt context. */
BOOL sce_encrypt_ctxt(sce_buffer_ctxt_t *ctxt, scetool::u8 *keyset);

/*! Write context to file. */
BOOL sce_write_ctxt(sce_buffer_ctxt_t *ctxt, scetool::s8 *fname);

/*! Decrypt header (use passed metadata_into if not NULL). */
BOOL sce_decrypt_header(sce_buffer_ctxt_t *ctxt, scetool::u8 *metadata_info, scetool::u8 *keyset);

/*! Decrypt data. */
BOOL sce_decrypt_data(sce_buffer_ctxt_t *ctxt);

/*! Print SCE file info. */
void sce_print_info(FILE *fp, sce_buffer_ctxt_t *ctxt);

/*! Get version string from version. */
scetool::s8 *sce_version_to_str(scetool::u64 version);

/*! Get version from version string. */
scetool::u64 sce_str_to_version(scetool::s8 *version);

/*! Convert hex version to dec version. */
scetool::u64 sce_hexver_to_decver(scetool::u64 version);

/*! Get control info. */
control_info_t *sce_get_ctrl_info(sce_buffer_ctxt_t *ctxt, scetool::u32 type);

/*! Get optional header. */
opt_header_t *sce_get_opt_header(sce_buffer_ctxt_t *ctxt, scetool::u32 type);

#endif
