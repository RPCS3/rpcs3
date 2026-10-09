// PS5: the menu a game pauses into (ps5_pause_menu.h).
//
// Laid out in RSX's virtual 1280x720 space, in the launcher's design
// (ps5_ui.h): its palette, fonts, glows and pills.

#include "stdafx.h"
#include "ps5_pause_menu.h"

#include "Emu/RSX/Overlays/overlay_manager.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Utilities/Config.h"

#include <algorithm>
#include <cmath>

LOG_CHANNEL(pause_log, "PauseMenu");

namespace rsx::overlays
{
	extern void reset_performance_overlay(); // overlay_perf_metrics.cpp

	using namespace ps5ui;

	namespace
	{
		// The card, in the middle of the screen
		constexpr s16 c_card_x = 190;
		constexpr s16 c_card_y = 52;
		constexpr u16 c_card_w = 900;
		constexpr u16 c_card_h = 616;
		constexpr s16 c_left = c_card_x + 40;
		constexpr s16 c_inner_right = c_card_x + c_card_w - 40;

		// The tabs' line, the rows under it, and the words under them
		constexpr f32 c_tabs_mid = c_card_y + 146.f;
		constexpr s16 c_rows_top = c_card_y + 186;
		constexpr f32 c_row_h = 38.f;
		constexpr s32 c_visible_rows = 8;
		constexpr s16 c_help_top = c_rows_top + static_cast<s16>(c_visible_rows * c_row_h) + 14;
		constexpr f32 c_hints_mid = c_card_y + c_card_h - 30.f;

		// The card rises in over this, and sinks away faster
		constexpr f32 c_open_seconds = 0.2f;
		constexpr f32 c_close_seconds = 0.14f;

		// What the Game tab offers of the settings: those a player reaches for mid-game
		struct quick_setting
		{
			const char* section;
			const char* key;
			const char* label;
		};
		constexpr quick_setting c_quick_settings[]
		{
			{"Video/Performance Overlay", "Enabled", "Performance overlay"},
			{"Video", "Frame limit", "Frame limit"},
			{"Video", "Vblank Rate", "Vblank rate"},
			{"Video", "Frame Generation", "Frame generation"},
			{"Video", "Frame Generation Frames", "Frames made between"},
			{"Video", "Frame Generation Quality", "Frame generation quality"},
			{"Video", "Frame Generation Step Aside", "Frame generation steps aside"},
			{"Audio", "Master Volume", "Volume"},
		};

		// A tooltip's first paragraph, short enough for the lines under the rows
		std::string short_help(std::string_view help)
		{
			std::string text(help.substr(0, help.find('\n')));
			if (text.size() > 230)
			{
				text.resize(text.rfind(' ', 226));
				text += "…";
			}
			return text;
		}
	}

	ps5_pause_menu::ps5_pause_menu()
	{
		m_allow_input_on_pause = true;
		return_code = selection_code::canceled;

		m_serial = Emu.GetTitleID();
		m_title = Emu.GetTitle().empty() ? m_serial : Emu.GetTitle();

		// The game's icon: an update's (the game's folder on the hard disk), else the disc's
		for (const bool disc : {false, true})
		{
			if (!m_icon)
			{
				if (const std::string dir = Emu.GetSfoDir(disc); !dir.empty())
				{
					m_icon = load_image(dir + "/ICON0.PNG");
				}
			}
		}

		m_glow = make_glow(m_glow_pixels);

		// The game's settings as it runs with them, a tab for each of the
		// desktop dialog's tabs
		m_settings = read_game_settings(m_serial, true);

		tab game;
		game.name = "Game";
		game.rows.push_back({action::resume, -1, "Resume", "Back to the game", "home/32/play-button-arrowhead.png", false});
		for (const quick_setting& quick : c_quick_settings)
		{
			for (usz i = 0; i < m_settings.size(); i++)
			{
				if (!m_settings[i].heading && m_settings[i].section == quick.section && m_settings[i].key == quick.key)
				{
					game.rows.push_back({action::none, static_cast<s32>(i), quick.label, "", nullptr, false});
					break;
				}
			}
		}
		game.rows.push_back({action::restart, -1, "Restart game", "Starts it again, with the settings changed here", "home/32/rotate-left-solid.png", true});
		game.rows.push_back({action::quit, -1, "Quit to library", "Ends the game and goes back to your games", "home/32/circle-left-solid.png", false});
		m_tabs.push_back(std::move(game));

		for (usz i = 0; i < m_settings.size(); i++)
		{
			if (m_settings[i].heading)
			{
				m_tabs.push_back({m_settings[i].label, {}});
			}
			else if (m_tabs.size() > 1)
			{
				m_tabs.back().rows.push_back({action::none, static_cast<s32>(i), m_settings[i].label, "", nullptr, false});
			}
		}
		std::erase_if(m_tabs, [](const tab& each) { return each.rows.empty(); });
	}

