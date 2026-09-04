#include "stdafx.h"
#include "overlay_big_picture_game_grid.h"
#include "overlay_big_picture_game_details.h"
#include "Emu/system_config.h"
#include "Utilities/Thread.h"

#include <algorithm>

namespace rsx
{
	namespace overlays
	{
		big_picture_game_tile::big_picture_game_tile(const big_picture_game_info& entry, u16 tile_width)
		{
			pack_padding = 8;
			back_color.a = 0.f;

			const u16 icon_h = icon_height(tile_width);

			std::unique_ptr<overlay_element> icon = std::make_unique<image_view>();
			icon->set_size(tile_width, icon_h);
			icon->back_color = color4f(1.f, 1.f, 1.f, 0.08f);

			if (auto icon_data = entry.load_icon())
			{
				m_icon_data = std::move(icon_data);
				// The renderer's texture cache is keyed by this object's address, which can be reused by an
				// unrelated image after the old one is freed - force a re-upload instead of trusting the cache.
				m_icon_data->dirty = true;
				static_cast<image_view*>(icon.get())->set_keep_aspect_ratio(true);
				static_cast<image_view*>(icon.get())->set_raw_image(m_icon_data.get());
			}
			else
			{
				static_cast<image_view*>(icon.get())->set_image_resource(resource_config::standard_image_resource::new_entry);
			}

			std::unique_ptr<overlay_element> title = std::make_unique<label>(entry.name.empty() ? entry.serial : entry.name);
			title->set_font("Arial", 13);
			title->set_size(tile_width, 84);
			title->set_wrap_text(true);
			title->align_text(text_align::center);
			title->back_color.a = 0.f;

			m_icon_view = add_element(icon);
			add_element(title);
		}

		big_picture_game_grid::big_picture_game_grid(s16 x, s16 y, u16 width, u16 height, home_menu_page* parent, std::function<void(std::string, std::string)> on_game_selected)
			: home_menu_page(x, y, width, height, false, parent, get_localized_string(localized_string_id::BIG_PICTURE_MENU_GAMES))
			, m_on_game_selected(std::move(on_game_selected))
		{
			m_placeholder_text = std::make_unique<label>(get_localized_string(localized_string_id::BIG_PICTURE_LOADING));
			m_placeholder_text->set_font("Arial", 20);
			m_placeholder_text->set_pos(x, y + (height / 2) - 20);
			m_placeholder_text->set_size(width, 40);
			m_placeholder_text->align_text(text_align::center);
			m_placeholder_text->back_color.a = 0.f;

			// Bezel around game cover art.
			m_highlight = std::make_unique<rounded_rect>();
			m_highlight->border_radius = 6;
			m_highlight->border_size = 4;
			m_highlight->border_color = color4f(0.3f, 0.65f, 1.f, 1.f);
			m_highlight->back_color = color4f(0.f, 0.f, 0.f, 0.f);
			m_highlight->pulse_effect_enabled = true;

			m_back_hint.set_image_resource(resource_config::standard_image_resource::circle);
			m_back_hint.set_text(localized_string_id::BIG_PICTURE_HINT_BACK);
			m_back_hint.set_font("Arial", 16);
			m_back_hint.set_pos(x + width - 2 * (30 + 120), y + height + 20);

			m_select_hint.set_image_resource(resource_config::standard_image_resource::cross);
			m_select_hint.set_text(localized_string_id::BIG_PICTURE_HINT_SELECT);
			m_select_hint.set_font("Arial", 16);
			m_select_hint.set_pos(x + width - (30 + 120), y + height + 20);

			m_details = std::make_unique<big_picture_game_details>(x, y, width, height);

			start_reload();
		}

		big_picture_game_grid::~big_picture_game_grid()
		{
			if (m_game_enumeration_thread)
			{
				*m_game_enumeration_thread = thread_state::aborting; // abort
				(*m_game_enumeration_thread)(); // wait
				m_game_enumeration_thread.reset(); // join
			}
		}

