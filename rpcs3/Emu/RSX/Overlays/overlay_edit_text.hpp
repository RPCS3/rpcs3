#pragma once

#include "overlay_controls.h"

namespace rsx
{
	namespace overlays
	{
		struct edit_text : public label
		{
			enum class direction
			{
				up,
				down,
				left,
				right
			};

			usz caret_position = 0;

			bool m_reset_caret_pulse = false;
			bool password_mode = false;

			std::u32string value;
			std::u32string placeholder;

			using label::label;

			void set_text(const std::string& text) override;
			void set_unicode_text(const std::u32string& text) override;

			void set_placeholder(const std::u32string& placeholder_text);

			void move_caret(direction dir);
			void insert_text(const std::u32string& str);
			void erase();
			void del();

			compiled_resource& get_compiled() override;
		};
	}
}
