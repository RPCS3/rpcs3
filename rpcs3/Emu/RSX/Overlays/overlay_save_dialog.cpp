#include "stdafx.h"
#include "overlay_save_dialog.h"
#include "overlay_video.h"
#include "Utilities/date_time.h"
#include "Emu/System.h"

namespace rsx
{
	namespace overlays
	{
		save_dialog::save_dialog_entry::save_dialog_entry(const std::string& text1, const std::string& text2, const std::string& text3, u8 resource_id, const std::vector<u8>& icon_buf, const std::string& video_path)
		{
			std::unique_ptr<overlay_element> image = resource_id != image_resource_id::raw_image
				? std::make_unique<video_view>(video_path, resource_id)
				: !icon_buf.empty() ? std::make_unique<video_view>(video_path, icon_buf)
				                    : std::make_unique<video_view>(video_path, resource_config::standard_image_resource::save); // Fallback
			image->set_size(160, 110);
			image->set_padding(36, 36, 11, 11); // Square image, 88x88

			if (resource_id == image_resource_id::raw_image && !icon_buf.empty())
			{
				image->set_padding(0, 0, 11, 11); // Half sized icon, 320x176->160x88
			}

			std::unique_ptr<overlay_element> text_stack  = std::make_unique<vertical_layout>();
			std::unique_ptr<overlay_element> padding     = std::make_unique<spacer>();
			std::unique_ptr<overlay_element> header_text = std::make_unique<label>(text1);
			std::unique_ptr<overlay_element> subtext     = std::make_unique<label>(text2);

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

			if (!text3.empty())
			{
				// Detail info actually exists
				std::unique_ptr<overlay_element> detail = std::make_unique<label>(text3);
				detail->set_size(800, 0);
				detail->set_font("Arial", 12);
				detail->set_wrap_text(true);
				detail->back_color.a = 0.f;
				static_cast<label*>(detail.get())->auto_resize(true);
				static_cast<vertical_layout*>(text_stack.get())->add_element(detail);
			}

			if (text_stack->h > image->h)
			{
				std::unique_ptr<overlay_element> padding2 = std::make_unique<spacer>();
				padding2->set_size(1, 5);
				static_cast<vertical_layout*>(text_stack.get())->add_element(padding2);
			}

			// Pack
			this->pack_padding = 15;
			m_image = add_element(image);
			add_element(text_stack);
		}

		void save_dialog::save_dialog_entry::set_selected(bool selected)
		{
			if (m_image)
			{
				static_cast<video_view*>(m_image)->set_active(selected);
			}
		}

		save_dialog::save_dialog()
		{
			m_dim_background = std::make_unique<overlay_element>();
			m_dim_background->set_size(virtual_width, virtual_height);

			m_list        = std::make_unique<list_view>(virtual_width - 2 * 20, 540);
			m_description = std::make_unique<label>();
			m_time_thingy = std::make_unique<label>();

			m_list->set_pos(20, 85);

			m_description->set_font("Arial", 20);
			m_description->set_pos(20, 37);
			m_description->set_text(localized_string_id::RSX_OVERLAYS_SAVE_DIALOG_TITLE);

			m_time_thingy->set_font("Arial", 14);
			m_time_thingy->set_pos(1000, 30);
			m_time_thingy->set_text(date_time::current_time());

			m_description->auto_resize();
			m_time_thingy->auto_resize();

			m_dim_background->back_color.a = 0.5f;
			m_description->back_color.a    = 0.f;
			m_time_thingy->back_color.a    = 0.f;

			fade_animation.duration_sec = 0.15f;

			return_code = selection_code::canceled;
		}

		void save_dialog::update(u64 timestamp_us)
		{
			m_time_thingy->set_text(date_time::current_time());
			m_time_thingy->auto_resize();

			if (fade_animation.active)
			{
				fade_animation.update(timestamp_us);
			}
		}

		void save_dialog::on_button_pressed(pad_button button_press, bool is_auto_repeat)
		{
			if (fade_animation.active) return;

			bool close_dialog = false;

			switch (button_press)
			{
			case pad_button::cross:
				if (m_no_saves)
					break;
				return_code = m_list->get_selected_index();
				Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_decide.wav");
				close_dialog = true;
				break;
			case pad_button::circle:
				Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_cancel.wav");
				close_dialog = true;
				break;
			case pad_button::dpad_up:
			case pad_button::ls_up:
				m_list->select_previous();
				break;
			case pad_button::dpad_down:
			case pad_button::ls_down:
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

			if (close_dialog)
			{
				fade_animation.current = color4f(1.f);
				fade_animation.end = color4f(0.f);
				fade_animation.active = true;

				fade_animation.on_finish = [this]
				{
					close(true, true);
				};
			}
			// Play a sound unless this is a fast auto repeat which would induce a nasty noise
			else if (!is_auto_repeat || m_auto_repeat_ms_interval >= m_auto_repeat_ms_interval_default)
			{
				Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_cursor.wav");
			}
		}

		compiled_resource save_dialog::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			compiled_resource result;
			result.add(m_dim_background->get_compiled());
			result.add(m_list->get_compiled());
			result.add(m_description->get_compiled());
			result.add(m_time_thingy->get_compiled());

			if (m_no_saves)
				result.add(m_no_saves_text->get_compiled());

			fade_animation.apply(result);

			return result;
		}