	void ps5_pause_menu::move_to(s32 next)
	{
		const s32 count = static_cast<s32>(m_tabs[m_tab].rows.size());
		next = std::clamp(next, 0, count - 1);
		if (next != m_row)
		{
			m_row = next;
			play_sound(sound_effect::cursor);
		}
	}

	void ps5_pause_menu::change(s32 direction)
	{
		const row& current = m_tabs[m_tab].rows[m_row];
		if (current.setting < 0)
		{
			return;
		}
		setting_row& setting = m_settings[current.setting];
		const auto it = std::find(setting.options.begin(), setting.options.end(), setting.value);
		const s32 at = static_cast<s32>(it - setting.options.begin());
		const s32 count = static_cast<s32>(setting.options.size());
		if (count < 2)
		{
			return;
		}
		setting.value = setting.options[(at + (direction < 0 ? count - 1 : 1)) % count];
		m_changed = true;
		apply_live(setting);
		play_sound(sound_effect::cursor);
	}

	void ps5_pause_menu::apply_live(const setting_row& setting)
	{
		// What the emulator reads as it runs takes the value now; the rest wait
		// for the game's next start, from its custom config
		cfg::_base* live = find_setting(g_cfg, setting.section, setting.key);
		if (!live || !live->get_is_dynamic())
		{
			return;
		}
		if (!live->from_string(setting.value))
		{
			pause_log.error("Could not set %s/%s to %s", setting.section, setting.key, setting.value);
			return;
		}
		pause_log.notice("%s/%s = %s, from now", setting.section, setting.key, setting.value);
		if (setting.section.starts_with("Video/Performance Overlay"))
		{
			reset_performance_overlay();
		}
	}

	void ps5_pause_menu::on_button_pressed(pad_button button_press, bool is_auto_repeat)
	{
		std::lock_guard lock(m_mutex);
		if (m_closing || m_tabs.empty())
		{
			return;
		}

		// Values change one step at a time even when held
		const bool left = button_press == pad_button::dpad_left || button_press == pad_button::ls_left;
		const bool right = button_press == pad_button::dpad_right || button_press == pad_button::ls_right;
		m_auto_repeat_ms_interval = (left || right) ? 150 : m_auto_repeat_ms_interval_default;

		switch (button_press)
		{
		case pad_button::L1:
		case pad_button::R1:
		{
			if (is_auto_repeat)
			{
				return;
			}
			const s32 count = static_cast<s32>(m_tabs.size());
			m_tab = (m_tab + (button_press == pad_button::L1 ? count - 1 : 1)) % count;
			m_row = 0;
			m_scroll = 0;
			play_sound(sound_effect::cursor);
			return;
		}
		case pad_button::dpad_up:
		case pad_button::ls_up:
			move_to(m_row - 1);
			return;
		case pad_button::dpad_down:
		case pad_button::ls_down:
			move_to(m_row + 1);
			return;
		case pad_button::cross:
		{
			if (is_auto_repeat)
			{
				return;
			}
			const row& current = m_tabs[m_tab].rows[m_row];
			if (current.act != action::none)
			{
				play_sound(current.act == action::resume ? sound_effect::cancel : sound_effect::accept);
				begin_close(current.act);
			}
			else
			{
				change(1);
			}
			return;
		}
		case pad_button::square:
		{
			const row& current = m_tabs[m_tab].rows[m_row];
			if (current.setting >= 0)
			{
				setting_row& setting = m_settings[current.setting];
				if (setting.value != setting.global)
				{
					setting.value = setting.global;
					m_changed = true;
					apply_live(setting);
					play_sound(sound_effect::cancel);
				}
			}
			return;
		}
		case pad_button::circle:
		case pad_button::start:
		case pad_button::ps:
			if (!is_auto_repeat)
			{
				play_sound(sound_effect::cancel);
				begin_close(action::resume);
			}
			return;
		default:
			break;
		}

		if (left || right)
		{
			change(left ? -1 : 1);
		}
	}

