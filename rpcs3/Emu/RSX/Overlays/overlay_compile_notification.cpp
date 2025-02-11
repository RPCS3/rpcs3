#include "stdafx.h"
#include "overlay_message.h"
#include "overlay_loading_icon.hpp"

namespace rsx
{
	namespace overlays
	{
		static std::shared_ptr<loading_icon24> s_shader_loading_icon24;
		static std::shared_ptr<loading_icon24> s_ppu_loading_icon24;

		void show_shader_compile_notification()
		{
			if (!s_shader_loading_icon24)
			{
				// Creating the icon requires FS read, so it is important to cache it
				s_shader_loading_icon24 = std::make_shared<loading_icon24>();
			}

			queue_message(
				localized_string_id::RSX_OVERLAYS_COMPILING_SHADERS,
				5'000'000,
				{},
				message_pin_location::bottom_left,
				s_shader_loading_icon24,
				true);
		}

		std::shared_ptr<atomic_t<u32>> show_ppu_compile_notification()
		{
			if (!s_ppu_loading_icon24)
			{
				// Creating the icon requires FS read, so it is important to cache it
				s_ppu_loading_icon24 = std::make_shared<loading_icon24>();
			}

			std::shared_ptr<atomic_t<u32>> refs = std::make_shared<atomic_t<u32>>(1);

			queue_message(
				localized_string_id::RSX_OVERLAYS_COMPILING_PPU_MODULES,
				20'000'000,
				refs,
				message_pin_location::bottom_left,
				s_ppu_loading_icon24,
				true);

			return refs;
		}
	}
}
