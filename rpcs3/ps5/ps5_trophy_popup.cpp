// PS5: a trophy's pop-up (ps5_trophy_popup.h).

#include "stdafx.h"
#include "ps5_trophy_popup.h"
#include "ps5_ui.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/RSX/Overlays/overlay_manager.h"

namespace rsx::overlays
{
	using namespace ps5ui;

	namespace
	{
		// One at a time: each takes a ticket as it is shown, and waits for its turn
		class popup_tickets
		{
			atomic_t<u64> m_taken{0};
			atomic_t<u64> m_done{0};

		public:
			u64 take() { return m_taken.fetch_add(1); }
			bool turn(u64 ticket) const { return ticket == m_done.load(); }
			void done() { m_done++; }
		};

		popup_tickets s_tickets;

		// The card, at the top right of the 1280x720 space
		constexpr u16 c_w = 420;
		constexpr u16 c_h = 84;
		constexpr s16 c_right = 40;
		constexpr s16 c_top = 40;
		constexpr u16 c_icon = 60;

		// Its timeline, in seconds from its turn
		constexpr f32 c_in = 0.45f;
		constexpr f32 c_hold = 5.5f;
		constexpr f32 c_out = 0.35f;

		const char* grade_name(u8 grade)
		{
			static const char* names[] = {"Platinum", "Gold", "Silver", "Bronze", "Trophy"};
			return names[std::min<u8>(grade, 4)];
		}
	}

	ps5_trophy_popup::ps5_trophy_popup()
	{
		visible = false;
	}

	void ps5_trophy_popup::update(u64 timestamp_us)
	{
		m_now_us = timestamp_us;
		if (!s_tickets.turn(m_ticket))
		{
			return;
		}
		if (!m_start_us)
		{
			m_start_us = timestamp_us;
			play_sound(sound_effect::trophy);
			return;
		}
		if (!m_released && (timestamp_us - m_start_us) / 1'000'000.f > c_hold + c_out + 0.05f)
		{
			m_released = true;
			s_tickets.done();
			close(false, false);
		}
	}

	compiled_resource ps5_trophy_popup::get_compiled()
	{
		if (!m_start_us || !visible)
		{
			return {};
		}

		const f32 t = (m_now_us - m_start_us) / 1'000'000.f;
		const f32 in = ease_out(progress(t, 0.f, c_in));
		const f32 out = ease_in_out(progress(t, c_hold, c_out));
		const f32 alpha = in * (1.f - out);
		if (alpha <= 0.f)
		{
			return {};
		}

		compiled_resource card;
		const s16 x = static_cast<s16>(virtual_width - c_right - c_w);
		const s16 y = c_top;
		const color4f& grade = trophy_grade_colour(m_grade);

		// A soft light behind, of the grade's colour
		if (m_glow)
		{
			image_view glow;
			glow.set_raw_image(m_glow.get());
			glow.back_color.a = 0.f;
			glow.fore_color = color4f(grade.r, grade.g, grade.b, 0.22f);
			glow.set_pos(static_cast<s16>(x - 50), static_cast<s16>(y - 40));
			glow.set_size(static_cast<u16>(c_w + 100), static_cast<u16>(c_h + 80));
			card.add(glow.get_compiled());
		}

		rounded_rect panel;
		panel.set_pos(x, y);
		panel.set_size(c_w, c_h);
		panel.border_radius = 18;
		panel.back_color = color4f(0.035f, 0.06f, 0.20f, 0.95f);
		panel.border_size = 1;
		panel.border_color = color4f(grade.r, grade.g, grade.b, 0.55f);
		card.add(panel.get_compiled());

		const s16 icon_x = static_cast<s16>(x + 12);
		const s16 icon_y = static_cast<s16>(y + (c_h - c_icon) / 2);
		if (m_icon)
		{
			rounded_image icon;
			icon.set_raw_image(m_icon.get());
			icon.back_color.a = 0.f;
			icon.border_radius = 10;
			icon.set_pos(icon_x, icon_y);
			icon.set_size(c_icon, c_icon);
			card.add(icon.get_compiled());
		}
		else
		{
			rounded_rect blank;
			blank.set_pos(icon_x, icon_y);
			blank.set_size(c_icon, c_icon);
			blank.border_radius = 10;
			blank.back_color = color4f(1.f, 1.f, 1.f, 0.08f);
			card.add(blank.get_compiled());
		}

		const s16 text_x = static_cast<s16>(icon_x + c_icon + 14);
		const u16 text_w = static_cast<u16>(x + c_w - 16 - text_x);

		auto kicker = make_label(spaced("TROPHY EARNED"), 9, f_semibold, c_accent);
		place(*kicker, text_x, y + 22.f);
		card.add(kicker->get_compiled());

		auto name = make_label("", 15, f_semibold, c_text);
		fit_text(*name, m_name, text_w);
		place(*name, text_x, y + 43.f);
		card.add(name->get_compiled());

		s16 grade_x = text_x;
		if (m_cup)
		{
			image_view cup;
			cup.set_raw_image(m_cup.get());
			cup.back_color.a = 0.f;
			cup.fore_color = grade;
			cup.set_pos(text_x, static_cast<s16>(y + 57));
			cup.set_size(14, 14);
			card.add(cup.get_compiled());
			grade_x = static_cast<s16>(text_x + 20);
		}
		auto grade_label = make_label(grade_name(m_grade), 11, f_medium, grade);
		place(*grade_label, grade_x, y + 64.f);
		card.add(grade_label->get_compiled());

		compiled_resource result;
		add_animated(result, card, alpha, (1.f - in) * 60.f + out * 60.f, 0.f);
		return result;
	}

	s32 ps5_trophy_popup::show(const SceNpTrophyDetails& trophy, const std::vector<uchar>& icon_buffer)
	{
		m_ticket = s_tickets.take();
		m_name = trophy.name;
		const u32 grade = trophy.trophyGrade;
		m_grade = grade >= SCE_NP_TROPHY_GRADE_PLATINUM && grade <= SCE_NP_TROPHY_GRADE_BRONZE ? static_cast<u8>(grade - SCE_NP_TROPHY_GRADE_PLATINUM) : u8{4};
		if (!icon_buffer.empty())
		{
			m_icon = std::make_unique<image_info>(icon_buffer);
			if (m_icon->get_data())
			{
				m_icon->dirty = true;
			}
			else
			{
				m_icon.reset();
			}
		}
		m_cup = resource_config::load_icon("home/32/trophy-solid.png");
		if (m_cup)
		{
			m_cup->dirty = true;
		}
		m_glow = make_glow(m_glow_pixels);
		visible = true;
		return CELL_OK;
	}
}
