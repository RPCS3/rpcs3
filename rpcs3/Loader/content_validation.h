#pragma once

#include "util/types.hpp"

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

	bool init_hash(std::string_view path);
	content_hash_status calculate_hash(std::string& hash);
};
