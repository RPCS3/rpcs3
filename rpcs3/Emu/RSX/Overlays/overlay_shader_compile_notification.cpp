#include "stdafx.h"
#include "overlays.h"
#include "overlay_message.h"
#include "overlay_loading_icon.hpp"

namespace rsx
{
	namespace overlays
	{
		static std::shared_ptr<loading_icon24> s_loading_icon24;

		void show_shader_compile_notification()
		{
			if (!s_loading_icon24)
			{
				// Creating the icon requires FS read, so it is important to cache it
				s_loading_icon24 = std::make_shared<loading_icon24>();
			}

			queue_message(
				localized_string_id::RSX_OVERLAYS_COMPILING_SHADERS,
				5'000'000,
				{},
				message_pin_location::bottom,
				s_loading_icon24);
		}
	}
}
