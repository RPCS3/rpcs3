#include "stdafx.h"
#include "camera_config.h"
#include <charconv>

LOG_CHANNEL(camera_log, "Camera");

cfg_camera g_cfg_camera;

cfg_camera::cfg_camera()
	: cfg::node()
	, path(fs::get_config_dir(true) + "camera.yml")
{
}

bool cfg_camera::load()
{
	camera_log.notice("Loading camera config from '%s'", path);

	if (fs::file cfg_file{ path, fs::read })
	{
		return from_string(cfg_file.to_string());
	}

	camera_log.notice("Camera config missing. Using default settings. Path: %s", path);
	from_default();
	return false;
}

void cfg_camera::save() const
{
	camera_log.notice("Saving camera config to '%s'", path);

	if (!cfg::node::save(path))
	{
		camera_log.error("Failed to save camera config to '%s' (error=%s)", path, fs::g_tls_error);
	}
}

cfg_camera::camera_setting cfg_camera::get_camera_setting(const std::string& camera, bool& success)
{
	camera_setting setting;
	const std::string value = cameras.get_value(camera);
	success = !value.empty();
	if (success)
	{
		setting.from_string(cameras.get_value(camera));
	}
	return setting;
}

void cfg_camera::set_camera_setting(const std::string& camera, const camera_setting& setting)
{
	if (camera.empty())
	{
		camera_log.error("String '%s' cannot be used as camera key.", camera);
		return;
	}

	cameras.set_value(camera, setting.to_string());
}

std::string cfg_camera::camera_setting::to_string() const
{
	return fmt::format("%d,%d,%f,%f,%d", width, height, min_fps, max_fps, format);
}

void cfg_camera::camera_setting::from_string(const std::string& text)
{
	if (text.empty())
	{
		return;
	}

	const std::vector<std::string> list = fmt::split(text, { "," });

	if (list.size() != member_count)
	{
		camera_log.error("String '%s' cannot be interpreted as camera_setting.", text);
		return;
	}

	const auto to_integer = [](const std::string& str, int& out) -> bool
	{
		auto [ptr, ec] = std::from_chars(str.c_str(), str.c_str() + str.size(), out);
		if (ec != std::errc{})
		{
			camera_log.error("String '%s' cannot be interpreted as integer.", str);
			return false;
		}
		return true;
	};

	const auto to_double = [](const std::string& str, double& out) -> bool
	{
		char* end{};
		out = std::strtod(str.c_str(), &end);
		if (end != str.c_str() + str.size())
		{
			camera_log.error("String '%s' cannot be interpreted as double.", str);
			return false;
		}
		return true;
	};

	if (!to_integer(::at32(list, 0), width) ||
		!to_integer(::at32(list, 1), height) ||
		!to_double(::at32(list, 2), min_fps) ||
		!to_double(::at32(list, 3), max_fps) ||
		!to_integer(::at32(list, 4), format))
	{
		width = 0;
		height = 0;
		min_fps = 0;
		max_fps = 0;
		format = 0;
	}
}
