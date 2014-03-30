#pragma once
#include "utils.h"
#include "key_vault.h"

// Constants
#define PKG_HEADER_SIZE 0xC0 //sizeof(pkg_header) + sizeof(pkg_unk_checksum)
#define PKG_RELEASE_TYPE_RELEASE      0x8000
#define PKG_RELEASE_TYPE_DEBUG        0x0000
#define PKG_PLATFORM_TYPE_PS3         0x0001
#define PKG_PLATFORM_TYPE_PSP         0x0002

#define PKG_FILE_ENTRY_NPDRM          0x0001
#define PKG_FILE_ENTRY_NPDRMEDAT      0x0002
#define PKG_FILE_ENTRY_REGULAR        0x0003
#define PKG_FILE_ENTRY_FOLDER         0x0004
#define PKG_FILE_ENTRY_SDAT           0x0009
#define PKG_FILE_ENTRY_OVERWRITE  0x80000000

#define HASH_LEN 16
#define BUF_SIZE 4096

// Structs
struct PKGHeader
{
	be_t<u32> pkg_magic;    // Magic (0x7f504b47)
	be_t<u16> pkg_type;     // Release type (Retail:0x8000, Debug:0x0000)
	be_t<u16> pkg_platform; // Platform type (PS3:0x0001, PSP:0x0002)
	be_t<u32> header_size;  // Header size (0xc0)
	be_t<u32> unk1;         // Some PKG version maybe?
	be_t<u32> meta_size;    // Size of metadata (block after header & hashes) 
	be_t<u32> file_count;   // Number of files
	be_t<u64> pkg_size;     // PKG size in bytes
	be_t<u64> data_offset;  // Encrypted data offset
	be_t<u64> data_size;    // Encrypted data size in bytes
	char title_id[48];      // Title ID
	u8 qa_digest[16];       // This should be the hash of "files + attribs"
	u8 klicensee[16];       // Nonce
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

extern int Unpack(wxFile& dec_pkg_f, std::string src, std::string dst);