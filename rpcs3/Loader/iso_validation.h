#pragma once

#include "util/types.hpp"

// Enum returned by calculating hash
enum class iso_hash_status
{
	INITIALIZED,
	COMPLETED,
	ABORTED
};

// Enum returned by checking integrity
enum class iso_integrity_status
{
	NO_MATCH,
	FOUND_MATCH,
	ERROR_OPENING_DB,
	ERROR_PARSING_DB
};

// ISO file validation class
class iso_file_validation
{
private:
	std::string m_path;
	u64 m_size = 0;
	u64 m_bytes_read = 0;
	iso_hash_status m_status = iso_hash_status::INITIALIZED;

public:
	static iso_integrity_status check_integrity(const std::string& hash, std::string* game_name = nullptr);

	const std::string& get_path() const { return m_path; }
	u64 get_size() const { return m_size; }
	u64 get_bytes_read() const { return m_bytes_read; }
	iso_hash_status get_status() const { return m_status; }
	void abort_hash() { m_status = iso_hash_status::ABORTED; }

	bool init_hash(const std::string& path);
	iso_hash_status calculate_hash(std::string& hash);
};
