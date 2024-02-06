#pragma once

#include "overlays.h"
#include "Emu/Cell/Modules/sceNpTrophy.h"

namespace rsx
{
	namespace overlays
	{
		struct trophy_notification : public user_interface
		{
		private:
			overlay_element frame;
			image_view image;
			label text_view;

			u64 display_sched_id = 0;
			u64 creation_time_us = 0;
			std::unique_ptr<image_info> icon_info;

			animation_translate sliding_animation;
			animation_color_interpolate fade_animation;

		public:
			trophy_notification();

			void update(u64 timestamp_us) override;

			compiled_resource get_compiled() override;

			s32 show(const SceNpTrophyDetails& trophy, const std::vector<uchar>& trophy_icon_buffer);
		};
	}
}
