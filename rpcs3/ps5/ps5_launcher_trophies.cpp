// PS5: the launcher's Trophies tab (ps5_launcher.h, ps5_trophies.h).
//
// The user's trophy sets in a row of cards, the latest earned first: each its
// ICON0.PNG, its name, its progress and its trophies by grade. CROSS opens
// the selected set in a box over the page, as the Xbox 360's achievements
// open over its dashboard: the set's progress across the top, its trophies in
// a list on the left, the selected one large on the right. A hidden trophy
// keeps its name and description to itself until it is earned.

#include "stdafx.h"
#include "ps5_launcher.h"
#include "ps5_ui.h"

#include "Utilities/File.h"
#include "Utilities/StrUtil.h"
#include "Utilities/Thread.h"

#include <algorithm>
#include <cmath>
#include <cstring>

LOG_CHANNEL(launcher_log, "Launcher");

namespace rsx::overlays
{
	using namespace ps5ui;

	namespace
	{
		constexpr s16 c_left = 64;
		constexpr s16 c_right_edge = 1216;

		// The row's cards
		constexpr u16 c_card_w = 320;
		constexpr u16 c_card_h = 268;
		constexpr u16 c_card_pitch = 344;
		constexpr s16 c_cards_y = 214;
		constexpr s32 c_cards_across = 3; // wholly in view; the next shows its edge

		// The icons' images: ICON0.PNG is 320x176, a trophy's icon 240x240
		constexpr u16 c_set_icon_w = 320;
		constexpr u16 c_set_icon_h = 176;
		constexpr u16 c_trophy_icon = 240;

		// The box
		constexpr s16 c_box_x = 80;
		constexpr s16 c_box_y = 72;
		constexpr u16 c_box_w = 1120;
		constexpr u16 c_box_h = 596;
		constexpr s16 c_list_x = 96;
		constexpr u16 c_list_w = 672;
		constexpr s16 c_list_y = 202;
		constexpr s16 c_list_row_h = 52;
		constexpr s32 c_list_rows = 8;
		constexpr s16 c_pane_x = 792;
		constexpr u16 c_pane_w = 384;

		constexpr const char* c_sort_names[] = {"In the game's order", "Earned first", "Not earned first", "By grade"};

		const color4f& grade_colour(ps5_trophies::grade kind)
		{
			static const color4f colours[] =
			{
				{0.80f, 0.88f, 1.00f, 1.f}, // platinum
				{0.98f, 0.78f, 0.30f, 1.f}, // gold
				{0.80f, 0.82f, 0.87f, 1.f}, // silver
				{0.86f, 0.55f, 0.33f, 1.f}, // bronze
				{1.f, 1.f, 1.f, 0.6f},
			};
			return colours[std::min<usz>(static_cast<usz>(kind), 4)];
		}

		// An image fitted into a w x h RGBA canvas, clear around it: bilinear,
		// averaged over up to 4x4 samples a pixel where it shrinks, its colour
		// weighted by its alpha so clear edges do not darken
		std::vector<u8> fit_pixels(const image_info_base& src, u16 w, u16 h)
		{
			std::vector<u8> out(usz{w} * h * 4, 0);
			const u8* data = src.get_data();
			if (!data || src.w <= 0 || src.h <= 0)
			{
				return {};
			}

			const f32 art = static_cast<f32>(src.w) / src.h;
			const f32 box = static_cast<f32>(w) / h;
			int dw = w, dh = h, ox = 0, oy = 0;
			if (art > box)
			{
				dh = std::max(1, static_cast<int>(std::lround(w / art)));
				oy = (h - dh) / 2;
			}
			else
			{
				dw = std::max(1, static_cast<int>(std::lround(h * art)));
				ox = (w - dw) / 2;
			}
			const int k = std::clamp(static_cast<int>(std::ceil(static_cast<f32>(src.w) / dw)), 1, 4);

			const auto texel = [&](int x, int y) { return data + (static_cast<usz>(std::clamp(y, 0, src.h - 1)) * src.w + std::clamp(x, 0, src.w - 1)) * 4; };
			for (int y = 0; y < dh; y++)
			{
				for (int x = 0; x < dw; x++)
				{
					f32 sum[4]{};
					for (int sy = 0; sy < k; sy++)
					{
						for (int sx = 0; sx < k; sx++)
						{
							const f32 fx = (x + (sx + 0.5f) / k) / dw * src.w - 0.5f;
							const f32 fy = (y + (sy + 0.5f) / k) / dh * src.h - 0.5f;
							const int x0 = static_cast<int>(std::floor(fx)), y0 = static_cast<int>(std::floor(fy));
							const f32 tx = fx - x0, ty = fy - y0;
							const u8* p[4] = {texel(x0, y0), texel(x0 + 1, y0), texel(x0, y0 + 1), texel(x0 + 1, y0 + 1)};
							const f32 wt[4] = {(1 - tx) * (1 - ty), tx * (1 - ty), (1 - tx) * ty, tx * ty};
							for (int i = 0; i < 4; i++)
							{
								const f32 a = p[i][3] * wt[i];
								sum[0] += p[i][0] * a;
								sum[1] += p[i][1] * a;
								sum[2] += p[i][2] * a;
								sum[3] += a;
							}
						}
					}
					u8* o = &out[(static_cast<usz>(y + oy) * w + x + ox) * 4];
					if (sum[3] > 0.f)
					{
						for (int c = 0; c < 3; c++)
						{
							o[c] = static_cast<u8>(std::clamp(sum[c] / sum[3], 0.f, 255.f));
						}
					}
					o[3] = static_cast<u8>(std::clamp(sum[3] / (k * k), 0.f, 255.f));
				}
			}
			return out;
		}

