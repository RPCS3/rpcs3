#pragma once

#include "util/types.hpp"

#include <string>
#include <vector>

namespace np::avatar
{
	u32 get_dimension(s32 size_type);
	u32 get_max_size(s32 size_type);
	std::vector<u8> load_file(const std::string& path);

	bool is_file_usable(const std::string& path);
	std::vector<std::string> get_installed_avatars(const std::string& user_id);
	std::string get_selected_avatar(const std::string& user_id);

	bool set_selected_avatar(const std::string& user_id, const std::string& path);
	std::vector<u8> get_user_image(const std::string& user_id, s32 size_type);
	std::string get_default_avatar_path(const std::string& user_id);
	std::vector<u8> get_default_image(const std::string& user_id, s32 size_type);
}
