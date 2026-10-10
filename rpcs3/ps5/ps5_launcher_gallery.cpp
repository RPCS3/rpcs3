// PS5: the launcher's Gallery tab (ps5_launcher.h): the screenshots the pause
// menu saved, as thumbnails in a grid, the latest first; one opens over the
// page at its full size.

#include "stdafx.h"
#include "ps5_launcher.h"
#include "ps5_ui.h"

#include "Utilities/File.h"
#include "Utilities/Thread.h"

#include <ctime>

LOG_CHANNEL(launcher_log, "Launcher");

namespace rsx::overlays
{
	using namespace ps5ui;

	namespace
	{
		constexpr s16 c_left = 64;
		constexpr u16 c_columns = 4;
		constexpr u16 c_gap = 24;
		constexpr u16 c_thumb_w = 270; // (1152 - 3 gaps) / 4
		constexpr u16 c_thumb_h = 152;
		constexpr s16 c_grid_y = 214;
		constexpr u16 c_row_pitch = c_thumb_h + 56;
		constexpr s32 c_rows_in_view = 2;
		constexpr usz c_most = 60; // the latest

		// The thumbnails' images
		constexpr u16 c_thumb_tex_w = 320;
		constexpr u16 c_thumb_tex_h = 180;

		// "10 Oct 2026, 21:50", in the console's local time
		std::string when_text(s64 seconds)
		{
			const std::time_t when = static_cast<std::time_t>(seconds);
			std::tm parts{};
			if (!seconds || (!localtime_r(&when, &parts) && !gmtime_r(&when, &parts)))
			{
				return {};
			}
			static constexpr const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
			return fmt::format("%d %s %d, %02d:%02d", parts.tm_mday, months[std::clamp(parts.tm_mon, 0, 11)], parts.tm_year + 1900, parts.tm_hour, parts.tm_min);
		}
	}

	std::string ps5_launcher_dialog::game_name(const std::string& serial) const
	{
		for (const ps5_launcher_game& game : m_games)
		{
			if (game.info.serial == serial && !game.info.name.empty())
			{
				return game.info.name;
			}
		}
		return serial;
	}

	void ps5_launcher_dialog::open_gallery_tab()
	{
		// Read again at each opening: a game played since may have saved some
		m_ga_view = false;
		m_ga_loading = true;
		m_ga_thread.reset();
		{
			std::lock_guard lock(m_ga_result_mutex);
			m_ga_result.reset();
		}
		const u32 generation = ++m_ga_generation;
		m_ga_thread = std::make_unique<named_thread<std::function<void()>>>("Launcher Gallery", [this, generation]()
		{
			auto result = std::make_unique<gallery_result>();
			result->generation = generation;
			result->shots = ps5_screenshots::list();
			if (result->shots.size() > c_most)
			{
				result->shots.resize(c_most);
			}
			for (const ps5_screenshots::shot& shot : result->shots)
			{
				if (thread_ctrl::state() == thread_state::aborting)
				{
					return;
				}
				std::vector<u8> pixels;
				if (const auto image = load_image(shot.path))
				{
					pixels = fit_pixels(*image, c_thumb_tex_w, c_thumb_tex_h);
				}
				result->thumbs.push_back(std::move(pixels));
			}
			std::lock_guard lock(m_ga_result_mutex);
			m_ga_result = std::move(result);
		});
		layout_hints();
	}

	void ps5_launcher_dialog::open_gallery_image(s32 index)
	{
		if (index < 0 || static_cast<usz>(index) >= m_ga_shots.size())
		{
			return;
		}
		m_ga_selected = index;
		m_ga_image_index = index;
		m_ga_image_thread.reset();
		{
			std::lock_guard lock(m_ga_result_mutex);
			m_ga_image_result.reset();
		}
		const u32 generation = ++m_ga_image_generation;
		m_ga_image_thread = std::make_unique<named_thread<std::function<void()>>>("Launcher Gallery Image", [this, generation, index, path = m_ga_shots[index].path]()
		{
			auto result = std::make_unique<gallery_image>();
			result->generation = generation;
			result->index = index;
			result->image = load_image(path);
			std::lock_guard lock(m_ga_result_mutex);
			m_ga_image_result = std::move(result);
		});
	}