	void ps5_pause_menu::begin_close(action then)
	{
		m_closing = true;
		m_close_us = m_now_us;
		m_then = then;
	}

	void ps5_pause_menu::update(u64 timestamp_us)
	{
		{
			std::lock_guard lock(m_mutex);
			m_now_us = timestamp_us;
			if (!m_open_us)
			{
				m_open_us = timestamp_us;
			}
			if (!m_closing || m_closed)
			{
				return;
			}
			if (!m_close_us)
			{
				m_close_us = timestamp_us;
			}
			if ((timestamp_us - m_close_us) / 1'000'000.f < c_close_seconds)
			{
				return;
			}
			m_closed = true;
		}
		finish_close();
	}

	void ps5_pause_menu::finish_close()
	{
		// The changes are the game's own from now on
		if (m_changed && !m_serial.empty())
		{
			write_game_settings(m_serial, m_settings);
		}

		close(true, true);

		switch (m_then)
		{
		case action::restart:
			pause_log.notice("Restarting the game");
			Emu.CallFromMainThread([]()
			{
				// The window stays, and the game boots again with its custom config
				Emu.SetContinuousMode(true);
				Emu.Restart(true);
			});
			break;
		case action::quit:
			pause_log.notice("Quitting to the library");
			Emu.CallFromMainThread([]()
			{
				Emu.GracefulShutdown(true, true);
			});
			break;
		default:
			pause_log.notice("Back to the game");
			break;
		}
	}

