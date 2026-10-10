#pragma once

#include "IRD_validation.h"

#include "util/types.hpp"
#include "Utilities/File.h"

#include <string>
#include <vector>

// Enum identifying the content file type
enum class content_file_type
{
	ISO,
	PSN_CONTENT,
	PSN_DLC,
	PSN_UPDATE
};

// Enum returned by calculating hash
enum class content_hash_status
{
	INITIALIZED,
	COMPLETED,
	ABORTED
};

// Enum returned by checking integrity
enum class content_integrity_status
{
	NO_MATCH,
	FOUND_MATCH,
	ERROR_OPENING_DB,
	ERROR_PARSING_DB
};

// Content validation class
class content_validation
{
private:
	std::string m_path;
	std::string m_name;
	u64 m_size = 0;
	u64 m_bytes_read = 0;
	u16 m_count = 0; // Set only by set_count()
	content_hash_status m_status = content_hash_status::INITIALIZED;

	// MD5 hash (lowercase hex) of a single file, the one every check goes through.
	//
	// - "buffer" is the scratch space the caller lends it: an empty one is sized on the first call and kept
	//   between them, so that a check over many files pays for it only once.
	// - "padded_size" is the size the file is supposed to have: whenever it is shorter, the missing tail is
	//   hashed as zeros, the way the dumping tools handle the padding of "PS3UPDAT.PUP". A "padded_size" of 0
	//   hashes the file as it is.
	//
	// How far it got is reported as it goes, and it gives up as soon as the check is called off
	bool hash_file(const fs::file& file, u64 padded_size, std::vector<u8>& buffer, std::string& hash);

public:
	static content_integrity_status check_integrity(content_file_type file_type, std::string_view hash, std::string* game_name = nullptr);

	const std::string& get_path() const { return m_path; }
	const std::string& get_name() const	{ return m_name; }
	u64 get_size() const { return m_size; }
	u64 get_bytes_read() const { return m_bytes_read; }
	u16 get_count() const { return m_count; }
	content_hash_status get_status() const { return m_status; }

	void set_count(u16 count) { m_count = count; }
	void abort_hash() { m_status = content_hash_status::ABORTED; }

	bool init_hash(const std::string& path);
	content_hash_status calculate_hash(std::string& hash);

	// Validates a game against the IRD file of the disc it comes from, comparing the MD5 hash of each of its
	// files. It reports how far it got through this very object, which is the one the progress dialog watches,
	// and gives up as soon as "abort_hash()" is called. See "IRD_validation.cpp" for what it does
	disc_check_status check_ird_content(const std::string& game_path, const std::string& ird_path, disc_check_report& report);
};
