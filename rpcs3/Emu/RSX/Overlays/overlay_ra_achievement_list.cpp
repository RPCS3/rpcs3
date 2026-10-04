#include "stdafx.h"
#ifdef RPCS3_RA_ENABLED

#include "overlay_ra_achievement_list.h"
#include "overlay_manager.h"
#include "Emu/System.h"

#include <ctime>

namespace rsx::overlays
{
	// ── Layout constants ──────────────────────────────────────────────────────
	static constexpr u16 LIST_X  = 0;
	static constexpr u16 LIST_Y  = 80;
	static constexpr u16 LIST_W  = overlay::virtual_width;
	static constexpr u16 LIST_H  = 590;

	static constexpr color4f COL_TRANSPARENT = {0.f, 0.f, 0.f, 0.f};
	static constexpr color4f COL_TEXT_DIM    = {0.6f, 0.6f, 0.6f, 1.f};
	static constexpr color4f COL_TEXT_HINT   = {0.5f, 0.5f, 0.5f, 1.f};
	static constexpr color4f COL_WHITE       = {1.f,  1.f,  1.f,  1.f};
	static constexpr color4f COL_GREEN       = {0.2f, 0.9f, 0.2f, 1.f};
	static constexpr color4f COL_GOLD        = {0.9f, 0.75f,0.2f, 1.f};

	// ── ra_ach_entry ─────────────────────────────────────────────────────────

	ra_achievement_list_overlay::ra_ach_entry::ra_ach_entry(const rpcs3::ra::ra_achievement_item& item)
	{
		const bool locked = (item.state != RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED);

		back_color = COL_TRANSPARENT;

		// Icon (110×110 container)
		std::unique_ptr<overlay_element> img = std::make_unique<image_view>();
		img->set_size(110, 110);
		img->set_padding(11, 11, 11, 11);
		img->back_color = COL_TRANSPARENT;

		const std::string badge_path = fs::get_config_dir() + "RACache/Badge/" + item.badge_name + ".png";
		if (!item.badge_name.empty() && fs::exists(badge_path))
		{
			icon_data = std::make_unique<image_info>(badge_path, locked);
			static_cast<image_view*>(img.get())->set_raw_image(icon_data.get());
		}
		// No fallback — leave transparent when badge not available

		// Text column
		std::unique_ptr<overlay_element> col    = std::make_unique<vertical_layout>();
		std::unique_ptr<overlay_element> pad    = std::make_unique<spacer>();
		std::unique_ptr<overlay_element> header = std::make_unique<label>();
		std::unique_ptr<overlay_element> sub    = std::make_unique<label>(item.description);

		col->back_color = COL_TRANSPARENT;
		pad->set_size(1, 8);

		std::string title_str = item.title;
		if (!locked)
			title_str += fmt::format("  (%u pts)", item.points);
		else if (item.measured_percent > 0.f && !item.measured_progress.empty())
			title_str += "  " + item.measured_progress;

		header->set_size(1050, 36);
		header->set_font("Arial", 16);
		header->set_wrap_text(true);
		static_cast<label*>(header.get())->set_text(title_str);
		header->back_color = COL_TRANSPARENT;
		header->fore_color = locked ? COL_TEXT_DIM : COL_WHITE;

		sub->set_size(1050, 0);
		sub->set_font("Arial", 14);
		sub->set_wrap_text(true);
		static_cast<label*>(sub.get())->auto_resize(true);
		sub->back_color = COL_TRANSPARENT;
		sub->fore_color = locked ? color4f{0.45f, 0.45f, 0.45f, 1.f} : COL_TEXT_DIM;

		auto* col_layout = static_cast<vertical_layout*>(col.get());
		col_layout->pack_padding = 4;
		col_layout->add_element(pad);
		col_layout->add_element(header);
		col_layout->add_element(sub);

		pack_padding = 12;
		add_element(img);
		add_element(col);
	}

	// ── ra_game_entry ─────────────────────────────────────────────────────────

