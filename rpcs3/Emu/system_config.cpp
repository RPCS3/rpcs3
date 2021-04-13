#include "stdafx.h"
#include "system_config.h"
#include "Utilities/StrUtil.h"
#include "Utilities/StrFmt.h"

#include "util/sysinfo.hpp"

cfg_root g_cfg{};

bool cfg_root::node_core::has_rtm()
{
	return utils::has_rtm();
}

std::string cfg_root::node_vfs::get(const cfg::string& _cfg, std::string_view emu_dir) const
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
