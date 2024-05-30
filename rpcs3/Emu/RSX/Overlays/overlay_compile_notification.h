#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"

namespace rsx
{
	namespace overlays
	{
		void show_shader_compile_notification();
		std::shared_ptr<atomic_t<u32>> show_ppu_compile_notification();
	}
}