	ra_achievement_list_overlay::ra_game_entry::ra_game_entry(const rpcs3::ra::ra_game_item& item)
		: game_id(item.id), is_current(item.is_current_game)
	{
		back_color = COL_TRANSPARENT;

		std::unique_ptr<overlay_element> col   = std::make_unique<vertical_layout>();
		std::unique_ptr<overlay_element> pad   = std::make_unique<spacer>();
		std::unique_ptr<overlay_element> title = std::make_unique<label>(item.title);
		std::unique_ptr<overlay_element> stats = std::make_unique<label>();

		col->back_color = COL_TRANSPARENT;
		pad->set_size(1, 10);

		title->set_size(1180, 36);
		title->set_font("Arial", 18);
		title->back_color = COL_TRANSPARENT;
		title->fore_color = item.is_current_game ? COL_GOLD : COL_WHITE;

		const float pct = item.num_achievements > 0
		                ? 100.f * item.num_unlocked / item.num_achievements
		                : 0.f;
		static_cast<label*>(stats.get())->set_text(fmt::format("%u / %u Unlocked  (%.0f%%)",
		                item.num_unlocked, item.num_achievements, pct));
		stats->set_size(1180, 28);
		stats->set_font("Arial", 14);
		stats->back_color = COL_TRANSPARENT;
		stats->fore_color = COL_TEXT_DIM;
		static_cast<label*>(stats.get())->auto_resize();

		auto* col_layout = static_cast<vertical_layout*>(col.get());
		col_layout->pack_padding = 4;
		col_layout->add_element(pad);
		col_layout->add_element(title);
		col_layout->add_element(stats);

		pack_padding = 20;
		set_size(LIST_W, 80);
		add_element(col);
	}

	// ── Constructor ───────────────────────────────────────────────────────────

	ra_achievement_list_overlay::ra_achievement_list_overlay()
	{
		m_allow_input_on_pause   = true;
		m_keyboard_input_enabled = true;

		m_bg = std::make_unique<overlay_element>();
		m_bg->set_size(virtual_width, virtual_height);
		m_bg->back_color = {0.04f, 0.04f, 0.08f, 0.80f};

		m_title_label = std::make_unique<label>();
		m_title_label->set_font("Arial", 22);
		m_title_label->set_pos(20, 18);
		m_title_label->set_size(900, 34);
		m_title_label->back_color = COL_TRANSPARENT;

		m_subtitle_label = std::make_unique<label>();
		m_subtitle_label->set_font("Arial", 15);
		m_subtitle_label->set_pos(20, 55);
		m_subtitle_label->set_size(900, 22);
		m_subtitle_label->back_color = COL_TRANSPARENT;
		m_subtitle_label->fore_color = COL_TEXT_DIM;

		m_list = std::make_unique<list_view>(LIST_W, LIST_H, false);
		m_list->set_pos(LIST_X, LIST_Y);
		m_list->back_color = COL_TRANSPARENT;
		m_list->hide_prompt_buttons();
		m_list->hide_scroll_indicator();

		m_det_icon = std::make_unique<image_view>();
		m_det_icon->set_size(160, 160);
		m_det_icon->set_pos(40, 140);
		m_det_icon->set_padding(0, 0, 0, 0);
		m_det_icon->back_color = COL_TRANSPARENT;

		auto init_label = [](std::unique_ptr<label>& lbl, u16 y, u16 h, u16 fs)
		{
			lbl = std::make_unique<label>();
			lbl->set_font("Arial", fs);
			lbl->set_pos(220, y);
			lbl->set_size(1000, h);
			lbl->set_wrap_text(true);
			lbl->back_color = COL_TRANSPARENT;
		};
		init_label(m_det_title,  140, 56, 20);
		init_label(m_det_desc,   205, 90, 15);
		init_label(m_det_points, 310, 28, 15);
		init_label(m_det_rarity, 345, 28, 15);
		init_label(m_det_status, 380, 28, 15);
		m_det_desc->fore_color   = COL_TEXT_DIM;
		m_det_points->fore_color = COL_TEXT_DIM;
		m_det_rarity->fore_color = COL_TEXT_DIM;

		m_loading_label = std::make_unique<label>("Loading...");
		m_loading_label->set_font("Arial", 18);
		m_loading_label->set_pos(30, LIST_Y + LIST_H / 2 - 12);
		m_loading_label->auto_resize();
		m_loading_label->back_color = COL_TRANSPARENT;
		m_loading_label->fore_color = COL_TEXT_DIM;

		m_hint = std::make_unique<label>();
		m_hint->set_font("Arial", 13);
		m_hint->set_pos(20, 690);
		m_hint->set_size(virtual_width - 40, 22);
		m_hint->back_color = COL_TRANSPARENT;
		m_hint->fore_color = COL_TEXT_HINT;

		m_fade.duration_sec = 0.12f;
		return_code = selection_code::canceled;
	}