	void ps5_launcher_dialog::take_gallery()
	{
		std::unique_ptr<gallery_result> result;
		std::unique_ptr<gallery_image> image;
		{
			std::lock_guard lock(m_ga_result_mutex);
			if (m_ga_result && m_ga_result->generation == m_ga_generation)
			{
				result = std::move(m_ga_result);
			}
			if (m_ga_image_result && m_ga_image_result->generation == m_ga_image_generation)
			{
				image = std::move(m_ga_image_result);
			}
		}

		if (image && image->index == m_ga_image_index)
		{
			m_ga_image = std::move(image->image);
		}

		if (!result)
		{
			return;
		}

		// Into the kept images, more made as needed
		for (usz i = 0; i < result->thumbs.size(); i++)
		{
			if (i >= m_ga_thumbs.size())
			{
				auto slot = std::make_unique<trophy_icon>();
				slot->pixels.resize(usz{c_thumb_tex_w} * c_thumb_tex_h * 4);
				slot->image = std::make_unique<memory_image_info>(c_thumb_tex_w, c_thumb_tex_h, u8{4}, slot->pixels.data());
				m_ga_thumbs.push_back(std::move(slot));
			}
			trophy_icon& slot = *m_ga_thumbs[i];
			slot.filled = result->thumbs[i].size() == slot.pixels.size();
			if (slot.filled)
			{
				std::memcpy(slot.pixels.data(), result->thumbs[i].data(), slot.pixels.size());
				slot.image->dirty = true;
			}
		}
		for (usz i = result->thumbs.size(); i < m_ga_thumbs.size(); i++)
		{
			m_ga_thumbs[i]->filled = false;
		}

		m_ga_shots = std::move(result->shots);
		m_ga_selected = std::clamp<s32>(m_ga_selected, 0, std::max<s32>(0, static_cast<s32>(m_ga_shots.size()) - 1));
		m_ga_loading = false;
		launcher_log.notice("Gallery: %u screenshots in %s", m_ga_shots.size(), ps5_screenshots::folder());
		layout_hints();
	}

	void ps5_launcher_dialog::handle_gallery(pad_button button_press)
	{
		const s32 count = static_cast<s32>(m_ga_shots.size());
		const bool left = button_press == pad_button::dpad_left || button_press == pad_button::ls_left;
		const bool right = button_press == pad_button::dpad_right || button_press == pad_button::ls_right;
		const bool up = button_press == pad_button::dpad_up || button_press == pad_button::ls_up;
		const bool down = button_press == pad_button::dpad_down || button_press == pad_button::ls_down;

		if (m_ga_view)
		{
			// One at its full size: the one before or after, or back to the grid
			if (left || right || button_press == pad_button::L2 || button_press == pad_button::R2 || button_press == pad_button::L1 || button_press == pad_button::R1)
			{
				const s32 step = (left || button_press == pad_button::L2 || button_press == pad_button::L1) ? -1 : 1;
				const s32 next = std::clamp(m_ga_selected + step, 0, count - 1);
				if (next != m_ga_selected)
				{
					open_gallery_image(next);
					play_sound(sound_effect::cursor);
				}
			}
			else if (button_press == pad_button::circle || button_press == pad_button::cross)
			{
				m_ga_view = false;
				m_ga_image.reset();
				m_ga_image_index = -1;
				play_sound(sound_effect::cancel);
				layout_hints();
			}
			return;
		}

		if (button_press == pad_button::circle)
		{
			set_tab(tab::home);
			return;
		}
		if (!count)
		{
			return;
		}

		s32 next = m_ga_selected;
		if (left) next--;
		else if (right) next++;
		else if (up) next -= c_columns;
		else if (down) next += c_columns;
		else if (button_press == pad_button::L2) next -= c_columns * c_rows_in_view;
		else if (button_press == pad_button::R2) next += c_columns * c_rows_in_view;
		else if (button_press == pad_button::cross)
		{
			m_ga_view = true;
			m_ga_view_us = m_now_us;
			m_ga_image.reset();
			open_gallery_image(m_ga_selected);
			play_sound(sound_effect::accept);
			layout_hints();
			return;
		}
		next = std::clamp(next, 0, count - 1);
		if (next != m_ga_selected)
		{
			m_ga_selected = next;
			play_sound(sound_effect::cursor);
		}
	}

