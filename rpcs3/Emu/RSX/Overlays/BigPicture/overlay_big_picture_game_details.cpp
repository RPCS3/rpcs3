#include "stdafx.h"
#include "overlay_big_picture_game_details.h"
#include "Emu/RSX/Overlays/HomeMenu/overlay_home_menu_components.h"

namespace rsx
{
	namespace overlays
	{
		big_picture_game_details::big_picture_game_details(s16 x, s16 y, u16 width, u16 height)
			: m_x(x), m_y(y), m_w(width), m_h(height)
		{
			m_pic_background.set_pos(0, 0);
			m_pic_background.set_size(overlay::virtual_width, overlay::virtual_height);
			m_pic_background.back_color.a = 0.f;

			m_dim_background.set_pos(0, 0);
			m_dim_background.set_size(overlay::virtual_width, overlay::virtual_height);
			m_dim_background.back_color.a = 0.75f;

			m_icon.set_pos(x + 40, y + 40);
			m_icon.set_size(320, 176);
			m_icon.back_color.a = 0.f;

			m_title.set_font("Arial", 28);
			m_title.set_pos(x + 40, y + 240);
			m_title.back_color.a = 0.f;
			m_title.set_wrap_text(true);
			m_title.set_size(width - 80, 80);

			m_info_text.set_font("Arial", 16);
			m_info_text.set_pos(x + 40, y + 330);
			m_info_text.back_color.a = 0.f;
			m_info_text.set_wrap_text(true);
			m_info_text.set_size(width - 80, 200);

			m_start_btn.set_image_resource(resource_config::standard_image_resource::cross);
			m_start_btn.set_text(localized_string_id::BIG_PICTURE_GAME_DETAILS_START);
			m_start_btn.set_font("Arial", 16);
			m_start_btn.set_pos(x + 40, y + height - 80);

			m_back_hint.set_image_resource(resource_config::standard_image_resource::circle);
			m_back_hint.set_text(localized_string_id::BIG_PICTURE_HINT_BACK);
			m_back_hint.set_font("Arial", 16);
			m_back_hint.set_pos(x + 40 + 120 + 20, y + height - 80);
		}

		void big_picture_game_details::show(const GameInfo& info, const image_info* icon_data)
		{
			m_title.set_text(info.name.empty() ? info.serial : info.name);
			m_title.auto_resize(false, m_w - 80);

			m_info_text.set_text(fmt::format("%s\n%s\n%s", info.serial, info.category, info.app_ver));

			const std::string game_dir = fs::get_parent_dir(info.icon_path);

			std::string pic_path = game_dir + "/PIC1.PNG";

			if (!fs::is_file(pic_path))
			{
				// Some games only ship the PIC0 overlay layer, not the PIC1 background layer.
				pic_path = game_dir + "/PIC0.PNG";
			}

			if (fs::is_file(pic_path))
			{
				m_pic_data = std::make_unique<image_info>(pic_path);
				// The renderer's texture cache is keyed by this object's address, which can be reused by an
				// unrelated image after the old one is freed - force a re-upload instead of trusting the cache.
				m_pic_data->dirty = true;
				m_pic_background.set_raw_image(m_pic_data.get());
				m_pic_background.back_color.a = 0.35f;
			}
			else
			{
				m_pic_data.reset();
				m_pic_background.clear_image();
				m_pic_background.back_color.a = 0.f;
			}

			m_pic_background.refresh();

			if (const std::string icon1_path = game_dir + "/ICON1.PAM"; fs::is_file(icon1_path))
			{
				const std::string snd0_path = game_dir + "/SND0.AT3";
				m_video = std::make_unique<video_view>(icon1_path, snd0_path, info.icon_path);
				m_video->set_pos(m_icon.x, m_icon.y);
				m_video->set_size(m_icon.w, m_icon.h);
				m_video->set_active(true);
			}
			else
			{
				m_video.reset();

				if (icon_data)
				{
					m_icon.set_raw_image(icon_data);
				}
				else
				{
					m_icon.set_image_resource(resource_config::standard_image_resource::new_entry);
				}

				// set_raw_image()/set_image_resource() don't invalidate the compiled vertex cache themselves.
				m_icon.refresh();
			}

			m_visible = true;
		}

		void big_picture_game_details::hide()
		{
			m_visible = false;

			if (m_video)
			{
				m_video->set_active(false);
			}
		}

		big_picture_game_details::result big_picture_game_details::handle_button_press(pad_button button_press)
		{
			switch (button_press)
			{
			case pad_button::cross:
				play_sound(sound_effect::accept);
				return result::start;
			case pad_button::circle:
				play_sound(sound_effect::cancel);
				return result::back;
			default:
				return result::stay;
			}
		}

		compiled_resource& big_picture_game_details::get_compiled()
		{
			m_compiled = {};

			if (!m_visible)
			{
				return m_compiled;
			}

			m_compiled.add(m_pic_background.get_compiled());
			m_compiled.add(m_dim_background.get_compiled());
			m_compiled.add(m_video ? m_video->get_compiled() : m_icon.get_compiled());
			m_compiled.add(m_title.get_compiled());
			m_compiled.add(m_info_text.get_compiled());
			m_compiled.add(m_start_btn.get_compiled());
			m_compiled.add(m_back_hint.get_compiled());

			return m_compiled;
		}
	}
}
