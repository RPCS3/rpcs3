// PS5: the launcher's Saves tab (ps5_launcher.h): the games' saves by game,
// the game with the latest save first, with the selected save's icon, words
// and backups beside the list. CROSS backs a save up, SQUARE restores its
// latest backup (the save as it is backed up first), TRIANGLE deletes it
// (backed up first), or a gone save's backups; each asks first but backing
// up, and runs on a thread of its own.

#include "stdafx.h"
#include "ps5_launcher.h"
#include "ps5_ui.h"

#include "Utilities/Thread.h"

#include <ctime>

LOG_CHANNEL(launcher_log, "Launcher");

namespace rsx::overlays
{
	using namespace ps5ui;

	namespace
	{
		constexpr s16 c_left = 64;
		constexpr u16 c_list_w = 640;
		constexpr s16 c_list_y = 214;
		constexpr u16 c_line_h = 56;
		constexpr s32 c_lines_in_view = 8;
		constexpr s16 c_panel_x = 744;
		constexpr u16 c_panel_w = 472;

		// The icons' images: ICON0.PNG's own size
		constexpr u16 c_icon_tex_w = 320;
		constexpr u16 c_icon_tex_h = 176;

		// "12 Oct 2026, 21:50", in the console's local time
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

		std::string size_text(u64 bytes)
		{
			if (bytes >= 1024 * 1024)
			{
				return fmt::format("%.1f MB", bytes / (1024. * 1024.));
			}
			return fmt::format("%u KB", std::max<u64>(1, (bytes + 1023) / 1024));
		}

		// A save's name in the list: its subtitle (the slot's own words), else
		// its title, else its folder
		const std::string& save_name(const ps5_saves::save& save)
		{
			return !save.subtitle.empty() ? save.subtitle : !save.title.empty() ? save.title : save.dir;
		}

		// The list's lines: a heading for each game, then its saves
		struct line
		{
			s32 save = -1;      // the save, or -1 for a heading
			std::string serial; // the heading's game
		};

		std::vector<line> make_lines(const std::vector<ps5_saves::save>& saves)
		{
			std::vector<line> lines;
			for (usz i = 0; i < saves.size(); i++)
			{
				if (i == 0 || saves[i].serial != saves[i - 1].serial)
				{
					lines.push_back({-1, saves[i].serial});
				}
				lines.push_back({static_cast<s32>(i), {}});
			}
			return lines;
		}
	}

	void ps5_launcher_dialog::open_saves_tab()
	{
		// Read again at each opening, and after each change: a game played since
		// may have saved
		m_sv_ask = save_ask::none;
		m_sv_loading = true;
		m_sv_thread.reset();
		{
			std::lock_guard lock(m_sv_result_mutex);
			m_sv_result.reset();
		}
		const u32 generation = ++m_sv_generation;
		m_sv_thread = std::make_unique<named_thread<std::function<void()>>>("Launcher Saves", [this, generation]()
		{
			auto result = std::make_unique<saves_result>();
			result->generation = generation;
			result->saves = ps5_saves::read_all();
			for (const ps5_saves::save& save : result->saves)
			{
				if (thread_ctrl::state() == thread_state::aborting)
				{
					return;
				}
				std::vector<u8> pixels;
				if (!save.icon_path.empty())
				{
					if (const auto image = load_image(save.icon_path))
					{
						pixels = fit_pixels(*image, c_icon_tex_w, c_icon_tex_h);
					}
				}
				result->icons.push_back(std::move(pixels));
			}
			std::lock_guard lock(m_sv_result_mutex);
			m_sv_result = std::move(result);
		});
		layout_hints();
	}

	void ps5_launcher_dialog::run_save_job(std::string doing, std::string done, std::function<std::string()> job)
	{
		m_sv_busy = true;
		m_sv_busy_text = std::move(doing);
		m_sv_job_done = std::move(done);
		m_sv_job.reset();
		m_sv_job = std::make_unique<named_thread<std::function<void()>>>("Launcher Save Job", [this, job = std::move(job)]()
		{
			auto result = std::make_unique<std::string>(job());
			std::lock_guard lock(m_sv_result_mutex);
			m_sv_job_result = std::move(result);
		});
		layout_hints();
	}

