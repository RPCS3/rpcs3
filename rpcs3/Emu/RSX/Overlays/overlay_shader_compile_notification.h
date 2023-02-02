#pragma once

#include "overlays.h"
#include "overlay_message.h"
#include "overlay_loading_icon.hpp"

namespace rsx
{
	namespace overlays
	{
		static inline void show_shader_compile_notification()
		{
			queue_message(
				localized_string_id::RSX_OVERLAYS_COMPILING_SHADERS,
				15'000'000,
				{},
				message_pin_location::bottom,
				std::make_unique<loading_icon24>());
		}
	}
}