		// Where an image is shown to FTP: /app0 is the title's folder
		std::string as_ftp_path(std::string path)
		{
			if (path.starts_with("/app0/"))
			{
				path = "/data/homebrew/PPSA99200/" + path.substr(6);
			}
			return path;
		}
	}

	void ps5_launcher_dialog::build_trophies_static()
	{
		m_tr_cup_data = resource_config::load_icon("home/32/trophy-solid.png");
		if (m_tr_cup_data)
		{
			m_tr_cup_data->dirty = true;
		}

		// A padlock, white on clear, drawn from distances so its edges are smooth
		constexpr int n = 64;
		m_tr_lock_pixels.assign(usz{n} * n * 4, 255);
		for (int y = 0; y < n; y++)
		{
			for (int x = 0; x < n; x++)
			{
				const f32 px = x + 0.5f, py = y + 0.5f;

				// The body: a rounded box, with a keyhole
				const f32 bx = std::abs(px - 32.f) - 17.f + 5.f, by = std::abs(py - 44.f) - 14.f + 5.f;
				const f32 body = std::hypot(std::max(bx, 0.f), std::max(by, 0.f)) + std::min(std::max(bx, by), 0.f) - 5.f;
				const f32 hole = std::min(std::hypot(px - 32.f, py - 41.f) - 4.f, std::max(std::abs(px - 32.f) - 1.8f, std::abs(py - 47.f) - 5.f));
				const f32 body_alpha = std::clamp(0.5f - body, 0.f, 1.f) * std::clamp(hole + 0.5f, 0.f, 1.f);

				// The shackle: a half ring over the body, and its legs down into it
				const f32 ring = py < 24.f ? std::abs(std::hypot(px - 32.f, py - 24.f) - 11.f) - 3.f : std::abs(std::abs(px - 32.f) - 11.f) - 3.f;
				const f32 shackle_alpha = py < 33.f ? std::clamp(0.5f - ring, 0.f, 1.f) : 0.f;

				m_tr_lock_pixels[(static_cast<usz>(y) * n + x) * 4 + 3] = static_cast<u8>(std::lround(255.f * std::max(body_alpha, shackle_alpha)));
			}
		}
		m_tr_lock_image = std::make_unique<memory_image_info>(u16{n}, u16{n}, u8{4}, m_tr_lock_pixels.data());
		m_tr_lock_image->dirty = true;
	}

	void ps5_launcher_dialog::open_trophies_tab()
	{
		// Read again at each opening: a game played since may have earned some
		m_tr_open = false;
		m_tr_loading = true;
		m_tr_thread.reset();
		{
			std::lock_guard lock(m_tr_result_mutex);
			m_tr_result.reset();
		}
		const u32 generation = ++m_tr_generation;
		m_tr_thread = std::make_unique<named_thread<std::function<void()>>>("Launcher Trophies", [this, generation]()
		{
			auto result = std::make_unique<trophy_result>();
			result->generation = generation;
			result->sets = true;
			result->games = ps5_trophies::read_all();
			for (const ps5_trophies::game& set : result->games)
			{
				if (thread_ctrl::state() == thread_state::aborting)
				{
					return;
				}
				std::vector<u8> pixels;
				if (const auto image = load_image(set.icon_path))
				{
					pixels = fit_pixels(*image, c_set_icon_w, c_set_icon_h);
				}
				result->pixels.push_back(std::move(pixels));
			}
			std::lock_guard lock(m_tr_result_mutex);
			m_tr_result = std::move(result);
		});
		layout_hints();
	}

	void ps5_launcher_dialog::open_trophy_box()
	{
		if (m_tr_loading || m_tr_selected < 0 || static_cast<usz>(m_tr_selected) >= m_tr_games.size())
		{
			return;
		}

		const ps5_trophies::game& set = m_tr_games[m_tr_selected];
		if (!m_tr_open)
		{
			m_tr_open = true;
			m_tr_open_us = m_now_us;
		}
		m_tr_row = 0;
		m_tr_scroll = 0;
		for (const auto& slot : m_tr_icons)
		{
			slot->filled = false;
		}
		sort_trophies();

		std::vector<std::string> paths;
		for (const ps5_trophies::trophy& t : set.trophies)
		{
			paths.push_back(t.icon_path);
		}
		m_tr_thread.reset();
		{
			std::lock_guard lock(m_tr_result_mutex);
			m_tr_result.reset();
		}
		const u32 generation = ++m_tr_generation;
		m_tr_thread = std::make_unique<named_thread<std::function<void()>>>("Launcher Trophies", [this, generation, paths = std::move(paths)]()
		{
			auto result = std::make_unique<trophy_result>();
			result->generation = generation;
			for (const std::string& path : paths)
			{
				if (thread_ctrl::state() == thread_state::aborting)
				{
					return;
				}
				std::vector<u8> pixels;
				if (const auto image = load_image(path))
				{
					pixels = fit_pixels(*image, c_trophy_icon, c_trophy_icon);
				}
				result->pixels.push_back(std::move(pixels));
			}
			std::lock_guard lock(m_tr_result_mutex);
			m_tr_result = std::move(result);
		});

		launcher_log.notice("Trophies: %s (%s): %u of %u earned, %d%%", set.name, set.folder, set.earned, set.trophies.size(), static_cast<int>(set.progress * 100.f));
		layout_hints();
	}