	compiled_resource ps5_pause_menu::get_compiled()
	{
		if (!visible)
		{
			return {};
		}

		std::lock_guard lock(m_mutex);
		compiled_resource result;

		const f32 opened = m_open_us ? ease_out(progress((m_now_us - m_open_us) / 1'000'000.f, 0.f, c_open_seconds)) : 0.f;
		const f32 closed = m_closing && m_close_us ? ease_out(progress((m_now_us - m_close_us) / 1'000'000.f, 0.f, c_close_seconds)) : 0.f;
		const f32 shown = opened * (1.f - closed);

		// The game, dimmed and washed blue
		{
			overlay_element dim;
			dim.set_size(virtual_width, virtual_height);
			dim.back_color = color4f(0.f, 0.01f, 0.05f, 0.62f * shown);
			result.add(dim.get_compiled());
		}

		compiled_resource card;
		const auto glow = [&](s16 cx, s16 cy, u16 w, u16 h, const color4f& color)
		{
			image_view view;
			view.set_raw_image(m_glow.get());
			view.back_color.a = 0.f;
			view.fore_color = color;
			view.set_pos(static_cast<s16>(cx - w / 2), static_cast<s16>(cy - h / 2));
			view.set_size(w, h);
			card.add(view.get_compiled());
		};
		const auto text = [&](std::string_view s, u16 size, std::string_view font_name, const color4f& colour, s16 x, f32 mid) -> std::unique_ptr<label>
		{
			auto l = make_label(s, size, font_name, colour);
			place(*l, x, mid);
			card.add(l->get_compiled());
			return l;
		};
		const auto icon = [&](const char* path, s16 x, s16 y, u16 size, const color4f& colour)
		{
			if (!m_icons.contains(path))
			{
				auto& kept = m_icons[path];
				if ((kept = resource_config::load_icon(path)))
				{
					kept->dirty = true;
				}
			}
			if (const auto it = m_icons.find(path); it != m_icons.end() && it->second)
			{
				image_view view;
				view.set_raw_image(it->second.get());
				view.set_size(size, size);
				view.set_pos(x, y);
				view.back_color.a = 0.f;
				view.fore_color = colour;
				card.add(view.get_compiled());
			}
		};
		const auto glyph = [&](u8 resource, s16 x, f32 mid)
		{
			image_view view;
			view.set_image_resource(resource);
			view.set_size(20, 20);
			view.set_pos(x, static_cast<s16>(std::lround(mid - 10.f)));
			view.back_color.a = 0.f;
			card.add(view.get_compiled());
		};

		// The glass: a blue glow behind, a rim of cyan light, the deep blue
		// card, and the launcher's cyan edge along its top
		glow(c_card_x + c_card_w / 2, c_card_y + c_card_h / 2, 1700, 1150, color4f(0.1f, 0.22f, 0.7f, 0.5f));
		glow(c_card_x + c_card_w / 2, c_card_y + c_card_h / 2, 1150, 900, color4f(c_accent.r, c_accent.g, c_accent.b, 0.12f));
		{
			rounded_rect rim;
			rim.set_pos(c_card_x - 2, c_card_y - 2);
			rim.set_size(c_card_w + 4, c_card_h + 4);
			rim.border_radius = 30;
			rim.back_color = color4f(c_accent.r, c_accent.g, c_accent.b, 0.45f);
			card.add(rim.get_compiled());

			rounded_rect body;
			body.set_pos(c_card_x, c_card_y);
			body.set_size(c_card_w, c_card_h);
			body.border_radius = 28;
			body.back_color = color4f(0.02f, 0.05f, 0.19f, 0.94f);
			card.add(body.get_compiled());

			// Light inside the glass: a brighter blue in its middle, inside its edges
			glow(c_card_x + c_card_w / 2, c_card_y + c_card_h / 2 - 40, c_card_w - 60, c_card_h - 40, color4f(0.16f, 0.36f, 1.f, 0.32f));

			rounded_rect edge;
			edge.set_pos(c_card_x + 28, c_card_y);
			edge.set_size(c_card_w - 56, 3);
			edge.border_radius = 2;
			edge.back_color = c_accent;
			card.add(edge.get_compiled());
		}

		// The game: its icon, PAUSED, its name, its serial
		s16 words_x = c_left;
		if (m_icon)
		{
			rounded_image art;
			art.set_raw_image(m_icon.get());
			art.set_size(128, 70);
			art.set_pos(c_left, c_card_y + 34);
			art.border_radius = 10;
			art.back_color.a = 0.f;
			card.add(art.get_compiled());
			words_x = c_left + 128 + 22;
		}
		text(spaced("PAUSED"), 9, f_semibold, c_accent, words_x, c_card_y + 48.f);
		{
			auto title = make_label("", 22, f_bold, c_text);
			fit_text(*title, m_title, static_cast<u16>(c_inner_right - words_x));
			place(*title, words_x, c_card_y + 72.f);
			card.add(title->get_compiled());
		}
		text(m_serial.empty() ? std::string("Changes here last until the game ends") : m_serial + "   ·   Changes here are saved for this game",
			11, f_medium, c_text_dim, words_x, c_card_y + 96.f);

		// The tabs, L1 and R1 at their ends; the chosen one in a cyan pill
		{
			std::vector<std::unique_ptr<label>> names;
			f32 words = 0.f;
			for (usz t = 0; t < m_tabs.size(); t++)
			{
				names.push_back(make_label(m_tabs[t].name, 13, static_cast<s32>(t) == m_tab ? f_bold : f_medium, static_cast<s32>(t) == m_tab ? c_button_text : c_text_dim));
				words += names.back()->w;
			}
			// What room the names leave, shared out as padding
			const f32 room = static_cast<f32>(c_inner_right - c_left) - 2 * 30.f - words;
			const f32 pad = std::clamp(room / (2.f * names.size()), 6.f, 16.f);
			f32 x = c_left;
			glyph(static_cast<u8>(resource_config::standard_image_resource::L1), static_cast<s16>(x), c_tabs_mid);
			x += 30.f;
			for (usz t = 0; t < names.size(); t++)
			{
				const f32 w = names[t]->w + 2 * pad;
				if (static_cast<s32>(t) == m_tab)
				{
					rounded_rect pill;
					pill.set_pos(static_cast<s16>(std::lround(x)), static_cast<s16>(c_tabs_mid - 15.f));
					pill.set_size(static_cast<u16>(std::lround(w)), 30);
					pill.border_radius = 15;
					pill.back_color = c_accent;
					card.add(pill.get_compiled());
				}
				place(*names[t], static_cast<s16>(std::lround(x + pad)), c_tabs_mid);
				card.add(names[t]->get_compiled());
				x += w;
			}
			glyph(static_cast<u8>(resource_config::standard_image_resource::R1), static_cast<s16>(std::lround(x + 10.f)), c_tabs_mid);

			overlay_element rule;
			rule.set_pos(c_left, static_cast<s16>(c_tabs_mid + 26.f));
			rule.set_size(static_cast<u16>(c_inner_right - c_left), 1);
			rule.back_color = color4f(1.f, 1.f, 1.f, 0.12f);
			card.add(rule.get_compiled());
		}

		// The tab's rows
		const tab& current_tab = m_tabs[m_tab];
		const s32 count = static_cast<s32>(current_tab.rows.size());
		if (m_row < m_scroll) m_scroll = m_row;
		if (m_row >= m_scroll + c_visible_rows) m_scroll = m_row - c_visible_rows + 1;
		for (s32 r = m_scroll; r < count && r < m_scroll + c_visible_rows; r++)
		{
			const row& entry = current_tab.rows[r];
			const f32 y = c_rows_top + (r - m_scroll) * c_row_h;
			const f32 mid = y + c_row_h / 2.f;
			const bool selected = r == m_row;
			const setting_row* setting = entry.setting >= 0 ? &m_settings[entry.setting] : nullptr;
			const bool changed = setting && setting->value != setting->global;
			const color4f ink = selected ? c_button_text : ((changed || !setting) ? c_text : c_text_dim);

			if (entry.rule_before && r > m_scroll)
			{
				overlay_element rule;
				rule.set_pos(c_left, static_cast<s16>(y - 1));
				rule.set_size(static_cast<u16>(c_inner_right - c_left), 1);
				rule.back_color = color4f(1.f, 1.f, 1.f, 0.16f);
				card.add(rule.get_compiled());
			}
			if (selected)
			{
				// A glow of its own under the lit pill
				glow(static_cast<s16>((c_left + c_inner_right) / 2), static_cast<s16>(mid), c_card_w, 120, color4f(c_accent.r, c_accent.g, c_accent.b, 0.22f));
				rounded_rect pill;
				pill.set_pos(c_left - 16, static_cast<s16>(y + 3));
				pill.set_size(static_cast<u16>(c_inner_right - c_left + 32), static_cast<u16>(c_row_h - 6));
				pill.border_radius = static_cast<u16>((c_row_h - 6) / 2);
				pill.back_color = c_accent;
				card.add(pill.get_compiled());
			}

			s16 name_x = c_left + 8;
			if (entry.icon)
			{
				icon(entry.icon, static_cast<s16>(c_left + 4), static_cast<s16>(std::lround(mid - 9.f)), 18, ink);
				name_x = c_left + 36;
			}
			if (changed)
			{
				ellipse dot;
				dot.set_size(6, 6);
				dot.set_pos(c_left - 4, static_cast<s16>(std::lround(mid - 3.f)));
				dot.back_color = selected ? c_button_text : c_accent;
				card.add(dot.get_compiled());
			}

			s16 name_room = static_cast<s16>(c_inner_right - name_x);
			if (setting)
			{
				auto value = make_label("", 13, f_semibold, selected ? c_button_text : (changed ? c_accent : c_text_dim));
				fit_text(*value, setting_text(setting->key, setting->value), 280);
				const s16 right = static_cast<s16>(c_inner_right - (selected ? 18 : 0));
				place(*value, static_cast<s16>(right - value->w), mid);
				card.add(value->get_compiled());
				if (selected)
				{
					text("‹", 16, f_bold, c_button_text, static_cast<s16>(value->x - 18), mid);
					text("›", 16, f_bold, c_button_text, static_cast<s16>(right + 8), mid);
				}
				name_room = static_cast<s16>(value->x - (selected ? 30 : 16) - name_x);
			}
			else if (selected)
			{
				text("›", 20, f_bold, c_button_text, static_cast<s16>(c_inner_right - 6), mid);
			}

			auto name = make_label("", 14, selected ? f_bold : f_medium, ink);
			fit_text(*name, entry.label, static_cast<u16>(std::max<s16>(80, name_room)));
			place(*name, name_x, mid);
			card.add(name->get_compiled());
		}
		if (count > c_visible_rows)
		{
			// Where in the tab
			const f32 track = c_visible_rows * c_row_h;
			const f32 thumb = track * c_visible_rows / count;
			rounded_rect bar;
			bar.set_pos(c_inner_right + 26, static_cast<s16>(c_rows_top + (track - thumb) * m_scroll / (count - c_visible_rows)));
			bar.set_size(4, static_cast<u16>(thumb));
			bar.border_radius = 2;
			bar.back_color = color4f(1.f, 1.f, 1.f, 0.25f);
			card.add(bar.get_compiled());
		}

		// The row, explained: a setting's tooltip and when it takes effect, or
		// what an action does
		{
			overlay_element rule;
			rule.set_pos(c_left, c_help_top - 6);
			rule.set_size(static_cast<u16>(c_inner_right - c_left), 1);
			rule.back_color = color4f(1.f, 1.f, 1.f, 0.12f);
			card.add(rule.get_compiled());

			const row& entry = current_tab.rows[std::clamp(m_row, 0, count - 1)];
			std::string help;
			std::string when;
			if (entry.setting >= 0)
			{
				const setting_row& setting = m_settings[entry.setting];
				help = short_help(setting.help);
				const cfg::_base* live = find_setting(g_cfg, setting.section, setting.key);
				when = (live && live->get_is_dynamic()) ? "Takes effect now." : "Takes effect after a restart of the game.";
				if (setting.value != setting.global)
				{
					when += "  Global: " + setting_text(setting.key, setting.global) + ".";
				}
			}
			else
			{
				help = entry.note;
			}

			label words;
			words.set_font(11, f_regular);
			words.fore_color = c_text_dim;
			words.back_color.a = 0.f;
			words.set_padding(0);
			words.set_wrap_text(true);
			words.set_text(help);
			words.set_pos(c_left, c_help_top + 6);
			words.set_size(static_cast<u16>(c_inner_right - c_left), 34);
			words.auto_resize(false, static_cast<u16>(c_inner_right - c_left), 34);
			card.add(words.get_compiled());
			if (!when.empty())
			{
				text(when, 11, f_semibold, c_accent, c_left, c_help_top + 52.f);
			}
		}

		// The buttons' prompts along the foot
		{
			f32 x = c_left;
			const auto prompt = [&](u8 resource, std::string_view what)
			{
				glyph(resource, static_cast<s16>(std::lround(x)), c_hints_mid);
				auto l = text(what, 12, f_semibold, c_text, static_cast<s16>(std::lround(x + 26.f)), c_hints_mid);
				x += 26.f + l->w + 26.f;
			};
			const row& entry = current_tab.rows[std::clamp(m_row, 0, count - 1)];
			prompt(resource_config::confirm_button_resource(), entry.setting >= 0 ? "Change" : "Select");
			prompt(resource_config::cancel_button_resource(), "Resume");
			if (entry.setting >= 0 && m_settings[entry.setting].value != m_settings[entry.setting].global)
			{
				prompt(static_cast<u8>(resource_config::standard_image_resource::square), "Global value");
			}

			auto tabs = make_label("Tabs", 12, f_semibold, c_text_dim);
			place(*tabs, static_cast<s16>(c_inner_right - tabs->w), c_hints_mid);
			card.add(tabs->get_compiled());
			glyph(static_cast<u8>(resource_config::standard_image_resource::R1), static_cast<s16>(tabs->x - 28), c_hints_mid);
			glyph(static_cast<u8>(resource_config::standard_image_resource::L1), static_cast<s16>(tabs->x - 52), c_hints_mid);
		}

		add_animated(result, card, shown, 0.f, 18.f * (1.f - opened) + 10.f * closed);
		return result;
	}

