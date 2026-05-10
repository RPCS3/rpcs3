#pragma once

#include "util/types.hpp"
#include "util/endian.hpp"
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

// PUP loading error
enum class pup_error : u32
{
	ok,

	stream,
	header_read,
	header_magic,
	header_file_count,
	expected_size,
	file_entries,
	hash_mismatch,
};

class pup_object
{
	fs::file m_file{};

	pup_error m_error{};
	std::string m_formatted_error{};

	std::vector<PUPFileEntry> m_file_tbl{};
	std::vector<PUPHashEntry> m_hash_tbl{};

	pup_error validate_hashes();

public:
	pup_object(fs::file&& file);
	fs::file &file() { return m_file; }

	explicit operator pup_error() const { return m_error; }
	const std::string& get_formatted_error() const { return m_formatted_error; }

	fs::file get_file(u64 entry_id) const;
};
