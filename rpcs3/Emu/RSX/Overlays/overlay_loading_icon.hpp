#pragma once

#include "overlay_animated_icon.h"

namespace rsx
{
	namespace overlays
	{
		class loading_icon24 : public animated_icon
		{
		public:
			loading_icon24()
				: animated_icon("spinner-24.png")
			{
				init_params();
			}

			loading_icon24(const std::vector<u8>& icon_data)
				: animated_icon(icon_data)
			{
				init_params();
			}

		private:
			void init_params()
			{
				m_frame_width = m_frame_height = 24;
				m_spacing_x = m_spacing_y = 6;

				set_size(24, 30);
				set_padding(4, 0, 2, 8);
			}
		};
	}
}

