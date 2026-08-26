#include "stdafx.h"
#include "overlay_big_picture_game_grid.h"
#include "overlay_big_picture_game_details.h"
#include "Emu/System.h"
#include "Emu/system_utils.hpp"
#include "Loader/PSF.h"
#include "Loader/ISO.h"
#include "Loader/iso_cache.h"

#include <algorithm>
#include <unordered_set>

namespace rsx
{
	namespace overlays
	{
		namespace
		{
			// ISO icons have no on-disk file of their own, so cache the extracted bytes to one.
			std::string get_iso_icon_cache_path(const std::string& title_id)
			{
				const std::string dir = rpcs3::utils::get_cache_dir() + "big_picture_iso_icons/";
				fs::create_path(dir);
				return dir + title_id + ".png";
			}
		}

		big_picture_game_tile::big_picture_game_tile(const big_picture_game_entry& entry, u16 tile_width)
		{
			pack_padding = 8;
			back_color.a = 0.f;

			const u16 icon_h = icon_height(tile_width);

			std::unique_ptr<overlay_element> icon = std::make_unique<image_view>();
			icon->set_size(tile_width, icon_h);
			icon->back_color = color4f(1.f, 1.f, 1.f, 0.08f);

			if (fs::exists(entry.info.icon_path))
			{
				m_icon_data = std::make_unique<image_info>(entry.info.icon_path);
				// The renderer's texture cache is keyed by this object's address, which can be reused by an
				// unrelated image after the old one is freed - force a re-upload instead of trusting the cache.
				m_icon_data->dirty = true;
				static_cast<image_view*>(icon.get())->set_raw_image(m_icon_data.get());
			}
			else
			{
				static_cast<image_view*>(icon.get())->set_image_resource(resource_config::standard_image_resource::new_entry);
			}

			std::unique_ptr<overlay_element> title = std::make_unique<label>(entry.info.name.empty() ? entry.info.serial : entry.info.name);
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
			m_no_games_text = std::make_unique<label>(get_localized_string(localized_string_id::BIG_PICTURE_NO_GAMES_FOUND));
			m_no_games_text->set_font("Arial", 20);
			m_no_games_text->set_pos(x, y + (height / 2) - 20);
			m_no_games_text->set_size(width, 40);
			m_no_games_text->align_text(text_align::center);
			m_no_games_text->back_color.a = 0.f;

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

			reload();
		}

		big_picture_game_grid::~big_picture_game_grid() = default;

		void big_picture_game_grid::reload()
		{
			rsx_log.notice("Big Picture Mode: reload() start");

			m_games.clear();
			m_tiles.clear();

			std::unordered_set<std::string> added_serials;

			const auto add_game_entry = [&](const std::string& title_id, const std::string& path, const psf::registry& psf, std::string icon_path)
			{
				if (psf::get_string(psf, "CATEGORY") == "GD")
				{
					// Game data (patches/installed extra content), not a bootable title.
					return;
				}

				GameInfo info{};
				info.path = path;
				info.icon_path = std::move(icon_path);
				info.serial = title_id;
				info.name = std::string(psf::get_string(psf, "TITLE", title_id));
				info.category = std::string(psf::get_string(psf, "CATEGORY"));
				info.app_ver = std::string(psf::get_string(psf, "APP_VER"));

				added_serials.insert(title_id);
				m_games.push_back({ std::move(info) });
			};

			const auto try_add_game = [&](const std::string& title_id, const std::string& raw_path)
			{
				std::string path = raw_path;
				path.resize(path.find_last_not_of('/') + 1);

				if (path.empty())
				{
					return;
				}

				if (is_iso_file(path))
				{
					iso_metadata_cache_entry cache_entry{};
					std::vector<u8> psf_data;
					std::vector<u8> icon_data;

					if (iso_cache::load(path, path, cache_entry))
					{
						psf_data = std::move(cache_entry.psf_data);
						icon_data = std::move(cache_entry.icon_data);
					}
					else
					{
						iso_archive archive(path);

						if (!archive.is_valid())
						{
							return;
						}

						const psf::registry iso_psf = archive.open_psf("PS3_GAME/PARAM.SFO");

						if (iso_psf.empty())
						{
							return;
						}

						psf_data = psf::save_object(iso_psf);

						if (archive.exists("PS3_GAME/ICON0.PNG"))
						{
							if (auto icon_file = archive.open("PS3_GAME/ICON0.PNG"); icon_file && icon_file->size() > 0)
							{
								icon_data.resize(icon_file->size());
								icon_file->read(icon_data.data(), icon_data.size());
							}
						}

						fs::stat_t iso_stat{};

						if (fs::get_stat(path, iso_stat))
						{
							iso_metadata_cache_entry entry_to_save{};
							entry_to_save.mtime = iso_stat.mtime;
							entry_to_save.psf_data = psf_data;
							entry_to_save.icon_data = icon_data;
							entry_to_save.icon_path = "PS3_GAME/ICON0.PNG";
							iso_cache::save(path, path, entry_to_save);
						}
					}

					if (psf_data.empty())
					{
						return;
					}

					const psf::registry psf = psf::load_object(fs::file(psf_data.data(), psf_data.size()), "PARAM.SFO");

					if (psf.empty())
					{
						return;
					}

					std::string icon_path;

					if (!icon_data.empty())
					{
						icon_path = get_iso_icon_cache_path(title_id);
						fs::write_file(icon_path, fs::rewrite, icon_data);
					}

					add_game_entry(title_id, path, psf, std::move(icon_path));
					return;
				}

				const std::string sfo_dir = rpcs3::utils::get_sfo_dir_from_game_path(path, title_id);
				const psf::registry psf = psf::load_object(sfo_dir + "/PARAM.SFO");

				if (psf.empty())
				{
					return;
				}

				add_game_entry(title_id, path, psf, sfo_dir + "/ICON0.PNG");
			};

			// dev_hdd0/game/ (regular PKG installs) is never registered into games.yml, so list it directly.
			const std::string hdd0_game_dir = rpcs3::utils::get_hdd0_game_dir();

			for (const auto& entry : fs::dir(hdd0_game_dir))
			{
				if (!entry.is_directory || entry.name == "." || entry.name == "..")
				{
					continue;
				}

				try_add_game(entry.name, hdd0_game_dir + entry.name);
			}

			for (const auto& [title_id, raw_path] : Emu.GetGamesConfig().get_games())
			{
				if (added_serials.contains(title_id))
				{
					continue;
				}

				try_add_game(title_id, raw_path);
			}

			std::sort(m_games.begin(), m_games.end(), [](const big_picture_game_entry& a, const big_picture_game_entry& b)
			{
				return a.info.name < b.info.name;
			});

			m_grid = std::make_unique<vertical_layout>();
			m_grid->set_pos(x, y);
			m_grid->pack_padding = 20;

			std::unique_ptr<horizontal_layout> row;

			for (usz i = 0; i < m_games.size(); i++)
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

				auto tile = std::make_unique<big_picture_game_tile>(m_games[i], m_tile_size);
				m_tiles.push_back(row->add_element(tile));
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

			if (!m_tiles.empty())
			{
				select_tile(m_selected_index);
			}

			rsx_log.notice("Big Picture Mode: reload() finished, games=%u, tiles=%u", m_games.size(), m_tiles.size());
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
		}

