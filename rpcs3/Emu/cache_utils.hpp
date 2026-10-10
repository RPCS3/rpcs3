#pragma once

namespace rpcs3::cache
{
	std::string get_ppu_cache();
	std::string get_shader_cache();
	void limit_cache_size();
}