	error_code ps5_pause_menu::show(std::function<void(s32 status)> on_close)
	{
		this->on_close = std::move(on_close);
		visible = true;

		// The emulation runs on under the menu, as a PS3's does under its own:
		// the game hears that the system menu opened (send_open_home_menu_cmds)
		// and pauses itself, GTA IV into its own pause menu. Pausing the
		// emulator too froze the app on my console (build 91): the menu opened,
		// and nothing drew or answered after
		const auto notify = std::make_shared<atomic_t<u32>>(0);
		auto& overlayman = g_fxo->get<display_manager>();
		overlayman.attach_thread_input(uid, "PS5 pause menu", [notify]()
		{
			pause_log.notice("Reading the controller");
			*notify = true;
			notify->notify_one();
		});
		pause_log.notice("Opened over '%s' (%s)", m_title, m_serial);

		while (!Emu.IsStopped() && !*notify)
		{
			notify->wait(false, atomic_wait_timeout{1'000'000});
		}
		return CELL_OK;
	}

	error_code open_ps5_pause_menu(std::function<void(s32 status)> on_close)
	{
		auto manager = g_fxo->try_get<display_manager>();
		if (!manager)
		{
			return CELL_EFAULT;
		}
		return manager->create<ps5_pause_menu>()->show(std::move(on_close));
	}
}
