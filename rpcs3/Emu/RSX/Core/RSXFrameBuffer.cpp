#include "stdafx.h"
#include "RSXFrameBuffer.h"

namespace rsx
{
	u8 framebuffer_layout::color_attachment_count() const
	{
		int surface_count = 0;
		switch (target)
		{
		default:
		case rsx::surface_target::none:
			return 0;
		case rsx::surface_target::surface_a:
			return actual_color_pitch[0] ? 1 : 0;
		case rsx::surface_target::surface_b:
			return actual_color_pitch[1] ? 1 : 0;
		case rsx::surface_target::surfaces_a_b:
			surface_count = 2; break;
		case rsx::surface_target::surfaces_a_b_c:
			surface_count = 3; break;
		case rsx::surface_target::surfaces_a_b_c_d:
			surface_count = 4; break;
		}

		u8 result = 0;
		for (int index = 0; index < surface_count; ++index)
		{
			if (actual_color_pitch[index]) result++;
		}
		return result;
	}
}