	void ps5_launcher_dialog::close_trophy_box()
	{
		m_tr_open = false;
		play_sound(sound_effect::cancel);
		layout_hints();
	}

	void ps5_launcher_dialog::sort_trophies()
	{
		m_tr_order.clear();
		if (m_tr_selected < 0 || static_cast<usz>(m_tr_selected) >= m_tr_games.size())
		{
			return;
		}

		const std::vector<ps5_trophies::trophy>& list = m_tr_games[m_tr_selected].trophies;
		for (s32 i = 0; i < static_cast<s32>(list.size()); i++)
		{
			m_tr_order.push_back(i);
		}
		std::stable_sort(m_tr_order.begin(), m_tr_order.end(), [&](s32 a, s32 b)
		{
			const ps5_trophies::trophy& x = list[a];
			const ps5_trophies::trophy& y = list[b];
			switch (m_tr_sort)
			{
			case 1: // earned first, the latest first; then the rest in the game's order
			case 2: // the rest first
				if (x.earned != y.earned)
				{
					return m_tr_sort == 1 ? x.earned : y.earned;
				}
				return x.earned && x.earned_tick > y.earned_tick;
			case 3:
				return x.kind < y.kind;
			default:
				return false;
			}
		});
	}

	f32 ps5_launcher_dialog::trophy_row_target() const
	{
		const s32 count = static_cast<s32>(m_tr_games.size());
		return static_cast<f32>(std::clamp(m_tr_selected - 1, 0, std::max(0, count - c_cards_across)));
	}

	void ps5_launcher_dialog::take_trophies()
	{
		std::unique_ptr<trophy_result> result;
		{
			std::lock_guard lock(m_tr_result_mutex);
			if (m_tr_result && m_tr_result->generation == m_tr_generation)
			{
				result = std::move(m_tr_result);
			}
		}
		if (!result)
		{
			return;
		}

		// Into the kept images, more made as needed
		auto& pool = result->sets ? m_tr_set_icons : m_tr_icons;
		const u16 w = result->sets ? c_set_icon_w : c_trophy_icon;
		const u16 h = result->sets ? c_set_icon_h : c_trophy_icon;
		for (usz i = 0; i < result->pixels.size(); i++)
		{
			if (i >= pool.size())
			{
				auto slot = std::make_unique<trophy_icon>();
				slot->pixels.resize(usz{w} * h * 4);
				slot->image = std::make_unique<memory_image_info>(w, h, u8{4}, slot->pixels.data());
				pool.push_back(std::move(slot));
			}
			trophy_icon& slot = *pool[i];
			slot.filled = result->pixels[i].size() == slot.pixels.size();
			if (slot.filled)
			{
				std::memcpy(slot.pixels.data(), result->pixels[i].data(), slot.pixels.size());
				slot.image->dirty = true;
			}
		}
		for (usz i = result->pixels.size(); i < pool.size(); i++)
		{
			pool[i]->filled = false;
		}

		if (!result->sets)
		{
			return;
		}

		// The same set stays selected, if it is still there
		const std::string was = m_tr_selected >= 0 && static_cast<usz>(m_tr_selected) < m_tr_games.size() ? m_tr_games[m_tr_selected].folder : std::string();
		m_tr_games = std::move(result->games);
		m_tr_selected = 0;
		for (usz i = 0; i < m_tr_games.size(); i++)
		{
			if (m_tr_games[i].folder == was)
			{
				m_tr_selected = static_cast<s32>(i);
			}
		}
		m_tr_flow = trophy_row_target();
		m_tr_loading = false;

		u32 earned = 0, total = 0;
		for (const ps5_trophies::game& set : m_tr_games)
		{
			earned += set.earned;
			total += static_cast<u32>(set.trophies.size());
		}
		launcher_log.notice("Trophies: %u sets in %s, %u of %u trophies earned", m_tr_games.size(), ps5_trophies::folder(), earned, total);
		layout_hints();
	}

