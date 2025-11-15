#include "stdafx.h"
#include "overlay_home_menu.h"
#include "../overlay_manager.h"
#include "Emu/system_config.h"
#include "Utilities/date_time.h"

extern atomic_t<bool> g_user_asked_for_screenshot;

namespace rsx
{
	namespace overlays
	{
		std::string get_time_string()
		{
			return date_time::fmt_time("%Y/%m/%d %H:%M:%S", time(nullptr));
		}

		home_menu_dialog::home_menu_dialog()
			: m_main_menu(20, 85, virtual_width - 2 * 20, 540, false, nullptr)
		{
			m_allow_input_on_pause = true;

			m_dim_background.set_size(virtual_width, virtual_height);
			m_dim_background.back_color.a = 0.5f;

			m_description.set_font("Arial", 20);
			m_description.set_pos(20, 37);
			m_description.set_text(m_main_menu.title);
			m_description.auto_resize();
			m_description.back_color.a = 0.f;

			m_time_display.set_font("Arial", 14);
			m_time_display.set_text(get_time_string());
			m_time_display.auto_resize();
			m_time_display.set_pos(virtual_width - (20 + m_time_display.w), (m_description.y + m_description.h) - m_time_display.h);
			m_time_display.back_color.a = 0.f;

			fade_animation.duration_sec = 0.15f;

			return_code = selection_code::canceled;
		}

		void home_menu_dialog::update(u64 timestamp_us)
		{
			if (fade_animation.active)
			{
				fade_animation.update(timestamp_us);
			}

			static std::string last_time;
			std::string new_time = get_time_string();

			if (last_time != new_time)
			{
				m_time_display.set_text(new_time);
				m_time_display.auto_resize();
				last_time = std::move(new_time);
			}
		}

		void home_menu_dialog::on_button_pressed(pad_button button_press, bool is_auto_repeat)
		{
			if (fade_animation.active) return;

			// Increase auto repeat interval for some buttons
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
						path = parent->title + "  >  " + path;
					}
					m_description.set_text(path);
					m_description.auto_resize();
				}
				break;
			}
			case page_navigation::exit:
			case page_navigation::exit_for_screenshot:
			{
				fade_animation.current = color4f(1.f);
				fade_animation.end = color4f(0.f);
				fade_animation.active = true;

				fade_animation.on_finish = [this, navigation]
				{
					close(true, true);

					if (g_cfg.misc.pause_during_home_menu)
					{
						Emu.BlockingCallFromMainThread([]()
						{
							Emu.Resume();
						});
					}

					if (navigation == page_navigation::exit_for_screenshot)
					{
						rsx_log.notice("Taking screenshot after exiting home menu");
						g_user_asked_for_screenshot = true;
					}
				};
				break;
			}
			case page_navigation::stay:
			{
				break;
			}
			}
		}

		compiled_resource home_menu_dialog::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			compiled_resource result;
			result.add(m_dim_background.get_compiled());
			result.add(m_main_menu.get_compiled());
			result.add(m_description.get_compiled());
			result.add(m_time_display.get_compiled());

			fade_animation.apply(result);

			return result;
		}

		error_code home_menu_dialog::show(std::function<void(s32 status)> on_close)
		{
			visible = false;

			fade_animation.current = color4f(0.f);
			fade_animation.end = color4f(1.f);
			fade_animation.active = true;

			this->on_close = std::move(on_close);
			visible = true;

			const auto notify = std::make_shared<atomic_t<u32>>(0);
			auto& overlayman = g_fxo->get<display_manager>();

			overlayman.attach_thread_input(
				uid, "Home menu",
				[notify]() { *notify = true; notify->notify_one(); }
			);

			if (g_cfg.misc.pause_during_home_menu)
			{
				Emu.BlockingCallFromMainThread([]()
				{
					Emu.Pause(false, false);
				});
			}

			while (!Emu.IsStopped() && !*notify)
			{
				notify->wait(false, atomic_wait_timeout{1'000'000});
			}

			return CELL_OK;
		}
	} // namespace overlays
} // namespace RSX
