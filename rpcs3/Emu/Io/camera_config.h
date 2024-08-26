#pragma once

#include "Utilities/Config.h"

struct cfg_camera final : cfg::node
{
	cfg_camera();
	bool load();
	void save() const;

	struct camera_setting
	{
		int width = 0;
		int height = 0;
		double min_fps = 0;
		double max_fps = 0;
		int format = 0;

		static constexpr u32 member_count = 5;

		std::string to_string() const;
		void from_string(const std::string& text);
	};
	camera_setting get_camera_setting(const std::string& camera, bool& success);
	void set_camera_setting(const std::string& camera, const camera_setting& setting);

	const std::string path;

	cfg::map_entry cameras{ this, "Cameras" }; // <camera>: <width>,<height>,<min_fps>,<max_fps>,<format>
};

extern cfg_camera g_cfg_camera;
