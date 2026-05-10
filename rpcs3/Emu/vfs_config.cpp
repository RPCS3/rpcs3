#include "stdafx.h"
#include "vfs_config.h"
#include "Utilities/StrUtil.h"

LOG_CHANNEL(vfs_log, "VFS");

cfg_vfs g_cfg_vfs{};

std::string cfg_vfs::get(const cfg::string& _cfg, std::string_view emu_dir) const
{
	return get(_cfg.to_string(), _cfg.def, emu_dir);
}

std::string cfg_vfs::get(const std::string& _cfg, const std::string& def, std::string_view emu_dir) const
{
	std::string path = _cfg;

	// Fallback
	if (path.empty())
	{
		if (def.empty())
		{
			vfs_log.notice("VFS config called with empty path and empty default");
			return {};
		}

		path = def;
	}

	std::string _emu_dir; // Storage only

	if (emu_dir.empty())
	{
		// Optimization if provided arg
		_emu_dir = emulator_dir;

		if (_emu_dir.empty())
		{
			_emu_dir = fs::get_config_dir() + '/';
		}
		// Check if path does not end with a delimiter
		else if (_emu_dir.back() != fs::delim[0] && _emu_dir.back() != fs::delim[1])
		{
			_emu_dir += '/';
		}

		emu_dir = _emu_dir;
	}

	path = fmt::replace_all(path, "$(EmulatorDir)", emu_dir);

	// Check if path does not end with a delimiter
	if (path.empty())
	{
		vfs_log.error("VFS config path empty (_cfg='%s', def='%s', emu_dir='%s')", _cfg, def, emu_dir);
	}
	else if (path.back() != fs::delim[0] && path.back() != fs::delim[1])
	{
		path += '/';
	}

	return path;
}

cfg::device_info cfg_vfs::get_device(const cfg::device_entry& _cfg, std::string_view key, std::string_view emu_dir) const
{
	return get_device_info(_cfg, key, emu_dir);
}

cfg::device_info cfg_vfs::get_device_info(const cfg::device_entry& _cfg, std::string_view key, std::string_view emu_dir) const
{
	const auto& device_map = _cfg.get_map();

	if (auto it = device_map.find(key); it != device_map.cend())
	{
		// Make sure the path is properly resolved
		cfg::device_info info = it->second;
		const auto& def_map = _cfg.get_default();
		std::string def_path;

		if (auto def_it = def_map.find(key); def_it != def_map.cend())
		{
			def_path = def_it->second.path;
		}

		if (info.path.empty() && def_path.empty())
		{
			return info;
		}

		info.path = get(info.path, def_path, emu_dir);
		return info;
	}

	return {};
}

void cfg_vfs::load()
{
	const std::string path = cfg_vfs::get_path();

	if (fs::file cfg_file{path, fs::read})
	{
		vfs_log.notice("Loading VFS config. Path: %s", path);
		from_string(cfg_file.to_string());
	}
	else
	{
		vfs_log.notice("VFS config missing. Using default settings. Path: %s", path);
		from_default();
	}
}

void cfg_vfs::save() const
{
#ifdef _WIN32
	const std::string path_to_cfg = fs::get_config_dir(true);
	if (!fs::create_path(path_to_cfg))
	{
		vfs_log.error("Could not create path: %s", path_to_cfg);
	}
#endif

	fs::pending_file temp(cfg_vfs::get_path());

	if (!temp.file)
	{
		vfs_log.error("Could not save config: \"%s\" (error=%s)", cfg_vfs::get_path(), fs::g_tls_error);
		return;
	}

	temp.file.write(to_string());

	if (!temp.commit())
	{
		vfs_log.error("Could not save config: \"%s\" (error=%s)", cfg_vfs::get_path(), fs::g_tls_error);
	}
}

std::string cfg_vfs::get_path()
{
	return fs::get_config_dir(true) + "vfs.yml";
}
