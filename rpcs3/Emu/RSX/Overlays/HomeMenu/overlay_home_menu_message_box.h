#pragma once

#include "overlay_home_menu_components.h"

namespace rsx
{
	namespace overlays
	{
		struct home_menu_message_box : public overlay_element
		{
		public:
			home_menu_message_box(s16 x, s16 y, u16 width, u16 height);
			compiled_resource& get_compiled() override;
			void show(const std::string& text, std::function<void()> on_accept = nullptr, std::function<void()> on_cancel = nullptr);
			void hide();
			page_navigation handle_button_press(pad_button button_press);

		private:
			label m_label{};
			image_button m_accept_btn;
			image_button m_cancel_btn;
			std::function<void()> m_on_accept;
			std::function<void()> m_on_cancel;
		};
	}
}