		void big_picture_game_grid::start_reload()
		{
			rsx_log.notice("Big Picture Mode: reload() start");

			m_loading = true;
			m_games.clear();
			m_tiles.clear();

			// Configure game enumeration
			m_game_enumeration.initialize_paths();
			m_game_enumeration.set_localization(g_cfg.sys.language, "Unknown", [](const std::string&){ return std::string(); });
			m_game_enumeration.set_show_custom_icons(true);
			m_game_enumeration.set_prefer_game_data_icons(true);
			m_game_enumeration.set_play_hover_movies(true);
			m_game_enumeration.set_play_hover_music(true);
			m_game_enumeration.set_canceled_callback([](){ return thread_ctrl::state() == thread_state::aborting; });

			m_game_enumeration_thread = std::make_unique<named_thread<std::function<void()>>>("BPM Reload", [this]()
			{
				// Parse directories
				m_game_enumeration.parse_directories();

				// Add VFS entry
				m_game_enumeration.add_vfs_entry();

				// Remove duplicate entries
				m_game_enumeration.remove_duplicates();

				// Parse entries (multithreaded)
				const std::vector<game_enumeration<big_picture_game_info>::path_entry>& entries = m_game_enumeration.path_entries();
				usz thread_count = std::min<usz>(utils::get_thread_count(), entries.size());
				map_workload("BPM Parser "sv, thread_count, entries.size(), [this, &entries](usz index)
				{
					m_game_enumeration.parse_entry(entries[index]);
				});

				// Try to update the app version for disc games if there is a patch
				// Also try to find updated game icons and movies
				m_game_enumeration.apply_patches();

				// Get final enumerated games
				for (big_picture_game_info& game : m_game_enumeration.take_games())
				{
					if (!game.bootable) continue; // Let's restrict this to bootable entries

					m_games.push_back(std::move(game));
				}

				// Reset enumeration
				m_game_enumeration.clear(true);

				// Sort by name
				std::sort(m_games.begin(), m_games.end(), [](const big_picture_game_info& a, const big_picture_game_info& b)
				{
					return a.name < b.name;
				});

				// Create tiles (multithreaded)
				std::vector<std::unique_ptr<big_picture_game_tile>> tiles(m_games.size());
				thread_count = std::min<usz>(utils::get_thread_count(), tiles.size());
				map_workload("BPM Tiles "sv, thread_count, tiles.size(), [this, &tiles](usz index)
				{
					tiles[index] = std::make_unique<big_picture_game_tile>(m_games[index], m_tile_size);
				});

				finish_reload(std::move(tiles));
			});
		}

		void big_picture_game_grid::finish_reload(std::vector<std::unique_ptr<big_picture_game_tile>>&& tiles)
		{
			std::lock_guard lock(m_mutex);

			if (thread_ctrl::state() == thread_state::aborting)
			{
				m_loading = false;
				return;
			}

			m_grid = std::make_unique<vertical_layout>();
			m_grid->set_pos(x, y);
			m_grid->pack_padding = 20;

			std::unique_ptr<horizontal_layout> row;

			for (usz i = 0; i < tiles.size(); i++)
			{
				if (i % m_columns == 0)
				{
					if (row)
					{
						m_grid->add_element(row);
					}

					row = std::make_unique<horizontal_layout>();
					row->pack_padding = 20;
				}

				m_tiles.push_back(row->add_element(tiles[i]));
			}

			if (row)
			{
				m_grid->add_element(row);
			}

			// Freeze the grid at the page's viewport height and scroll it manually instead of letting it
			// auto-size to the full (potentially much taller) content height of every row combined.
			m_content_height = m_grid->h;
			m_row_stride = m_tiles.empty() ? 0 : static_cast<u16>(m_tiles.front()->h + m_grid->pack_padding);
			m_grid->auto_resize = false;
			m_grid->set_size(m_grid->w, h);
			m_grid->scroll_offset_value = 0;

			m_selected_index = m_tiles.empty() ? 0 : std::clamp(m_selected_index, 0, static_cast<s32>(m_tiles.size()) - 1);

			if (m_tiles.empty())
			{
				m_placeholder_text->set_text(get_localized_string(localized_string_id::BIG_PICTURE_NO_GAMES_FOUND));
			}
			else
			{
				select_tile(m_selected_index);
			}

			rsx_log.notice("Big Picture Mode: reload() finished, games=%u, tiles=%u", m_games.size(), m_tiles.size());

			m_loading = false;
			refresh();
		}

		void big_picture_game_grid::select_tile(s32 index)
		{
			rsx_log.notice("Big Picture Mode: select_tile(%d), tile_count=%u", index, m_tiles.size());

			if (index < 0 || static_cast<usz>(index) >= m_tiles.size())
			{
				return;
			}

			m_selected_index = index;

			// Scroll the selected row into view if the grid has more rows than fit on screen.
			if (m_row_stride > 0 && m_grid)
			{
				const s32 viewport_h = h;
				const s32 row = m_selected_index / m_columns;
				const s32 row_top = row * m_row_stride;
				const s32 row_bottom = row_top + m_row_stride;
				const s32 max_offset = std::max(0, static_cast<s32>(m_content_height) - viewport_h);

				s32 offset = m_grid->scroll_offset_value;
				if (row_top < offset)
				{
					offset = row_top;
				}
				else if (row_bottom > offset + viewport_h)
				{
					offset = row_bottom - viewport_h;
				}

				m_grid->scroll_offset_value = static_cast<u16>(std::clamp(offset, 0, max_offset));
				m_grid->refresh();
			}

			const big_picture_game_tile* tile = m_tiles[m_selected_index];
			const overlay_element* icon = tile->get_icon_view();

			// Pad the bezel out a few pixels past the cover art so the border doesn't clip its corners.
			constexpr s16 bezel_margin = 3;
			m_highlight->set_pos(icon->x - bezel_margin, icon->y - m_grid->scroll_offset_value - bezel_margin);
			m_highlight->set_size(icon->w + bezel_margin * 2, icon->h + bezel_margin * 2);
			m_highlight->set_sinus_offset(1.6f);
			m_highlight->refresh();

			refresh();
		}