	// ── rebuild_ach_list ─────────────────────────────────────────────────────

	void ra_achievement_list_overlay::rebuild_ach_list()
	{
		m_list->clear_items();
		for (const auto& item : m_achievements)
		{
			auto entry = std::make_unique<ra_ach_entry>(item);
			m_list->add_entry(entry);
		}
		if (!m_achievements.empty())
			m_list->select_entry(0);
	}

	// ── rebuild_games_list ───────────────────────────────────────────────────

	void ra_achievement_list_overlay::rebuild_games_list()
	{
		m_list->clear_items();

		{
			auto entry = std::make_unique<ra_game_entry>(m_current_game_item);
			m_list->add_entry(entry);
		}

		for (const auto& g : m_games)
		{
			if (g.id == m_current_game_item.id) continue;
			auto entry = std::make_unique<ra_game_entry>(g);
			m_list->add_entry(entry);
		}

		if (m_list->get_elements_count() > 0)
			m_list->select_entry(0);
	}

	// ── open_detail ──────────────────────────────────────────────────────────

	void ra_achievement_list_overlay::open_detail(s32 index)
	{
		if (index < 0 || index >= static_cast<s32>(m_achievements.size()))
			return;

		const auto& item = m_achievements[index];
		const bool locked = (item.state != RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED);

		m_det_icon_data.reset();
		const std::string badge = fs::get_config_dir() + "RACache/Badge/" + item.badge_name + ".png";
		if (!item.badge_name.empty() && fs::exists(badge))
		{
			m_det_icon_data = std::make_unique<image_info>(badge, locked);
			m_det_icon->set_raw_image(m_det_icon_data.get());
		}
		else
		{
			m_det_icon->set_raw_image(nullptr);
		}

		m_det_title->set_text(item.title);
		m_det_title->fore_color = locked ? COL_TEXT_DIM : COL_WHITE;

		m_det_desc->set_text(item.description);
		m_det_points->set_text(fmt::format("Points: %u", item.points));

		if (item.rarity > 0.f)
			m_det_rarity->set_text(fmt::format("Rarity: %.1f%%  (%.1f%% hardcore)", item.rarity, item.rarity_hardcore));
		else
			m_det_rarity->set_text("Rarity: N/A");

		if (!locked)
		{
			std::string s = "Unlocked";
			if (item.unlock_time)
			{
				std::time_t t = item.unlock_time;
				std::tm tm{};
#ifdef _WIN32
				localtime_s(&tm, &t);
#else
				localtime_r(&t, &tm);
#endif
				char buf[32] = {};
				std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm);
				s += fmt::format(" on %s", buf);
			}
			m_det_status->set_text(s);
			m_det_status->fore_color = COL_GREEN;
		}
		else if (item.measured_percent > 0.f && !item.measured_progress.empty())
		{
			m_det_status->set_text(fmt::format("Locked  —  %s", item.measured_progress));
			m_det_status->fore_color = COL_GOLD;
		}
		else
		{
			m_det_status->set_text("Locked");
			m_det_status->fore_color = COL_TEXT_DIM;
		}
	}

	// ── update_hint ──────────────────────────────────────────────────────────

	void ra_achievement_list_overlay::update_hint()
	{
		if (m_in_detail)
		{
			m_hint->set_text("O: Back");
			return;
		}
		switch (m_view)
		{
		case view_state::current_game:
		case view_state::other_game:
			m_hint->set_text("X: Details   O: Games   L1/R1: Page   Start: Resume");
			break;
		case view_state::games_list:
			m_hint->set_text("X: Open   O: Close   L1/R1: Page   Start: Resume");
			break;
		}
	}

	// ── update_title ─────────────────────────────────────────────────────────

	void ra_achievement_list_overlay::update_title()
	{
		if (m_in_detail) return;

		switch (m_view)
		{
		case view_state::games_list:
			m_title_label->set_text("Trophies");
			m_subtitle_label->set_text("Recently played games");
			break;
		case view_state::current_game:
			m_title_label->set_text(m_current_game_item.title.empty() ? "Achievements" : m_current_game_item.title);
			{
				u32 unlocked = 0;
				for (const auto& a : m_achievements)
					if (a.state == RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED) ++unlocked;
				m_subtitle_label->set_text(fmt::format("%u / %zu Unlocked", unlocked, m_achievements.size()));
			}
			break;
		case view_state::other_game:
			m_title_label->set_text(m_other_game_title.empty() ? "Achievements" : m_other_game_title);
			{
				u32 unlocked = 0;
				for (const auto& a : m_achievements)
					if (a.state == RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED) ++unlocked;
				m_subtitle_label->set_text(fmt::format("%u / %zu Unlocked", unlocked, m_achievements.size()));
			}
			break;
		}
	}

	// ── start_load_games ─────────────────────────────────────────────────────

	void ra_achievement_list_overlay::start_load_games()
	{
		if (m_games_loading.exchange(true)) return;
		m_games_ready = false;
		m_games_error = false;

		rpcs3::ra::fetch_recently_played_games([this](std::vector<rpcs3::ra::ra_game_item> games, bool ok)
		{
			m_games         = std::move(games);
			m_games_error   = !ok;
			m_games_ready   = true;
			m_games_loading = false;
		});
	}

	// ── start_load_other_ach ─────────────────────────────────────────────────

	void ra_achievement_list_overlay::start_load_other_ach(uint32_t game_id)
	{
		if (m_ach_loading.exchange(true)) return;
		m_ach_ready = false;

		rpcs3::ra::fetch_game_achievements_for_id(game_id,
		    [this](std::vector<rpcs3::ra::ra_achievement_item> achs, std::string title, bool ok)
		{
			if (ok)
			{
				m_achievements     = std::move(achs);
				m_other_game_title = std::move(title);
			}
			m_ach_ready   = true;
			m_ach_loading = false;
		});
	}

	// ── close_overlay ────────────────────────────────────────────────────────

	void ra_achievement_list_overlay::close_overlay()
	{
		m_fade.current   = color4f(1.f);
		m_fade.end       = color4f(0.f);
		m_fade.active    = true;
		m_fade.on_finish = [this] { close(true, true); };
	}

	// ── update ───────────────────────────────────────────────────────────────

	void ra_achievement_list_overlay::update(u64 timestamp_us)
	{
		if (m_fade.active)
			m_fade.update(timestamp_us);

		if (m_view == view_state::games_list && m_games_ready.exchange(false))
		{
			rebuild_games_list();
			update_title();
		}

		if (m_view == view_state::other_game && m_ach_ready.exchange(false))
		{
			rebuild_ach_list();
			update_title();
		}
	}

	// ── on_button_pressed ────────────────────────────────────────────────────

	void ra_achievement_list_overlay::on_button_pressed(pad_button btn, bool is_auto_repeat)
	{
		if (m_fade.active) return;

		if (btn == pad_button::start)
		{
			Emu.Resume();
			close(true, true);
			return;
		}

		if (m_in_detail)
		{
			if (btn == pad_button::circle)
			{
				play_sound(sound_effect::cancel);
				m_in_detail = false;
				update_hint();
			}
			return;
		}

		switch (m_view)
		{
		case view_state::current_game:
		case view_state::other_game:
		{
			switch (btn)
			{
			case pad_button::circle:
				play_sound(sound_effect::cancel);
				m_view = view_state::games_list;
				if (!m_games_ready && !m_games_loading)
					start_load_games();
				rebuild_games_list();
				update_title();
				update_hint();
				break;

			case pad_button::cross:
				if (m_list->get_elements_count() > 0 && !m_achievements.empty())
				{
					play_sound(sound_effect::accept);
					open_detail(m_list->get_selected_index());
					m_in_detail = true;
					update_hint();
				}
				break;

			case pad_button::dpad_up:
			case pad_button::ls_up:
				if (!is_auto_repeat) play_sound(sound_effect::cursor);
				m_list->select_previous();
				break;
			case pad_button::dpad_down:
			case pad_button::ls_down:
				if (!is_auto_repeat) play_sound(sound_effect::cursor);
				m_list->select_next();
				break;
			case pad_button::L1:
				play_sound(sound_effect::cursor);
				m_list->select_previous(10);
				break;
			case pad_button::R1:
				play_sound(sound_effect::cursor);
				m_list->select_next(10);
				break;
			default: break;
			}
			break;
		}

		case view_state::games_list:
		{
			switch (btn)
			{
			case pad_button::circle:
				play_sound(sound_effect::cancel);
				close_overlay();
				break;

			case pad_button::cross:
			{
				if (m_ach_loading || m_list->get_elements_count() == 0) break;
				const overlay_element* sel = m_list->get_selected_entry();
				if (!sel) break;
				const auto* entry = dynamic_cast<const ra_game_entry*>(sel);
				if (!entry) break;

				play_sound(sound_effect::accept);

				if (entry->is_current)
				{
					m_view = view_state::current_game;
					// m_achievements already holds current game's list (set in show())
				}
				else
				{
					m_view = view_state::other_game;
					start_load_other_ach(entry->game_id);
					m_achievements.clear();
				}
				rebuild_ach_list();
				update_title();
				update_hint();
				break;
			}

			case pad_button::dpad_up:
			case pad_button::ls_up:
				if (!is_auto_repeat) play_sound(sound_effect::cursor);
				m_list->select_previous();
				break;
			case pad_button::dpad_down:
			case pad_button::ls_down:
				if (!is_auto_repeat) play_sound(sound_effect::cursor);
				m_list->select_next();
				break;
			case pad_button::L1:
				play_sound(sound_effect::cursor);
				m_list->select_previous(10);
				break;
			case pad_button::R1:
				play_sound(sound_effect::cursor);
				m_list->select_next(10);
				break;
			default: break;
			}
			break;
		}
		}
	}

	// ── get_compiled ─────────────────────────────────────────────────────────

	compiled_resource ra_achievement_list_overlay::get_compiled()
	{
		if (!visible) return {};

		compiled_resource result;
		result.add(m_bg->get_compiled());
		result.add(m_title_label->get_compiled());
		result.add(m_subtitle_label->get_compiled());

		if (m_in_detail)
		{
			result.add(m_det_icon->get_compiled());
			result.add(m_det_title->get_compiled());
			result.add(m_det_desc->get_compiled());
			result.add(m_det_points->get_compiled());
			result.add(m_det_rarity->get_compiled());
			result.add(m_det_status->get_compiled());
		}
		else if (m_ach_loading || (m_view == view_state::games_list && m_games_loading))
		{
			result.add(m_loading_label->get_compiled());
		}
		else
		{
			result.add(m_list->get_compiled());
		}

		result.add(m_hint->get_compiled());
		m_fade.apply(result);
		return result;
	}

	// ── show ─────────────────────────────────────────────────────────────────

	void ra_achievement_list_overlay::show(std::vector<rpcs3::ra::ra_achievement_item> achievements,
	                                        rpcs3::ra::ra_game_item current_game)
	{
		m_current_game_item = std::move(current_game);
		m_achievements      = std::move(achievements);
		m_view              = view_state::current_game;
		m_in_detail         = false;

		rebuild_ach_list();
		update_title();
		update_hint();

		m_fade.current = color4f(0.f);
		m_fade.end     = color4f(1.f);
		m_fade.active  = true;
		visible = true;

		const auto notify = std::make_shared<atomic_t<u32>>(0);
		auto& overlayman  = g_fxo->get<rsx::overlays::display_manager>();

		overlayman.attach_thread_input(
			uid, "RA Achievement List",
			[notify]() { *notify = 1; notify->notify_one(); }
		);

		while (!Emu.IsStopped() && !*notify)
		{
			if (Emu.IsRunning())
			{
				close(true, true);
				break;
			}
			notify->wait(0, atomic_wait_timeout{1'000'000});
		}
	}

} // namespace rsx::overlays

#endif // RPCS3_RA_ENABLED
