#pragma once

// PS5: a trophy's pop-up, drawn as the launcher draws: a card of deep blue
// glass that slides in at the top right with the trophy's icon, its name and
// its grade in the grade's colour, to the title's trophy sound; one at a time,
// in the order they are earned. In place of RPCS3's own trophy_notification,
// whose plain grey box this redraws.

#include "Emu/RSX/Overlays/overlays.h"
#include "Emu/Cell/Modules/sceNpTrophy.h"

#include <memory>
#include <string>
#include <vector>

namespace rsx::overlays
{
	struct ps5_trophy_popup : public user_interface
	{
		ps5_trophy_popup();

		void update(u64 timestamp_us) override;
		compiled_resource get_compiled() override;

		s32 show(const SceNpTrophyDetails& trophy, const std::vector<uchar>& icon_buffer);

	private:
		u64 m_ticket = 0;   // its turn among the pop-ups
		u64 m_start_us = 0; // when its turn came
		u64 m_now_us = 0;
		bool m_released = false;
		std::string m_name;
		u8 m_grade = 4;     // as ps5ui::trophy_grade_colour
		std::unique_ptr<image_info> m_icon;
		std::unique_ptr<image_info> m_cup;
		std::vector<u8> m_glow_pixels;
		std::unique_ptr<memory_image_info> m_glow;
	};
}
