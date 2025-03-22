#include "stdafx.h"
#include "overlays.h"
#include "overlay_manager.h"
#include "overlay_media_list_dialog.h"

#include "Emu/Cell/Modules/cellMusic.h"
#include "Emu/System.h"
#include "Emu/VFS.h"
#include "Utilities/StrUtil.h"
#include "Utilities/Thread.h"

namespace rsx
{
	namespace overlays
	{
		media_list_dialog::media_list_entry::media_list_entry(const media_list_dialog::media_entry& entry)
		{
			std::unique_ptr<overlay_element> image = std::make_unique<image_view>();
			image->set_size(160, 110);
			image->set_padding(36, 36, 11, 11); // Square image, 88x88

			switch (entry.type)
			{
			case media_list_dialog::media_type::audio:
			{
				// TODO: use thumbnail or proper icon
				static_cast<image_view*>(image.get())->set_image_resource(resource_config::standard_image_resource::new_entry);
				break;
			}
			case media_list_dialog::media_type::video:
			{
				// TODO: use thumbnail or proper icon
				static_cast<image_view*>(image.get())->set_image_resource(resource_config::standard_image_resource::new_entry);
				break;
			}
			case media_list_dialog::media_type::photo:
			{
				if (fs::exists(entry.info.path))
				{
					// Fit the new image into the available space
					if (entry.info.width > 0 && entry.info.height > 0)
					{
						const u16 target_width = image->w - (image->padding_left + image->padding_right);
						const u16 target_height = image->h - (image->padding_top + image->padding_bottom);
						const f32 target_ratio = target_width / static_cast<f32>(target_height);
						const f32 image_ratio = entry.info.width / static_cast<f32>(entry.info.height);
						const f32 convert_ratio = image_ratio / target_ratio;

						if (convert_ratio > 1.0f)
						{
							const u16 new_padding = static_cast<u16>(target_height - target_height / convert_ratio) / 2;
							image->set_padding(image->padding_left, image->padding_right, new_padding + image->padding_top, new_padding + image->padding_bottom);
						}
						else if (convert_ratio < 1.0f)
						{
							const u16 new_padding = static_cast<u16>(target_width - target_width * convert_ratio) / 2;
							image->set_padding(image->padding_left + new_padding, image->padding_right + new_padding, image->padding_top, image->padding_bottom);
						}
					}

					icon_data = std::make_unique<image_info>(entry.info.path);
					static_cast<image_view*>(image.get())->set_raw_image(icon_data.get());
				}
				else
				{
					// Fallback
					// TODO: use proper icon
					static_cast<image_view*>(image.get())->set_image_resource(resource_config::standard_image_resource::new_entry);
				}
				break;
			}
			case media_list_dialog::media_type::directory:
			{
				static_cast<image_view*>(image.get())->set_image_resource(resource_config::standard_image_resource::save);
				break;
			}
			case media_list_dialog::media_type::invalid:
				fmt::throw_exception("Unexpected media type");
			}

			char title[384]{}; // CELL_SEARCH_TITLE_LEN_MAX
			char artist[384]{}; // CELL_SEARCH_TITLE_LEN_MAX

			if (entry.type == media_type::directory)
			{
				strcpy_trunc(title, entry.name);
			}
			else
			{
				utils::parse_metadata(title, entry.info, "title", entry.name.substr(0, entry.name.find_last_of('.')), 384); // CELL_SEARCH_TITLE_LEN_MAX
				utils::parse_metadata(artist, entry.info, "artist", "Unknown Artist", 384); // CELL_SEARCH_TITLE_LEN_MAX
			}

			std::unique_ptr<overlay_element> text_stack  = std::make_unique<vertical_layout>();
			std::unique_ptr<overlay_element> padding     = std::make_unique<spacer>();
			std::unique_ptr<overlay_element> header_text = std::make_unique<label>(title);
			std::unique_ptr<overlay_element> subtext     = std::make_unique<label>(artist);

			padding->set_size(1, 1);
			header_text->set_size(800, 40);
			header_text->set_font("Arial", 16);
			header_text->set_wrap_text(true);

			subtext->set_size(800, 0);
			subtext->set_font("Arial", 14);
			subtext->set_wrap_text(true);
			static_cast<label*>(subtext.get())->auto_resize(true);

			// Make back color transparent for text
			header_text->back_color.a = 0.f;
			subtext->back_color.a     = 0.f;

			static_cast<vertical_layout*>(text_stack.get())->pack_padding = 5;
			static_cast<vertical_layout*>(text_stack.get())->add_element(padding);
			static_cast<vertical_layout*>(text_stack.get())->add_element(header_text);
			static_cast<vertical_layout*>(text_stack.get())->add_element(subtext);

			if (text_stack->h > image->h)
			{
				std::unique_ptr<overlay_element> padding2 = std::make_unique<spacer>();
				padding2->set_size(1, 5);
				static_cast<vertical_layout*>(text_stack.get())->add_element(padding2);
			}

			// Pack
			pack_padding = 15;
			add_element(image);
			add_element(text_stack);
		}