	void ps5_launcher_dialog::handle_trophies(pad_button button_press)
	{
		const bool up = button_press == pad_button::dpad_up || button_press == pad_button::ls_up;
		const bool down = button_press == pad_button::dpad_down || button_press == pad_button::ls_down;
		const bool left = button_press == pad_button::dpad_left || button_press == pad_button::ls_left;
		const bool right = button_press == pad_button::dpad_right || button_press == pad_button::ls_right;

		if (m_tr_open)
		{
			const s32 count = static_cast<s32>(m_tr_order.size());
			if (button_press == pad_button::circle)
			{
				close_trophy_box();
			}
			else if ((up || down || button_press == pad_button::L2 || button_press == pad_button::R2) && count)
			{
				const s32 step = up ? -1 : down ? 1 : button_press == pad_button::L2 ? -c_list_rows : c_list_rows;
				const s32 next = std::clamp(m_tr_row + step, 0, count - 1);
				if (next != m_tr_row)
				{
					m_tr_row = next;
					play_sound(sound_effect::cursor);
				}
			}
			else if (button_press == pad_button::square && count)
			{
				// The next order, the selected trophy kept selected
				const s32 kept = m_tr_order[m_tr_row];
				m_tr_sort = static_cast<u8>((m_tr_sort + 1) % std::size(c_sort_names));
				sort_trophies();
				m_tr_row = static_cast<s32>(std::find(m_tr_order.begin(), m_tr_order.end(), kept) - m_tr_order.begin());
				m_tr_scroll = std::clamp(m_tr_row - c_list_rows / 2, 0, std::max(0, count - c_list_rows));
				play_sound(sound_effect::cursor);
			}
			else if (button_press == pad_button::L1 || button_press == pad_button::R1)
			{
				// The set before or after, in the box
				const s32 next = m_tr_selected + (button_press == pad_button::L1 ? -1 : 1);
				if (next >= 0 && next < static_cast<s32>(m_tr_games.size()))
				{
					m_tr_selected = next;
					play_sound(sound_effect::cursor);
					open_trophy_box();
				}
			}
			return;
		}

		if (button_press == pad_button::circle)
		{
			set_tab(tab::home);
			return;
		}
		if (m_tr_games.empty())
		{
			return;
		}

		const s32 count = static_cast<s32>(m_tr_games.size());
		s32 next = m_tr_selected;
		if (left) next--;
		else if (right) next++;
		else if (button_press == pad_button::L2) next -= c_cards_across;
		else if (button_press == pad_button::R2) next += c_cards_across;
		else if (button_press == pad_button::cross && !m_tr_loading)
		{
			play_sound(sound_effect::accept);
			open_trophy_box();
			return;
		}
		next = std::clamp(next, 0, count - 1);
		if (next != m_tr_selected)
		{
			m_tr_selected = next;
			play_sound(sound_effect::cursor);
		}
	}

