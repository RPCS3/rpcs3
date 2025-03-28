#include "stdafx.h"
#include "overlay_home_menu_message_box.h"
#include "Emu/System.h"
#include "Emu/system_config.h"

namespace rsx
{
	namespace overlays
	{
		home_menu_message_box::home_menu_message_box(s16 x, s16 y, u16 width, u16 height)
			: overlay_element()
			, m_accept_btn(120, 30)
			, m_cancel_btn(120, 30)
		{
			back_color = {0.15f, 0.15f, 0.15f, 0.95f};

			set_size(width, height);
			set_pos(x, y);

			m_label.align_text(text_align::center);
			m_label.set_font("Arial", 16);
			m_label.back_color.a = 0.0f;

			if (g_cfg.sys.enter_button_assignment == enter_button_assign::circle)
			{
				m_accept_btn.set_image_resource(resource_config::standard_image_resource::circle);
				m_cancel_btn.set_image_resource(resource_config::standard_image_resource::cross);
			}
			else
			{
				m_accept_btn.set_image_resource(resource_config::standard_image_resource::cross);
				m_cancel_btn.set_image_resource(resource_config::standard_image_resource::circle);
			}

			m_accept_btn.set_pos(x + 30, y + height + 20);
			m_cancel_btn.set_pos(x + 180, y + height + 20);

			m_accept_btn.set_text(localized_string_id::RSX_OVERLAYS_LIST_SELECT);
			m_cancel_btn.set_text(localized_string_id::RSX_OVERLAYS_LIST_CANCEL);

			m_accept_btn.set_font("Arial", 16);
			m_cancel_btn.set_font("Arial", 16);
		}

		compiled_resource& home_menu_message_box::get_compiled()
		{
			if (!is_compiled())
			{
				compiled_resource& compiled = overlay_element::get_compiled();
				compiled.add(m_label.get_compiled());
				compiled.add(m_cancel_btn.get_compiled());
				compiled.add(m_accept_btn.get_compiled());
			}
			return compiled_resources;
		}

		void home_menu_message_box::show(const std::string& text, std::function<void()> on_accept, std::function<void()> on_cancel)
		{
			m_on_accept = std::move(on_accept);
			m_on_cancel = std::move(on_cancel);
			m_label.set_text(text);
			m_label.auto_resize();
			m_label.set_pos(x + (w - m_label.w) / 2, y + (h - m_label.h) / 2);
			visible = true;
			refresh();
		}

		void home_menu_message_box::hide()
		{
			visible = false;
			refresh();
		}

		page_navigation home_menu_message_box::handle_button_press(pad_button button_press)
		{
			switch (button_press)
			{
			case pad_button::cross:
			{
				Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_decide.wav");
				if (m_on_accept)
				{
					m_on_accept();
				}
				return page_navigation::next;
			}
			case pad_button::circle:
			{
				Emu.GetCallbacks().play_sound(fs::get_config_dir() + "sounds/snd_cancel.wav");
				if (m_on_cancel)
				{
					m_on_cancel();
				}
				return page_navigation::back;
			}
			default:
			{
				return page_navigation::stay;
			}
			}
		}
	}
}
