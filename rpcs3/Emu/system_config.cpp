#include "stdafx.h"
#include "system_config.h"

#include "util/sysinfo.hpp"

cfg_root g_cfg{};
cfg_root g_backup_cfg{};

bool cfg_root::node_core::enable_tsx_by_default()
{
	return utils::has_rtm() && utils::has_mpx() && !utils::has_tsx_force_abort();
}

std::string cfg_root::node_sys::get_random_system_name()
{
	std::srand(static_cast<u32>(std::time(nullptr)));
	return "RPCS3-" + std::to_string(100 + std::rand() % 899);
}
