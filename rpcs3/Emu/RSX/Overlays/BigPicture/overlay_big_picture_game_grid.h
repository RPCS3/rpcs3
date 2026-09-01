#pragma once

#include "Emu/RSX/Overlays/HomeMenu/overlay_home_menu_page.h"
#include "Emu/game_enumeration.h"
#include "overlay_big_picture_game_info.h"

#include <functional>

namespace rsx
{
	namespace overlays
	{
		struct big_picture_game_details;

		// A single selectable tile (icon + title) inside the game grid.
		struct big_picture_game_tile : public vertical_layout
		{
			big_picture_game_tile(const big_picture_game_info& entry, u16 tile_width);

			// PS3 ICON0.PNG is always 320x176 - keep tiles at that exact aspect ratio instead of stretching it.
			static constexpr u16 icon_height(u16 tile_width) { return static_cast<u16>(tile_width * 176 / 320); }

			// Already-decoded icon, reused by the detail panel to avoid re-decoding and stale-texture-cache issues.
			const image_info* get_icon_data() const { return m_icon_data.get(); }

			// The icon's own live bounding box (it is offset within the tile by the layout's padding,
			// so callers must not assume it starts at the tile's own x/y).
			const overlay_element* get_icon_view() const { return m_icon_view; }

		private:
			std::unique_ptr<image_info> m_icon_data;
			overlay_element* m_icon_view = nullptr; // non-owning, owned by vertical_layout::m_items
		};

		// Controller-friendly grid of installed games. Cross opens a detail panel with a "Start" prompt.
		struct big_picture_game_grid : public home_menu_page
		{
			big_picture_game_grid(s16 x, s16 y, u16 width, u16 height, home_menu_page* parent, std::function<void(std::string, std::string)> on_game_selected);
			~big_picture_game_grid() override;

			page_navigation handle_button_press(pad_button button_press, bool is_auto_repeat, u64 auto_repeat_interval_ms) override;
			compiled_resource& get_compiled() override;

		private:
			void start_reload();
			void finish_reload(std::vector<std::unique_ptr<big_picture_game_tile>>&& tiles);
			void select_tile(s32 index);

			u16 column(s32 tile_index) const;
			u16 row(s32 tile_index) const;

			static constexpr u16 m_columns = 5;
			static constexpr u16 m_tile_size = 200;

			std::mutex m_reload_mutex;
			std::unique_ptr<named_thread<std::function<void()>>> m_game_enumeration_thread;

			game_enumeration<big_picture_game_info> m_game_enumeration;
			std::vector<big_picture_game_info> m_games;
			std::vector<big_picture_game_tile*> m_tiles; // non-owning, owned by m_grid
			std::unique_ptr<vertical_layout> m_grid;
			std::unique_ptr<label> m_placeholder_text;
			std::unique_ptr<rounded_rect> m_highlight;
			std::unique_ptr<big_picture_game_details> m_details;
			std::function<void(std::string, std::string)> m_on_game_selected;

			image_button m_back_hint{ 120, 30 };
			image_button m_select_hint{ 120, 30 };

			s32 m_selected_index = 0;
			u16 m_row_stride = 0;
			u16 m_content_height = 0;

			atomic_t<bool> m_loading = true;
		};
	}
}
