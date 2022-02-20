#include "stdafx.h"
#include "vfs_config.h"
#include "Utilities/StrUtil.h"
#include "Utilities/StrFmt.h"
#include "util/yaml.hpp"

LOG_CHANNEL(vfs_log, "VFS");

cfg_vfs g_cfg_vfs{};

std::string cfg_vfs::get(const cfg::string& _cfg, std::string_view emu_dir) const
{
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

	std::string path = _cfg.to_string();

	if (path.empty())
	{
		// Fallback
		path = _cfg.def;
	}

	path = fmt::replace_all(path, "$(EmulatorDir)", emu_dir);

	// Check if path does not end with a delimiter
	if (path.back() != fs::delim[0] && path.back() != fs::delim[1])
	{
		path += '/';
	}

	return path;
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
	const std::string path_to_cfg = fs::get_config_dir() + "config/";
	if (!fs::create_path(path_to_cfg))
	{
		vfs_log.error("Could not create path: %s", path_to_cfg);
	}
#endif

	fs::pending_file temp(cfg_vfs::get_path());

	if (!temp.file)
	{
		vfs_log.error("Could not save config: \"%s\"", cfg_vfs::get_path());
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
#ifdef _WIN32
	return fs::get_config_dir() + "config/vfs.yml";
#else
	return fs::get_config_dir() + "vfs.yml";
#endif
}
