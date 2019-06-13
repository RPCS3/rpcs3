#include "stdafx.h"
#include "overlays.h"

namespace rsx
{
	namespace overlays
	{
		trophy_notification::trophy_notification()
		{
			frame.set_pos(0, 0);
			frame.set_size(260, 80);
			frame.back_color.a = 0.85f;

			image.set_pos(8, 8);
			image.set_size(64, 64);
			image.back_color.a = 0.f;

			text_view.set_pos(85, 0);
			text_view.set_padding(0, 0, 24, 0);
			text_view.set_font("Arial", 8);
			text_view.align_text(overlay_element::text_align::center);
			text_view.back_color.a = 0.f;
		}
		void trophy_notification::update()
		{
			u64 t = get_system_time();
			if (((t - creation_time) / 1000) > 7500)
				close();
		}

		compiled_resource trophy_notification::get_compiled()
		{
			auto result = frame.get_compiled();
			result.add(image.get_compiled());
			result.add(text_view.get_compiled());

			return result;
		}

		s32 trophy_notification::show(const SceNpTrophyDetails& trophy, const std::vector<uchar>& trophy_icon_buffer)
		{
			if (!trophy_icon_buffer.empty())
			{
				icon_info = std::make_unique<image_info>(trophy_icon_buffer);
				image.set_raw_image(icon_info.get());
			}

			std::string trophy_message;
			switch (trophy.trophyGrade)
			{
			case SCE_NP_TROPHY_GRADE_BRONZE: trophy_message = "bronze"; break;
			case SCE_NP_TROPHY_GRADE_SILVER: trophy_message = "silver"; break;
			case SCE_NP_TROPHY_GRADE_GOLD: trophy_message = "gold"; break;
			case SCE_NP_TROPHY_GRADE_PLATINUM: trophy_message = "platinum"; break;
			default: break;
			}

			trophy_message = "You have earned the " + trophy_message + " trophy\n" + utf8_to_ascii8(trophy.name);
			text_view.set_text(trophy_message);
			text_view.auto_resize();

			// Resize background to cover the text
			u16 margin_sz = text_view.x - image.w - image.x;
			frame.w       = text_view.x + text_view.w + margin_sz;

			creation_time = get_system_time();
			return CELL_OK;
		}
	} // namespace overlays
} // namespace rsx
