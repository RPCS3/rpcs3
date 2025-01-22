#pragma once

#include "util/serialization_ext.hpp"

struct version_entry
{
	u16 type;
	u16 version;

	ENABLE_BITWISE_SERIALIZATION;
};

struct savestate_header
{
	ENABLE_BITWISE_SERIALIZATION;

	nse_t<u64, 1> magic;
	bool LE_format;
	bool state_inspection_support;
	nse_t<u64, 1> offset;
	b8 flag_versions_is_following_data;
};

struct hle_locks_t
{
	atomic_t<s64> lock_val{0};

	enum states : s64
	{
		waiting_for_evaluation = -1,
		finalized = -2,
	};

	[[noreturn]] void lock();
	bool try_lock();
	void unlock();
	bool try_finalize(std::function<bool()> test);
};

std::shared_ptr<utils::serial> make_savestate_reader(const std::string& path);
bool load_and_check_reserved(utils::serial& ar, usz size);
bool is_savestate_version_compatible(const std::vector<version_entry>& data, bool is_boot_check);
std::vector<version_entry> get_savestate_versioning_data(fs::file&& file, std::string_view filepath);
bool is_savestate_compatible(fs::file&& file, std::string_view filepath);
bool is_savestate_compatible(const std::string& filepath);
std::vector<version_entry> read_used_savestate_versions();
std::string get_savestate_file(std::string_view title_id, std::string_view boot_path, s64 abs_id, s64 rel_id);
