#pragma once
#ifdef RPCS3_RA_ENABLED

#include "overlays.h"
#include "overlay_list_view.hpp"
#include "../../RetroAchievements.h"

#include <vector>
#include <memory>

namespace rsx::overlays
{
	struct ra_achievement_list_overlay : public user_interface
	{
	private:
		enum class view_state { games_list, current_game, other_game };

		// ── List entry for achievements ────────────────────────────────────
		struct ra_ach_entry : public horizontal_layout
		{
			std::unique_ptr<image_info> icon_data;
			explicit ra_ach_entry(const rpcs3::ra::ra_achievement_item& item);
		};

		// ── List entry for games ───────────────────────────────────────────
		struct ra_game_entry : public horizontal_layout
		{
			uint32_t game_id = 0;
			bool is_current  = false;
			explicit ra_game_entry(const rpcs3::ra::ra_game_item& item);
		};

		// ── Background + title bar ─────────────────────────────────────────
		std::unique_ptr<overlay_element> m_bg;
		std::unique_ptr<label>           m_title_label;
		std::unique_ptr<label>           m_subtitle_label;

		// ── Shared list ────────────────────────────────────────────────────
		std::unique_ptr<list_view>       m_list;

		// ── Detail view (achievement detail) ──────────────────────────────
		std::unique_ptr<image_view>      m_det_icon;
		std::unique_ptr<image_info>      m_det_icon_data;
		std::unique_ptr<label>           m_det_title;
		std::unique_ptr<label>           m_det_desc;
		std::unique_ptr<label>           m_det_points;
		std::unique_ptr<label>           m_det_rarity;
		std::unique_ptr<label>           m_det_status;

		// ── Hint bar ──────────────────────────────────────────────────────
		std::unique_ptr<label>           m_hint;

		// ── Loading label ─────────────────────────────────────────────────
		std::unique_ptr<label>           m_loading_label;

		// ── Animation ────────────────────────────────────────────────────
		animation_color_interpolate      m_fade;

		// ── State ────────────────────────────────────────────────────────
		view_state m_view = view_state::current_game;
		bool       m_in_detail = false;

		// Achievements for the current / other selected game
		std::vector<rpcs3::ra::ra_achievement_item> m_achievements;
		std::string m_other_game_title;

		// Games list
		std::vector<rpcs3::ra::ra_game_item> m_games;
		atomic_t<bool> m_games_loading{false};
		atomic_t<bool> m_games_ready{false};
		atomic_t<bool> m_games_error{false};

		// Other game achievements fetch
		atomic_t<bool> m_ach_loading{false};
		atomic_t<bool> m_ach_ready{false};

		// Current game item (set in show())
		rpcs3::ra::ra_game_item m_current_game_item;

		void rebuild_ach_list();
		void rebuild_games_list();
		void open_detail(s32 index);
		void close_overlay();
		void start_load_games();
		void start_load_other_ach(uint32_t game_id);
		void update_hint();
		void update_title();

	public:
		ra_achievement_list_overlay();

		void update(u64 timestamp_us) override;
		void on_button_pressed(pad_button button_press, bool is_auto_repeat) override;
		compiled_resource get_compiled() override;

		void show(std::vector<rpcs3::ra::ra_achievement_item> achievements,
		          rpcs3::ra::ra_game_item current_game);
	};
}

#endif // RPCS3_RA_ENABLED
