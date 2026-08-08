#include "stdafx.h"
#include "overlay_big_picture.h"
#include "../overlay_manager.h"
#include "Emu/System.h"

atomic_t<bool> g_big_picture_mode_active = false;

namespace rsx
{
	namespace overlays
	{
		big_picture_dialog::big_picture_dialog()
			: m_main_menu(20, 85, virtual_width - 2 * 20, 540, nullptr, [](std::string path, std::string title_id)
			{
				rsx_log.notice("Big Picture Mode: starting boot handoff for '%s' (path='%s')", title_id, path);

				Emu.CallFromMainThread([path, title_id]()
				{
					// Keep the game window alive across the handoff instead of closing and reopening it.
					Emu.SetContinuousMode(true);
					rsx_log.notice("Big Picture Mode: shutting down shell before boot");
					Emu.GracefulShutdown(false);
					rsx_log.notice("Big Picture Mode: shell shut down, is_stopped=%d", Emu.IsStopped());

					// Only mark the session as BPM-launched now, so the transitional shutdown above doesn't
					// itself get treated as "the game closed" and re-trigger Big Picture Mode.
					g_big_picture_mode_active = true;

					const game_boot_result result = Emu.BootGame(path, title_id);
					rsx_log.notice("Big Picture Mode: BootGame result=%s", result);

					if (is_error(result))
					{
						g_big_picture_mode_active = false;
					}
				});
			})
		{
			m_allow_input_on_pause = true;

			m_dim_background.set_size(virtual_width, virtual_height);
			m_dim_background.back_color.a = 0.85f;

			m_description.set_font("Arial", 20);
			m_description.set_pos(20, 37);
			m_description.set_text(get_localized_string(localized_string_id::BIG_PICTURE_MODE_TITLE));
			m_description.auto_resize();
			m_description.back_color.a = 0.f;

			fade_animation.duration_sec = 0.15f;

			return_code = selection_code::canceled;
		}

		void big_picture_dialog::update(u64 timestamp_us)
		{
			if (fade_animation.active)
			{
				fade_animation.update(timestamp_us);
			}

			m_main_menu.update(timestamp_us);
		}

		void big_picture_dialog::on_button_pressed(pad_button button_press, bool is_auto_repeat)
		{
			if (fade_animation.active) return;

			switch (button_press)
			{
			case pad_button::dpad_left:
			case pad_button::dpad_right:
			case pad_button::ls_left:
			case pad_button::ls_right:
				m_auto_repeat_ms_interval = 10;
				break;
			default:
				m_auto_repeat_ms_interval = m_auto_repeat_ms_interval_default;
				break;
			}

			const page_navigation navigation = m_main_menu.handle_button_press(button_press, is_auto_repeat, m_auto_repeat_ms_interval);

			switch (navigation)
			{
			case page_navigation::back:
			case page_navigation::next:
			{
				if (home_menu_page* page = m_main_menu.get_current_page(true))
				{
					std::string path = page->title;
					for (home_menu_page* parent = page->parent; parent; parent = parent->parent)
					{
						if (parent->title.empty())
						{
							break;
						}

						path = parent->title + "  >  " + path;
					}
					m_description.set_text(path.empty() ? get_localized_string(localized_string_id::BIG_PICTURE_MODE_TITLE) : path);
					m_description.auto_resize();
				}
				break;
			}
			case page_navigation::exit:
			{
				// Don't call close() synchronously from the input thread - just like the pause menu's own
				// "Exit Game", tearing down the shell on the main thread takes this dialog down as a side effect.
				g_big_picture_mode_active = false;

				Emu.CallFromMainThread([]()
				{
					Emu.GracefulShutdown(true, true);
				});
				break;
			}
			default:
				break;
			}
		}

		compiled_resource big_picture_dialog::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			compiled_resource result;
			result.add(m_dim_background.get_compiled());
			result.add(m_main_menu.get_compiled());
			result.add(m_description.get_compiled());

			fade_animation.apply(result);

			return result;
		}

		void big_picture_dialog::show()
		{
			visible = false;

			fade_animation.current = color4f(0.f);
			fade_animation.end = color4f(1.f);
			fade_animation.active = true;

			visible = true;

			auto& overlayman = g_fxo->get<display_manager>();
			overlayman.attach_thread_input(uid, "Big Picture Mode");
		}

		void open_big_picture_mode()
		{
			auto& overlayman = g_fxo->get<display_manager>();
			const auto dialog = overlayman.create<big_picture_dialog>();
			dialog->show();

			// Both exit paths (starting a game or leaving Big Picture Mode) tear down this shell via
			// Emu.GracefulShutdown on the main thread, which destroys the display_manager (and this dialog
			// with it) as a side effect. Just wait for that to happen instead of waiting on the dialog itself.
			while (!Emu.IsStopped())
			{
				thread_ctrl::wait_for(50'000);
			}
		}
	} // namespace overlays
} // namespace rsx
