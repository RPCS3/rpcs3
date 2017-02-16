#pragma once

#include "../../Utilities/types.h"
#include "../../Utilities/File.h"

#include <vector>

typedef struct {
	le_t<u64> magic;
	be_t<u64> package_version;
	be_t<u64> image_version;
	be_t<u64> file_count;
	be_t<u64> header_length;
	be_t<u64> data_length;
} PUPHeader;

typedef struct {
	be_t<u64> entry_id;
	be_t<u64> data_offset;
	be_t<u64> data_length;
	u8 padding[8];
} PUPFileEntry;

typedef struct {
	be_t<u64> entry_id;
	be_t<u8> hash[20];
	be_t<u8> padding[4];
} PUPHashEntry;

class pup_object {
	const fs::file& m_file;
	bool isValid = true;
	
	std::vector<PUPFileEntry> m_file_tbl;
	std::vector<PUPHashEntry> m_hash_tbl;

public:
	pup_object(const fs::file& file);

	operator bool() const { return isValid; };

	fs::file get_file(u64 entry_id);
};