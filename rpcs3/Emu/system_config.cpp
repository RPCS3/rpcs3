#include "stdafx.h"
#include "system_config.h"

#include "util/sysinfo.hpp"

cfg_root g_cfg{};

bool cfg_root::node_core::has_rtm()
{
	return utils::has_rtm();
}