	void ps5_launcher_dialog::take_saves()
	{
		std::unique_ptr<saves_result> result;
		std::unique_ptr<std::string> job;
		{
			std::lock_guard lock(m_sv_result_mutex);
			if (m_sv_result && m_sv_result->generation == m_sv_generation)
			{
				result = std::move(m_sv_result);
			}
			job = std::move(m_sv_job_result);
		}

		if (job)
		{
			// Done: what happened, and the list read again
			m_sv_busy = false;
			show_toast(job->empty() ? m_sv_job_done : *job);
			launcher_log.notice("Saves: %s", job->empty() ? m_sv_job_done : *job);
			if (m_tab == tab::saves)
			{
				open_saves_tab();
			}
			layout_hints();
		}

		if (!result)
		{
			return;
		}

		// The selection stays on the same save where it still is
		const std::string selected_dir = static_cast<usz>(m_sv_selected) < m_sv_saves.size() ? m_sv_saves[m_sv_selected].dir : std::string();

		for (usz i = 0; i < result->icons.size(); i++)
		{
			if (i >= m_sv_icons.size())
			{
				auto slot = std::make_unique<trophy_icon>();
				slot->pixels.resize(usz{c_icon_tex_w} * c_icon_tex_h * 4);
				slot->image = std::make_unique<memory_image_info>(c_icon_tex_w, c_icon_tex_h, u8{4}, slot->pixels.data());
				m_sv_icons.push_back(std::move(slot));
			}
			trophy_icon& slot = *m_sv_icons[i];
			slot.filled = result->icons[i].size() == slot.pixels.size();
			if (slot.filled)
			{
				std::memcpy(slot.pixels.data(), result->icons[i].data(), slot.pixels.size());
				slot.image->dirty = true;
			}
		}
		for (usz i = result->icons.size(); i < m_sv_icons.size(); i++)
		{
			m_sv_icons[i]->filled = false;
		}

		m_sv_saves = std::move(result->saves);
		m_sv_selected = 0;
		for (usz i = 0; i < m_sv_saves.size(); i++)
		{
			if (m_sv_saves[i].dir == selected_dir)
			{
				m_sv_selected = static_cast<s32>(i);
			}
		}
		m_sv_loading = false;
		usz backups = 0;
		for (const ps5_saves::save& save : m_sv_saves)
		{
			backups += save.backups.size();
		}
		launcher_log.notice("Saves: %u in %s, %u backups in %s", m_sv_saves.size(), ps5_saves::folder(), backups, ps5_saves::backup_folder());
		layout_hints();
	}

	void ps5_launcher_dialog::handle_saves(pad_button button_press)
	{
		if (m_sv_busy)
		{
			return;
		}

		const s32 count = static_cast<s32>(m_sv_saves.size());
		const ps5_saves::save* save = (m_sv_selected >= 0 && m_sv_selected < count) ? &m_sv_saves[m_sv_selected] : nullptr;

		if (m_sv_ask != save_ask::none)
		{
			// The question: CROSS does it, CIRCLE does not
			const save_ask ask = m_sv_ask;
			if (button_press == pad_button::circle)
			{
				m_sv_ask = save_ask::none;
				play_sound(sound_effect::cancel);
				layout_hints();
			}
			else if (button_press == pad_button::cross && save)
			{
				m_sv_ask = save_ask::none;
				play_sound(sound_effect::accept);
				const ps5_saves::save target = *save;
				if (ask == save_ask::restore && !target.backups.empty())
				{
					run_save_job("Restoring...", "Restored from the backup of " + when_text(target.backups.front().time) + ".",
						[target]() { return ps5_saves::restore(target, target.backups.front()); });
				}
				else if (ask == save_ask::remove)
				{
					run_save_job("Deleting...", "Deleted. Its backup is kept here.", [target]() { return ps5_saves::remove(target); });
				}
				else if (ask == save_ask::remove_backups)
				{
					run_save_job("Deleting its backups...", "Its backups are deleted.", [target]() { return ps5_saves::remove_backups(target); });
				}
			}
			return;
		}

		if (button_press == pad_button::circle)
		{
			set_tab(tab::home);
			return;
		}
		if (!save || m_sv_loading)
		{
			return;
		}

		const bool up = button_press == pad_button::dpad_up || button_press == pad_button::ls_up;
		const bool down = button_press == pad_button::dpad_down || button_press == pad_button::ls_down;
		s32 next = m_sv_selected;
		if (up) next--;
		else if (down) next++;
		else if (button_press == pad_button::L2) next -= c_lines_in_view;
		else if (button_press == pad_button::R2) next += c_lines_in_view;
		else if (button_press == pad_button::cross && !save->path.empty())
		{
			play_sound(sound_effect::accept);
			const ps5_saves::save target = *save;
			run_save_job("Backing up...", "Backed up, to rpcs3/save_backups.", [target]() { return ps5_saves::back_up(target); });
			return;
		}
		else if (button_press == pad_button::square && !save->backups.empty())
		{
			m_sv_ask = save_ask::restore;
			play_sound(sound_effect::accept);
			layout_hints();
			return;
		}
		else if (button_press == pad_button::triangle)
		{
			m_sv_ask = save->path.empty() ? save_ask::remove_backups : save_ask::remove;
			play_sound(sound_effect::accept);
			layout_hints();
			return;
		}
		next = std::clamp(next, 0, count - 1);
		if (next != m_sv_selected)
		{
			m_sv_selected = next;
			play_sound(sound_effect::cursor);
			layout_hints();
		}
	}

