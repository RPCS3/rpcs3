#include "stdafx.h"
#include "overlay_home_menu.h"
#include "Emu/RSX/RSXThread.h"

extern atomic_t<bool> g_user_asked_for_recording;
extern atomic_t<bool> g_user_asked_for_screenshot;

namespace rsx
{
	namespace overlays
	{
		home_menu_dialog::home_menu_entry::home_menu_entry(const std::string& text)
		{
			std::unique_ptr<overlay_element> text_stack  = std::make_unique<vertical_layout>();
			std::unique_ptr<overlay_element> padding     = std::make_unique<spacer>();
			std::unique_ptr<overlay_element> header_text = std::make_unique<label>(text);

			padding->set_size(1, 1);
			header_text->set_size(1240, height);
			header_text->set_font("Arial", 16);
			header_text->set_wrap_text(true);
			header_text->align_text(text_align::center);

			// Make back color transparent for text
			header_text->back_color.a = 0.f;

			static_cast<vertical_layout*>(text_stack.get())->pack_padding = 5;
			static_cast<vertical_layout*>(text_stack.get())->add_element(padding);
			static_cast<vertical_layout*>(text_stack.get())->add_element(header_text);

			// Pack
			this->pack_padding = 15;
			add_element(text_stack);
		}

		home_menu_dialog::home_menu_dialog()
		{
			m_allow_input_on_pause = true;

			m_dim_background = std::make_unique<overlay_element>();
			m_dim_background->set_size(1280, 720);
			m_dim_background->back_color.a = 0.5f;

			m_list = std::make_unique<list_view>(1240, 540, false);
			m_list->set_pos(20, 85);

			m_description = std::make_unique<label>();
			m_description->set_font("Arial", 20);
			m_description->set_pos(20, 37);
			m_description->set_text(get_localized_string(localized_string_id::HOME_MENU_TITLE));
			m_description->auto_resize();
			m_description->back_color.a	= 0.f;

			fade_animation.duration = 0.15f;

			return_code = selection_code::canceled;
		}

		void home_menu_dialog::update()
		{
			static u64 frame = 0;

			if (Emu.IsPaused())
			{
				// Let's keep updating the animation anyway
				frame++;
			}
			else
			{
				frame = rsx::get_current_renderer()->vblank_count;
			}

			if (fade_animation.active)
			{
				fade_animation.update(frame);
			}
		}

		void home_menu_dialog::on_button_pressed(pad_button button_press)
		{
			if (fade_animation.active) return;

			bool close_dialog = false;

			switch (button_press)
			{
			case pad_button::cross:
				Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_decide.wav");

				if (const usz index = static_cast<usz>(m_list->get_selected_index()); index < m_callbacks.size())
				{
					if (const std::function<bool()>& func = ::at32(m_callbacks, index))
					{
						close_dialog = func();
					}
				}
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

					if (g_cfg.misc.pause_during_home_menu)
					{
						Emu.BlockingCallFromMainThread([]()
						{
							Emu.Resume();
						});
					}
				};
			}
			else
			{
				Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_cursor.wav");
			}
		}

		compiled_resource home_menu_dialog::get_compiled()
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

		struct home_menu_dialog_thread
		{
			static constexpr auto thread_name = "Home Menu Thread"sv;
		};

		void home_menu_dialog::add_item(const std::string& text, std::function<bool()> callback)
		{
			m_callbacks.push_back(std::move(callback));
			m_entries.push_back(std::make_unique<home_menu_entry>(text));
		}

		error_code home_menu_dialog::show(std::function<void(s32 status)> on_close)
		{
			visible = false;

			add_item(get_localized_string(localized_string_id::HOME_MENU_RESUME), []() -> bool
			{
				rsx_log.notice("User selected resume in home menu");
				return true;
			});

			add_item(get_localized_string(localized_string_id::HOME_MENU_SCREENSHOT), []() -> bool
			{
				rsx_log.notice("User selected screenshot in home menu");
				g_user_asked_for_screenshot = true;
				return true;
			});

			add_item(get_localized_string(localized_string_id::HOME_MENU_RECORDING), []() -> bool
			{
				rsx_log.notice("User selected recording in home menu");
				g_user_asked_for_recording = true;
				return true;
			});

			add_item(get_localized_string(localized_string_id::HOME_MENU_EXIT_GAME), []() -> bool
			{
				rsx_log.notice("User selected exit game in home menu");
				Emu.CallFromMainThread([]
				{
					Emu.GracefulShutdown(false, true);
				});
				return false;
			});

			// Center vertically if necessary
			if (const usz total_height = home_menu_entry::height * m_entries.size(); total_height < m_list->h)
			{
				m_list->advance_pos = (m_list->h - total_height) / 2;
			}

			for (auto& entry : m_entries)
			{
				m_list->add_entry(entry);
			}

			fade_animation.current = color4f(0.f);
			fade_animation.end = color4f(1.f);
			fade_animation.active = true;

			this->on_close = std::move(on_close);
			visible = true;

			auto& list_thread = g_fxo->get<named_thread<home_menu_dialog_thread>>();

			const auto notify = std::make_shared<atomic_t<bool>>(false);

			list_thread([&, notify]()
			{
				const u64 tbit = alloc_thread_bit();
				g_thread_bit = tbit;

				*notify = true;
				notify->notify_one();

				auto ref = g_fxo->get<display_manager>().get(uid);

				if (const auto error = run_input_loop())
				{
					if (error != selection_code::canceled)
					{
						rsx_log.error("Home menu dialog input loop exited with error code=%d", error);
					}
				}

				thread_bits &= ~tbit;
				thread_bits.notify_all();
			});

			if (g_cfg.misc.pause_during_home_menu)
			{
				Emu.BlockingCallFromMainThread([]()
				{
					Emu.Pause(false, false);
				});
			}

			while (list_thread < thread_state::errored && !*notify)
			{
				notify->wait(false, atomic_wait_timeout{1'000'000});
			}

			return CELL_OK;
		}
	} // namespace overlays
} // namespace RSX
