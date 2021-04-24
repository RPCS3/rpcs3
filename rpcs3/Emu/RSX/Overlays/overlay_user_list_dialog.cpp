#include "stdafx.h"
#include "overlay_user_list_dialog.h"
#include "Emu/system_config.h"
#include "Emu/system_utils.hpp"
#include "Utilities/StrUtil.h"
#include "Utilities/Thread.h"

namespace rsx
{
	namespace overlays
	{
		user_list_dialog::user_list_entry::user_list_entry(const std::string& username, const std::string& user_id, const std::string& avatar_path)
		{
			std::unique_ptr<overlay_element> image = std::make_unique<image_view>();
			image->set_size(160, 110);
			image->set_padding(36, 36, 11, 11); // Square image, 88x88

			if (fs::exists(avatar_path))
			{
				icon_data = std::make_unique<image_info>(avatar_path.c_str());
				static_cast<image_view*>(image.get())->set_raw_image(icon_data.get());
			}
			else
			{
				// Fallback
				static_cast<image_view*>(image.get())->set_image_resource(resource_config::standard_image_resource::square);
			}

			std::unique_ptr<overlay_element> text_stack  = std::make_unique<vertical_layout>();
			std::unique_ptr<overlay_element> padding     = std::make_unique<spacer>();
			std::unique_ptr<overlay_element> header_text = std::make_unique<label>(username);
			std::unique_ptr<overlay_element> subtext     = std::make_unique<label>(user_id);

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
			this->pack_padding = 15;
			add_element(image);
			add_element(text_stack);
		}

		user_list_dialog::user_list_dialog()
		{
			m_dim_background = std::make_unique<overlay_element>();
			m_dim_background->set_size(1280, 720);
			m_dim_background->back_color.a = 0.5f;

			m_list = std::make_unique<list_view>(1240, 540);
			m_list->set_pos(20, 85);

			m_description = std::make_unique<label>();
			m_description->set_font("Arial", 20);
			m_description->set_pos(20, 37);
			m_description->set_text("Select user"); // Fallback. I don't think this will ever be used, so I won't localize it.
			m_description->auto_resize();
			m_description->back_color.a	= 0.f;

			return_code = selection_code::canceled;
		}

		void user_list_dialog::on_button_pressed(pad_button button_press)
		{
			switch (button_press)
			{
			case pad_button::cross:
				if (m_list->m_items.empty())
					break;

				if (const usz index = static_cast<usz>(m_list->get_selected_index()); index < m_entry_ids.size())
				{
					return_code = static_cast<s32>(m_entry_ids[index]);
				}
				else
				{
					return_code = selection_code::error;
				}
				[[fallthrough]];
			case pad_button::circle:
				close(true, true);
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
		}

		compiled_resource user_list_dialog::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			compiled_resource result;
			result.add(m_dim_background->get_compiled());
			result.add(m_list->get_compiled());
			result.add(m_description->get_compiled());
			return result;
		}

		struct user_list_dialog_thread
		{
			static constexpr auto thread_name = "UserList Thread"sv;
		};

		error_code user_list_dialog::show(const std::string& title, u32 focused, const std::vector<u32>& user_ids, bool enable_overlay, std::function<void(s32 status)> on_close)
		{
			visible = false;

			if (enable_overlay)
			{
				m_dim_background->back_color.a = 0.9f;
			}
			else
			{
				m_dim_background->back_color.a = 0.5f;
			}

			std::vector<u8> icon;
			std::vector<std::unique_ptr<overlay_element>> entries;

			const std::string home_dir = rpcs3::utils::get_hdd0_dir() + "home/";
			s32 selected_index = 0;

			for (const auto& id : user_ids)
			{
				const std::string user_id = fmt::format("%08d", id);

				if (const fs::file file{home_dir + user_id + "/localusername"})
				{
					if (id == focused)
					{
						selected_index = static_cast<s32>(entries.size());
					}

					// Let's assume there are 26 avatar pngs (like in my installation)
					const std::string avatar_path = g_cfg.vfs.get_dev_flash() + fmt::format("vsh/resource/explore/user/%03d.png", id % 26);
					const std::string username = file.to_string();
					std::unique_ptr<overlay_element> entry = std::make_unique<user_list_entry>(username, user_id, avatar_path);
					entries.emplace_back(std::move(entry));
					m_entry_ids.emplace_back(id);
				}
			}

			for (auto& entry : entries)
			{
				m_list->add_entry(entry);
			}

			if (m_list->m_items.empty())
			{
				m_list->set_cancel_only(true);
			}
			else
			{
				// Only select an entry if there are entries available
				m_list->select_entry(selected_index);
			}

			m_description->set_text(title);
			m_description->auto_resize();

			this->on_close = std::move(on_close);
			visible = true;

			g_fxo->get<named_thread<user_list_dialog_thread>>()([&, tbit = alloc_thread_bit()]()
			{
				g_thread_bit = tbit;

				auto ref = g_fxo->get<display_manager>().get(uid);

				if (const auto error = run_input_loop())
				{
					rsx_log.error("Dialog input loop exited with error code=%d", error);
				}

				thread_bits &= ~tbit;
				thread_bits.notify_all();
			});

			return CELL_OK;
		}
	} // namespace overlays
} // namespace RSX