	void ps5_launcher_dialog::compile_saves(compiled_resource& result)
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
		const auto words = [&](compiled_resource& out, std::string_view s, u16 size, std::string_view font_name, const color4f& colour, s16 x, s16 y, u16 w, u16 h)
		{
			label l;
			l.set_font(size, font_name);
			l.fore_color = colour;
			l.back_color.a = 0.f;
			l.set_padding(0);
			l.set_wrap_text(true);
			l.set_text(s);
			l.set_pos(x, y);
			l.set_size(w, h);
			l.auto_resize(false, w, h);
			out.add(l.get_compiled());
			return l.h;
		};
		const auto icon = [&](compiled_resource& out, s32 index, s16 x, s16 y, u16 w, u16 h, u16 radius)
		{
			rounded_rect card;
			card.set_pos(x, y);
			card.set_size(w, h);
			card.border_radius = radius;
			card.back_color = color4f(0.01f, 0.02f, 0.08f, 1.f);
			out.add(card.get_compiled());
			if (index >= 0 && static_cast<usz>(index) < m_sv_icons.size() && m_sv_icons[index]->filled)
			{
				rounded_image picture;
				picture.set_raw_image(m_sv_icons[index]->image.get());
				picture.back_color.a = 0.f;
				picture.border_radius = radius;
				picture.set_pos(x, y);
				picture.set_size(w, h);
				out.add(picture.get_compiled());
			}
		};

		// Heading
		const s32 count = static_cast<s32>(m_sv_saves.size());
		usz games = 0;
		for (s32 i = 0; i < count; i++)
		{
			games += (i == 0 || m_sv_saves[i].serial != m_sv_saves[i - 1].serial) ? 1 : 0;
		}
		text(result, spaced("SAVES"), 9, f_semibold, c_accent, c_left + 2, 112.f);
		text(result, "Your saves", 26, f_bold, c_text, c_left, 142.f);
		text(result, count ? fmt::format("%d save%s for %u game%s  ·  backups kept in rpcs3/save_backups", count, count == 1 ? "" : "s", games, games == 1 ? "" : "s") :
			std::string(m_sv_loading ? "Reading..." : "None yet"), 12, f_regular, c_text_dim, c_left, 172.f);

		if (!count)
		{
			if (!m_sv_loading)
			{
				text(result, "No saves yet", 17, f_semibold, c_text, c_left, c_list_y + 40.f);
				words(result, "A game's saves show here once it has saved. Each can be backed up to rpcs3/save_backups, which FTP reaches, restored from its latest backup, or deleted.",
					12, f_regular, c_text_dim, c_left, static_cast<s16>(c_list_y + 62), 720, 120);
			}
			return;
		}

