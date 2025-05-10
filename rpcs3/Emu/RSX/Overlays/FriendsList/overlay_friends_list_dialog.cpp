#include "stdafx.h"
#include "../overlay_manager.h"
#include "overlay_friends_list_dialog.h"
#include "Emu/System.h"
#include "Emu/NP/rpcn_config.h"
#include "Emu/vfs_config.h"

namespace rsx
{
	namespace overlays
	{
		void friend_callback(void* param, rpcn::NotificationType ntype, const std::string& username, bool status)
		{
			auto* dlg = static_cast<friends_list_dialog*>(param);
			dlg->callback_handler(ntype, username, status);
		}

		friends_list_dialog::friends_list_entry::friends_list_entry(friends_list_dialog_page page, const std::string& username, const rpcn::friend_online_data& data)
		{
			std::unique_ptr<overlay_element> image = std::make_unique<image_view>();
			image->set_size(160, 110);
			image->set_padding(36, 36, 11, 11); // Square image, 88x88

			std::string avatar_path = g_cfg_vfs.get_dev_flash() + "vsh/resource/explore/user/";
			std::string text;

			switch (page)
			{
			case friends_list_dialog_page::friends:
			{
				avatar_path += data.online ? "013.png" : "009.png";
				text = get_localized_string(data.online ? localized_string_id::HOME_MENU_FRIENDS_STATUS_ONLINE : localized_string_id::HOME_MENU_FRIENDS_STATUS_OFFLINE);

				if (data.online)
				{
					if (!data.pr_title.empty())
					{
						fmt::append(text, " - %s", data.pr_title);
					}

					if (!data.pr_status.empty())
					{
						fmt::append(text, " - %s", data.pr_status);
					}
				}
				break;
			}
			case friends_list_dialog_page::invites:
			{
				// We use "online" to show whether an invite was sent or received
				avatar_path += data.online ? "012.png" : "011.png";
				text = get_localized_string(data.online ? localized_string_id::HOME_MENU_FRIENDS_REQUEST_RECEIVED : localized_string_id::HOME_MENU_FRIENDS_REQUEST_SENT);
				break;
			}
			case friends_list_dialog_page::blocked:
			{
				avatar_path += "010.png";
				text = get_localized_string(localized_string_id::HOME_MENU_FRIENDS_STATUS_BLOCKED);
				break;
			}
			}

			if (fs::exists(avatar_path))
			{
				icon_data = std::make_unique<image_info>(avatar_path);
				static_cast<image_view*>(image.get())->set_raw_image(icon_data.get());
			}
			else
			{
				// Fallback
				// TODO: use proper icon
				static_cast<image_view*>(image.get())->set_image_resource(resource_config::standard_image_resource::square);
			}

			std::unique_ptr<overlay_element> text_stack  = std::make_unique<vertical_layout>();
			std::unique_ptr<overlay_element> padding     = std::make_unique<spacer>();
			std::unique_ptr<overlay_element> header_text = std::make_unique<label>(username);
			std::unique_ptr<overlay_element> subtext     = std::make_unique<label>(text);

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

		friends_list_dialog::friends_list_dialog()
			: m_page_btn(120, 30)
			, m_extra_btn(120, 30)
		{
			m_allow_input_on_pause = true;

			m_dim_background = std::make_unique<overlay_element>();
			m_dim_background->set_size(virtual_width, virtual_height);
			m_dim_background->back_color.a = 0.5f;

			m_list = std::make_unique<list_view>(virtual_width - 2 * 20, 540);
			m_list->set_pos(20, 85);

			m_message_box = std::make_shared<home_menu_message_box>(20, 85, virtual_width - 2 * 20, 540);
			m_message_box->visible = false;

			m_description = std::make_unique<label>();
			m_description->set_font("Arial", 20);
			m_description->set_pos(20, 37);
			m_description->set_text("Select user"); // Fallback. I don't think this will ever be used, so I won't localize it.
			m_description->auto_resize();
			m_description->back_color.a = 0.f;

			fade_animation.duration_sec = 0.15f;

			return_code = selection_code::canceled;

			m_extra_btn.set_image_resource(resource_config::standard_image_resource::triangle);
			m_extra_btn.set_pos(330, m_list->y + m_list->h + 20);
			m_extra_btn.set_text("");
			m_extra_btn.set_font("Arial", 16);

			m_page_btn.set_image_resource(resource_config::standard_image_resource::square);
			m_page_btn.set_pos(m_list->x + m_list->w - (30 + 120), m_list->y + m_list->h + 20);
			m_page_btn.set_text(get_localized_string(localized_string_id::HOME_MENU_FRIENDS_NEXT_LIST));
			m_page_btn.set_font("Arial", 16);
		}

		void friends_list_dialog::update(u64 timestamp_us)
		{
			if (fade_animation.active)
			{
				fade_animation.update(timestamp_us);
			}
		}

		void friends_list_dialog::on_button_pressed(pad_button button_press, bool is_auto_repeat)
		{
			if (fade_animation.active) return;

			if (m_message_box && m_message_box->visible)
			{
				const page_navigation navigation = m_message_box->handle_button_press(button_press);
				if (navigation != page_navigation::stay)
				{
					m_message_box->hide();
					refresh();
				}
				return;
			}

			std::lock_guard lock(m_list_mutex);
			bool close_dialog = false;

			switch (button_press)
			{
			case pad_button::cross:
			case pad_button::triangle:
			{
				if (!m_list || m_list->m_items.empty())
					break;

				if (button_press == pad_button::triangle && m_current_page != friends_list_dialog_page::invites)
					break;

				const usz index = static_cast<usz>(m_list->get_selected_index());

				switch (m_current_page)
				{
				case friends_list_dialog_page::friends:
				{
					// Get selected user
					usz user_index = 0;
					std::string selected_username;
					for (const auto& [username, data] : m_friend_data.friends)
					{
						if (data.online && user_index++ == index)
						{
							selected_username = username;
							break;
						}
					}
					for (const auto& [username, data] : m_friend_data.friends)
					{
						if (!selected_username.empty())
							break;

						if (!data.online && user_index++ == index)
						{
							selected_username = username;
							break;
						}
					}

					if (!selected_username.empty() && m_message_box && !m_message_box->visible)
					{
						m_message_box->show(get_localized_string(localized_string_id::HOME_MENU_FRIENDS_REMOVE_USER_MSG, selected_username.c_str()), [this, selected_username]()
						{
							m_rpcn->remove_friend(selected_username);
						});
						refresh();
					}
					break;
				}
				case friends_list_dialog_page::invites:
				{
					// Get selected user
					usz user_index = 0;
					std::string selected_username;
					for (const std::string& username : m_friend_data.requests_received)
					{
						if (user_index == index)
						{
							selected_username = username;
							break;
						}
						user_index++;
					}
					for (const std::string& username : m_friend_data.requests_sent)
					{
						if (!selected_username.empty())
							break;

						if (user_index == index)
						{
							selected_username = username;
							break;
						}
						user_index++;
					}

					if (!selected_username.empty() && m_message_box && !m_message_box->visible)
					{
						if (user_index < m_friend_data.requests_received.size())
						{
							if (button_press == pad_button::triangle)
							{
								m_message_box->show(get_localized_string(localized_string_id::HOME_MENU_FRIENDS_REJECT_REQUEST_MSG, selected_username.c_str()), [this, selected_username]()
								{
									m_rpcn->remove_friend(selected_username);
								});
							}
							else
							{
								m_message_box->show(get_localized_string(localized_string_id::HOME_MENU_FRIENDS_ACCEPT_REQUEST_MSG, selected_username.c_str()), [this, selected_username]()
								{
									m_rpcn->add_friend(selected_username);
								});
							}
						}
						else
						{
							m_message_box->show(get_localized_string(localized_string_id::HOME_MENU_FRIENDS_CANCEL_REQUEST_MSG, selected_username.c_str()), [this, selected_username]()
							{
								m_rpcn->remove_friend(selected_username);
							});
						}
						refresh();
					}
					break;
				}
				case friends_list_dialog_page::blocked:
				{
					// Get selected user
					usz user_index = 0;
					std::string selected_username;
					for (const std::string& username : m_friend_data.blocked)
					{
						if (user_index++ == index)
						{
							selected_username = username;
							break;
						}
					}

					if (!selected_username.empty() && m_message_box && !m_message_box->visible)
					{
						m_message_box->show(get_localized_string(localized_string_id::HOME_MENU_FRIENDS_UNBLOCK_USER_MSG, selected_username.c_str()), []()
						{
							// TODO
						});
						refresh();
					}
					break;
				}
				}

				Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_decide.wav");
				return;
			}
			case pad_button::circle:
				Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_cancel.wav");
				close_dialog = true;
				break;
			case pad_button::square:
				switch (m_current_page)
				{
				case friends_list_dialog_page::friends: m_current_page = friends_list_dialog_page::invites; break;
				case friends_list_dialog_page::invites: m_current_page = friends_list_dialog_page::blocked; break;
				case friends_list_dialog_page::blocked: m_current_page = friends_list_dialog_page::friends; break;
				}
				m_list_dirty = true;
				break;
			case pad_button::dpad_up:
			case pad_button::ls_up:
				if (m_list)
					m_list->select_previous();
				break;
			case pad_button::dpad_down:
			case pad_button::ls_down:
				if (m_list)
					m_list->select_next();
				break;
			case pad_button::L1:
				if (m_list)
					m_list->select_previous(10);
				break;
			case pad_button::R1:
				if (m_list)
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

		compiled_resource friends_list_dialog::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			compiled_resource result;
			result.add(m_dim_background->get_compiled());
			{
				std::lock_guard lock(m_list_mutex);

				if (m_list_dirty.exchange(false))
				{
					if (m_current_page != m_last_page)
					{
						localized_string_id title_id = localized_string_id::INVALID;
						switch (m_current_page)
						{
						case friends_list_dialog_page::friends: title_id = localized_string_id::HOME_MENU_FRIENDS; break;
						case friends_list_dialog_page::invites: title_id = localized_string_id::HOME_MENU_FRIENDS_REQUESTS; break;
						case friends_list_dialog_page::blocked: title_id = localized_string_id::HOME_MENU_FRIENDS_BLOCKED; break;
						}
						m_description->set_text(get_localized_string(title_id));
						m_description->auto_resize();
					}

					reload();

					m_last_page.store(m_current_page);
				}

				if (m_message_box && m_message_box->visible)
				{
					result.add(m_message_box->get_compiled());
				}
				else
				{
					if (m_list)
					{
						result.add(m_list->get_compiled());

						if (!m_list->m_items.empty() && m_current_page == friends_list_dialog_page::invites)
						{
							// Get selected user
							const usz index = static_cast<usz>(m_list->get_selected_index());

							if (index < m_friend_data.requests_received.size())
							{
								m_extra_btn.set_text(get_localized_string(localized_string_id::HOME_MENU_FRIENDS_REJECT_REQUEST));
								result.add(m_extra_btn.get_compiled());
							}
						}
					}
					result.add(m_page_btn.get_compiled());
				}
			}
			result.add(m_description->get_compiled());

			fade_animation.apply(result);

			return result;
		}

		void friends_list_dialog::callback_handler(rpcn::NotificationType ntype, const std::string& /*username*/, bool /*status*/)
		{
			switch (ntype)
			{
			case rpcn::NotificationType::FriendNew: // Add a friend to the friendlist(either accepted a friend request or friend accepted it)
			case rpcn::NotificationType::FriendStatus: // Set status of friend to Offline or Online
			case rpcn::NotificationType::FriendQuery: // Other user sent a friend request
			case rpcn::NotificationType::FriendLost: // Remove friend from the friendlist(user removed friend or friend removed friend)
			case rpcn::NotificationType::FriendPresenceChanged:
			{
				m_list_dirty = true;
				break;
			}
			default:
			{
				rsx_log.fatal("An unhandled notification type was received by the RPCN friends dialog callback!");
				break;
			}
			}
		}

		void friends_list_dialog::reload()
		{
			std::vector<std::unique_ptr<overlay_element>> entries;
			std::string selected_user;
			s32 selected_index = 0;
			bool rpcn_connected = true;

			// Get selected user name
			if (m_list && m_current_page == m_last_page)
			{
				const s32 old_index = m_list->get_selected_index();
				s32 i = 0;

				switch (m_current_page)
				{
				case friends_list_dialog_page::friends:
				{
					for (const auto& [username, data] : m_friend_data.friends)
					{
						if (i++ == old_index)
						{
							selected_user = username;
							break;
						}
					}
					break;
				}
				case friends_list_dialog_page::invites:
				{
					for (const std::string& username : m_friend_data.requests_received)
					{
						if (i++ == old_index)
						{
							selected_user = username;
							break;
						}
					}
					for (const std::string& username : m_friend_data.requests_sent)
					{
						if (!selected_user.empty())
							break;

						if (i++ == old_index)
						{
							selected_user = username;
							break;
						}
					}
					break;
				}
				case friends_list_dialog_page::blocked:
				{
					for (const std::string& username : m_friend_data.blocked)
					{
						if (i++ == old_index)
						{
							selected_user = username;
							break;
						}
					}
					break;
				}
				}
			}

			if (auto res = m_rpcn->wait_for_connection(); res != rpcn::rpcn_state::failure_no_failure)
			{
				rsx_log.error("Failed to connect to RPCN: %s", rpcn::rpcn_state_to_string(res));
				status_flags |= status_bits::invalidate_image_cache;
				rpcn_connected = false;
			}

			if (auto res = m_rpcn->wait_for_authentified(); res != rpcn::rpcn_state::failure_no_failure)
			{
				rsx_log.error("Failed to authentify to RPCN: %s", rpcn::rpcn_state_to_string(res));
				status_flags |= status_bits::invalidate_image_cache;
				rpcn_connected = false;
			}

			// Get friends
			if (rpcn_connected)
			{
				m_rpcn->get_friends(m_friend_data);
			}
			else
			{
				m_friend_data = {};
			}

			switch (m_current_page)
			{
			case friends_list_dialog_page::friends:
			{
				// Sort users by online status
				std::vector<std::pair<const std::string&, const rpcn::friend_online_data&>> friends_online;
				std::vector<std::pair<const std::string&, const rpcn::friend_online_data&>> friends_offline;
				for (const auto& [username, data] : m_friend_data.friends)
				{
					if (data.online)
					{
						friends_online.push_back({ username, data });
					}
					else
					{
						friends_offline.push_back({ username, data });
					}
				}

				// Add users and try to find the old selected user again
				for (const auto& [username, data] : friends_online)
				{
					if (username == selected_user)
					{
						selected_index = ::size32(entries);
					}

					std::unique_ptr<overlay_element> entry = std::make_unique<friends_list_entry>(m_current_page, username, data);
					entries.emplace_back(std::move(entry));
				}
				for (const auto& [username, data] : friends_offline)
				{
					if (username == selected_user)
					{
						selected_index = ::size32(entries);
					}

					std::unique_ptr<overlay_element> entry = std::make_unique<friends_list_entry>(m_current_page, username, data);
					entries.emplace_back(std::move(entry));
				}
				break;
			}
			case friends_list_dialog_page::invites:
			{
				for (const std::string& username : m_friend_data.requests_received)
				{
					if (username == selected_user)
					{
						selected_index = ::size32(entries);
					}

					std::unique_ptr<overlay_element> entry = std::make_unique<friends_list_entry>(m_current_page, username, rpcn::friend_online_data(true, 0));
					entries.emplace_back(std::move(entry));
				}
				for (const std::string& username : m_friend_data.requests_sent)
				{
					if (username == selected_user)
					{
						selected_index = ::size32(entries);
					}

					std::unique_ptr<overlay_element> entry = std::make_unique<friends_list_entry>(m_current_page, username, rpcn::friend_online_data(false, 0));
					entries.emplace_back(std::move(entry));
				}
				break;
			}
			case friends_list_dialog_page::blocked:
			{
				for (const std::string& username : m_friend_data.blocked)
				{
					if (username == selected_user)
					{
						selected_index = ::size32(entries);
					}

					std::unique_ptr<overlay_element> entry = std::make_unique<friends_list_entry>(m_current_page, username, rpcn::friend_online_data(false, 0));
					entries.emplace_back(std::move(entry));
				}
				break;
			}
			}

			// Recreate list
			if (m_list)
			{
				status_flags |= status_bits::invalidate_image_cache;
			}

			m_list = std::make_unique<list_view>(virtual_width - 2 * 20, 540);
			m_list->set_pos(20, 85);

			for (auto& entry : entries)
			{
				m_list->add_entry(entry);
			}

			if (!m_list->m_items.empty())
			{
				// Only select an entry if there are entries available
				m_list->select_entry(selected_index);
			}
		}

		error_code friends_list_dialog::show(bool enable_overlay, std::function<void(s32 status)> on_close)
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

			g_cfg_rpcn.load(); // Ensures config is loaded even if rpcn is not running for simulated

			m_rpcn = rpcn::rpcn_client::get_instance(0);

			m_rpcn->register_friend_cb(friend_callback, this);

			m_description->set_text(get_localized_string(localized_string_id::HOME_MENU_FRIENDS));
			m_description->auto_resize();

			fade_animation.current = color4f(0.f);
			fade_animation.end = color4f(1.f);
			fade_animation.active = true;

			this->on_close = std::move(on_close);
			visible = true;

			const auto notify = std::make_shared<atomic_t<u32>>(0);
			auto& overlayman = g_fxo->get<display_manager>();

			overlayman.attach_thread_input(
				uid, "Friends list dialog",
				[notify]() { *notify = true; notify->notify_one(); }
			);

			while (!Emu.IsStopped() && !*notify)
			{
				notify->wait(0, atomic_wait_timeout{1'000'000});
			}

			return CELL_OK;
		}

		bool friends_list_dialog::rpcn_configured()
		{
			cfg_rpcn cfg;
			cfg.load();
			return !cfg.get_npid().empty() && !cfg.get_password().empty();
		}
	} // namespace overlays
} // namespace RSX
