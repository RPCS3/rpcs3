#include "stdafx.h"
#include "../overlay_manager.h"
#include "overlay_sendmessage_dialog.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/NP/rpcn_client.h"
#include "Utilities/Thread.h"

namespace rsx
{
	namespace overlays
	{
		void sendmessage_friend_callback(void* param, rpcn::NotificationType ntype, const std::string& username, bool status)
		{
			auto* dlg = static_cast<sendmessage_dialog*>(param);
			dlg->callback_handler(ntype, username, status);
		}

		sendmessage_dialog::list_entry::list_entry(const std::string& msg)
		{
			std::unique_ptr<overlay_element> text_stack  = std::make_unique<vertical_layout>();
			std::unique_ptr<overlay_element> padding     = std::make_unique<spacer>();
			std::unique_ptr<overlay_element> text_label  = std::make_unique<label>(msg);

			padding->set_size(1, 1);
			text_label->set_size(800, 40);
			text_label->set_font("Arial", 16);
			text_label->set_wrap_text(true);

			// Make back color transparent for text
			text_label->back_color.a = 0.f;

			static_cast<vertical_layout*>(text_stack.get())->pack_padding = 5;
			static_cast<vertical_layout*>(text_stack.get())->add_element(padding);
			static_cast<vertical_layout*>(text_stack.get())->add_element(text_label);

			// Pack
			pack_padding = 15;
			add_element(text_stack);
		}

		sendmessage_dialog::sendmessage_dialog() : SendMessageDialogBase()
		{
			m_dim_background = std::make_unique<overlay_element>();
			m_dim_background->set_size(virtual_width, virtual_height);
			m_dim_background->back_color.a = 0.5f;

			m_description = std::make_unique<label>();
			m_description->set_font("Arial", 20);
			m_description->set_pos(20, 37);
			m_description->set_text(get_localized_string(localized_string_id::CELL_NP_SENDMESSAGE_DIALOG_TITLE));
			m_description->auto_resize();
			m_description->back_color.a = 0.f;

			fade_animation.duration = 0.15f;

			return_code = selection_code::canceled;
		}

		void sendmessage_dialog::update()
		{
			if (fade_animation.active)
			{
				fade_animation.update(rsx::get_current_renderer()->vblank_count);
			}
		}

