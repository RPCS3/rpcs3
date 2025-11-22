#include "stdafx.h"
#include "system_config.h"

#include "util/sysinfo.hpp"

#include <random>

cfg_root g_cfg{};
cfg_root g_backup_cfg{};

std::string cfg_root::node_sys::get_random_system_name()
{
	std::srand(static_cast<u32>(std::time(nullptr)));
	return "RPCS3-" + std::to_string(100 + std::rand() % 899);
}

u128 cfg_root::node_sys::get_random_psid()
{
	// Generate seed for each 32-bits for maximum entropy using Standard Library
	std::uniform_int_distribution<u32> uniform_dist;
	std::mt19937 prng;

	u128 result{};
	prng.seed(std::random_device()()), result += u128{uniform_dist(prng)} << 0;
	prng.seed(std::random_device()()), result += u128{uniform_dist(prng)} << 32;
	prng.seed(std::random_device()()), result += u128{uniform_dist(prng)} << 64;
	prng.seed(std::random_device()()), result += u128{uniform_dist(prng)} << 96;

	return result;
}
