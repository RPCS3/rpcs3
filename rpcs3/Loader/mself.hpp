#pragma once

#include "util/types.hpp"
#include "util/endian.hpp"

struct mself_header
{
	nse_t<u32> magic; // "MSF\x00"
	be_t<u32> ver; // 1
	be_t<u64> size; // File size
	be_t<u32> count; // Number of records
	be_t<u32> header_size; // ???
	u8 reserved[0x28];

	u32 get_count(u64 file_size)
	{
		// Fast sanity check
		if (magic != "MSF"_u32 || ver != u32{1} || this->size != file_size) [[unlikely]]
			return 0;

		return count;
	}
};

CHECK_SIZE(mself_header, 0x40);

struct mself_record
{
	char name[0x20];
	be_t<u64> off;
	be_t<u64> size;
	u8 reserved[0x10];

	u64 get_pos(u64 file_size)
	{
		// Fast sanity check
		if (off < file_size && off + size <= file_size) [[likely]]
			return off;

		return 0;
	}
};

CHECK_SIZE(mself_record, 0x40);