		media_list_dialog::media_list_dialog()
		{
			m_dim_background = std::make_unique<overlay_element>();
			m_dim_background->set_size(virtual_width, virtual_height);
			m_dim_background->back_color.a = 0.5f;

			m_description = std::make_unique<label>();
			m_description->set_font("Arial", 20);
			m_description->set_pos(20, 37);
			m_description->set_text("Select media"); // Fallback. I don't think this will ever be used, so I won't localize it.
			m_description->auto_resize();
			m_description->back_color.a = 0.f;
		}

		void media_list_dialog::on_button_pressed(pad_button button_press, bool is_auto_repeat)
		{
			bool play_cursor_sound = true;

			switch (button_press)
			{
			case pad_button::cross:
				if (m_no_media_text)
					break;
				return_code = m_list->get_selected_index();
				m_stop_input_loop = true;
				play_cursor_sound = false;
				Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_decide.wav");
				break;
			case pad_button::circle:
				return_code = selection_code::canceled;
				m_stop_input_loop = true;
				play_cursor_sound = false;
				Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_cancel.wav");
				break;
			case pad_button::dpad_up:
				m_list->select_previous();
				break;
			case pad_button::dpad_down:
				m_list->select_next();
				break;
			case pad_button::L1:
				m_list->select_previous(10);
				break;
			case pad_button::R1:
				m_list->select_next(10);
				break;
			default:
				rsx_log.trace("[ui] Button %d pressed", static_cast<u8>(button_press));
				break;
			}

			// Play a sound unless this is a fast auto repeat which would induce a nasty noise
			if (play_cursor_sound && (!is_auto_repeat || m_auto_repeat_ms_interval >= m_auto_repeat_ms_interval_default))
			{
				Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_cursor.wav");
			}
		}

		compiled_resource media_list_dialog::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			compiled_resource result;
			result.add(m_dim_background->get_compiled());
			result.add(m_list->get_compiled());
			result.add(m_description->get_compiled());

			if (m_no_media_text)
				result.add(m_no_media_text->get_compiled());

			return result;
		}

		s32 media_list_dialog::show(media_entry* root, media_entry& result, const std::string& title, u32 focused, bool enable_overlay)
		{
			auto ref = g_fxo->get<display_manager>().get(uid);

			m_media = root;
			result = {};

			if (enable_overlay)
			{
				m_dim_background->back_color.a = 0.9f;
			}
			else
			{
				m_dim_background->back_color.a = 0.5f;
			}

			while (thread_ctrl::state() != thread_state::aborting && return_code >= 0 && m_media && m_media->type == media_list_dialog::media_type::directory)
			{
				reload(title, focused);
				m_stop_input_loop = false;

				if (const auto error = run_input_loop())
				{
					if (error != selection_code::canceled)
					{
						rsx_log.error("Media list dialog input loop exited with error code=%d", error);
					}
					return error;
				}

				if (return_code >= 0)
				{
					focused = 0;
					ensure(static_cast<size_t>(return_code) < m_media->children.size());
					m_media = &m_media->children[return_code];
					rsx_log.notice("Media dialog: selected entry: %d ('%s')", return_code, m_media->path);
					continue;
				}

				if (return_code == user_interface::selection_code::canceled)
				{
					if (m_media == root)
					{
						rsx_log.notice("Media list dialog canceled");
						break;
					}

					focused = m_media->index;
					m_media = m_media->parent;
					return_code = 0;
					rsx_log.notice("Media list dialog moving to parent directory (focused=%d)", focused);
					continue;
				}

				rsx_log.error("Left media list dialog with error: %d", return_code);
				break;
			}

			m_interactive = false; // KLUDGE: Set interactive to false in order to stop pad interaction properly in close.
			close(false, true);

			if (return_code >= 0 && m_media && m_media->type != media_list_dialog::media_type::directory)
			{
				result = *m_media;
				rsx_log.notice("Left media list dialog: return_code=%d, type=%d, path=%s", return_code, static_cast<s32>(result.type), result.info.path);
			}

			return return_code;
		}