		s32 save_dialog::show(const std::string& base_dir, std::vector<SaveDataEntry>& save_entries, u32 focused, u32 op, vm::ptr<CellSaveDataListSet> listSet, bool enable_overlay)
		{
			rsx_log.notice("Showing native UI save_dialog (save_entries=%d, focused=%d, op=0x%x, listSet=*0x%x, enable_overlay=%d)", save_entries.size(), focused, op, listSet, enable_overlay);

			visible = false;

			if (enable_overlay)
			{
				m_dim_background->back_color.a = 0.9f;
			}
			else
			{
				m_dim_background->back_color.a = 0.5f;
			}

			std::vector<std::unique_ptr<overlay_element>> entries;

			for (auto& entry : save_entries)
			{
				const std::string date_and_size = fmt::format("%s   %s", entry.date(), entry.data_size());
				std::unique_ptr<overlay_element> e;
				e = std::make_unique<save_dialog_entry>(entry.subtitle, date_and_size, entry.details, image_resource_id::raw_image, entry.iconBuf, base_dir + entry.dirName + "/ICON1.PAM");
				entries.emplace_back(std::move(e));
			}

			if (op >= 8)
			{
				m_description->set_text(localized_string_id::RSX_OVERLAYS_SAVE_DIALOG_DELETE);
			}
			else if (op & 1)
			{
				m_description->set_text(localized_string_id::RSX_OVERLAYS_SAVE_DIALOG_LOAD);
			}
			else
			{
				m_description->set_text(localized_string_id::RSX_OVERLAYS_SAVE_DIALOG_SAVE);
			}

			const bool newpos_head = listSet && listSet->newData && listSet->newData->iconPosition == CELL_SAVEDATA_ICONPOS_HEAD;

			if (!newpos_head)
			{
				for (auto& entry : entries)
				{
					m_list->add_entry(entry);
				}
			}

			if (listSet && listSet->newData)
			{
				std::string title = get_localized_string(localized_string_id::CELL_SAVEDATA_NEW_SAVED_DATA_TITLE);

				std::vector<u8> icon;
				int id = resource_config::standard_image_resource::new_entry;

				if (const auto picon = +listSet->newData->icon)
				{
					if (picon->title)
						title = picon->title.get_ptr();

					if (picon->iconBuf && picon->iconBufSize && picon->iconBufSize <= 225280)
					{
						const auto iconBuf = static_cast<u8*>(picon->iconBuf.get_ptr());
						const auto iconEnd = iconBuf + picon->iconBufSize;
						icon.assign(iconBuf, iconEnd);
					}
				}

				if (!icon.empty())
				{
					id = image_resource_id::raw_image;
				}

				std::unique_ptr<overlay_element> new_stub = std::make_unique<save_dialog_entry>(title, get_localized_string(localized_string_id::CELL_SAVEDATA_NEW_SAVED_DATA_SUB_TITLE), "", id, icon, "");

				m_list->add_entry(new_stub);
			}

			if (newpos_head)
			{
				for (auto& entry : entries)
				{
					m_list->add_entry(entry);
				}
			}

			if (m_list->m_items.empty())
			{
				m_no_saves_text = std::make_unique<label>(get_localized_string(localized_string_id::CELL_SAVEDATA_NO_DATA));
				m_no_saves_text->set_font("Arial", 20);
				m_no_saves_text->align_text(overlay_element::text_align::center);
				m_no_saves_text->set_pos(m_list->x, m_list->y + m_list->h / 2);
				m_no_saves_text->set_size(m_list->w, 30);
				m_no_saves_text->back_color.a = 0;

				m_no_saves = true;
				m_list->set_cancel_only(true);
			}
			else
			{
				// Only select an entry if there are entries available
				m_list->select_entry(focused);
			}

			m_description->auto_resize();

			fade_animation.current = color4f(0.f);
			fade_animation.end = color4f(1.f);
			fade_animation.active = true;

			visible = true;

			if (const auto error = run_input_loop())
			{
				if (error != selection_code::canceled)
				{
					rsx_log.error("Save dialog input loop exited with error code=%d", error);
				}
				return error;
			}

			if (return_code >= 0)
			{
				if (newpos_head)
				{
					return return_code - 1;
				}
				if (static_cast<usz>(return_code) == entries.size())
				{
					return selection_code::new_save;
				}
			}

			return return_code;
		}
	} // namespace overlays
} // namespace RSX
