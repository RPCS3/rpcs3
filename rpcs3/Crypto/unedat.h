#pragma once

#include <stdio.h>
#include <string.h>
#include "utils.h"

constexpr u32 SDAT_FLAG = 0x01000000;
constexpr u32 EDAT_COMPRESSED_FLAG = 0x00000001;
constexpr u32 EDAT_FLAG_0x02 = 0x00000002;
constexpr u32 EDAT_ENCRYPTED_KEY_FLAG = 0x00000008;
constexpr u32 EDAT_FLAG_0x10 = 0x00000010;
constexpr u32 EDAT_FLAG_0x20 = 0x00000020;
constexpr u32 EDAT_DEBUG_DATA_FLAG = 0x80000000;

struct NPD_HEADER
{
	u32 magic;
	s32 version;
	s32 license;
	s32 type;
	u8 content_id[0x30];
	u8 digest[0x10];
	u8 title_hash[0x10];
	u8 dev_hash[0x10];
	u64 unk1;
	u64 unk2;
};

struct EDAT_HEADER
{
	s32 flags;
	s32 block_size;
	u64 file_size;
};

// Decrypts full file, or null/empty file
extern fs::file DecryptEDAT(const fs::file& input, const std::string& input_file_name, int mode, const std::string& rap_file_name, u8 *custom_klic, bool verbose);

struct SDATADecrypter final : fs::file_base
{
	// file stream
	const fs::file sdata_file;
	const u64 file_offset;
	u64 file_size{0};
	u32 total_blocks{0};
	u64 pos{0};

	NPD_HEADER npdHeader;
	EDAT_HEADER edatHeader;

	// Internal data buffers.
	std::unique_ptr<u8[]> data_buf;
	u64 data_buf_size{0};

	std::array<u8, 0x10> dec_key{};
public:
	SDATADecrypter(fs::file&& input, u64 offset=0);
	~SDATADecrypter() override {}
	// false if invalid 
	bool ReadHeader();
	u64 ReadData(u64 pos, u8* data, u64 size);

	fs::stat_t stat() override
	{
		fs::stat_t stats;
		stats.is_directory = false;
		stats.is_writable = false;
		stats.size = file_size;
		stats.atime = -1;
		stats.ctime = -1;
		stats.mtime = -1;
		return stats;
	}
	bool trunc(u64 length) override 
	{
		return true;
	};
	u64 read(void* buffer, u64 size) override
	{
		u64 bytesRead = ReadData(pos, (u8*)buffer, size);
		pos += bytesRead;
		return bytesRead;
	}
	u64 write(const void* buffer, u64 size) override
	{
		return 0;
	}

	u64 seek(s64 offset, fs::seek_mode whence) override
	{
		return
			whence == fs::seek_set ? pos = offset :
			whence == fs::seek_cur ? pos = offset + pos :
			whence == fs::seek_end ? pos = offset + size() :
			(fmt::raw_error("SDATADecrypter::seek(): invalid whence"), 0);
	}
	u64 size() override { return file_size; }
};
