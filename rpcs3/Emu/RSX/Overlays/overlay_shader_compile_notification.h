#pragma once

#include "overlays.h"

namespace rsx
{
	namespace overlays
	{
		struct shader_compile_notification : user_interface
		{
			label m_text;

			overlay_element dots[3];
			u8 current_dot = 255;

			u64 creation_time = 0;
			u64 expire_time = 0; // Time to end the prompt
			u64 urgency_ctr = 0; // How critical it is to show to the user

			shader_compile_notification();

			void update_animation(u64 t);
			void touch();
			void update() override;

			compiled_resource get_compiled() override;
		};
	}
}
