#include "stdafx.h"
#include "system_config.h"
#include "Utilities/StrUtil.h"
#include "Utilities/sysinfo.h"

cfg_root g_cfg;

bool cfg_root::node_core::has_rtm() const
{
	return utils::has_rtm();
}

std::string cfg_root::node_vfs::get(const cfg::string& _cfg, const char* _def) const
{
	auto [spath, sshared] = _cfg.get();

	if (spath.empty())
	{
		return fs::get_config_dir() + _def;
	}

	auto [semudir, sshared2] = emulator_dir.get();

	return fmt::replace_all(spath, "$(EmulatorDir)", semudir.empty() ? fs::get_config_dir() : semudir);
}