		// The list, scrolled to keep the selection in view
		const std::vector<line> lines = make_lines(m_sv_saves);
		s32 selected_line = 0;
		for (usz i = 0; i < lines.size(); i++)
		{
			if (lines[i].save == m_sv_selected)
			{
				selected_line = static_cast<s32>(i);
			}
		}
		// Its game's heading in view too, where it fits
		const s32 wanted_top = (selected_line > 0 && lines[selected_line - 1].save < 0) ? selected_line - 1 : selected_line;
		if (wanted_top < m_sv_scroll) m_sv_scroll = wanted_top;
		if (selected_line >= m_sv_scroll + c_lines_in_view) m_sv_scroll = selected_line - c_lines_in_view + 1;
		m_sv_scroll = std::clamp<s32>(m_sv_scroll, 0, std::max<s32>(0, static_cast<s32>(lines.size()) - c_lines_in_view));

		for (s32 n = m_sv_scroll; n < static_cast<s32>(lines.size()) && n < m_sv_scroll + c_lines_in_view; n++)
		{
			const line& entry = lines[n];
			const s16 y = static_cast<s16>(c_list_y + (n - m_sv_scroll) * c_line_h);
			if (entry.save < 0)
			{
				// A game: its name (the save's own title for a game not in the
				// library), and a rule under it
				std::string name = game_name(entry.serial);
				if (name == entry.serial && !m_sv_saves[lines[n + 1].save].title.empty())
				{
					name = m_sv_saves[lines[n + 1].save].title;
				}
				text(result, spaced(name), 10, f_semibold, c_accent, c_left + 2, y + 34.f, c_list_w - 4);
				overlay_element rule;
				rule.set_pos(c_left, static_cast<s16>(y + 46));
				rule.set_size(c_list_w, 1);
				rule.back_color = color4f(1.f, 1.f, 1.f, 0.12f);
				result.add(rule.get_compiled());
				continue;
			}

			const ps5_saves::save& save = m_sv_saves[entry.save];
			const bool selected = entry.save == m_sv_selected;
			if (selected)
			{
				rounded_rect band;
				band.set_pos(static_cast<s16>(c_left - 10), static_cast<s16>(y + 2));
				band.set_size(c_list_w + 20, c_line_h - 4);
				band.border_radius = 12;
				band.back_color = color4f(c_accent.r, c_accent.g, c_accent.b, 0.16f);
				band.border_size = 1;
				band.border_color = color4f(c_accent.r, c_accent.g, c_accent.b, 0.6f);
				result.add(band.get_compiled());
			}
			icon(result, entry.save, c_left, static_cast<s16>(y + 6), 80, 44, 6);

			const color4f name_colour = save.path.empty() ? c_text_dim : (selected ? c_text : color4f(1.f, 1.f, 1.f, 0.88f));
			text(result, save_name(save), 14, f_semibold, name_colour, c_left + 96, y + 20.f, c_list_w - 104);
			std::string about = save.path.empty() ? std::string("Deleted, a backup kept") : when_text(save.time) + "  ·  " + size_text(save.size);
			if (!save.backups.empty() && !save.path.empty())
			{
				about += save.backups.size() == 1 ? std::string("  ·  1 backup") : fmt::format("  ·  %u backups", save.backups.size());
			}
			else if (save.backups.size() > 1)
			{
				about = fmt::format("Deleted, %u backups kept", save.backups.size());
			}
			text(result, about, 11, f_regular, c_text_dim, c_left + 96, y + 39.f, c_list_w - 104);
		}

		// Where in the list, bottom left
		text(result, fmt::format("%d / %d", m_sv_selected + 1, count), 11, f_medium, c_text_dim, 40, 690.f);