		u16 big_picture_game_grid::column(s32 tile_index) const
		{
			return (tile_index < 0) ? 0 : (tile_index % m_columns);
		}

		u16 big_picture_game_grid::row(s32 tile_index) const
		{
			return (tile_index < 0) ? 0 : (tile_index / m_columns);
		}

		page_navigation big_picture_game_grid::handle_button_press(pad_button button_press, bool is_auto_repeat, u64 auto_repeat_interval_ms)
		{
			std::lock_guard lock(m_mutex);

			if (m_loading) return page_navigation::stay;

			const bool do_play_sound = !is_auto_repeat || auto_repeat_interval_ms >= user_interface::m_auto_repeat_ms_interval_default;

			if (m_details && m_details->is_visible())
			{
				const auto details_result = m_details->handle_button_press(button_press);
				rsx_log.notice("Big Picture Mode: details handle_button_press(%d) -> %d", static_cast<int>(button_press), static_cast<int>(details_result));

				switch (details_result)
				{
				case big_picture_game_details::result::back:
					m_details->hide();
					refresh();
					break;
				case big_picture_game_details::result::start:
				{
					rsx_log.notice("Big Picture Mode: Start pressed for index=%d", m_selected_index);
					const big_picture_game_info& info = m_games[m_selected_index];
					rsx_log.notice("Big Picture Mode: selected game path='%s' serial='%s'", info.path, info.serial);
					m_details->hide();
					if (m_on_game_selected)
					{
						rsx_log.notice("Big Picture Mode: invoking on_game_selected callback");
						m_on_game_selected(info.path, info.serial);
						rsx_log.notice("Big Picture Mode: on_game_selected callback returned");
					}
					refresh();
					break;
				}
				default:
					break;
				}

				return page_navigation::stay;
			}

			if (m_tiles.empty())
			{
				if (button_press == pad_button::circle)
				{
					play_sound(sound_effect::cancel);
					set_current_page(ensure(parent));
					return page_navigation::back;
				}

				return page_navigation::stay;
			}

			switch (button_press)
			{
			case pad_button::dpad_left:
			case pad_button::ls_left:
				if (column(m_selected_index) > 0)
				{
					select_tile(m_selected_index - 1);
				}
				break;
			case pad_button::dpad_right:
			case pad_button::ls_right:
				if ((column(m_selected_index) + 1) < m_columns && (m_selected_index + 1) < static_cast<s32>(m_tiles.size()))
				{
					select_tile(m_selected_index + 1);
				}
				break;
			case pad_button::dpad_up:
			case pad_button::ls_up:
				if ((m_selected_index - m_columns) >= 0)
				{
					select_tile(m_selected_index - m_columns);
				}
				break;
			case pad_button::dpad_down:
			case pad_button::ls_down:
				if (!m_tiles.empty() && row(m_selected_index) < row(static_cast<s32>(m_tiles.size()) - 1))
				{
					select_tile(std::min(m_selected_index + m_columns, static_cast<s32>(m_tiles.size()) - 1));
				}
				break;
			case pad_button::cross:
				play_sound(sound_effect::accept);
				rsx_log.notice("Big Picture Mode: opening details for index=%d", m_selected_index);
				m_details->show(m_games[m_selected_index], m_tiles[m_selected_index]->get_icon_data());
				rsx_log.notice("Big Picture Mode: details shown");
				refresh();
				return page_navigation::stay;
			case pad_button::circle:
				play_sound(sound_effect::cancel);
				set_current_page(ensure(parent));
				return page_navigation::back;
			default:
				return page_navigation::stay;
			}

			if (do_play_sound)
			{
				play_sound(sound_effect::cursor);
			}

			return page_navigation::stay;
		}

		compiled_resource& big_picture_game_grid::get_compiled()
		{
			std::lock_guard lock(m_mutex);

			if (!m_highlight->is_compiled() ||
				(!m_tiles.empty() && m_grid && !m_grid->is_compiled()) ||
				(m_details && m_details->is_visible()))
			{
				m_is_compiled = false;
			}

			if (is_compiled())
			{
				return compiled_resources;
			}

			m_is_compiled = true;
			compiled_resources.clear();

			if (!visible)
			{
				return compiled_resources;
			}

			if (m_tiles.empty())
			{
				compiled_resources.add(m_placeholder_text->get_compiled());
			}
			else if (m_grid)
			{
				compiled_resources.add(m_grid->get_compiled());
				compiled_resources.add(m_highlight->get_compiled());
			}

			if (m_details && m_details->is_visible())
			{
				compiled_resources.add(m_details->get_compiled());
			}
			else
			{
				compiled_resources.add(m_back_hint.get_compiled());

				if (!m_tiles.empty())
				{
					compiled_resources.add(m_select_hint.get_compiled());
				}
			}

			return compiled_resources;
		}
	}
}