		void sendmessage_dialog::on_button_pressed(pad_button button_press, bool is_auto_repeat)
		{
			if (fade_animation.active) return;

			bool close_dialog = false;

			std::lock_guard lock(m_mutex);

			switch (button_press)
			{
			case pad_button::cross:
				if (m_list->m_items.empty())
					break;

				if (!get_current_selection().empty())
				{
					return_code = selection_code::ok;
				}
				else
				{
					return_code = selection_code::error;
				}
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

		compiled_resource sendmessage_dialog::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			compiled_resource result;
			result.add(m_dim_background->get_compiled());

			if (m_list)
			{
				result.add(m_list->get_compiled());
			}

			result.add(m_description->get_compiled());

			fade_animation.apply(result);

			return result;
		}

		error_code sendmessage_dialog::Exec(message_data& msg_data, std::set<std::string>& npids)
		{
			visible = false;

			switch (msg_data.mainType)
			{
			case SceNpBasicMessageMainType::SCE_NP_BASIC_MESSAGE_MAIN_TYPE_ADD_FRIEND:
				m_description->set_text(get_localized_string(localized_string_id::CELL_NP_SENDMESSAGE_DIALOG_TITLE_ADD_FRIEND));
				m_description->auto_resize();
				break;
			case SceNpBasicMessageMainType::SCE_NP_BASIC_MESSAGE_MAIN_TYPE_INVITE:
				m_description->set_text(get_localized_string(localized_string_id::CELL_NP_SENDMESSAGE_DIALOG_TITLE_INVITE));
				m_description->auto_resize();
				break;
			case SceNpBasicMessageMainType::SCE_NP_BASIC_MESSAGE_MAIN_TYPE_DATA_ATTACHMENT:
			case SceNpBasicMessageMainType::SCE_NP_BASIC_MESSAGE_MAIN_TYPE_GENERAL:
			case SceNpBasicMessageMainType::SCE_NP_BASIC_MESSAGE_MAIN_TYPE_CUSTOM_DATA:
			case SceNpBasicMessageMainType::SCE_NP_BASIC_MESSAGE_MAIN_TYPE_URL_ATTACHMENT:
			default:
				break; // Title already set in constructor
			}

			m_rpcn = rpcn::rpcn_client::get_instance(true);

			// Get list of messages
			rpcn::friend_data data;
			m_rpcn->get_friends_and_register_cb(data, sendmessage_friend_callback, this);
			{
				std::lock_guard lock(m_mutex);

				for (const auto& [name, online_data] : data.friends)
				{
					// Only add online friends to the list
					if (online_data.online)
					{
						if (std::any_of(m_entry_names.cbegin(), m_entry_names.cend(), [&name](const std::string& entry){ return entry == name; }))
							continue;

						m_entry_names.push_back(name);
					}
				}

				reload({});
			}

			fade_animation.current = color4f(0.f);
			fade_animation.end = color4f(1.f);
			fade_animation.active = true;

			visible = true;

			const auto notify = std::make_shared<atomic_t<u32>>(0);
			auto& overlayman = g_fxo->get<display_manager>();

			// Block until the user exits the dialog
			overlayman.attach_thread_input(
				uid, "Sendmessage dialog", nullptr,
				[notify](s32) { *notify = true; notify->notify_one(); }
			);

			while (!Emu.IsStopped() && !*notify)
			{
				notify->wait(0, atomic_wait_timeout{1'000'000});
			}

			m_rpcn->remove_friend_cb(sendmessage_friend_callback, this);

			error_code result = CELL_CANCEL;

			switch (return_code)
			{
			case selection_code::ok:
			{
				const std::string current_selection = get_current_selection();

				if (current_selection.empty())
				{
					rsx_log.fatal("sendmessage dialog can't send message to empty npid");
					break;
				}

				npids.insert(current_selection);

				// Send the message
				if (m_rpcn->send_message(msg_data, npids))
				{
					result = CELL_OK;
				}
				break;
			}
			default:
				rsx_log.error("sendmessage dialog exited with error: %d", return_code);
				break;
			}

			return result;
		}

		std::string sendmessage_dialog::get_current_selection() const
		{
			if (const s32 index = m_list->get_selected_index(); index >= 0 && static_cast<usz>(index) < m_entry_names.size())
			{
				return m_entry_names[index];
			}

			return {};
		}

		void sendmessage_dialog::reload(const std::string& previous_selection)
		{
			if (m_list)
			{
				status_flags |= status_bits::invalidate_image_cache;
			}
			
			m_list = std::make_unique<list_view>(virtual_width - 2 * 20, 540, false);
			m_list->set_pos(20, 85);

			for (const std::string& name : m_entry_names)
			{
				std::unique_ptr<overlay_element> entry = std::make_unique<list_entry>(name);
				m_list->add_entry(entry);
			}

			if (m_list->m_items.empty())
			{
				m_list->set_cancel_only(true);
			}
			else if (m_list->get_cancel_only())
			{
				m_list->set_cancel_only(false);
				m_list->select_entry(0);
			}
			else
			{
				// Only select an entry if there are entries available
				s32 selected_index = 0;

				// Try to select the previous selection
				if (!previous_selection.empty())
				{
					for (s32 i = 0; i < ::narrow<s32>(m_entry_names.size()); i++)
					{
						if (m_entry_names[i] == previous_selection)
						{
							selected_index = i;
							break;
						}
					}
				}

				m_list->select_entry(selected_index);
			}
		}

		void sendmessage_dialog::callback_handler(u16 ntype, const std::string& username, bool status)
		{
			std::lock_guard lock(m_mutex);

			const auto add_friend = [&]()
			{
				if (std::any_of(m_entry_names.cbegin(), m_entry_names.cend(), [&username](const std::string& entry){ return entry == username; }))
					return;

				const std::string current_selection = get_current_selection();
				m_entry_names.push_back(username);
				reload(current_selection);
			};

			const auto remove_friend = [&]()
			{
				const auto it = std::find(m_entry_names.cbegin(), m_entry_names.cend(), username);
				if (it == m_entry_names.cend())
					return;

				const std::string current_selection = get_current_selection();
				m_entry_names.erase(it);
				reload(current_selection);
			};

			switch (ntype)
			{
			case rpcn::NotificationType::FriendQuery: // Other user sent a friend request
				break;
			case rpcn::NotificationType::FriendNew: // Add a friend to the friendlist(either accepted a friend request or friend accepted it)
			{
				if (status)
				{
					add_friend();
				}
				break;
			}
			case rpcn::NotificationType::FriendLost: // Remove friend from the friendlist(user removed friend or friend removed friend)
			{
				remove_friend();
				break;
			}
			case rpcn::NotificationType::FriendStatus: // Set status of friend to Offline or Online
			{
				if (status)
				{
					add_friend();
				}
				else
				{
					remove_friend();
				}
				break;
			}
			default:
			{
				rsx_log.fatal("An unhandled notification type was received by the sendmessage dialog callback!");
				break;
			}
			}
		}
	} // namespace overlays
} // namespace RSX
