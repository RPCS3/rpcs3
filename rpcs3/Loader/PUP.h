#pragma once

#include "util/types.hpp"
#include "../../Utilities/File.h"

#include <vector>

struct PUPHeader
{
	le_t<u64> magic;
	be_t<u64> package_version;
	be_t<u64> image_version;
	be_t<u64> file_count;
	be_t<u64> header_length;
	be_t<u64> data_length;
};

struct PUPFileEntry
{
	be_t<u64> entry_id;
	be_t<u64> data_offset;
	be_t<u64> data_length;
	u8 padding[8];
};

struct PUPHashEntry
{
	be_t<u64> entry_id;
	u8 hash[20];
	u8 padding[4];
};

class pup_object
{
	const fs::file& m_file;
	bool isValid = false;

	std::vector<PUPFileEntry> m_file_tbl;
	std::vector<PUPHashEntry> m_hash_tbl;

public:
	pup_object(const fs::file& file);

	explicit operator bool() const { return isValid; }

	fs::file get_file(u64 entry_id);
	bool validate_hashes();
};