		page_navigation big_picture_game_grid::handle_button_press(pad_button button_press, bool is_auto_repeat, u64 auto_repeat_interval_ms)
		{
			const bool do_play_sound = !is_auto_repeat || auto_repeat_interval_ms >= user_interface::m_auto_repeat_ms_interval_default;

			if (m_details && m_details->is_visible())
			{
				const auto details_result = m_details->handle_button_press(button_press);
				rsx_log.notice("Big Picture Mode: details handle_button_press(%d) -> %d", static_cast<int>(button_press), static_cast<int>(details_result));

				switch (details_result)
				{
				case big_picture_game_details::result::back:
					m_details->hide();
					break;
				case big_picture_game_details::result::start:
				{
					rsx_log.notice("Big Picture Mode: Start pressed for index=%d", m_selected_index);
					const GameInfo& info = m_games[m_selected_index].info;
					rsx_log.notice("Big Picture Mode: selected game path='%s' serial='%s'", info.path, info.serial);
					m_details->hide();
					if (m_on_game_selected)
					{
						rsx_log.notice("Big Picture Mode: invoking on_game_selected callback");
						m_on_game_selected(info.path, info.serial);
						rsx_log.notice("Big Picture Mode: on_game_selected callback returned");
					}
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
					if (parent)
					{
						set_current_page(parent);
						return page_navigation::back;
					}
					return page_navigation::exit;
				}

				return page_navigation::stay;
			}

			switch (button_press)
			{
			case pad_button::dpad_left:
			case pad_button::ls_left:
				if ((m_selected_index % m_columns) > 0)
				{
					select_tile(m_selected_index - 1);
				}
				break;
			case pad_button::dpad_right:
			case pad_button::ls_right:
				if (((m_selected_index % m_columns) + 1) < m_columns && (m_selected_index + 1) < static_cast<s32>(m_tiles.size()))
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
				if ((m_selected_index + m_columns) < static_cast<s32>(m_tiles.size()))
				{
					select_tile(m_selected_index + m_columns);
				}
				break;
			case pad_button::cross:
				play_sound(sound_effect::accept);
				rsx_log.notice("Big Picture Mode: opening details for index=%d", m_selected_index);
				m_details->show(m_games[m_selected_index].info, m_tiles[m_selected_index]->get_icon_data());
				rsx_log.notice("Big Picture Mode: details shown");
				return page_navigation::stay;
			case pad_button::circle:
				play_sound(sound_effect::cancel);
				if (parent)
				{
					set_current_page(parent);
					return page_navigation::back;
				}
				return page_navigation::exit;
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
			m_compiled_grid.clear();

			if (m_tiles.empty())
			{
				m_compiled_grid.add(m_no_games_text->get_compiled());
			}
			else if (m_grid)
			{
				m_compiled_grid.add(m_grid->get_compiled());
				m_compiled_grid.add(m_highlight->get_compiled());
			}

			const bool details_visible = m_details && m_details->is_visible();

			if (!details_visible)
			{
				m_compiled_grid.add(m_back_hint.get_compiled());

				if (!m_tiles.empty())
				{
					m_compiled_grid.add(m_select_hint.get_compiled());
				}
			}

			if (m_details)
			{
				m_compiled_grid.add(m_details->get_compiled());
			}

			return m_compiled_grid;
		}
	}
}
