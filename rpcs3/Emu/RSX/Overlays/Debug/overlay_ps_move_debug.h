#pragma once

#include "../overlays.h"
#include "Emu/Io/pad_types.h"

namespace rsx
{
	namespace overlays
	{
		struct ps_move_debug_overlay : public user_interface
		{
		public:
			ps_move_debug_overlay();

			compiled_resource get_compiled() override;

			void show(const ps_move_data& md, f32 r, f32 g, f32 b, f32 q0, f32 q1, f32 q2, f32 q3);

		private:
			overlay_element m_frame;
			label m_text_view;

			ps_move_data m_move_data {};
			std::array<f32, 3> m_rgb {};
			std::array<f32, 4> m_quaternion {};
		};
	}
}
