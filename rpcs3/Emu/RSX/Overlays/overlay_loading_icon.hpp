#pragma once

#include "overlay_animated_icon.h"

namespace rsx
{
	namespace overlays
	{
		struct loading_icon24 : public animated_icon
		{
			loading_icon24()
				: animated_icon("spinner-24.png")
			{
				m_frame_width = m_frame_height = 24;
				m_spacing_x = m_spacing_y = 6;

				set_size(24, 30);
				set_padding(4, 0, 2, 8);
			}
		};
	}
}