	void ps5_launcher_dialog::compile_gallery(compiled_resource& result)
	{
		// The deep blue, its glow and texture, as the other tabs
		{
			overlay_element base;
			base.set_size(virtual_width, virtual_height);
			base.back_color = c_library_blue;
			result.add(base.get_compiled());
		}
		const auto glow = [&](compiled_resource& out, s16 cx, s16 cy, u16 w, u16 h, const color4f& color)
		{
			image_view view;
			view.set_raw_image(m_glow_image.get());
			view.back_color.a = 0.f;
			view.fore_color = color;
			view.set_pos(static_cast<s16>(cx - w / 2), static_cast<s16>(cy - h / 2));
			view.set_size(w, h);
			out.add(view.get_compiled());
		};
		glow(result, 640, 340, 1600, 1000, color4f(0.1f, 0.2f, 0.65f, 0.34f));
		glow(result, 620, 360, 900, 700, color4f(c_accent.r, c_accent.g, c_accent.b, 0.06f));
		m_texture.add(result, texture_seconds());

		const auto text = [&](compiled_resource& out, std::string_view s, u16 size, std::string_view font_name, const color4f& colour, s16 x, f32 mid, u16 max_w = 0)
		{
			auto l = make_label("", size, font_name, colour);
			if (max_w)
			{
				fit_text(*l, s, max_w);
			}
			else
			{
				l->set_text(s);
				l->auto_resize();
			}
			place(*l, x, mid);
			out.add(l->get_compiled());
			return l;
		};

		// Heading
		const s32 count = static_cast<s32>(m_ga_shots.size());
		text(result, spaced("GALLERY"), 9, f_semibold, c_accent, c_left + 2, 112.f);
		text(result, "Your screenshots", 26, f_bold, c_text, c_left, 142.f);
		text(result, count ? fmt::format("%d screenshot%s%s  ·  kept in rpcs3/screenshots, a folder for each game", count, count == 1 ? "" : "s", count >= static_cast<s32>(c_most) ? " (the latest)" : "") :
			std::string(m_ga_loading ? "Reading..." : "None yet"), 12, f_regular, c_text_dim, c_left, 172.f);

		if (!count)
		{
			if (!m_ga_loading)
			{
				text(result, "No screenshots yet", 17, f_semibold, c_text, c_left, c_grid_y + 40.f);
				label words;
				words.set_font(12, f_regular);
				words.fore_color = c_text_dim;
				words.back_color.a = 0.f;
				words.set_padding(0);
				words.set_wrap_text(true);
				words.set_text("In a game, open the pause menu (L1 + R1 + the touch pad's click) and choose Save screenshot: it saves the game as it was when the menu opened, without the menu. They show here, and are kept in rpcs3/screenshots, a folder for each game, which FTP reaches.");
				words.set_pos(c_left, static_cast<s16>(c_grid_y + 62));
				words.set_size(720, 120);
				words.auto_resize(false, 720, 120);
				result.add(words.get_compiled());
			}
			return;
		}

		// The grid, scrolled to keep the selection in view
		const s32 selected_row = m_ga_selected / c_columns;
		if (selected_row < m_ga_scroll) m_ga_scroll = selected_row;
		if (selected_row >= m_ga_scroll + c_rows_in_view) m_ga_scroll = selected_row - c_rows_in_view + 1;
		for (s32 i = m_ga_scroll * c_columns; i < count && i < (m_ga_scroll + c_rows_in_view) * c_columns; i++)
		{
			const s32 row = i / c_columns - m_ga_scroll;
			const s32 column = i % c_columns;
			const s16 x = static_cast<s16>(c_left + column * (c_thumb_w + c_gap));
			const s16 y = static_cast<s16>(c_grid_y + row * c_row_pitch);
			const bool selected = i == m_ga_selected;

			if (selected)
			{
				glow(result, static_cast<s16>(x + c_thumb_w / 2), static_cast<s16>(y + c_thumb_h + 6), c_thumb_w + 80, 70, color4f(c_accent.r, c_accent.g, c_accent.b, 0.22f));
				rounded_rect rim;
				rim.set_pos(static_cast<s16>(x - 3), static_cast<s16>(y - 3));
				rim.set_size(c_thumb_w + 6, c_thumb_h + 6);
				rim.border_radius = 15;
				rim.back_color = c_accent;
				result.add(rim.get_compiled());
			}
			rounded_rect card;
			card.set_pos(x, y);
			card.set_size(c_thumb_w, c_thumb_h);
			card.border_radius = 12;
			card.back_color = color4f(0.01f, 0.02f, 0.08f, 1.f);
			result.add(card.get_compiled());
			if (static_cast<usz>(i) < m_ga_thumbs.size() && m_ga_thumbs[i]->filled)
			{
				rounded_image picture;
				picture.set_raw_image(m_ga_thumbs[i]->image.get());
				picture.back_color.a = 0.f;
				picture.border_radius = 12;
				picture.set_pos(x, y);
				picture.set_size(c_thumb_w, c_thumb_h);
				result.add(picture.get_compiled());
			}

			const ps5_screenshots::shot& shot = m_ga_shots[i];
			text(result, game_name(shot.serial), 13, f_semibold, selected ? c_text : color4f(1.f, 1.f, 1.f, 0.88f), x, y + c_thumb_h + 20.f, c_thumb_w);
			text(result, when_text(shot.time), 11, f_regular, c_text_dim, x, y + c_thumb_h + 38.f, c_thumb_w);
		}

		// Where in the list, bottom left
		text(result, fmt::format("%d / %d", m_ga_selected + 1, count), 11, f_medium, c_text_dim, 40, 690.f);

		// One at its full size, over the page: the full image once read, its
		// thumbnail stretched until then
		if (m_ga_view)
		{
			const f32 shown = ease_out(progress((m_now_us - m_ga_view_us) / 1'000'000.f, 0.f, 0.2f));
			compiled_resource view;
			overlay_element dim;
			dim.set_size(virtual_width, virtual_height);
			dim.back_color = color4f(0.f, 0.f, 0.02f, 0.9f);
			view.add(dim.get_compiled());

			const image_info_base* image = nullptr;
			f32 iw = 16.f, ih = 9.f;
			if (m_ga_image && m_ga_image_index == m_ga_selected)
			{
				image = m_ga_image.get();
				iw = static_cast<f32>(m_ga_image->w);
				ih = static_cast<f32>(m_ga_image->h);
			}
			else if (static_cast<usz>(m_ga_selected) < m_ga_thumbs.size() && m_ga_thumbs[m_ga_selected]->filled)
			{
				image = m_ga_thumbs[m_ga_selected]->image.get(); // letterboxed into 16:9 already
			}
			if (image)
			{
				constexpr f32 box_w = 1200.f, box_h = 600.f;
				const f32 scale = std::min(box_w / iw, box_h / ih);
				const u16 w = static_cast<u16>(std::lround(iw * scale)), h = static_cast<u16>(std::lround(ih * scale));
				rounded_image picture;
				picture.set_raw_image(image);
				picture.back_color.a = 0.f;
				picture.border_radius = 10;
				picture.set_pos(static_cast<s16>(640 - w / 2), static_cast<s16>(368 - h / 2));
				picture.set_size(w, h);
				view.add(picture.get_compiled());
			}

			const ps5_screenshots::shot& shot = m_ga_shots[std::clamp(m_ga_selected, 0, count - 1)];
			text(view, fmt::format("%s   ·   %s   ·   %d of %d", game_name(shot.serial), when_text(shot.time), m_ga_selected + 1, count), 12, f_medium, c_text_dim, 40, 690.f, 760);
			add_animated(result, view, shown);
		}
	}
}
