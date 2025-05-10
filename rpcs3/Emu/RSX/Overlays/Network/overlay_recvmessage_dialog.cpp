#include "stdafx.h"
#include "../overlay_manager.h"
#include "overlay_recvmessage_dialog.h"
#include "Emu/System.h"
#include "Emu/NP/rpcn_client.h"

namespace rsx
{
	namespace overlays
	{
		void recvmessage_callback(void* param, shared_ptr<std::pair<std::string, message_data>> new_msg, u64 msg_id)
		{
			auto* dlg = static_cast<recvmessage_dialog*>(param);
			dlg->callback_handler(std::move(new_msg), msg_id);
		}

		recvmessage_dialog::list_entry::list_entry(const std::string& name, const std::string& subj, const std::string& body)
		{
			std::unique_ptr<overlay_element> prefix_stack = std::make_unique<vertical_layout>();
			std::unique_ptr<overlay_element> text_stack = std::make_unique<vertical_layout>();
			std::unique_ptr<overlay_element> name_label = std::make_unique<label>(name);
			std::unique_ptr<overlay_element> subj_label = std::make_unique<label>(subj);
			std::unique_ptr<overlay_element> body_label = std::make_unique<label>(body);
			std::unique_ptr<overlay_element> name_prefix_label = std::make_unique<label>(get_localized_string(localized_string_id::CELL_NP_RECVMESSAGE_DIALOG_FROM));
			std::unique_ptr<overlay_element> subj_prefix_label = std::make_unique<label>(get_localized_string(localized_string_id::CELL_NP_RECVMESSAGE_DIALOG_SUBJECT));

			name_prefix_label->set_size(0, 40);
			name_prefix_label->set_font("Arial", 16);
			name_prefix_label->set_wrap_text(false);
			static_cast<label*>(name_prefix_label.get())->auto_resize(true);

			subj_prefix_label->set_size(0, 40);
			subj_prefix_label->set_font("Arial", 16);
			subj_prefix_label->set_wrap_text(false);
			static_cast<label*>(subj_prefix_label.get())->auto_resize(true);

			name_label->set_size(200, 40);
			name_label->set_font("Arial", 16);
			name_label->set_wrap_text(false);

			subj_label->set_size(600, 40);
			subj_label->set_font("Arial", 16);
			subj_label->set_wrap_text(false);

			body_label->set_size(800, 0);
			body_label->set_font("Arial", 16);
			body_label->set_wrap_text(true);
			static_cast<label*>(body_label.get())->auto_resize(true);

			// Make back color transparent for text
			name_label->back_color.a = 0.f;
			subj_label->back_color.a = 0.f;
			body_label->back_color.a = 0.f;
			name_prefix_label->back_color.a = 0.f;
			subj_prefix_label->back_color.a = 0.f;

			static_cast<vertical_layout*>(prefix_stack.get())->pack_padding = 5;
			static_cast<vertical_layout*>(prefix_stack.get())->add_spacer();
			static_cast<vertical_layout*>(prefix_stack.get())->add_element(name_prefix_label);
			static_cast<vertical_layout*>(prefix_stack.get())->add_element(subj_prefix_label);

			static_cast<vertical_layout*>(text_stack.get())->pack_padding = 5;
			static_cast<vertical_layout*>(text_stack.get())->add_spacer();
			static_cast<vertical_layout*>(text_stack.get())->add_element(name_label);
			static_cast<vertical_layout*>(text_stack.get())->add_element(subj_label);
			static_cast<vertical_layout*>(text_stack.get())->add_element(body_label);

			// Add spacer to make the thing look a bit nicer at the bottom... should ideally not be necessary
			static_cast<vertical_layout*>(text_stack.get())->pack_padding = 25;
			static_cast<vertical_layout*>(text_stack.get())->add_spacer();

			// Pack
			pack_padding = 15;
			add_element(prefix_stack);
			add_element(text_stack);
		}

		recvmessage_dialog::recvmessage_dialog() : RecvMessageDialogBase()
		{
			m_dim_background = std::make_unique<overlay_element>();
			m_dim_background->set_size(virtual_width, virtual_height);
			m_dim_background->back_color.a = 0.5f;

			m_list = std::make_unique<list_view>(virtual_width - 2 * 20, 540, true, true);
			m_list->set_pos(20, 85);

			m_description = std::make_unique<label>();
			m_description->set_font("Arial", 20);
			m_description->set_pos(20, 37);
			m_description->set_text(get_localized_string(localized_string_id::CELL_NP_RECVMESSAGE_DIALOG_TITLE));
			m_description->auto_resize();
			m_description->back_color.a = 0.f;

			fade_animation.duration_sec = 0.15f;

			return_code = selection_code::canceled;
		}

		void recvmessage_dialog::update(u64 timestamp_us)
		{
			if (fade_animation.active)
			{
				fade_animation.update(timestamp_us);
			}
		}

