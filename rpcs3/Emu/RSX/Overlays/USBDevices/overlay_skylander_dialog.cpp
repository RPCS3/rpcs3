#include "stdafx.h"
#include "../overlay_manager.h"
#include "overlay_skylander_dialog.h"
#include "Emu/System.h"
#include "Emu/VFS.h"

namespace rsx
{
	namespace overlays
	{
		static constexpr u16 trophy_list_y = 85;
		static constexpr u16 trophy_list_h = 540;

		skylander_dialog::skylander_dialog()
		{
			m_allow_input_on_pause = true;

			m_dim_background = std::make_unique<overlay_element>();
			m_dim_background->set_size(virtual_width, virtual_height);
			m_dim_background->back_color.a = 0.9f;

			m_description = std::make_unique<label>();
			m_description->set_font("Arial", 20);
			m_description->set_pos(20, 37);
			m_description->set_text("Emulated Skylander Portal");
			m_description->auto_resize();
			m_description->back_color.a = 0.f;

			m_show_hidden_trophies_button = std::make_unique<image_button>();
			m_show_hidden_trophies_button->set_text(m_show_hidden_trophies ? localized_string_id::HOME_MENU_TROPHY_HIDE_HIDDEN_TROPHIES : localized_string_id::HOME_MENU_TROPHY_SHOW_HIDDEN_TROPHIES);
			m_show_hidden_trophies_button->set_image_resource(resource_config::standard_image_resource::square);
			m_show_hidden_trophies_button->set_size(120, 30);
			m_show_hidden_trophies_button->set_pos(180, trophy_list_y + trophy_list_h + 20);
			m_show_hidden_trophies_button->set_font("Arial", 16);

			fade_animation.duration_sec = 0.15f;

			return_code = selection_code::canceled;
		}

		void skylander_dialog::update(u64 timestamp_us)
		{
			if (fade_animation.active)
			{
				fade_animation.update(timestamp_us);
			}
		}

		void skylander_dialog::on_button_pressed(pad_button button_press, bool is_auto_repeat)
		{
			if (fade_animation.active)
				return;

			bool close_dialog = false;

			switch (button_press)
			{
			case pad_button::circle:
				play_sound(sound_effect::cancel);
				close_dialog = true;
				break;
			case pad_button::square:
				break;
			case pad_button::dpad_up:
			case pad_button::ls_up:
				break;
			case pad_button::dpad_down:
			case pad_button::ls_down:
				break;
			case pad_button::L1:
				break;
			case pad_button::R1:
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
				play_sound(sound_effect::cursor);
			}
		}

		compiled_resource skylander_dialog::get_compiled()
		{
			if (!visible)
			{
				return {};
			}

			compiled_resource result;
			result.add(m_dim_background->get_compiled());

			result.add(m_description->get_compiled());
			result.add(m_show_hidden_trophies_button->get_compiled());

			fade_animation.apply(result);

			return result;
		}

		void skylander_dialog::show()
		{
			visible = false;

			fade_animation.current = color4f(0.f);
			fade_animation.end = color4f(1.f);
			fade_animation.active = true;

			visible = true;

			const auto notify = std::make_shared<atomic_t<u32>>(0);
			auto& overlayman = g_fxo->get<display_manager>();

			overlayman.attach_thread_input(
				uid, "Skylander dialog",
				[notify]()
				{
					*notify = true;
					notify->notify_one();
				});

			while (!Emu.IsStopped() && !*notify)
			{
				notify->wait(0, atomic_wait_timeout{1'000'000});
			}
		}
	} // namespace overlays
} // namespace rsx
