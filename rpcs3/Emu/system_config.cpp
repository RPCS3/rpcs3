#include "stdafx.h"
#include "system_config.h"

#include "util/sysinfo.hpp"

cfg_root g_cfg{};
cfg_root g_backup_cfg{};

bool cfg_root::node_core::enable_tsx_by_default()
{
	return utils::has_rtm() && utils::has_mpx() && !utils::has_tsx_force_abort();
}