		void media_list_dialog::reload(const std::string& title, u32 focused)
		{
			ensure(m_media);

			rsx_log.notice("Media dialog: showing entry '%s' ('%s')", m_media->name, m_media->path);

			if (m_list)
			{
				status_flags |= status_bits::invalidate_image_cache;
			}

			m_list = std::make_unique<list_view>(virtual_width - 2 * 20, 540);
			m_list->set_pos(20, 85);

			for (const media_entry& child : m_media->children)
			{
				std::unique_ptr<overlay_element> entry = std::make_unique<media_list_entry>(child);
				m_list->add_entry(entry);
			}

			if (m_list->m_items.empty())
			{
				m_no_media_text = std::make_unique<label>(get_localized_string(localized_string_id::RSX_OVERLAYS_MEDIA_DIALOG_EMPTY));
				m_no_media_text->set_font("Arial", 20);
				m_no_media_text->align_text(overlay_element::text_align::center);
				m_no_media_text->set_pos(m_list->x, m_list->y + m_list->h / 2);
				m_no_media_text->set_size(m_list->w, 30);
				m_no_media_text->back_color.a = 0;

				m_list->set_cancel_only(true);
			}
			else
			{
				// Only select an entry if there are entries available
				m_list->select_entry(focused);
			}

			m_description->set_text(title);
			m_description->auto_resize();

			visible = true;
		}

		struct media_list_dialog_thread
		{
			static constexpr auto thread_name = "MediaList Thread"sv;
		};

		void parse_media_recursive(u32 depth, const std::string& media_path, const std::string& name, media_list_dialog::media_type type, media_list_dialog::media_entry& current_entry)
		{
			if (depth++ > music_selection_context::max_depth)
			{
				return;
			}

			if (fs::is_dir(media_path))
			{
				for (auto&& dir_entry : fs::dir{media_path})
				{
					if (dir_entry.name == "." || dir_entry.name == "..")
					{
						continue;
					}

					const std::string unescaped_name = vfs::unescape(dir_entry.name);

					media_list_dialog::media_entry new_entry{};
					parse_media_recursive(depth, media_path + "/" + dir_entry.name, unescaped_name, type, new_entry);
					if (new_entry.type != media_list_dialog::media_type::invalid)
					{
						new_entry.parent = &current_entry;
						new_entry.index = ::narrow<u32>(current_entry.children.size());
						current_entry.children.emplace_back(std::move(new_entry));
					}
				}

				// Only keep directories that contain valid entries
				if (current_entry.children.empty())
				{
					rsx_log.notice("parse_media_recursive: No matches in directory '%s'", media_path);
				}
				else
				{
					rsx_log.notice("parse_media_recursive: Found %d matches in directory '%s'", current_entry.children.size(), media_path);
					current_entry.type = media_list_dialog::media_type::directory;
					current_entry.info.path = media_path;
				}
			}
			else
			{
				// Try to peek into the file
				const s32 av_media_type = type == media_list_dialog::media_type::photo ? -1
				                        : type == media_list_dialog::media_type::video ? 0 /*AVMEDIA_TYPE_VIDEO*/
				                        : 1 /*AVMEDIA_TYPE_AUDIO*/;
				auto [success, info] = utils::get_media_info(media_path, av_media_type);
				if (success)
				{
					current_entry.type = type;
					current_entry.info = std::move(info);
					rsx_log.notice("parse_media_recursive: Found media '%s'", media_path);
				}
			}

			if (current_entry.type != media_list_dialog::media_type::invalid)
			{
				current_entry.path = media_path;
				current_entry.name = name;
			}
		}

		error_code show_media_list_dialog(media_list_dialog::media_type type, const std::string& path, const std::string& title, std::function<void(s32 status, utils::media_info info)> on_finished)
		{
			rsx_log.todo("show_media_list_dialog(type=%d, path='%s', title='%s', on_finished=%d)", static_cast<s32>(type), path, title, !!on_finished);

			if (!on_finished)
			{
				return CELL_CANCEL;
			}

			g_fxo->get<named_thread<media_list_dialog_thread>>()([=]()
			{
				media_list_dialog::media_entry root_media_entry{};
				root_media_entry.type = media_list_dialog::media_type::directory;

				if (fs::is_dir(path))
				{
					parse_media_recursive(0, path, title, type, root_media_entry);
				}
				else
				{
					rsx_log.error("Media list: Failed to open path: '%s'", path);
				}

				media_list_dialog::media_entry media{};
				s32 result = 0;
				u32 focused = 0;

				if (auto manager = g_fxo->try_get<rsx::overlays::display_manager>())
				{
					result = manager->create<rsx::overlays::media_list_dialog>()->show(&root_media_entry, media, title, focused, true);
				}
				else
				{
					result = user_interface::selection_code::canceled;
					rsx_log.error("Media selection is only possible when the native user interface is enabled in the settings. The action will be canceled.");
				}

				if (result >= 0 && media.type == type)
				{
					on_finished(CELL_OK, media.info);
				}
				else
				{
					on_finished(result, {});
				}
			});

			return CELL_OK;
		}
	} // namespace overlays
} // namespace RSX
