#pragma once

#include "Utilities/BEType.h"

// Constants
enum
{
	PKG_HEADER_SIZE  = 0xC0, //sizeof(pkg_header) + sizeof(pkg_unk_checksum)
	PKG_HEADER_SIZE2 = 0x280,
};

enum : u16
{
	PKG_RELEASE_TYPE_RELEASE      = 0x8000,
	PKG_RELEASE_TYPE_DEBUG        = 0x0000,

	PKG_PLATFORM_TYPE_PS3         = 0x0001,
	PKG_PLATFORM_TYPE_PSP_PSVITA  = 0x0002,
};

enum : u32
{
	PKG_FILE_ENTRY_NPDRM          = 1,
	PKG_FILE_ENTRY_NPDRMEDAT      = 2,
	PKG_FILE_ENTRY_REGULAR        = 3,
	PKG_FILE_ENTRY_FOLDER         = 4,
	PKG_FILE_ENTRY_UNK0           = 5,
	PKG_FILE_ENTRY_UNK1           = 6,
	PKG_FILE_ENTRY_SDAT           = 9,

	PKG_FILE_ENTRY_OVERWRITE      = 0x80000000,
	PKG_FILE_ENTRY_PSP            = 0x10000000,
};

// Structs
struct PKGHeader
{
	le_t<u32> pkg_magic;    // Magic (0x7f504b47) (" PKG")
	be_t<u16> pkg_type;     // Release type (Retail:0x8000, Debug:0x0000)
	be_t<u16> pkg_platform; // Platform type (PS3:0x0001, PSP:0x0002)
	be_t<u32> meta_offset;  // Metadata offset. Usually 0xC0 for PS3, usually 0x280 for PSP and PSVita
	be_t<u32> meta_count;   // Metadata item count
	be_t<u32> meta_size;    // Metadata size.
	be_t<u32> file_count;   // Number of files
	be_t<u64> pkg_size;     // PKG size in bytes
	be_t<u64> data_offset;  // Encrypted data offset
	be_t<u64> data_size;    // Encrypted data size in bytes
	char title_id[48];      // Title ID
	be_t<u64> qa_digest[2]; // This should be the hash of "files + attribs"
	be_t<u128> klicensee;   // Nonce
	// + some stuff
};

// Extended header in PSP and PSVita packages
struct PKGExtHeader
{
	le_t<u32> magic;                            // 0x7F657874 (" ext")
	be_t<u32> unknown_1;                        // Maybe version. always 1
	be_t<u32> ext_hdr_size;                     // Extended header size. ex: 0x40
	be_t<u32> ext_data_size;                    // ex: 0x180
	be_t<u32> main_and_ext_headers_hmac_offset; // ex: 0x100
	be_t<u32> metadata_header_hmac_offset;      // ex: 0x360, 0x390, 0x490 
	be_t<u64> tail_offset;                      // tail size seams to be always 0x1A0
	be_t<u32> padding1;
	be_t<u32> pkg_key_id;                       // Id of the AES key used for decryption. PSP = 0x1, PSVita = 0xC0000002, PSM = 0xC0000004
	be_t<u32> full_header_hmac_offset;          // ex: none (old pkg): 0, 0x930
	u8 padding2[0x14];
};

struct PKGEntry
{
	be_t<u32> name_offset;  // File name offset
	be_t<u32> name_size;    // File name size
	be_t<u64> file_offset;  // File offset
	be_t<u64> file_size;    // File size
	be_t<u32> type;         // File type
	be_t<u32> pad;          // Padding (zeros)
};

// https://www.psdevwiki.com/ps3/PKG_files#PKG_Metadata
struct PKGMetaData
{
	be_t<u32> drm_type{ 0 };
	be_t<u32> content_type{ 0 };
	be_t<u32> package_type{ 0 };
	be_t<u64> package_size{ 0 };
	be_t<u32> package_revision{ 0 };
	be_t<u64> software_revision{ 0 };
	std::string title_id;
	std::string install_dir;
};

bool pkg_install(const std::string& path, atomic_t<double>&);