		void recvmessage_dialog::on_button_pressed(pad_button button_press, bool is_auto_repeat)
		{
			if (fade_animation.active) return;

			bool close_dialog = false;

			std::lock_guard lock(m_mutex);

			switch (button_press)
			{
			case pad_button::cross:
			case pad_button::triangle:
				if (m_list->m_items.empty())
					break;

				if (const usz index = static_cast<usz>(m_list->get_selected_index()); index < m_entry_ids.size())
				{
					return_code = button_press == pad_button::cross ? selection_code::ok : selection_code::no;
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

		compiled_resource recvmessage_dialog::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			compiled_resource result;
			result.add(m_dim_background->get_compiled());
			result.add(m_list->get_compiled());
			result.add(m_description->get_compiled());

			fade_animation.apply(result);

			return result;
		}

		error_code recvmessage_dialog::Exec(SceNpBasicMessageMainType type, SceNpBasicMessageRecvOptions options, SceNpBasicMessageRecvAction& recv_result, u64& chosen_msg_id)
		{
			visible = false;

			switch (type)
			{
			case SceNpBasicMessageMainType::SCE_NP_BASIC_MESSAGE_MAIN_TYPE_ADD_FRIEND:
				m_description->set_text(get_localized_string(localized_string_id::CELL_NP_RECVMESSAGE_DIALOG_TITLE_ADD_FRIEND));
				m_description->auto_resize();
				break;
			case SceNpBasicMessageMainType::SCE_NP_BASIC_MESSAGE_MAIN_TYPE_INVITE:
				m_description->set_text(get_localized_string(localized_string_id::CELL_NP_RECVMESSAGE_DIALOG_TITLE_INVITE));
				m_description->auto_resize();
				break;
			case SceNpBasicMessageMainType::SCE_NP_BASIC_MESSAGE_MAIN_TYPE_DATA_ATTACHMENT:
			case SceNpBasicMessageMainType::SCE_NP_BASIC_MESSAGE_MAIN_TYPE_GENERAL:
			case SceNpBasicMessageMainType::SCE_NP_BASIC_MESSAGE_MAIN_TYPE_CUSTOM_DATA:
			case SceNpBasicMessageMainType::SCE_NP_BASIC_MESSAGE_MAIN_TYPE_URL_ATTACHMENT:
			default:
				break; // Title already set in constructor
			}

			const bool preserve         = options & SCE_NP_BASIC_RECV_MESSAGE_OPTIONS_PRESERVE;
			const bool include_bootable = options & SCE_NP_BASIC_RECV_MESSAGE_OPTIONS_INCLUDE_BOOTABLE;

			m_rpcn = rpcn::rpcn_client::get_instance(0, true);

			// Get list of messages
			const auto messages = m_rpcn->get_messages_and_register_cb(type, include_bootable, recvmessage_callback, this);
			{
				std::lock_guard lock(m_mutex);

				for (const auto& [id, message] : messages)
				{
					ensure(message);
					std::unique_ptr<overlay_element> entry = std::make_unique<list_entry>(message->first, message->second.subject, message->second.body);
					m_entries.emplace_back(std::move(entry));
					m_entry_ids.push_back(id);
				}

				for (auto& entry : m_entries)
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
					m_list->select_entry(0);
				}
			}

			fade_animation.current = color4f(0.f);
			fade_animation.end = color4f(1.f);
			fade_animation.active = true;

			visible = true;

			const auto notify = std::make_shared<atomic_t<u32>>(0);
			auto& overlayman = g_fxo->get<display_manager>();
			auto& nps = g_fxo->get<np_state>();

			// Block until the user exits the dialog
			overlayman.attach_thread_input(
				uid, "Recvmessage dialog", nullptr,
				[notify](s32) { *notify = true; notify->notify_one(); }
			);

			while (!Emu.IsStopped() && !*notify && !nps.abort_gui_flag)
			{
				notify->wait(0, atomic_wait_timeout{1'000'000});
			}

			m_rpcn->remove_message_cb(recvmessage_callback, this);

			error_code result = CELL_CANCEL;

			if (nps.abort_gui_flag.exchange(false))
			{
				rsx_log.warning("Recvmessage dialog aborted by sceNp!");
				close(false, true);
				return result;
			}

			auto accept_or_deny = [preserve, this, &result, &recv_result, &chosen_msg_id](SceNpBasicMessageRecvAction result_from_action)
			{
				{
					std::lock_guard lock(m_mutex);
					const int selected_index = m_list->get_selected_index();

					if (selected_index < 0 || static_cast<usz>(selected_index) >= m_entry_ids.size())
					{
						rsx_log.error("recvmessage dialog exited with unexpected selection: index=%d, entries=%d", selected_index, m_entry_ids.size());
						return;
					}

					chosen_msg_id = ::at32(m_entry_ids, selected_index);
				}
				recv_result   = result_from_action;
				result        = CELL_OK;

				if (!preserve)
				{
					m_rpcn->mark_message_used(chosen_msg_id);
				}
			};

			switch (return_code)
			{
			case selection_code::ok:
				accept_or_deny(SCE_NP_BASIC_MESSAGE_ACTION_ACCEPT);
				break;
			case selection_code::no:
				accept_or_deny(SCE_NP_BASIC_MESSAGE_ACTION_DENY);
				break;
			case selection_code::canceled:
				rsx_log.notice("recvmessage dialog was canceled");
				break;
			default:
				rsx_log.error("recvmessage dialog exited with error: %d", return_code);
				break;
			}

			return result;
		}

		void recvmessage_dialog::callback_handler(shared_ptr<std::pair<std::string, message_data>> new_msg, u64 msg_id)
		{
			ensure(new_msg);

			std::lock_guard lock(m_mutex);

			std::unique_ptr<overlay_element> entry = std::make_unique<list_entry>(new_msg->first, new_msg->second.subject, new_msg->second.body);
			m_entries.emplace_back(std::move(entry));
			m_entry_ids.push_back(msg_id);
			m_list->add_entry(m_entries.back());

			if (m_list->get_cancel_only())
			{
				m_list->set_cancel_only(false);
				m_list->select_entry(0);
			}
		}
	} // namespace overlays
} // namespace RSX
