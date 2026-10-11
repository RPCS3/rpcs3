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

cfg_camera::camera_setting cfg_camera::get_camera_setting(std::string_view handler, std::string_view camera, bool& success)
{
	camera_setting setting {};
	const std::string value = cameras.get_value(fmt::format("%s-%s", handler, camera));
	success = !value.empty();
	if (success)
	{
		setting.from_string(value);
	}
	return setting;
}

void cfg_camera::set_camera_setting(std::string_view handler, std::string_view camera, const camera_setting& setting)
{
	if (handler.empty())
	{
		camera_log.error("String '%s' cannot be used as handler key.", handler);
		return;
	}

	if (camera.empty())
	{
		camera_log.error("String '%s' cannot be used as camera key.", camera);
		return;
	}

	cameras.set_value(fmt::format("%s-%s", handler, camera), setting.to_string());
}

std::string cfg_camera::camera_setting::to_string() const
{
	return fmt::format("%d,%d,%f,%f,%d,%d", width, height, min_fps, max_fps, format, colorspace);
}

void cfg_camera::camera_setting::from_string(std::string_view text)
{
	if (text.empty())
	{
		return;
	}

	const std::vector<std::string_view> list = fmt::split_sv(text, { "," });

	if (list.size() != member_count)
	{
		camera_log.error("String '%s' cannot be interpreted as camera_setting.", text);
		return;
	}

	const auto to_integer = [](std::string_view str, int& out) -> bool
	{
		if (str.empty())
		{
			camera_log.error("Empty string cannot be interpreted as integer.");
			return false;
		}

		auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), out);
		if (ec != std::errc{})
		{
			camera_log.error("String '%s' cannot be interpreted as integer.", str);
			return false;
		}
		return true;
	};

	const auto to_double = [](std::string_view str, double& out) -> bool
	{
		if (str.empty())
		{
			camera_log.error("Empty string cannot be interpreted as double.");
			return false;
		}

		char* end{};
		out = std::strtod(str.data(), &end);
		if (end != str.data() + str.size())
		{
			camera_log.error("String '%s' cannot be interpreted as double.", str);
			return false;
		}
		return true;
	};

	usz pos = 0;
	if (!to_integer(::at32(list, pos++), width) ||
		!to_integer(::at32(list, pos++), height) ||
		!to_double(::at32(list, pos++), min_fps) ||
		!to_double(::at32(list, pos++), max_fps) ||
		!to_integer(::at32(list, pos++), format) ||
		!to_integer(::at32(list, pos++), colorspace))
	{
		width = 0;
		height = 0;
		min_fps = 0;
		max_fps = 0;
		format = 0;
		colorspace = 0;
	}
}