	void ps5_launcher_dialog::compile_trophies(compiled_resource& result)
	{
		compiled_resource* target = &result;

		// The Settings tab's deep blue and glow, with the selected set's icon,
		// blurred, washed over it
		{
			overlay_element base;
			base.set_size(virtual_width, virtual_height);
			base.back_color = c_library_blue;
			result.add(base.get_compiled());
		}
		const auto glow = [&](s16 cx, s16 cy, u16 w, u16 h, const color4f& color)
		{
			image_view view;
			view.set_raw_image(m_glow_image.get());
			view.back_color.a = 0.f;
			view.fore_color = color;
			view.set_pos(static_cast<s16>(cx - w / 2), static_cast<s16>(cy - h / 2));
			view.set_size(w, h);
			target->add(view.get_compiled());
		};
		const bool have_sets = !m_tr_games.empty();
		const ps5_trophies::game* set = have_sets && m_tr_selected >= 0 && static_cast<usz>(m_tr_selected) < m_tr_games.size() ? &m_tr_games[m_tr_selected] : nullptr;
		const trophy_icon* set_icon = set && static_cast<usz>(m_tr_selected) < m_tr_set_icons.size() && m_tr_set_icons[m_tr_selected]->filled ? m_tr_set_icons[m_tr_selected].get() : nullptr;
		if (set_icon)
		{
			image_view art;
			art.set_raw_image(set_icon->image.get());
			art.back_color.a = 0.f;
			art.fore_color = color4f(1.f, 1.f, 1.f, 0.32f);
			art.set_blur_strength(60);
			art.set_pos(-80, -60);
			art.set_size(virtual_width + 160, virtual_height + 120);
			result.add(art.get_compiled());
		}
		glow(640, 340, 1600, 1000, color4f(0.1f, 0.2f, 0.65f, 0.30f));
		glow(640, 380, 1100, 640, color4f(c_accent.r, c_accent.g, c_accent.b, 0.05f));

		const auto text = [&](std::string_view s, u16 size, std::string_view font_name, const color4f& colour, s16 x, f32 mid, u16 max_w = 0) -> std::unique_ptr<label>
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
			target->add(l->get_compiled());
			return l;
		};
		// Right-aligned at `right`; its left edge back
		const auto text_right = [&](std::string_view s, u16 size, std::string_view font_name, const color4f& colour, s16 right, f32 mid, u16 max_w = 0) -> s16
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
			place(*l, static_cast<s16>(right - l->w), mid);
			target->add(l->get_compiled());
			return l->x;
		};
		const auto wrapped = [&](std::string_view s, u16 size, std::string_view font_name, const color4f& colour, s16 x, s16 y, u16 w, u16 max_h) -> s16
		{
			label words;
			words.set_font(size, font_name);
			words.fore_color = colour;
			words.back_color.a = 0.f;
			words.set_padding(0);
			words.set_wrap_text(true);
			words.set_text(s);
			words.set_pos(x, y);
			words.set_size(w, max_h);
			words.auto_resize(false, w, max_h);
			target->add(words.get_compiled());
			return static_cast<s16>(words.y + words.h);
		};
		const auto box = [&](s16 x, s16 y, u16 w, u16 h, u16 radius, const color4f& colour, u16 border = 0, const color4f& border_colour = {})
		{
			rounded_rect r;
			r.set_pos(x, y);
			r.set_size(w, h);
			r.border_radius = radius;
			r.back_color = colour;
			r.border_size = border;
			r.border_color = border_colour;
			target->add(r.get_compiled());
		};
		const auto picture = [&](const image_info_base* image, s16 x, s16 y, u16 w, u16 h, u16 radius, const color4f& tint)
		{
			if (!radius)
			{
				image_view view;
				view.set_raw_image(image);
				view.back_color.a = 0.f;
				view.fore_color = tint;
				view.set_pos(x, y);
				view.set_size(w, h);
				target->add(view.get_compiled());
				return;
			}
			rounded_image view;
			view.set_raw_image(image);
			view.back_color.a = 0.f;
			view.fore_color = tint;
			view.border_radius = radius;
			view.set_pos(x, y);
			view.set_size(w, h);
			target->add(view.get_compiled());
		};
		const auto cup = [&](ps5_trophies::grade kind, s16 x, s16 y, u16 size, f32 alpha = 1.f)
		{
			if (!m_tr_cup_data)
			{
				return;
			}
			color4f tint = grade_colour(kind);
			tint.a *= alpha;
			picture(m_tr_cup_data.get(), x, y, size, size, 0, tint);
		};
		const auto bar = [&](s16 x, s16 y, u16 w, u16 h, f32 fraction)
		{
			box(x, y, w, h, h / 2, color4f(1.f, 1.f, 1.f, 0.12f));
			if (const u16 filled = static_cast<u16>(std::lround(w * std::clamp(fraction, 0.f, 1.f))); filled >= h)
			{
				box(x, y, filled, h, h / 2, c_accent);
			}
		};
		const auto percent = [](f32 fraction) { return fmt::format("%d%%", static_cast<int>(std::floor(fraction * 100.f + 0.0001f))); };

		// Heading
		u32 earned_all = 0, total_all = 0;
		u32 earned_by[4]{};
		for (const ps5_trophies::game& g : m_tr_games)
		{
			earned_all += g.earned;
			total_all += static_cast<u32>(g.trophies.size());
			for (int k = 0; k < 4; k++) earned_by[k] += g.earned_count[k];
		}
		text(spaced("TROPHIES"), 9, f_semibold, c_accent, c_left + 2, 112.f);
		text("Your trophies", 26, f_bold, c_text, c_left, 142.f);
		text(have_sets ? fmt::format("%u game%s  ·  %u of %u trophies earned", m_tr_games.size(), m_tr_games.size() == 1 ? "" : "s", earned_all, total_all) : std::string(m_tr_loading ? "Reading..." : "None yet"),
			12, f_regular, c_text_dim, c_left, 172.f);

		// Every set's trophies by grade, top right
		if (have_sets)
		{
			s16 gx = static_cast<s16>(c_right_edge - 4 * 104 + 16);
			for (int k = 0; k < 4; k++)
			{
				const auto kind = static_cast<ps5_trophies::grade>(k);
				cup(kind, gx, 124, 26);
				text(fmt::format("%u", earned_by[k]), 18, f_bold, c_text, static_cast<s16>(gx + 34), 138.f);
				text(spaced(ps5_trophies::grade_text(kind)), 8, f_semibold, c_text_dim, gx, 166.f);
				gx = static_cast<s16>(gx + 104);
			}
		}

		if (!have_sets)
		{
			const std::string where = as_ftp_path(ps5_trophies::folder());
			text(m_tr_loading ? "Reading your trophies..." : "No trophies yet", 17, f_semibold, c_text, c_left, c_cards_y + 40.f);
			if (!m_tr_loading)
			{
				wrapped(fmt::format("A game's trophies show here once it has been played: it sets them up the first time it starts, and each one earned is kept with the date.\n\nRPCS3 keeps them in %s, a folder for each game. A PC's RPCS3 keeps them in dev_hdd0/home/00000001/trophy, which can be copied over.", where),
					12, f_regular, c_text_dim, c_left, static_cast<s16>(c_cards_y + 62), 720, 200);
			}
			return;
		}

		// The cards, sliding to keep the selection in view
		for (s32 i = 0; i < static_cast<s32>(m_tr_games.size()); i++)
		{
			const f32 fx = c_left + (i - m_tr_flow) * c_card_pitch;
			if (fx + c_card_w < 0.f || fx > virtual_width)
			{
				continue;
			}
			const ps5_trophies::game& g = m_tr_games[i];
			const bool selected = i == m_tr_selected;

			compiled_resource card_part;
			target = &card_part;
			const s16 x = static_cast<s16>(std::lround(fx));
			const s16 y = static_cast<s16>(c_cards_y - (selected ? 8 : 0));
			if (selected)
			{
				glow(static_cast<s16>(x + c_card_w / 2), static_cast<s16>(y + c_card_h + 6), c_card_w + 80, 70, color4f(c_accent.r, c_accent.g, c_accent.b, 0.22f));
				box(x, y, c_card_w, c_card_h, 18, color4f(0.07f, 0.11f, 0.28f, 0.94f), 2, c_accent);
			}
			else
			{
				box(x, y, c_card_w, c_card_h, 18, color4f(0.02f, 0.04f, 0.16f, 0.78f), 1, color4f(1.f, 1.f, 1.f, 0.10f));
			}

			// The set's icon, or its cup where there is none
			const s16 ix = static_cast<s16>(x + 8), iy = static_cast<s16>(y + 8);
			if (static_cast<usz>(i) < m_tr_set_icons.size() && m_tr_set_icons[i]->filled)
			{
				picture(m_tr_set_icons[i]->image.get(), ix, iy, c_card_w - 16, 167, 12, c_text);
			}
			else
			{
				box(ix, iy, c_card_w - 16, 167, 12, color4f(1.f, 1.f, 1.f, 0.06f));
				cup(ps5_trophies::grade::unknown, static_cast<s16>(x + c_card_w / 2 - 20), static_cast<s16>(iy + 64), 40, 0.5f);
			}

			text(g.name, 14, f_semibold, selected ? c_text : color4f(1.f, 1.f, 1.f, 0.88f), static_cast<s16>(x + 18), y + 198.f, c_card_w - 36);
			bar(static_cast<s16>(x + 18), static_cast<s16>(y + 220), 226, 6, g.progress);
			text_right(percent(g.progress), 12, f_semibold, selected ? c_accent : c_text_dim, static_cast<s16>(x + c_card_w - 18), y + 223.f);

			// Earned by grade, along the foot
			s16 cx = static_cast<s16>(x + 18);
			for (int k = 0; k < 4; k++)
			{
				const auto kind = static_cast<ps5_trophies::grade>(k);
				if (!g.count[k])
				{
					continue;
				}
				cup(kind, cx, static_cast<s16>(y + 240), 15, g.earned_count[k] ? 1.f : 0.45f);
				auto count = text(fmt::format("%u/%u", g.earned_count[k], g.count[k]), 11, f_medium, g.earned_count[k] ? c_text : c_text_dim, static_cast<s16>(cx + 20), y + 248.f);
				cx = static_cast<s16>(count->x + count->w + 16);
			}

			// Faded where it runs off the screen
			const f32 off = std::max(0.f, (c_left - 24.f) - fx) + std::max(0.f, fx + c_card_w - (c_right_edge + 24.f));
			target = &result;
			add_animated(result, card_part, 1.f - 0.75f * std::min(1.f, off / (c_card_w * 0.6f)));
		}

		// The selected set, under the row
		if (set)
		{
			text(set->name, 20, f_bold, c_text, c_left, 526.f, 800);
			const ps5_trophies::trophy* last = nullptr;
			u32 hidden_left = 0;
			for (const ps5_trophies::trophy& t : set->trophies)
			{
				if (t.earned && (!last || t.earned_tick > last->earned_tick))
				{
					last = &t;
				}
				hidden_left += t.hidden && !t.earned;
			}
			if (last)
			{
				auto caption = text("Last earned", 12, f_medium, c_text_dim, c_left, 556.f);
				const s16 cx = static_cast<s16>(caption->x + caption->w + 12);
				cup(last->kind, cx, 548, 15);
				const std::string date = ps5_trophies::date_text(last->earned_tick);
				text(date.empty() ? last->name : fmt::format("%s  ·  %s", last->name, date), 12, f_semibold, c_text, static_cast<s16>(cx + 21), 556.f, 640);
			}
			else
			{
				text("Nothing earned in this game yet.", 12, f_medium, c_text_dim, c_left, 556.f);
			}
			text(hidden_left ? fmt::format("%s  ·  %u hidden trophies still to find", set->folder, hidden_left) : set->folder, 11, f_regular, color4f(1.f, 1.f, 1.f, 0.5f), c_left, 580.f);

			text_right(percent(set->progress), 30, f_bold, c_text, c_right_edge, 530.f);
			text_right(fmt::format("%u of %u trophies", set->earned, set->trophies.size()), 12, f_medium, c_text_dim, c_right_edge, 562.f);
		}
		if (m_tr_loading)
		{
			text("Reading your trophies...", 11, f_medium, c_accent, c_left, 618.f);
		}

		if (!m_tr_open || !set)
		{
			return;
		}

		// The box: over a dimmed page, rising into place as it fades in
		const f32 page = ease_out(progress((m_now_us - m_tr_open_us) / 1'000'000.f, 0.f, 0.25f));
		{
			overlay_element dim;
			dim.set_size(virtual_width, virtual_height);
			dim.back_color = color4f(0.f, 0.f, 0.02f, 0.66f);
			add_animated(result, dim.get_compiled(), page);
		}
		compiled_resource box_part;
		target = &box_part;

		box(c_box_x, c_box_y, c_box_w, c_box_h, 22, color4f(0.03f, 0.04f, 0.12f, 0.97f), 1, color4f(1.f, 1.f, 1.f, 0.16f));
		box(static_cast<s16>(c_box_x + 28), c_box_y, c_box_w - 56, 3, 2, c_accent);

		// Its heading: the set's icon, name and progress, and its trophies by grade
		if (set_icon)
		{
			picture(set_icon->image.get(), c_box_x + 24, c_box_y + 24, 160, 88, 10, c_text);
		}
		else
		{
			box(c_box_x + 24, c_box_y + 24, 160, 88, 10, color4f(1.f, 1.f, 1.f, 0.06f));
		}
		constexpr s16 head_x = c_box_x + 208;
		text(set->name, 20, f_bold, c_text, head_x, c_box_y + 44.f, 480);
		bar(head_x, c_box_y + 66, 340, 6, set->progress);
		text(percent(set->progress), 13, f_semibold, c_accent, static_cast<s16>(head_x + 354), c_box_y + 69.f);
		text(fmt::format("%u of %u trophies earned", set->earned, set->trophies.size()), 11, f_regular, c_text_dim, head_x, c_box_y + 92.f);

		s16 gx = static_cast<s16>(c_box_x + c_box_w - 24 - 4 * 92);
		for (int k = 0; k < 4; k++)
		{
			const auto kind = static_cast<ps5_trophies::grade>(k);
			cup(kind, gx, c_box_y + 32, 20, set->count[k] ? 1.f : 0.35f);
			text(fmt::format("%u/%u", set->earned_count[k], set->count[k]), 13, f_semibold, set->count[k] ? c_text : c_text_dim, static_cast<s16>(gx + 27), c_box_y + 43.f);
			text(spaced(ps5_trophies::grade_text(kind)), 8, f_semibold, c_text_dim, gx, c_box_y + 66.f);
			gx = static_cast<s16>(gx + 92);
		}
		{
			// How the list is sorted, and the button that changes it
			const s16 sort_x = text_right(c_sort_names[m_tr_sort % std::size(c_sort_names)], 11, f_semibold, c_text, static_cast<s16>(c_box_x + c_box_w - 24), c_box_y + 94.f);
			image_view glyph;
			glyph.set_image_resource(static_cast<u8>(resource_config::standard_image_resource::square));
			glyph.back_color.a = 0.f;
			glyph.set_size(18, 18);
			glyph.set_pos(static_cast<s16>(sort_x - 24), c_box_y + 85);
			target->add(glyph.get_compiled());
			text_right("Sorted", 11, f_medium, c_text_dim, static_cast<s16>(sort_x - 32), c_box_y + 94.f);
		}

		{
			overlay_element rule;
			rule.set_pos(c_box_x + 24, c_box_y + 120);
			rule.set_size(c_box_w - 48, 1);
			rule.back_color = color4f(1.f, 1.f, 1.f, 0.12f);
			target->add(rule.get_compiled());
		}

		// The list, scrolled to keep the selection in view
		const s32 count = static_cast<s32>(m_tr_order.size());
		m_tr_row = std::clamp(m_tr_row, 0, std::max(0, count - 1));
		if (m_tr_row < m_tr_scroll) m_tr_scroll = m_tr_row;
		if (m_tr_row >= m_tr_scroll + c_list_rows) m_tr_scroll = m_tr_row - c_list_rows + 1;

		const auto icon_of = [&](s32 index) -> const trophy_icon*
		{
			return static_cast<usz>(index) < m_tr_icons.size() && m_tr_icons[index]->filled ? m_tr_icons[index].get() : nullptr;
		};
		// A trophy's icon: dimmed with a lock until earned; a hidden one's
		// only a lock
		const auto trophy_picture = [&](const ps5_trophies::trophy& t, s32 index, s16 x, s16 y, u16 size, u16 radius)
		{
			const bool secret = t.hidden && !t.earned;
			const trophy_icon* icon = secret ? nullptr : icon_of(index);
			if (icon)
			{
				picture(icon->image.get(), x, y, size, size, radius, t.earned ? c_text : color4f(1.f, 1.f, 1.f, 0.38f));
			}
			else
			{
				box(x, y, size, size, radius, color4f(1.f, 1.f, 1.f, 0.07f));
			}
			if (secret)
			{
				const u16 lock = static_cast<u16>(size * 0.46f);
				picture(m_tr_lock_image.get(), static_cast<s16>(x + (size - lock) / 2), static_cast<s16>(y + (size - lock) / 2), lock, lock, 0, color4f(1.f, 1.f, 1.f, 0.6f));
			}
			else if (!t.earned)
			{
				const u16 badge = static_cast<u16>(std::max(16.f, size * 0.36f));
				ellipse disc;
				disc.set_pos(static_cast<s16>(x + size - badge + 3), static_cast<s16>(y + size - badge + 3));
				disc.set_size(badge, badge);
				disc.back_color = color4f(0.02f, 0.03f, 0.09f, 0.92f);
				target->add(disc.get_compiled());
				const u16 lock = static_cast<u16>(badge * 0.62f);
				picture(m_tr_lock_image.get(), static_cast<s16>(disc.x + (badge - lock) / 2), static_cast<s16>(disc.y + (badge - lock) / 2 - 1), lock, lock, 0, color4f(1.f, 1.f, 1.f, 0.85f));
			}
		};

		for (s32 r = m_tr_scroll; r < count && r < m_tr_scroll + c_list_rows; r++)
		{
			const s32 index = m_tr_order[r];
			const ps5_trophies::trophy& t = set->trophies[index];
			const bool selected = r == m_tr_row;
			const bool secret = t.hidden && !t.earned;
			const s16 y = static_cast<s16>(c_list_y + (r - m_tr_scroll) * c_list_row_h);
			const f32 mid = y + c_list_row_h / 2.f;

			if (selected)
			{
				box(c_list_x, static_cast<s16>(y + 2), c_list_w, c_list_row_h - 4, 12, color4f(1.f, 1.f, 1.f, 0.08f), 2, c_accent);
			}
			else if (r + 1 < m_tr_scroll + c_list_rows && r + 1 < count && r + 1 != m_tr_row)
			{
				overlay_element rule;
				rule.set_pos(static_cast<s16>(c_list_x + 66), static_cast<s16>(y + c_list_row_h - 1));
				rule.set_size(c_list_w - 82, 1);
				rule.back_color = color4f(1.f, 1.f, 1.f, 0.06f);
				target->add(rule.get_compiled());
			}

			trophy_picture(t, index, static_cast<s16>(c_list_x + 12), static_cast<s16>(mid - 20), 40, 8);

			constexpr u16 words_w = c_list_w - 66 - 150;
			const bool lit = selected || t.earned;
			text(secret ? "Hidden trophy" : t.name, 13, lit ? f_semibold : f_medium, lit ? c_text : color4f(1.f, 1.f, 1.f, 0.72f), static_cast<s16>(c_list_x + 66), mid - 8.f, words_w);
			text(secret ? "Earn it to see what it is." : t.detail, 11, f_regular, color4f(1.f, 1.f, 1.f, t.earned ? 0.62f : 0.46f), static_cast<s16>(c_list_x + 66), mid + 10.f, words_w);

			const s16 right = static_cast<s16>(c_list_x + c_list_w - 16);
			const std::string date = ps5_trophies::date_text(t.earned_tick);
			text_right(t.earned ? (date.empty() ? std::string("Earned") : date) : std::string("Not earned"), 11, t.earned ? f_semibold : f_medium,
				t.earned ? c_text : color4f(1.f, 1.f, 1.f, 0.45f), right, mid - 8.f);
			const s16 grade_x = text_right(ps5_trophies::grade_text(t.kind), 10, f_medium, c_text_dim, right, mid + 10.f);
			cup(t.kind, static_cast<s16>(grade_x - 18), static_cast<s16>(mid + 3), 13, t.earned ? 1.f : 0.5f);
		}

		// Where the list is
		if (count > c_list_rows)
		{
			constexpr s16 track_h = c_list_rows * c_list_row_h;
			box(c_list_x + c_list_w + 6, c_list_y + 4, 3, track_h - 8, 1, color4f(1.f, 1.f, 1.f, 0.08f));
			const f32 thumb_h = std::max(24.f, (track_h - 8.f) * c_list_rows / count);
			const f32 thumb_y = c_list_y + 4 + (track_h - 8.f - thumb_h) * m_tr_scroll / std::max(1, count - c_list_rows);
			box(c_list_x + c_list_w + 6, static_cast<s16>(thumb_y), 3, static_cast<u16>(thumb_h), 1, c_accent);
		}
		text(count ? fmt::format("Trophy %d of %d", m_tr_row + 1, count) : std::string("No trophies"), 10, f_medium, color4f(1.f, 1.f, 1.f, 0.45f), c_list_x + 12, c_box_y + c_box_h - 24.f);

		// The selected trophy, large, on the right
		if (count)
		{
			const s32 index = m_tr_order[m_tr_row];
			const ps5_trophies::trophy& t = set->trophies[index];
			const bool secret = t.hidden && !t.earned;
			constexpr s16 pane_y = c_list_y;
			constexpr u16 pane_h = c_box_y + c_box_h - 18 - c_list_y;
			box(c_pane_x, pane_y, c_pane_w, pane_h, 16, color4f(1.f, 1.f, 1.f, 0.04f), 1, color4f(1.f, 1.f, 1.f, 0.08f));

			constexpr s16 inner_x = c_pane_x + 24;
			constexpr u16 inner_w = c_pane_w - 48;
			trophy_picture(t, index, inner_x, pane_y + 24, 144, 18);

			const s16 side_x = static_cast<s16>(inner_x + 160);
			const u16 side_w = static_cast<u16>(inner_w - 160);
			cup(t.kind, side_x, static_cast<s16>(pane_y + 36), 20);
			text(fmt::format("%s trophy", ps5_trophies::grade_text(t.kind)), 12, f_semibold, grade_colour(t.kind), static_cast<s16>(side_x + 27), pane_y + 46.f, static_cast<u16>(side_w - 27));
			text(t.earned ? "Earned" : "Not earned yet", 13, f_bold, t.earned ? c_accent : c_text_dim, side_x, pane_y + 78.f, side_w);
			const std::string date = ps5_trophies::date_text(t.earned_tick);
			if (t.earned && !date.empty())
			{
				text(date, 11, f_medium, c_text_dim, side_x, pane_y + 100.f, side_w);
			}
			if (!t.group.empty())
			{
				wrapped(t.group, 10, f_regular, color4f(1.f, 1.f, 1.f, 0.5f), side_x, static_cast<s16>(pane_y + 116), side_w, 36);
			}

			const s16 below = wrapped(secret ? "Hidden trophy" : t.name, 16, f_semibold, c_text, inner_x, static_cast<s16>(pane_y + 186), inner_w, 46);
			wrapped(secret ? "This trophy's name and description show once it is earned." : (t.detail.empty() ? "No description." : t.detail), 12, f_regular, c_text_dim,
				inner_x, static_cast<s16>(below + 10), inner_w, static_cast<u16>(std::max(20, pane_y + pane_h - 20 - (below + 10))));
		}

		target = &result;
		add_animated(result, box_part, page, 0.f, 16.f * (1.f - page));
	}
}
