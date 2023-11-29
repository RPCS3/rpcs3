#pragma once

#include "util/serialization_ext.hpp"

struct version_entry
{
	u16 type;
	u16 version;

	ENABLE_BITWISE_SERIALIZATION;
};

bool load_and_check_reserved(utils::serial& ar, usz size);
bool is_savestate_version_compatible(const std::vector<version_entry>& data, bool is_boot_check);
std::vector<version_entry> get_savestate_versioning_data(fs::file&& file, std::string_view filepath);
bool is_savestate_compatible(fs::file&& file, std::string_view filepath);
std::vector<version_entry> read_used_savestate_versions();
std::string get_savestate_file(std::string_view title_id, std::string_view boot_path, s64 abs_id, s64 rel_id);