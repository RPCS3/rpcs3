#include "stdafx.h"
#include "overlays.h"

namespace rsx
{
	namespace overlays
	{
		save_dialog::save_dialog_entry::save_dialog_entry(const std::string& text1, const std::string& text2, const std::string& text3, u8 resource_id, const std::vector<u8>& icon_buf)
		{
			std::unique_ptr<overlay_element> image = std::make_unique<image_view>();
			image->set_size(160, 110);
			image->set_padding(36, 36, 11, 11); // Square image, 88x88

			if (resource_id != image_resource_id::raw_image)
			{
				static_cast<image_view*>(image.get())->set_image_resource(resource_id);
			}
			else if (!icon_buf.empty())
			{
				image->set_padding(0, 0, 11, 11); // Half sized icon, 320x176->160x88
				icon_data = std::make_unique<image_info>(icon_buf);
				static_cast<image_view*>(image.get())->set_raw_image(icon_data.get());
			}
			else
			{
				// Fallback
				static_cast<image_view*>(image.get())->set_image_resource(resource_config::standard_image_resource::save);
			}

			std::unique_ptr<overlay_element> text_stack  = std::make_unique<vertical_layout>();
			std::unique_ptr<overlay_element> padding     = std::make_unique<spacer>();
			std::unique_ptr<overlay_element> header_text = std::make_unique<label>(utf8_to_ascii8(text1));
			std::unique_ptr<overlay_element> subtext     = std::make_unique<label>(utf8_to_ascii8(text2));

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
				std::unique_ptr<overlay_element> detail = std::make_unique<label>(utf8_to_ascii8(text3));
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
			add_element(image);
			add_element(text_stack);
		}

		save_dialog::save_dialog()
		{
			m_dim_background = std::make_unique<overlay_element>();
			m_dim_background->set_size(1280, 720);

			m_list        = std::make_unique<list_view>(1240, 540);
			m_description = std::make_unique<label>();
			m_time_thingy = std::make_unique<label>();

			m_list->set_pos(20, 85);

			m_description->set_font("Arial", 20);
			m_description->set_pos(20, 37);
			m_description->set_text("Save Dialog");

			m_time_thingy->set_font("Arial", 14);
			m_time_thingy->set_pos(1000, 30);
			m_time_thingy->set_text(date_time::current_time());

			static_cast<label*>(m_description.get())->auto_resize();
			static_cast<label*>(m_time_thingy.get())->auto_resize();

			m_dim_background->back_color.a = 0.8f;
			m_description->back_color.a    = 0.f;
			m_time_thingy->back_color.a    = 0.f;

			return_code = selection_code::canceled;
		}

		void save_dialog::update()
		{
			m_time_thingy->set_text(date_time::current_time());
			static_cast<label*>(m_time_thingy.get())->auto_resize();
		}

		void save_dialog::on_button_pressed(pad_button button_press)
		{
			switch (button_press)
			{
			case pad_button::cross:
				if (m_no_saves)
					break;
				return_code = m_list->get_selected_index();
				// Fall through
			case pad_button::circle:
				close();
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
			}
		}

		compiled_resource save_dialog::get_compiled()
		{
			compiled_resource result;
			result.add(m_dim_background->get_compiled());
			result.add(m_list->get_compiled());
			result.add(m_description->get_compiled());
			result.add(m_time_thingy->get_compiled());

			if (m_no_saves)
				result.add(m_no_saves_text->get_compiled());

			return result;
		}

		s32 save_dialog::show(std::vector<SaveDataEntry>& save_entries, u32 focused, u32 op, vm::ptr<CellSaveDataListSet> listSet)
		{
			std::vector<u8> icon;
			std::vector<std::unique_ptr<overlay_element>> entries;

			for (auto& entry : save_entries)
			{
				std::unique_ptr<overlay_element> e;
				e = std::make_unique<save_dialog_entry>(entry.title, entry.subtitle, entry.details, image_resource_id::raw_image, entry.iconBuf);
				entries.emplace_back(std::move(e));
			}

			if (op >= 8)
			{
				m_description->set_text("Delete Save");
			}
			else if (op & 1)
			{
				m_description->set_text("Load Save");
			}
			else
			{
				m_description->set_text("Save");
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
				std::unique_ptr<overlay_element> new_stub;

				const char* title = "Create New";

				int id = resource_config::standard_image_resource::new_entry;

				if (auto picon = +listSet->newData->icon)
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

				new_stub = std::make_unique<save_dialog_entry>(title, "Select to create a new entry", "", id, icon);

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
				m_no_saves_text = std::make_unique<label>("There is no saved data.");
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

			static_cast<label*>(m_description.get())->auto_resize();

			if (auto err = run_input_loop())
				return err;

			if (return_code == entries.size() && !newpos_head)
				return selection_code::new_save;
			if (return_code >= 0 && newpos_head)
				return return_code - 1;

			return return_code;
		}
	} // namespace overlays
} // namespace RSX
