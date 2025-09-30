#include "stdafx.h"
#include "system_config.h"

#include "util/sysinfo.hpp"

#include <random>

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

u128 cfg_root::node_sys::get_random_psid()
{
	std::random_device rnd;
	std::mt19937_64 prng(rnd());
	std::uniform_int_distribution<u64> uniformDist;
	u128 result = uniformDist(prng);
	result += u128{uniformDist(prng)} << 64;
	return result;
}