		// The selected save, beside the list
		const ps5_saves::save& save = m_sv_saves[std::clamp(m_sv_selected, 0, count - 1)];
		{
			rounded_rect panel;
			panel.set_pos(c_panel_x, c_list_y);
			panel.set_size(c_panel_w, 440);
			panel.border_radius = 18;
			panel.back_color = c_glass;
			panel.border_size = 1;
			panel.border_color = c_glass_border;
			result.add(panel.get_compiled());

			const s16 x = c_panel_x + 28;
			const u16 w = c_panel_w - 56;
			icon(result, m_sv_selected, x, c_list_y + 24, w, static_cast<u16>(w * c_icon_tex_h / c_icon_tex_w), 10);
			s16 y = static_cast<s16>(c_list_y + 24 + w * c_icon_tex_h / c_icon_tex_w + 16);
			y = static_cast<s16>(y + words(result, save.title.empty() ? game_name(save.serial) : save.title, 17, f_bold, c_text, x, y, w, 46) + 6);
			if (!save.subtitle.empty())
			{
				y = static_cast<s16>(y + words(result, save.subtitle, 13, f_semibold, color4f(1.f, 1.f, 1.f, 0.88f), x, y, w, 36) + 6);
			}
			if (!save.detail.empty())
			{
				y = static_cast<s16>(y + words(result, save.detail, 11, f_regular, c_text_dim, x, y, w, 46) + 8);
			}
			const std::string state = save.path.empty() ? std::string("Deleted: restore it from its backup") : fmt::format("Saved %s  ·  %s", when_text(save.time), size_text(save.size));
			text(result, state, 11, f_medium, c_text, x, y + 8.f, w);
			text(result, save.backups.empty() ? std::string("No backups yet") :
				fmt::format("%u backup%s, the latest %s", save.backups.size(), save.backups.size() == 1 ? "" : "s", when_text(save.backups.front().time)),
				11, f_regular, c_text_dim, x, y + 26.f, w);
		}

		// A question, or a job running, over the page
		if (m_sv_ask != save_ask::none || m_sv_busy)
		{
			overlay_element dim;
			dim.set_size(virtual_width, virtual_height);
			dim.back_color = color4f(0.f, 0.f, 0.f, 0.62f);
			result.add(dim.get_compiled());

			const u16 panel_h = m_sv_busy ? 120 : 220;
			rounded_rect panel;
			panel.set_size(600, panel_h);
			panel.set_pos((virtual_width - 600) / 2, static_cast<s16>((virtual_height - panel_h) / 2));
			panel.border_radius = 20;
			panel.back_color = color4f(0.06f, 0.07f, 0.12f, 0.98f);
			panel.border_size = 1;
			panel.border_color = color4f(1.f, 1.f, 1.f, 0.16f);
			result.add(panel.get_compiled());

			const s16 left = static_cast<s16>(panel.x + 36);
			if (m_sv_busy)
			{
				text(result, m_sv_busy_text, 18, f_bold, c_text, left, panel.y + panel_h / 2.f);
				return;
			}

			std::string title, body, yes;
			if (m_sv_ask == save_ask::restore)
			{
				title = "Restore this save?";
				body = fmt::format("Its latest backup, from %s, replaces \"%s\". The save as it is now is backed up first, so this can be undone.",
					save.backups.empty() ? std::string() : when_text(save.backups.front().time), save_name(save));
				yes = "Restore";
			}
			else if (m_sv_ask == save_ask::remove)
			{
				title = "Delete this save?";
				body = fmt::format("\"%s\" is backed up first, to rpcs3/save_backups, so it can be restored from this page.", save_name(save));
				yes = "Delete";
			}
			else
			{
				title = "Delete its backups?";
				body = fmt::format("All %u backup%s of \"%s\" are deleted for good.", save.backups.size(), save.backups.size() == 1 ? "" : "s", save_name(save));
				yes = "Delete";
			}
			const s16 title_y = static_cast<s16>(panel.y + 32);
			const u16 title_h = words(result, title, 20, f_bold, c_text, left, title_y, 528, 60);
			words(result, body, 12, f_regular, c_text_dim, left, static_cast<s16>(title_y + title_h + 14), 528, 80);

			const f32 hints_y = panel.y + panel_h - 36.f;
			s16 x = left;
			for (const auto& [image, what] : {std::pair<u8, std::string>{resource_config::confirm_button_resource(), yes}, std::pair<u8, std::string>{resource_config::cancel_button_resource(), "Cancel"}})
			{
				image_view glyph;
				glyph.set_image_resource(image);
				glyph.set_size(20, 20);
				glyph.back_color.a = 0.f;
				glyph.set_pos(x, static_cast<s16>(hints_y - 10));
				result.add(glyph.get_compiled());
				const auto l = text(result, what, 12, f_medium, c_text, static_cast<s16>(x + 26), hints_y);
				x = static_cast<s16>(x + 26 + l->w + 28);
			}
		}
	}
}
